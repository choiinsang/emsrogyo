#include "EmsWorkerManager.h"
#include "EmsWorkerThreadPool.h"
#include "EmsConfig.h"
#include "EmsLog.h"

extern CEmsLog    * gLog;
extern const char * getHeaderStringFromKey(int hNumber);


//-----------------------------------------------
// Mail Campaign 별로 큐를 연결하여 가져온 큐메시지를 
// 스레드가 처리할 수 있는 메시지로 변경하여 처리
// 역할: Worker 스레드 관리 / 메시지 큐관리 및 큐 전달
//-----------------------------------------------
CEmsWorkerThreadPool::CEmsWorkerThreadPool( const char *pwtpName, unordered_map<std::string, shared_ptr<CEmsDKIM> > *pEmsDKIMList, int iThreadMode, int nThreads )
	: CThreadPool<CEmsRequest, CEmsThread>(nThreads)  // WorkerThread 설정
	, m_pAMQPMsgQueue (NULL)
	, m_bIsStop       (false)
	, m_bIsRun        (false)
	, m_bIsPause      (false)
	, m_ThreadMode    (iThreadMode)
	, m_iThreadCount  (nThreads)
	, m_bCheckMailInfo(false)
	, m_pEmsDKIMList  (pEmsDKIMList)
	, m_iDomainQueueMsgCount(0)
{
	assert(pwtpName != NULL);
	int tmpNameLen = strlen(pwtpName);
	assert(tmpNameLen != 0);
	m_chWTPName = strdup(pwtpName);
	
	m_mapDomainQueueList.clear();

	setRMQExchange(iThreadMode);
	connAMQPQueue(m_chWTPName);

	m_cpMailInfo = shared_ptr<stCpMailInfo>(new stCpMailInfo);
	
	gLog->Write("[%s][NAME: %s][MDOE:%s]", __FUNCTION__, pwtpName, (iThreadMode == MODE_I)?"PERSONAL":"GROUP");

	//Set Campaign Information To EmsThreads
	shared_ptr<CEmsThread> spEmsThread;
	for(int i=0; i < m_iThreadCount; i++){
		spEmsThread = getThread(i);
		if(spEmsThread.get() != NULL){
			spEmsThread.get()->setThreadMode(iThreadMode);
			if(iThreadMode == MODE_I)
				spEmsThread.get()->InitThreadBuf();

			spEmsThread.get()->setEmsDKIMList(m_pEmsDKIMList);
		}
	}
	
	time(&m_tLastWorkTime);
}

//-----------------------------------------------
// EMS WorkerThreadPool Destructor
//-----------------------------------------------
void CEmsWorkerThreadPool::setRMQExchange(int iThreadMode)
{
	if(iThreadMode == MODE_I){
		m_pRMQExchange = RMQ_EXCHANGE_COMMON;
	}
	else if(iThreadMode == MODE_M){
		m_pRMQExchange = RMQ_EXCHANGE_MAIL;
	}
	else{
		gLog->Write("[%s][%s][%d]", __FILE__, __FUNCTION__, __LINE__, iThreadMode);
	}
}
	
//-----------------------------------------------
// EMS WorkerThreadPool Destructor
//-----------------------------------------------
CEmsWorkerThreadPool::~CEmsWorkerThreadPool()
{
	setStopState(true);
	setRunState(false);
	
	for(int i=0; i < m_iThreadCount; i++){
		if(getThread(i).get()!=NULL){
			getThread(i).get()->stopThread();
		}
	}

	pthread_join(m_EmsWMTPThread, NULL);
	pthread_join(m_EmsMsgWorkerThread, NULL);
	
	if(!m_bIsPause)
		gLog->CloseInfo(m_chWTPName);

	if(m_chWTPName != NULL){
		theEmsRMQManager()->deleteRMQConn(m_pRMQExchange, m_chWTPName, false);
		m_pAMQPMsgQueue = NULL;
	}
	
	if(m_chWTPName != NULL)
		free(m_chWTPName);	
	m_chWTPName = NULL;
}

//-----------------------------------------------
// AMQPQueue 연결
//-----------------------------------------------
bool CEmsWorkerThreadPool::connAMQPQueue(const char *qname)
{
	bool retval = false;

	try{
		if(qname == NULL){
			//--ischoi Queue 이름이 없으므로 프로세스 종료
			gLog->Write("[%s][%s][%d] AMQPQueue Name is NULL", __FILE__, __FUNCTION__, __LINE__);
			exit(0);
		}
	
		do{
			m_pAMQPMsgQueue = theEmsRMQManager()->getQueue(m_pRMQExchange, qname);
			
			if(m_pAMQPMsgQueue != NULL){
				// Queue가 없는 경우에는 생성하고, 
				// 이미 있는 경우라도, 연결이 비정상 종료 된 경우에 큐손실을 없애기 위해
				// 큐를 삭제하지 않는다. 큐 삭제는 EmsAgent서버에서 하도록 한다.
				retval = true;
				break;
			}
			else
				sleep(THREAD_RMQ_WAIT_TIME);
				
		}while(m_pAMQPMsgQueue == NULL);
	}
	catch(AMQPException ex){
		gLog->Write("[%s][%s][%d][ERROR][MSG:%s]", __FILE__, __FUNCTION__, __LINE__, ex.getMessage().c_str());
		theEmsRMQManager()->deleteRMQConn(m_pRMQExchange, qname);
		m_pAMQPMsgQueue = NULL;
	}
	catch(...){
		gLog->Write("[%s][%s][%d][ERROR][Unknown Error Occurred]", __FILE__, __FUNCTION__, __LINE__);
		theEmsRMQManager()->deleteRMQConn(m_pRMQExchange, qname);
		m_pAMQPMsgQueue = NULL;
	}

	return retval;
}


//-----------------------------------------------
// CEmsWorkerManager 관리하기 위한 스레드 시작
// => Rabbitmq의 Campaign에 있는 메시지를 처리
//-----------------------------------------------
void CEmsWorkerThreadPool::startThread ()
{
	//RabbitMQ->DomainQueue
	int retval = pthread_create(&m_EmsWMTPThread, NULL, &threadProc, this);
	if( retval != 0 ){
		gLog->Write("[%s][ERROR][EMS WorkerThreadPool1 Thread Create Failed!]", __FUNCTION__);
		exit(0);
	}
	sleep(1);
	//DomainQueue->ServerQueue
	retval = pthread_create(&m_EmsMsgWorkerThread, NULL, &threadProcMsg, this);
	if( retval != 0 ){
		gLog->Write("[%s][ERROR][EMS WorkerThreadPool2 Thread Create Failed!]", __FUNCTION__);
		exit(0);
	}

	gLog->Write("[%s][EMS WorkerThreadPool Thread Create Success!]", __FUNCTION__);

	setRunState(true);
}


//-----------------------------------------------
// WorkerThreadPool Set Running  state
//-----------------------------------------------
void CEmsWorkerThreadPool::setRunState (bool bState) 
{ 
	m_bIsRun = bState;
}


//-----------------------------------------------
// Only Use In Group Mode : Campaign Info Init
//-----------------------------------------------
bool CEmsWorkerThreadPool::setCpMailInfo  (AMQPMessage * message)
{
	if(m_bCheckMailInfo == true)
		return true;
		
	AMQPMessage * amqpmsg = (AMQPMessage *)message;
	
	if(m_cpMailInfo.get() == NULL){
		 m_cpMailInfo = shared_ptr<stCpMailInfo>(new stCpMailInfo);
	}
	
	const char * errstr = NULL;
	try{
		
		errstr = MHEADER_CPMAILKEY;
		m_cpMailInfo.get()->_cpIndex        = strdup(amqpmsg->getHeader(MHEADER_CPMAILKEY).c_str());

		errstr = MHEADER_SENDERNAME;
		m_cpMailInfo.get()->_cpSenderName   = strdup(amqpmsg->getHeader(MHEADER_SENDERNAME).c_str());

		errstr = MHEADER_SENDEREMAIL;
		m_cpMailInfo.get()->_cpSenderMail   = strdup(amqpmsg->getHeader(MHEADER_SENDEREMAIL).c_str());

		errstr = MHEADER_MAILTITLE;
		m_cpMailInfo.get()->_cpMailTitle     = strdup(amqpmsg->getHeader(MHEADER_MAILTITLE).c_str());
		m_cpMailInfo.get()->_icpMailTitleLen = strlen(m_cpMailInfo.get()->_cpMailTitle);

		errstr = MHEADER_SENDERDOMAIN;
		m_cpMailInfo.get()->_cpSenderDomain = strdup(amqpmsg->getHeader(MHEADER_SENDERDOMAIN).c_str());

		errstr = MHEADER_USEWSTR;
		m_cpMailInfo.get()->_bcpUseWStr     = (strcmp(amqpmsg->getHeader(MHEADER_USEWSTR).c_str(), "1")==0 ? true:false);
		
		errstr = MAIL_MSG_BODY;
		{
			uint32_t j;
			char * pMailBody = amqpmsg->getMessage(&j);
			if(pMailBody != NULL){
				m_cpMailInfo.get()->_cpMailBody     = strdup(pMailBody);
				m_cpMailInfo.get()->_icpMailBodyLen = strlen(pMailBody);
			}
			else{
				m_cpMailInfo.get()->_cpMailBody     = NULL;
				m_cpMailInfo.get()->_icpMailBodyLen = 0;
			}
		}
		
		//--ischoi : EmsThread에 Campaign 정보를 전달한다.
		shared_ptr<CEmsThread> spEmsThread;
		for(int i=0; i < m_iThreadCount; i++){
			spEmsThread = getThread(i);
			if(spEmsThread.get()!=NULL){
				spEmsThread.get()->setCpMailInfo(m_cpMailInfo);
			}
		}
		
		//gLog->Write("[%s][%s][%d][%s]", __FILE__, __FUNCTION__, __LINE__, "INIT INFO Set HERE!");
	}
	catch(...){
		gLog->Write("[%s][%s][%d]Error Occurred![%s]", __FILE__, __FUNCTION__, __LINE__, errstr);
		return false;
	}
	
	m_bCheckMailInfo = true;
	return true;
}

void CEmsWorkerThreadPool::timeUpdate()
{
	time(&m_tLastWorkTime);
}

bool CEmsWorkerThreadPool::isWaitTimeOver(int iMaxWaitTime)
{
	time_t tCurrtime;
	time(&tCurrtime);
	
	int retval = difftime(tCurrtime, m_tLastWorkTime);
	if(retval > iMaxWaitTime )
		return true;
	else
		return false;
}

//-----------------------------------------------
// Uncomplete Worke List Insert Into Queue Function
//-----------------------------------------------
void CEmsWorkerThreadPool::uncompleteWorkCheck()
{
	shared_ptr<CEmsQueue> spEmsQueue = shared_ptr<CEmsQueue>(new CEmsQueue);

	//Uncomplete Previous Message Process
	gLog->ReadInfo(getWorkerName(), spEmsQueue);
	if(spEmsQueue.get() == NULL)
		return;
		
	while(spEmsQueue.get()->getQueueSize() > 0){
		addToDomainQueueList(spEmsQueue.get()->getCpMail());
	}
}

//-----------------------------------------------
// Thread Function
// RabbitMQ로부터 도메인큐에 메시지를 삽입
//-----------------------------------------------
void * CEmsWorkerThreadPool::threadProc (void * param)
{
	CEmsWorkerThreadPool *pEmsWTPThread = (CEmsWorkerThreadPool*)param;
	int iResult     = -1;
	//-----------------------------------
	//Uncompleted Job Insert into Queue
	pEmsWTPThread->uncompleteWorkCheck();
	pEmsWTPThread->timeUpdate();
	usleep(THREAD_SLEEPTIME_1SEC);
	//-----------------------------------

	while( true )
	{
		if(!pEmsWTPThread->isStop()){
			try{
				iResult = pEmsWTPThread->threadProcess(); // AMQPQueue 메시지 처리 프로세스
			}
			catch(...){
				if(pEmsWTPThread != NULL)
					iResult = -4;
				else
					iResult = -3;
				gLog->Write("[%s][%s][%d][ERROR][WorkerThreadPool Error Occurred[RetCode:%d]", __FILE__, __FUNCTION__, __LINE__, iResult);
			}
			
			//iResult
			// -4:WTPThread NULL, -3:Unknown Error,
			// -2:Queue is Null,  -1:Queue Disconnected, 
			//  0:Connected State, 1:Queue Size Full
			//-----------------------------------------------
			if(iResult < 0){
				if(pEmsWTPThread->isStop()==true){
					break;
				}
				//재시작
				gLog->Write("[%s][%s][%d][ERROR][WorkerThreadPool Error Occurred[RetCode:%d]", __FILE__, __FUNCTION__, __LINE__, iResult);
				sleep(THREAD_RMQ_WAIT_TIME);
				continue;
			}
			else{
				if(pEmsWTPThread->isWaitTimeOver(INIT_DEFAULT_MAILRETYRPERIOD*3)==true){
					//Check Uncomplete Worke
					pEmsWTPThread->uncompleteWorkCheck();
					pEmsWTPThread->timeUpdate();
					usleep(THREAD_SLEEPTIME_1SEC);
				}

				if(iResult > 0){
					usleep(THREAD_SLEEPTIME_50MSEC);
				}
			 	else{
			 		usleep(THREAD_SLEEPTIME_1MSEC);
					pEmsWTPThread->timeUpdate();
			 	}
			}
	 	}
	 	else{
	 		gLog->Write("[EMS WorkerThreadPool Thread Stop:%s]", pEmsWTPThread->getWorkerName());
	 		break;
	 	}
	}
	
	pEmsWTPThread->setRunState(false);

	return (void *) NULL;
}


//-----------------------------------------------
// 전송 처리 대기중인 Queue로부터 메일 메시지 수신 처리
// Return Value
// -3:Unknown Error,
// -2:Queue is Null,  -1:Queue Disconnected, 
//  0:Connected State, 1:Queue Size Full, New Message is Null
//-----------------------------------------------
int CEmsWorkerThreadPool::threadProcess ()
{
	if(m_pAMQPMsgQueue == NULL){
		if(isStop()==true){
			return -1;
		}
		
		gLog->Write("[%s][%s][%d][(m_pAMQPMsgQueue == NULL)][Exchange:%s]", __FILE__, __FUNCTION__, __LINE__, m_pRMQExchange);
		if(connAMQPQueue(getWorkerName()) != true){
			gLog->Write("[%s][%s][%d][ERROR][connAMQPQueue() Failed][RETURN -1]", __FILE__, __FUNCTION__, __LINE__);
			return -1;  // AMQPQueue Connection Fail
		}
		
		if(m_pAMQPMsgQueue == NULL){
			gLog->Write("[%s][%s][%d][ERROR][m_pAMQPMsgQueue is NULL][RETURN -1]", __FILE__, __FUNCTION__, __LINE__);
			return -1;  // AMQPQueue Connection Fail
		}
	}

	if(m_iDomainQueueMsgCount > MAX_TOTAL_MSG_COUNT)
		return 1;
	
	try{
		m_pAMQPMsgQueue->Get(AMQP_NOACK);
		
		AMQPMessage * msg = m_pAMQPMsgQueue->getMessage();
		if(msg == NULL){
			if((getQueue().get()->size() > 0 )|| (checkWorkServerQ(1) == false))
				return 0;
			else
				return 1; // New Message is Null
		}
		
		if (msg->getMessageCount() > -1) {
			boost::shared_ptr<CCpMail> tmpspCpMail = boost::shared_ptr<CCpMail>(new CCpMail);
				
			if(tmpspCpMail.get()!=NULL){
				const char * tmpType = NULL;
				const char * tmpData = NULL;

				//Mail Header Info
				//------------------------------------
				for(int i=0; i < CP_MAILINFO_END; i++){
					tmpType = getHeaderStringFromKey(i);
					if(tmpType == NULL)                              // header type 없는 경우
						continue;
					
					tmpData = msg->getHeader(tmpType).c_str();       //header type의 Data가 없는 경우
					if((tmpData != NULL)&&(strlen(tmpData) >0)){     // message Header에 값이 있다면 CCpMail에 삽입
						tmpspCpMail.get()->setMailInfo(tmpType, tmpData);
						//gLog->Write("[%s][%s][%d] Header[%s]:[%s]", __FILE__, __FUNCTION__, __LINE__, tmpType, tmpData );
					}
				}
				
				tmpspCpMail.get()->setMailInfo(MHEADER_SMTPCODE, "0");
				tmpspCpMail.get()->setMailInfo(MHEADER_SMTPSTEP, "0");

				// CCpMail에 해당 Worker Queue 이름을 설정한다.
				if(getThreadMode() == MODE_I){
					tmpspCpMail.get()->setWorkerName(getWorkerName());
				}
				else { //if(getThreadMode() == MODE_M)
					tmpspCpMail.get()->setWorkerName(tmpspCpMail.get()->getMailInfo(MHEADER_QINDEX));
				}

				//Mail Body Info
				//------------------------------------
				uint32_t j = -1;
				tmpData = msg->getMessage(&j);
				if(j > 0){
					tmpspCpMail.get()->setMailInfo(MAIL_MSG_BODY, tmpData);
				}
				else{
					if(getThreadMode() == MODE_I){
						gLog->Write("[%s][%s][%d][WARN][MESSAGE Body is NULL][%s]", 
						__FILE__, __FUNCTION__, __LINE__, msg->getHeader(MHEADER_CPMAILKEY).c_str());
					}
				}

				//헤더 정보들을 파일에 저장
				gLog->WriteInfo(tmpspCpMail, true);
				//Msg:GroupNo/CpNo/ID/Domain
				gLog->Write("[GroupNO:%s][CpNo:%s][MailIdx:%s][Id:%s][Domain:%s]", tmpspCpMail.get()->getMailInfo(MHEADER_GROUPNUMBER)
				                                                       ,tmpspCpMail.get()->getMailInfo(MHEADER_CPNUMBER)
				                                                       ,tmpspCpMail.get()->getMailInfo(MHEADER_MAILINDEX)
				                                                       ,tmpspCpMail.get()->getMailInfo(MHEADER_TOID)
				                                                       ,tmpspCpMail.get()->getMailInfo(MHEADER_TODOMAIN));
				//------------------------------------
				//도메인 큐에 메시지를 넣는다.
				addToDomainQueueList(tmpspCpMail);
				
			}
			else{
				gLog->Write("[%s][%s][%d][Create CpMail Failed]", __FILE__, __FUNCTION__, __LINE__);
			}
			return 0;
		}
		else{
			if((getQueue().get()->size() > 0 )|| (checkWorkServerQ(1) == false))
				return 0;
			else
				return 1;
		}

	}
	catch(AMQPException ex){
		//Error Occurred :  AMQPException::AMQP_RESPONSE_SERVER_EXCEPTION
		if( ex.getReplyCode() != 404 ){
			gLog->Write("[%s][%s][%d][ERROR][AMQP Connection Error Occurred!][CODE:%d][%s]",
			            __FILE__,  __FUNCTION__, __LINE__, (int)ex.getReplyCode(), ex.getMessage().c_str() );
		}
		//Try Reconnect
		try{
			if(m_pAMQPMsgQueue != NULL){
				if(theEmsRMQManager()->deleteRMQConn(m_pRMQExchange, getWorkerName(), false) == true)
					gLog->Write("[%s][Delete Queue:%s]", __FUNCTION__, getWorkerName());
			}
			m_pAMQPMsgQueue = NULL;
		}
		catch(...){
			gLog->Write("[%s][%s][%d][ERROR] AMQPdeleteRMQConn Error Occurred!\n[CODE:%d][%s]!",
			            __FILE__,  __FUNCTION__, __LINE__, (int)ex.getReplyCode(), ex.getMessage().c_str() );
		}
		gLog->Write("[%s][%s][%d][ERROR][RETURN -1][stop:%d]", __FILE__, __FUNCTION__, __LINE__, isStop());
		return -1;
	}
	catch(...){
		gLog->Write("[%s][%s][%d][ERROR][Unknown ERROR Occurred!]", __FILE__, __FUNCTION__, __LINE__);
		return -3;
	}

	return 1;
}


//-----------------------------------------------
// Thread Function
// 도메인큐와 서버큐를 체크하여 서버큐에 메시지를 삽입
//-----------------------------------------------
void * CEmsWorkerThreadPool::threadProcMsg (void * param)
{
	CEmsWorkerThreadPool *pEmsWTPThread = (CEmsWorkerThreadPool*)param;
	int  iResult            = -1;
	char DomainName[SZ_NAME]= {0,};
	strcpy(DomainName, NULLDOMAINNAME);

	while( true )
	{
		if(!pEmsWTPThread->isStop()){
			iResult = pEmsWTPThread->threadProcessMsg(DomainName); // DomainQueue 메시지 처리 프로세스
			
			//[iResult] -1:DomainQueue is NULL, 0:Insert OK
			//-----------------------------------------------
			if(iResult < 0){
				usleep(THREAD_SLEEPTIME_50MSEC);
				continue;
			}
			else{ // if(iResult >= 0)
				usleep(THREAD_SLEEPTIME_1MSEC);
			}
	 	}
	 	else{
	 		gLog->Write("[EMS WorkerThreadPool Queue Worker Thread Stop:%s]", pEmsWTPThread->getWorkerName());
	 		break;
	 	}
	}
	
	pEmsWTPThread->setRunState(false);

	return (void *) NULL;
}


//-----------------------------------------------
// DomainQueue의 메시지를 ServerQueue로 전달
// Return Value
// -1:DomainQueue is NULL, 0:Insert OK
//-----------------------------------------------
int CEmsWorkerThreadPool::threadProcessMsg (char *pDomainName)
{
	try{
		//DomainQueue에 메시지 체크
		//if nothing, then do nothing
		//EmsWorkServer Queue 사이즈를 확인하고 만약 가득차있는경우 패스
		//큐에 입력 가능한 상태인 경우 입력하도록 한다.
		
		if(checkWorkServerQ(MAX_CONNECTION_COUNT) == false){// 서버 작업큐가 꽉차있다면 작업이 있는 상태라고 볼 수있다.
			return 0;
		}
		
		// Message Client Count Check 
		if(getQueue().get() == NULL) {
			gLog->Write("[%s][%s][%d][ERROR][getQueue() Failed][RETURN -2]", __FILE__, __FUNCTION__, __LINE__);
			return -2; // Queue is Null
		}
		
		if(m_mapDomainQueueList.size() == 0){
			return -1;
		}
		
		//DomainQueue의 메시지를 ServerQueue에 넣을 수 있는 경우 넣자
		unordered_map<std::string, shared_ptr<CEmsQueue> >::iterator itr_dq;
		shared_ptr<CEmsQueue> tmpspEmsQueue;
		int tmpTotalCount = 0;

		//Check Domain Map Count
		Lock();
		for(itr_dq = m_mapDomainQueueList.begin();itr_dq != m_mapDomainQueueList.end();){
			tmpspEmsQueue = itr_dq->second;
			if( tmpspEmsQueue.get() != NULL){
				if(tmpspEmsQueue.get()->getQueueSize() == 0){
					itr_dq = m_mapDomainQueueList.erase(itr_dq);
				}
				else{
					tmpTotalCount +=  tmpspEmsQueue.get()->getQueueSize();
					itr_dq++;
				}
			}
			else{
				itr_dq = m_mapDomainQueueList.erase(itr_dq);
			}
		}
		Unlock();
		
		if(tmpTotalCount <= 0){
			m_iDomainQueueMsgCount = 0;
			return -1;
		}
		else{
			m_iDomainQueueMsgCount = tmpTotalCount;
		}

		//Set Last Check Domain Name
		if(strcmp(pDomainName, NULLDOMAINNAME)==0)
			itr_dq = m_mapDomainQueueList.begin();
		else{
			itr_dq = m_mapDomainQueueList.find(pDomainName);
			if(itr_dq == m_mapDomainQueueList.end()){
				itr_dq = m_mapDomainQueueList.begin();
			}
		}
		
		if(itr_dq == m_mapDomainQueueList.end()){
			//gLog->Write("[%s][%s][%d][NO m_mapDomainQueueList in Queue]", __FILE__, __FUNCTION__, __LINE__);
			return -2;
		}


		//ServerQueue에 가득 채운다.
		shared_ptr<CCpMail> spCpMail;
		int iCount = MAX_CONNECTION_COUNT - (m_spEmsQueue.get()->getQueueSize());
		if(m_iDomainQueueMsgCount < iCount){
			iCount = m_iDomainQueueMsgCount ;
		}
		
		shared_ptr<CEmsQueue> spEmsQueue;
		while(iCount > 0){
			//gLog->Write("[iCount:%d][TotalMsgCnt:%d][DQCnt:%d]", iCount, m_iDomainQueueMsgCount, m_mapDomainQueueList.size());
			spEmsQueue = itr_dq->second;
			
			if(spEmsQueue.get() != NULL){
				spCpMail = spEmsQueue.get()->getCpMail();
				
				if(spCpMail.get() != NULL){
					addToWorkServerQ(spCpMail);
					iCount--;
					m_iDomainQueueMsgCount--;
				}
				++itr_dq;
			}
			
			//DomainQueue Next Step
			if(itr_dq == m_mapDomainQueueList.end()){
				itr_dq = m_mapDomainQueueList.begin();
				if(itr_dq == m_mapDomainQueueList.end()){
					strncpy(pDomainName, NULLDOMAINNAME, SZ_NAME);
					break;
				}
			}

			if(iCount > 0){
				continue;
			}
			else{
				strncpy(pDomainName, itr_dq->first.c_str(), SZ_NAME);
				break;
			}
		}		
	}
	catch(...){
	}

	return 1;
}

//-----------------------------------------------
// Check WorkServer Queue Size
//-----------------------------------------------
bool CEmsWorkerThreadPool::checkWorkServerQ(int maxMsgCount)
{
	if (m_spEmsQueue.get() == NULL)
		return false;
		
	if(maxMsgCount > m_spEmsQueue.get()->getQueueSize()){
		return true;
	}
	else
		return false;
}


//-----------------------------------------------
// Send to EmsWorkServer CCpMail data
//-----------------------------------------------
void CEmsWorkerThreadPool::addToWorkServerQ(boost::shared_ptr<CCpMail> spCpMail)
{
	spCpMail.get()->setMode(getThreadMode());
	m_spEmsQueue.get()->addCpMail(spCpMail);
	//gLog->Write("[%s][%d]---[%s][%s]", __FUNCTION__, __LINE__, spCpMail.get()->getMailInfo(MHEADER_TODOMAIN), spCpMail.get()->getMailInfo(MHEADER_QINDEX));
	//setSignal();
}


//-----------------------------------------------
// WorkServer와 공통으로 사용하는 큐를 설정
//-----------------------------------------------
bool CEmsWorkerThreadPool::setEmsQueue (shared_ptr<CEmsQueue> workServerEmsQueue)
{
	if(workServerEmsQueue.get() != NULL){
		m_spEmsQueue = workServerEmsQueue;
		shared_ptr<CEmsThread> spEmsThread;
		for(int i=0; i < m_iThreadCount; i++){
			spEmsThread = getThread(i);
			if(spEmsThread.get()!=NULL){
				spEmsThread.get()->setThreadPool(this);
			}
		}
	}
	return true;
}

//-----------------------------------------------
// Rabbitmq CMD Queue에 처리해야할 요청이 있는지 체크
//-----------------------------------------------
bool CEmsWorkerThreadPool::checkCMDQ ()
{
	return true;
}

//-----------------------------------------------
// cmdMsg 를 처리하는 프로세스 함수(임시로 string으로 type을 설정하였으나 차후 변경 예정(코드 넘버등...))
//-----------------------------------------------
int CEmsWorkerThreadPool::processCMD (string cmdMsg)
{
	return 0;
}

//-----------------------------------------------
//
//-----------------------------------------------
int CEmsWorkerThreadPool::procCMDPraser ()
{
	return 0;
}


//-----------------------------------------------
// 큐(Campaign) 처리 프로세스를 정지(대기) 시킨다.
//-----------------------------------------------
void CEmsWorkerThreadPool::pause()
{
	//shared_ptr<CEmsThread> spEmsThread;
	//for(int i=0; i < m_iThreadCount; i++){
	//	spEmsThread = getThread(i);
	//	if(spEmsThread.get()!=NULL){
	//		spEmsThread.get()->setPause();
	//	}
	//}
	m_bIsPause = true;
}
	
//-----------------------------------------------
// 큐(Campaign) 처리 프로세스를 다시 시작하도록 한다.
//-----------------------------------------------
void CEmsWorkerThreadPool::restart()
{
	shared_ptr<CEmsThread> spEmsThread;
	for(int i=0; i < m_iThreadCount; i++){
		spEmsThread = getThread(i);
		if(spEmsThread.get()!=NULL){
			spEmsThread.get()->setRestart();
		}
	}
}


void CEmsWorkerThreadPool::addToDomainQueueList(shared_ptr<CCpMail> spCpMail)
{
	if(spCpMail.get() != NULL){
		const char *pDomainName = spCpMail.get()->getMailInfo(MHEADER_TODOMAIN);
		if(pDomainName == NULL){
			return;
		}
		else {
			unordered_map<std::string, shared_ptr<CEmsQueue> >::iterator itr_dq;
			Lock();
			itr_dq = m_mapDomainQueueList.find(pDomainName);
			if(itr_dq != m_mapDomainQueueList.end()){
				//만약 도메인 큐가 존재한다면 삽입
				try{
					shared_ptr<CEmsQueue> tmpspEmsQueue = itr_dq->second;
					tmpspEmsQueue.get()->addCpMail(spCpMail);
					m_iDomainQueueMsgCount++;
				}
				catch(...){
				}
			}
			else{
				//만약 도메인 큐가 존재하지 않는다면 생성  & 삽입.
				shared_ptr<CEmsQueue> tmpspEmsQueue = shared_ptr<CEmsQueue>(new CEmsQueue);
				if(tmpspEmsQueue.get() != NULL){
					try{
						tmpspEmsQueue.get()->addCpMail(spCpMail);
						m_mapDomainQueueList.insert(std::pair<string, shared_ptr<CEmsQueue> >(pDomainName, tmpspEmsQueue));
						m_iDomainQueueMsgCount++;
					}
					catch(...){
					}
				}
			}
			Unlock();
		}
	}
}
