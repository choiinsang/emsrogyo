#include "EmsWorkServer.h"
#include "EmsQueue.h"
#include "EmsMXManager.h"
#include "EmsConfig.h"
#include "EmsLog.h"
#include "EmsDomainListSet.h"

extern CEmsLog *gLog;
extern CEmsDomainListSet *gDomainList;
extern int  GetExpResultCode(int _SmtpStep, int _iCode3Ch, char* _StrErrExp);
extern void to_lowercase(char *str);


//---------------------------------------------------
// EmsWorkServer Constructor
//---------------------------------------------------
CEmsWorkServer::CEmsWorkServer(int iMaxClientReadBuffer, int iMaxClientWriteBuffer, int iPort, int delay, int nMaxClients, int timer, FILE *fp)
: CServer<CEmsClient >(iMaxClientReadBuffer, iMaxClientWriteBuffer, iPort, delay, nMaxClients, timer, fp)
{
	m_iRetryCount  = theEMSConfig()->getMailRetryCount();
	m_iRetryPeriod = theEMSConfig()->getMailRetryPeriod();
	m_spEmsQueue   = shared_ptr<CEmsQueue>(new CEmsQueue);
	m_iIncForTimer = 0;
}

//---------------------------------------------------
// EmsWorkServer Destructor
//---------------------------------------------------
CEmsWorkServer::~CEmsWorkServer ()
{
	if(m_bStopThread != true){
		m_bStopThread = true;
	}
	
	pthread_join(m_ThreadProcQueue, NULL);
}


//---------------------------------------------------
// EmsClient Response 처리를 위한 ParserMain (size)
//---------------------------------------------------
void CEmsWorkServer::ParserMain( shared_ptr<CEmsClient> pClient, unsigned long _ulSize )
{
	shared_ptr<CEmsRequest> pRequest = shared_ptr<CEmsRequest >(new CEmsRequest);
	PACKET_HEADER          *pph      = (PACKET_HEADER *)pClient->m_ReadBuffer.get();

	pRequest->setPacket((char *)pph, _ulSize);
	pRequest->setClient(pClient);

	relayEmsRequest(pRequest);
}


//---------------------------------------------------
// CEmsWorkServer에 WorkerManager의 WorkersThreadPool Map 포인터 전달
//---------------------------------------------------
bool CEmsWorkServer::setWorkersPool(unordered_map< std::string, shared_ptr<CEmsWorkerThreadPool> > * pmapWorkersPool)
{
	bool retval = false;
	if(pmapWorkersPool != NULL){
		m_pmapEmsWorkersPool = pmapWorkersPool;
		retval = true;
	}
	return retval;
}

//---------------------------------------------------
// WorkerManager의 WorkersThreadPool 선택하여  Request 전달
//---------------------------------------------------
bool CEmsWorkServer::relayEmsRequest(shared_ptr<CEmsRequest> pRequest)
{
	bool        retval  = false;
	const char *pMapKey = NULL;
	
	unordered_map< std::string, shared_ptr<CEmsWorkerThreadPool> >::iterator itr_map;
	shared_ptr<CEmsWorkerThreadPool> emsWorkerThreadPool;
			
	if(pRequest.get() != NULL){
		
		//gLog->Write("[%s][%s][%d][MODE_TYPE]:[%d][Name:%s]", __FILE__, __FUNCTION__, __LINE__, pRequest.get()->getClient()->getCpMail()->getMode(), pRequest.get()->getClient()->getCampaignNoKey().c_str());
		if((pRequest.get()->getClient()->getCpMail().get() != NULL)
			&& (pRequest.get()->getClient()->getCpMail()->getCpMailState() == false)){
			pRequest.get()->getClient()->getCpMail().get()->setCpMailState(true);
		}
		else{
			return retval;
		}
		
		if(pRequest.get()->getClient()->getCpMail()->getMode() ==  MODE_I){
			//pMapKey = RMQ_ROUTE_KEY_COMMON;                           // RMQ_ROUTE_KEY_COMMON으로 ThreadPool을 찾음
			pMapKey = pRequest.get()->getClient()->getCpMail()->getWorkerName();
			//gLog->Write("********[%s][%s][%d][MapKey:%s]", __FILE__, __FUNCTION__, __LINE__, pMapKey);
		}
		else{
			pMapKey = pRequest.get()->getClient()->getCampaignNoKey().c_str();  // Client Campaign Key로 ThreadPool을 찾음
			//gLog->Write("********[%s][%s][%d][MapKey:%s]", __FILE__, __FUNCTION__, __LINE__, pMapKey);
		}
		
		if(pMapKey != NULL){
			//gLog->Write("********[%s][%s][%d][PoolSize:%d]", __FILE__, __FUNCTION__, __LINE__, m_pmapEmsWorkersPool->size());
			itr_map = m_pmapEmsWorkersPool->find(pMapKey);

			if(itr_map != m_pmapEmsWorkersPool->end()){
				emsWorkerThreadPool = itr_map->second; 
				//gLog->Write("********[%s][%s][%d][WorkerName:%s]", __FILE__, __FUNCTION__, __LINE__, emsWorkerThreadPool->getWorkerName());
				emsWorkerThreadPool.get()->addQueue(pRequest);   // Request를 해당 WorkerThreadPool에 전달한다.
				emsWorkerThreadPool.get()->timeUpdate();
				retval = true;
			}
			else{
				//gLog->Write("********[%s][%s][%d]", __FILE__, __FUNCTION__, __LINE__);
				pRequest.get()->getClient()->getCpMail().get()->setCpMailState(false);
			}

		}
	}

	return retval;
}

//---------------------------------------------------
// 서버연결이 실패할 경우 메일 메시지 처리 
// (Server Connect Fail)
//---------------------------------------------------
void CEmsWorkServer::connectFail( shared_ptr< CEmsClient > pClient, int iState )
{
	if(pClient.get()->m_bRemove == true){
		return;
	}

	shared_ptr<CCpMail> spCpMail = pClient.get()->getCpMail ();

	if(spCpMail.get() == NULL)
		return ;
		
	switch(iState){
		case 1:
		case 4:
			errorProc(spCpMail, STEP21_C101_ConErr, STEP21_C101Exp_ConErr, m_iRetryCount);
			break;
		case 2:
		case 3:
			errorProc(spCpMail, STEP21_C110_ConErr, STEP21_C110Exp_ConErr, m_iRetryCount);
			break;
	}

	pClient->RemoveClient();
}

//---------------------------------------------------
//연결된 서버에 메시지 전송 실패 처리
//(Send Buffer Data Fail)
//---------------------------------------------------
void CEmsWorkServer::sendBufFail( shared_ptr< CEmsClient > pClient, int iState )
{
	if(pClient.get()->m_bRemove == true){
		return;
	}

	shared_ptr<CCpMail> spCpMail = pClient.get()->getCpMail ();
	
	if(spCpMail.get() == NULL)
		return ;

	errorProc(spCpMail, STEP21_C301_DisconErr, STEP21_C301Exp_DisconErr, m_iRetryCount);

	pClient->RemoveClient();
}

//---------------------------------------------------
//서버 연결에 에러 발생
//(Server Connection Error Occurred)
//---------------------------------------------------
void CEmsWorkServer::connectError( shared_ptr< CEmsClient > pClient, int iState )
{
	if(pClient.get()->m_bRemove == true){
		return;
	}

	shared_ptr<CCpMail> spCpMail = pClient.get()->getCpMail ();
	
	if(spCpMail.get() == NULL)
		return ;

	errorProc(spCpMail, STEP21_C101_ConErr, STEP21_C101Exp_ConErr, m_iRetryCount);
	pClient->RemoveClient();
}


void CEmsWorkServer::bindQueue(const char * qname)
{
	unordered_map< std::string, shared_ptr<CEmsWorkerThreadPool> >::iterator itr_map;
	itr_map = m_pmapEmsWorkersPool->find(string(qname));         // Client Campaign Key로 ThreadPool을 찾고
	if(itr_map != m_pmapEmsWorkersPool->end()){
		shared_ptr<CEmsWorkerThreadPool> emsWorkerThreadPool = itr_map->second; 
		emsWorkerThreadPool.get()->setEmsQueue(m_spEmsQueue);     // 공통으로 사용할 EmsQueue를 해당 WorkerThreadPool에 전달한다.
	}
}


//---------------------------------------------------
// WorkersThreadPool에 공유된 m_spEmsQueue를 처리한다.
// Thread로 처리
//---------------------------------------------------
bool  CEmsWorkServer::StartProcessQueue()
{
	m_bStopThread = false;
	pthread_create(&m_ThreadProcQueue, NULL, &ThreadFuncProcQueue, this);
	gLog->Write("[EMS CEmsWorkServer Thread Create Success!]");
	return true;
}


//---------------------------------------------------
// 재전송 처리를 하기위한 에러처리 함수
//---------------------------------------------------
void CEmsWorkServer::errorProc(shared_ptr<CCpMail> spCpMail, int errCode, const char *errStr, int maxRetryCount)
{
	if(spCpMail.get() == NULL)
		return ;

	//CpMail State 체크
	if(spCpMail.get()->getCpMailState() == true)
		return;
	else{
		spCpMail.get()->setCpMailState(true);

		if(strcmp(DEFAULT_NO_IPADDRESS, spCpMail.get()->getConnIP()) != 0)
			gDomainList->disconnClient(spCpMail.get()->getMailInfo(MHEADER_TODOMAIN), spCpMail.get()->getConnIP());

		if(spCpMail.get()->getStepComplete() == false){
			char tmp_StrErrExpLower[LEN_ExplainLong+1]={0,};
			strcpy(tmp_StrErrExpLower, errStr);
			to_lowercase(tmp_StrErrExpLower);
			int iExpResultCode = GetExpResultCode(STEP21, errCode, tmp_StrErrExpLower);
			
			spCpMail.get()->setSmtpStep(STEP21);
			spCpMail.get()->setSmtpCode(errCode);
			spCpMail.get()->setExpResultCode(iExpResultCode);
			spCpMail.get()->setExpResultStr(errStr);
		
			if(spCpMail.get()->getRetryCount() < maxRetryCount)
				spCpMail.get()->incRetryCount();
	
			gLog->sendMsgToQueue(LOG_WQ, MSG_TYPE_DBUPDATE, spCpMail, iQryIns_CodeExpResult);
			gLog->sendMsgToQueue(LOG_WQ, MSG_TYPE_DBUPDATE, spCpMail, iQryUdt_Mail_SmtpChk7);
			
			if(spCpMail.get()->getRetryCount() < maxRetryCount){
				spCpMail.get()->setTimeStamp();
				spCpMail.get()->setSmtpStep(SMTP_STEP00_Init);
				spCpMail.get()->setSmtpCode(0);
				//ThreadPool을 검색하여 해당 Pool에 입수시킨다.
				enqueueMsg(spCpMail);
			}
		}
		spCpMail.get()->setCpMailState(false);
	}
}

//---------------------------------------------------
// 메시지를 해당 WorkerPool Thread에 재입수 처리
//---------------------------------------------------
void CEmsWorkServer::enqueueMsg(shared_ptr<CCpMail> spCpMail)
{
	if(spCpMail.get() != NULL){
		const char *pWorkerName = spCpMail.get()->getWorkerName();

		if(m_pmapEmsWorkersPool != NULL){
			unordered_map< std::string, shared_ptr<CEmsWorkerThreadPool> >::iterator itr_wtp = m_pmapEmsWorkersPool->find(pWorkerName);
			if(itr_wtp != m_pmapEmsWorkersPool->end()){
				shared_ptr<CEmsWorkerThreadPool> spWorkerThreadPool = itr_wtp->second;
				spWorkerThreadPool->addToDomainQueueList(spCpMail);
			}
		}			
	}
}

//######################################### TimerFunc ##########################################
void CEmsWorkServer::TimerFunc()
{
	m_iIncForTimer++;
	if(m_iIncForTimer == 10)
	{
		Check_ClientListConnected();
		m_iIncForTimer = 0;
	}
}
//##############################################################################################

//---------------------------------------------------
// (Thread Function)
// 메일 요청 메시지를 처리하기위해 서버 연결
//---------------------------------------------------
void * CEmsWorkServer::ThreadFuncProcQueue(void * param)
{
	CEmsWorkServer *pEmsWS      = (CEmsWorkServer *)param;
	int             RetryCount  = pEmsWS->getRetryCount();
	int             RetryPeriod = pEmsWS->getRetryPeriod();

	const char      *pWorkerName;
	unordered_map< std::string, shared_ptr<CEmsWorkerThreadPool> >::iterator itr_map;
	
	while(!pEmsWS->m_bStopThread)	{
		//Message Client Proc
		//if( pEmsWS->m_nCurrClients < pEmsWS->m_nMaxClients )	//서버에 접속한 클라이언트 수를 관리하여 임의 개수 이상 넘어가지 않도록 한다
		if( pEmsWS->m_nCurrClients < MAX_CONNECTION_COUNT )	//서버에 접속한 클라이언트 수를 관리하여 임의 개수 이상 넘어가지 않도록 한다
		{
			if(pEmsWS->m_spEmsQueue.get()->getQueueSize() == 0){
				usleep(THREAD_SLEEPTIME_50MSEC);
				continue;
			}
			shared_ptr<CCpMail>  tmpspCpMail = pEmsWS->m_spEmsQueue.get()->getCpMail() ;
			if(tmpspCpMail.get() == NULL){
				//gLog->Write("[%s][%s][%d][spCpMail is NULL]", __FILE__, __FUNCTION__, __LINE__);
				usleep(THREAD_SLEEPTIME_5MSEC);
				continue;
			}
			
			pWorkerName = tmpspCpMail.get()->getWorkerName();

			if(pWorkerName == NULL){
				usleep(THREAD_SLEEPTIME_1MSEC);
				continue;
			}
				
			itr_map = pEmsWS->m_pmapEmsWorkersPool->find(pWorkerName);
			if(itr_map == pEmsWS->m_pmapEmsWorkersPool->end()){
				usleep(THREAD_SLEEPTIME_1MSEC);
				continue;
			}
			
			if(tmpspCpMail.get()->getRetryCount() > 0){
				time_t currTime;
				time(&currTime);
				int retval = (int)difftime(currTime, tmpspCpMail.get()->getTimeStamp());
				if( retval < RetryPeriod ){ // RetryPeriod 이전이면 재입수 시킨다.
					usleep(THREAD_SLEEPTIME_1MSEC);
					pEmsWS->enqueueMsg(tmpspCpMail);
					continue;
				}
			}
			
			const char *pDomainName = tmpspCpMail.get()->getMailInfo(MHEADER_TODOMAIN);
			const char *pMailNo     = tmpspCpMail.get()->getMailInfo(MHEADER_MAILNUMBER);
			char        mxIPAddr[16]= {0x00,};
			int         errCode     = 0;
			string      errStr;
			bool        bResult     = false;
			int         interval    = -1;
			int         conCnt      = -1;

			if((pDomainName == NULL) || (pMailNo == NULL)){
				gLog->Write("[%s][%d][ERROR][Domain:%s][MailnNo:%s]", __FUNCTION__, __LINE__, pDomainName, pMailNo);
				usleep(THREAD_SLEEPTIME_5MSEC);
				continue;
			}
			
			//1. DomainList에 있는지 체크
			bResult = gDomainList->getDomainInterval(pDomainName, interval, conCnt);
			if(bResult == false){
				gLog->Write("[%s][%d][ERROR][Get Domain Interval Info Failed][%s][%s]", __FUNCTION__, __LINE__, pMailNo, pDomainName);
				usleep(THREAD_SLEEPTIME_1MSEC);
				continue;
			}
			
			//2. MX 값이 존재하지 않는 경우와 대기시간이 남은 경우(0), 처리 가능한 경우(1)
			boost::shared_ptr<CEmsMX> spEmsMX = theMXManager()->getMXFromMXList(pDomainName, errCode, errStr);

			if(spEmsMX.get() == NULL){
				pEmsWS->errorProc(tmpspCpMail, errCode, errStr.c_str(), RetryCount);

				gLog->Write("CEmsMX Value Get Failed!..Continue...[Domain:%s]", pDomainName);
				usleep(THREAD_SLEEPTIME_1MSEC);
				continue;
			}

			bResult = spEmsMX.get()->getMXIPAddress(mxIPAddr, interval, conCnt);
			
			if(bResult == true){ //사용 가능한 IP가 존재하는 경우
				usleep(THREAD_SLEEPTIME_1MSEC);
			}
			else { // if(bResult == false) //사용 가능한 IP가 없는 경우
				usleep(THREAD_SLEEPTIME_5MSEC);
				pEmsWS->enqueueMsg(tmpspCpMail);
				continue;
			}
			
			//Check Server IP
			//---------------
			if(strlen(mxIPAddr) < 7){
				tmpspCpMail.get()->setConnIP(DEFAULT_NO_IPADDRESS);
				tmpspCpMail.get()->setRetryCount(RetryCount);
				pEmsWS->errorProc(tmpspCpMail, STEP21_C201_CreatMxErr, STEP21_C201Exp_CreatMxErr, RetryCount);

				gLog->Write("[%s][%s][%d][Get MX IPAddress Failed][Domain:%s]", __FILE__, __FUNCTION__, __LINE__, pDomainName);
			}
			else
			{
				//--------------------------------
				//TEST LOG PRINT 
				//gLog->Write("[Connect][Index:%s][%s][%s]",  pMailNo, pDomainName, mxIPAddr);
				//--ischoi TEST CODE
				//usleep(THREAD_SLEEPTIME_5MSEC);
				//continue;
				//string strIPAddr = "127.0.0.1";
				//--------------------------------
				if(strcmp("127.0.0.1", mxIPAddr)==0){
					tmpspCpMail.get()->setConnIP(mxIPAddr);
					tmpspCpMail.get()->setRetryCount(RetryCount);
					pEmsWS->errorProc(tmpspCpMail, STEP21_C110_ConErr, STEP21_C110Exp_ConErr, RetryCount);
				}
				else{
					//Connect Mail Server
					int retval = -1;
					shared_ptr<CEmsClient> tmpspClient;
					tmpspCpMail.get()->setConnIP(mxIPAddr);
					retval = pEmsWS->Connect(mxIPAddr, MAIL_PORT, tmpspClient);
	
					if(retval<= CONNECT_ConNet_MaxFail){
						//Connect Failed 
						if(retval == CONNECT_ConNet_Failed){
							pEmsWS->errorProc(tmpspCpMail, STEP21_C101_ConErr, STEP21_C101Exp_ConErr, RetryCount);
						}
						else{
							pEmsWS->errorProc(tmpspCpMail, STEP21_C102_AddCltErr, STEP21_C102Exp_AddCltErr, RetryCount);
						}
						
						if(tmpspClient.get() != NULL){
							tmpspClient->RemoveClient();
						}
	
						gLog->Write("[%s][%s][%d] Connect Failed[Domain:%s][%s]", __FILE__, __FUNCTION__, __LINE__, pDomainName, mxIPAddr);
					}
					else{
						tmpspClient.get()->setCampaignNoKey(tmpspCpMail.get()->getWorkerName());
						tmpspClient.get()->setCpMail(tmpspCpMail);
						gLog->Write("[Connect][%s][%d][Index:%s][%s][%s]", tmpspCpMail.get()->getWorkerName(), retval, pMailNo, pDomainName, mxIPAddr);
					}
				}
			}
			usleep(THREAD_SLEEPTIME_1MSEC);
		}
		else 
			usleep(THREAD_SLEEPTIME_10MSEC);		
	}
	return (void *) NULL;
}

bool CEmsWorkServer::IsNullClientCount()
{
	if(m_ClientList.Begin() == m_ClientList.End()) // Client Queue is Not Null
		return true;
	else
		return false;
}

