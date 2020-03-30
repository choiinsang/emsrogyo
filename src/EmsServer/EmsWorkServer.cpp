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
// EmsClient Response ó���� ���� ParserMain (size)
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
// CEmsWorkServer�� WorkerManager�� WorkersThreadPool Map ������ ����
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
// WorkerManager�� WorkersThreadPool �����Ͽ�  Request ����
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
			//pMapKey = RMQ_ROUTE_KEY_COMMON;                           // RMQ_ROUTE_KEY_COMMON���� ThreadPool�� ã��
			pMapKey = pRequest.get()->getClient()->getCpMail()->getWorkerName();
			//gLog->Write("********[%s][%s][%d][MapKey:%s]", __FILE__, __FUNCTION__, __LINE__, pMapKey);
		}
		else{
			pMapKey = pRequest.get()->getClient()->getCampaignNoKey().c_str();  // Client Campaign Key�� ThreadPool�� ã��
			//gLog->Write("********[%s][%s][%d][MapKey:%s]", __FILE__, __FUNCTION__, __LINE__, pMapKey);
		}
		
		if(pMapKey != NULL){
			//gLog->Write("********[%s][%s][%d][PoolSize:%d]", __FILE__, __FUNCTION__, __LINE__, m_pmapEmsWorkersPool->size());
			itr_map = m_pmapEmsWorkersPool->find(pMapKey);

			if(itr_map != m_pmapEmsWorkersPool->end()){
				emsWorkerThreadPool = itr_map->second; 
				//gLog->Write("********[%s][%s][%d][WorkerName:%s]", __FILE__, __FUNCTION__, __LINE__, emsWorkerThreadPool->getWorkerName());
				emsWorkerThreadPool.get()->addQueue(pRequest);   // Request�� �ش� WorkerThreadPool�� �����Ѵ�.
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
// ���������� ������ ��� ���� �޽��� ó�� 
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
//����� ������ �޽��� ���� ���� ó��
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
//���� ���ῡ ���� �߻�
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
	itr_map = m_pmapEmsWorkersPool->find(string(qname));         // Client Campaign Key�� ThreadPool�� ã��
	if(itr_map != m_pmapEmsWorkersPool->end()){
		shared_ptr<CEmsWorkerThreadPool> emsWorkerThreadPool = itr_map->second; 
		emsWorkerThreadPool.get()->setEmsQueue(m_spEmsQueue);     // �������� ����� EmsQueue�� �ش� WorkerThreadPool�� �����Ѵ�.
	}
}


//---------------------------------------------------
// WorkersThreadPool�� ������ m_spEmsQueue�� ó���Ѵ�.
// Thread�� ó��
//---------------------------------------------------
bool  CEmsWorkServer::StartProcessQueue()
{
	m_bStopThread = false;
	pthread_create(&m_ThreadProcQueue, NULL, &ThreadFuncProcQueue, this);
	gLog->Write("[EMS CEmsWorkServer Thread Create Success!]");
	return true;
}


//---------------------------------------------------
// ������ ó���� �ϱ����� ����ó�� �Լ�
//---------------------------------------------------
void CEmsWorkServer::errorProc(shared_ptr<CCpMail> spCpMail, int errCode, const char *errStr, int maxRetryCount)
{
	if(spCpMail.get() == NULL)
		return ;

	//CpMail State üũ
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
				//ThreadPool�� �˻��Ͽ� �ش� Pool�� �Լ���Ų��.
				enqueueMsg(spCpMail);
			}
		}
		spCpMail.get()->setCpMailState(false);
	}
}

//---------------------------------------------------
// �޽����� �ش� WorkerPool Thread�� ���Լ� ó��
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
// ���� ��û �޽����� ó���ϱ����� ���� ����
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
		//if( pEmsWS->m_nCurrClients < pEmsWS->m_nMaxClients )	//������ ������ Ŭ���̾�Ʈ ���� �����Ͽ� ���� ���� �̻� �Ѿ�� �ʵ��� �Ѵ�
		if( pEmsWS->m_nCurrClients < MAX_CONNECTION_COUNT )	//������ ������ Ŭ���̾�Ʈ ���� �����Ͽ� ���� ���� �̻� �Ѿ�� �ʵ��� �Ѵ�
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
				if( retval < RetryPeriod ){ // RetryPeriod �����̸� ���Լ� ��Ų��.
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
			
			//1. DomainList�� �ִ��� üũ
			bResult = gDomainList->getDomainInterval(pDomainName, interval, conCnt);
			if(bResult == false){
				gLog->Write("[%s][%d][ERROR][Get Domain Interval Info Failed][%s][%s]", __FUNCTION__, __LINE__, pMailNo, pDomainName);
				usleep(THREAD_SLEEPTIME_1MSEC);
				continue;
			}
			
			//2. MX ���� �������� �ʴ� ���� ���ð��� ���� ���(0), ó�� ������ ���(1)
			boost::shared_ptr<CEmsMX> spEmsMX = theMXManager()->getMXFromMXList(pDomainName, errCode, errStr);

			if(spEmsMX.get() == NULL){
				pEmsWS->errorProc(tmpspCpMail, errCode, errStr.c_str(), RetryCount);

				gLog->Write("CEmsMX Value Get Failed!..Continue...[Domain:%s]", pDomainName);
				usleep(THREAD_SLEEPTIME_1MSEC);
				continue;
			}

			bResult = spEmsMX.get()->getMXIPAddress(mxIPAddr, interval, conCnt);
			
			if(bResult == true){ //��� ������ IP�� �����ϴ� ���
				usleep(THREAD_SLEEPTIME_1MSEC);
			}
			else { // if(bResult == false) //��� ������ IP�� ���� ���
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

