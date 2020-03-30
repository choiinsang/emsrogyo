#include <boost/shared_array.hpp>
#include "EmsConfig.h"
#include "EmsWorkerManager.h"
#include "EmsLog.h"

extern CEmsLog * gLog;

using namespace boost; 

CEmsWorkerManager * CEmsWorkerManager::m_pEmsWorkerManager = NULL;

CEmsWorkerManager::CEmsWorkerManager()
:	m_bRMQUsable          (false)
, m_CMDQueue            (NULL)
,	m_WorkServer          (NULL)
,	m_bIsStop             (false)
,	m_bIsRun              (false)	
,	m_bIsActiveCommonQueue(false)
{
	//memset(m_strCommonQueueName, '\0', (SZ_NAME+1));
	//memset(m_strServerNo,        '\0', (SZ_INT_NUMBER+1));
	
	setServerName(theEMSConfig()->getServerName());
	InitEmsDKIM ();
}


CEmsWorkerManager::~CEmsWorkerManager()
{
	if(!isStop()){ //Running State
		setStopState(true);
	}
}

CEmsWorkerManager * CEmsWorkerManager::getInstance()
{
	if(m_pEmsWorkerManager == NULL){
		m_pEmsWorkerManager = new CEmsWorkerManager();
	}
	return m_pEmsWorkerManager;
}


//--------------------------------------------
//CEmsWorkerManager 관리하기 위한 스레드 시작
//--------------------------------------------
void CEmsWorkerManager::startThread()
{
	// Message Queue 대기 상태로 전환(CMD 대기) - Wait Point!!
	bool bResult = false;
	do{
		try{
			bResult = connectEACMDQueue();
			if(bResult == false){
				throw AMQPException("EACMD Queue Connect Failed");				
			}
			bResult = createCMDQueue();
			if(bResult == false){
				throw AMQPException("CMD Queue Create Failed");
			}
		}
		catch(AMQPException ex){
			gLog->Write("[%s][%s][%d][%s]", __FILE__, __FUNCTION__, __LINE__, ex.getMessage().c_str());
			sleep(THREAD_RMQ_WAIT_TIME);
			continue;
		}
		catch(...){
			gLog->Write("[%s][%s][%d][ERROR][Unknown Error Occurred]", __FILE__, __FUNCTION__, __LINE__);
			sleep(THREAD_RMQ_WAIT_TIME);
			continue;
		}
		resetRMQConn();
		sleep(THREAD_RMQ_WAIT_TIME);
	}while(isStop()==false);

}


//--------------------------------------------
//Common Queue 처리를 위한 프로세스 설정
//--------------------------------------------
void CEmsWorkerManager::createCommonQueue(const char *strCommonQueueName)
{
	processCreateWorkerThread(strCommonQueueName, MODE_I);
}

//--------------------------------------------
// RabbitMQ [CMD] Message Parsing Function
//--------------------------------------------
bool CEmsWorkerManager::checkGroupNo(int iGroupNo)
{
	unordered_map< int, shared_ptr<stGroupInfo> >::iterator itr_map;
	itr_map = getGroupInfos()->find(iGroupNo);
	if(itr_map != getGroupInfos()->end()){
		return true;
	}
	return false;
}

//--------------------------------------------
// RabbitMQ [CMD] Message Parsing Function
//--------------------------------------------
bool CEmsWorkerManager::parseCMDMsg(AMQPMessage * message)
{
	bool retval = false;
	
	//parse CMD Message 
	if(message != NULL){
		
		int         iCmd_Type = -1;
		
		const char *cmd_Type       = NULL
		         , *cmd_IndexKey   = NULL
		         , *cmd_GroupNo    = NULL;
		
		cmd_Type       = message->getHeader(CMD_TYPE).c_str();
		cmd_IndexKey   = message->getHeader(CMD_INDEX_KEY).c_str();         // IEmsSerer에서는 Campaign_no가 IndexKey로 사용됨.
		cmd_GroupNo    = message->getHeader(CMD_SERVER_GROUPNUMBER).c_str();
		
		if(cmd_Type != NULL){
			int i=0;
			for(; i< CMD_TYPE_INFO_END; i++){
				if(strcmp(cmd_Type, CMD_TYPE_LIST[i]) == 0){
					iCmd_Type = i;
					break;
				}
			}
			
			if(i >= CMD_TYPE_INFO_END){
				gLog->Write("[%s][%s][%d][ERROR][CMD_TYPE_INFO IS NOT EXIST][cmd_Type:%s][%s]"
			          , __FILE__, __FUNCTION__, __LINE__, cmd_Type, cmd_IndexKey);
				return false;
			}
		}
		else{
			gLog->Write("[%s][%s][%d][ERROR][CMD PARSE ERROR][cmd_Type is NULL][%s]"
			          , __FILE__, __FUNCTION__, __LINE__,  cmd_IndexKey);
			return false;
		}
		
		//Command Message Process
		//-----------------------
		//1.Check Thread Run State
		//-----------------------
		if(this->isRun()==false){
			
			if(	iCmd_Type == CMD_TYPE_INFO_SET_SERVER_CONF){
				gLog->Write("[CMD_TYPE_SET_SERVER_CONF][%d][CMD:%d][GroupNo:%s]", __LINE__, iCmd_Type, cmd_GroupNo);
				processSetServerConf(message);
				// IEmsAgent에게 프로세스 리스트를 요청하자
				// 받은 리스트로 프로세스를 셋팅한 후 프로세스 처리
				int iGroupNo = atoi(cmd_GroupNo);
				sendReqProcList(iGroupNo);
			}
			return true;
		}
		
		//만약 다중 그룹설정인 경우
		if(	iCmd_Type == CMD_TYPE_INFO_SET_SERVER_CONF){
			const char *bMultiGroupConf = message->getHeader(CMD_SERVER_MULTI_GROUP).c_str();
			if(strcmp(bMultiGroupConf, CMD_YES)==0){
				gLog->Write("[CMD_TYPE_SET_SERVER_CONF][%d][CMD:%d][GroupNo:%s]", __LINE__, iCmd_Type, cmd_GroupNo);
				processSetServerConf(message);
				int iGroupNo = atoi(cmd_GroupNo);
				sendReqProcList(iGroupNo);
			}
			return true;
		}
		
		//2. Check Group Number
		//-----------------------
		int iGroupNo = atoi(cmd_GroupNo);

		if(checkGroupNo(iGroupNo) == false){
			return false;
		}
				
		//3. Process CMD Type
		//-----------------------
		if((iCmd_Type != -1) && (this->isRun() == true)){ // CMD_TYPE_PROC
			
			switch(iCmd_Type){
				case CMD_TYPE_INFO_SET_SERVER_CONF:{
					gLog->Write("[CMD_TYPE_SET_SERVER_CONF][CMD:%d]", iCmd_Type);
					processSetServerConf(message);
					break;
				}
				case CMD_TYPE_INFO_CREATE_QUEUE: {
					gLog->Write("[CMD_CREATE_QUEUE][CMD:%d]", iCmd_Type);
					processCreateWorkerThread(cmd_IndexKey, MODE_M);
					break;
				}
				case CMD_TYPE_INFO_INIT_CAMPAIGN: {
					gLog->Write("[CMD_INIT_CAMPAIGN][CMD:%d][QueueName:%s]", iCmd_Type, cmd_IndexKey);
					processInitMailCampaign(cmd_IndexKey, message);
					break;
				}
				case CMD_TYPE_INFO_DELETE_QUEUE: {
					gLog->Write("[CMD_DELETE_QUEUE][QueueName:%s]", cmd_IndexKey);
					processDeleteWorkerThread(cmd_IndexKey );					
					break;
				}
				case CMD_TYPE_INFO_PAUSE_QUEUE:  {
					gLog->Write("[CMD_TYPE_INFO_PAUSE_QUEUE][CMD:%d]", iCmd_Type);
					processPauseWorkerThread  (cmd_IndexKey);
					break;
				}
				case CMD_TYPE_INFO_RESTART_QUEUE:  {
					//stopWorkerThreads(cmdValue);
					gLog->Write("[CMD_TYPE_INFO_RESTART_QUEUE][CMD:%d]", iCmd_Type);
					break;
				}
				case CMD_TYPE_INFO_STOP_QUEUE:  {
					//stopWorkerThreads(cmdValue);
					gLog->Write("[CMD_STOP_QUEUE][CMD:%d]", iCmd_Type);
					break;
				}
				case CMD_TYPE_INFO_STOP_SERVER:  {
					//Server Stop 
					gLog->Write("[CMD_TYPE_INFO_STOP_SERVER][CMD:%d]", iCmd_Type);
					processServerStop();
					break;
				}
				case CMD_TYPE_INFO_CHK_PROCESSLIST: {
					//Check Process LIst
					processCheckList(message);
					//gLog->Write("[%s][%s][%d] CMD_TYPE_CHK_PROCESSLIST[CMD:%d]", __FILE__, __FUNCTION__, __LINE__, iCmd_Type);
					break;
				}
				case CMD_TYPE_INFO_CHK_CONN_HB: {
					//Keep CMD Connection
					break;
				}
				default:
					gLog->Write("[%s][%s][%d][CMD is Not Defined][CMD:%d]", __FILE__, __FUNCTION__, __LINE__, iCmd_Type);
			}
		}
		else{
			gLog->Write("[%s][%s][%d] [CMD:%d][RUNSTATE:%s]", __FILE__, __FUNCTION__, __LINE__, iCmd_Type, (this->isRun()==false?"Not Running":"Running"));
		}

		retval = true;
	}
	else{
		retval = false;
	}

	return retval;
}


//--------------------------------------------
// 큐(Campaign)이름으로 스레드 풀 생성/추가
//--------------------------------------------
void CEmsWorkerManager::processCreateWorkerThread(const char * qname, int iThreadMode, int nThreads){
	gLog->Write("[QName:%s][MODE:%d]", qname, iThreadMode);
	addToWorkersPool(qname, iThreadMode, nThreads);
	m_WorkServer->bindQueue(qname);
}

//--------------------------------------------
// 큐(Campaign)이름의 Campaign 정보 초기화
//--------------------------------------------
void CEmsWorkerManager::processInitMailCampaign(const char * qname, AMQPMessage *pMessage) //qname = Index Key
{
	gLog->Write("[CMD][INIT Campaign][IndexKey:%s]", qname);
	unordered_map< std::string, shared_ptr<CEmsWorkerThreadPool> >::iterator itr_map;
	itr_map = m_mapEmsWorkersPool.find(string(qname));
	
	if(itr_map == m_mapEmsWorkersPool.end()){         //'qname'의 WorkerThreadPool이 없음
		processCreateWorkerThread(qname, MODE_M);	

		itr_map = m_mapEmsWorkersPool.find(string(qname));  //생성 후 재검색
		if(itr_map == m_mapEmsWorkersPool.end()){
			gLog->Write("[CMD]Mail Queue Create Failed[KeyIndex:%s]", qname);
			return;  // 'qname'의 WorkerThreadPool 생성 실패
		}
	}

	shared_ptr<CEmsWorkerThreadPool> spWorkerThreadPool = itr_map->second;
	if(spWorkerThreadPool.get()->checkMailInfo() == false)
		spWorkerThreadPool.get()->setCpMailInfo(pMessage);  

}

//--------------------------------------------
// 프로세스 정지(qname이 CMD_ALL_INDEX 인 경우 모든 프로세스 정지)
// processDeleteWorkerThread() 할 경우 쓰레드를 그냥 종료 시켜버리므로 
// 서버에서 메시지 더이상 가져가지 못하도록 처리
// 기존 처리중이던 메시지는 정상 종료되도록 처리
// 클라이언트 큐에 더 이상 처리할 메시지(Client Connection)이 없다면 프로세스 (대기)종료
//--------------------------------------------
void CEmsWorkerManager::processServerStop()
{
	unordered_map<std::string, shared_ptr<CEmsWorkerThreadPool> >::iterator itr_wtp;
	shared_ptr<CEmsWorkerThreadPool>  tmpspEmsWorkersPool;
	itr_wtp = m_mapEmsWorkersPool.begin();
	for(; itr_wtp !=  m_mapEmsWorkersPool.end(); itr_wtp++){
		tmpspEmsWorkersPool = itr_wtp->second;
		if(tmpspEmsWorkersPool.get() != NULL){
			tmpspEmsWorkersPool.get()->setStopState(true);
		}
	}
}

//--------------------------------------------
// 처리중인 Campaign 리스트의 비교 갱신
//--------------------------------------------
void CEmsWorkerManager::processCheckList(AMQPMessage *pMessage)
{
	//Message에서 프로세스 리스트를 구한 다음
	//진행중인 프로세스 리스트와 맞지 않는다면 Ems Agent에 실행중인 리스트 정보를 다시 요청한다.
	uint32_t j            = 0;
	char       *pProcList = pMessage->getMessage(&j);
	int         listLen   = strlen(pProcList);
	const char *pGroupNo  = pMessage->getHeader(CMD_SERVER_GROUPNUMBER).c_str();
	int         iGroupNo  = atoi(pGroupNo);
	
	vector<string> recvProcList;
	vector<string> curProcList;
	char           tmpProcName[SZ_NAME];
	int            iPos   = 0;
	int            curPos = 0;
	
	//Process List 추출
	while(curPos <= listLen){
		for(; curPos < listLen; curPos++){
			if((pProcList[curPos] == CMD_DELIMITER) || (pProcList[curPos] == CMD_DELIMITER_END))
				break;
		}

		if(iPos != curPos){
			memset(tmpProcName, 0, SZ_NAME);
			memcpy(tmpProcName, &pProcList[iPos], (curPos-iPos));
			tmpProcName[curPos-iPos] = '\0';
			recvProcList.push_back(tmpProcName);

			if(pProcList[curPos] == CMD_DELIMITER_END)
				break;

			iPos = curPos+1;
			curPos++;
			if(iPos >= listLen)
				break;
		}
		else{
			iPos++;
			curPos++;
		}
	}

	if(recvProcList.empty()==true){
		if(m_mapEmsWorkersPool.empty()){
			//gLog->Write("[%s][%s][%d][recvProcList.empty]", __FILE__, __FUNCTION__, __LINE__);
			return;
		}
		else{ /* delete process list*/}
	}
	else{
		if(m_mapEmsWorkersPool.empty()){
			sendReqProcList(iGroupNo);
			gLog->Write("[%s][%s][%d][sendReqProcList()]", __FILE__, __FUNCTION__, __LINE__, pProcList);
			return;
		}
		else{
			gLog->Write("[%s][%s][%d][CList:%d][RList:%d]"
			            , __FILE__, __FUNCTION__, __LINE__, m_mapEmsWorkersPool.size(), recvProcList.size());
		}
	}
	
	//처리중인 Process List 추출
	int curCount  = 0;
	unordered_map< std::string, shared_ptr<CEmsWorkerThreadPool> >::iterator itr_map;
	itr_map = m_mapEmsWorkersPool.begin();
	
	for(;itr_map != m_mapEmsWorkersPool.end(); itr_map++){
		if((itr_map->second.get()->getThreadMode() == MODE_M) &&
			  (itr_map->second.get()->getGroupNo() == iGroupNo)){
			curProcList.push_back(itr_map->first);
			gLog->Write("CurrentList:%s, %s", itr_map->first.c_str(), itr_map->second.get()->getWorkerName());
		}
	}

	curCount = curProcList.size();
	
	//리스트 비교해서 recvProcList에는 있지만 curProcList에는 없는 목록 삭제,
	//두 리스트가 같지 않다면 프로세스 리스트 요청
	vector<string>::iterator itr_list1 = recvProcList.begin();
	vector<string>::iterator itr_list2;
	bool bExist = true;
	bool bCheck = false;

	//수신된 리스트로 현재 가지고 있는 리스트를 순회하면서 
	//중복된 리스를 삭제하여 현재 처리 중인 리스트를 만들어낸다.
	for(; itr_list1 != recvProcList.end(); itr_list1++){
		bCheck = false;
		for(itr_list2 = curProcList.begin(); itr_list2 != curProcList.end(); itr_list2++){
			//gLog->Write("ischoi==>[Queue Name][C:%s][R:%s]", itr_list1->c_str(), itr_list2->c_str());
			if(*itr_list1 == *itr_list2){
				bCheck = true;
				break;				
			}
		}
		
		if(bCheck==true){ //exist
			curProcList.erase(itr_list2);
			//gLog->Write("ischoi==>[Delete QName][C:%s]", itr_list2->c_str());
		}
		else{ //not exist
			bExist = false;
		}
	}
	
	if(curProcList.size() > 0){
		// 처리 완료 되었거나 현재 작업중이지 않는 리스트를 작업 리스트에서 삭제하도록 함
		itr_list1 = curProcList.begin();
		itr_map   = m_mapEmsWorkersPool.begin();
		
		Lock();
		try{
			for(; itr_list1 != curProcList.end(); itr_list1++){
				itr_map   = m_mapEmsWorkersPool.find(*itr_list1);
				if(itr_map != m_mapEmsWorkersPool.end()){
					m_mapEmsWorkersPool_Del.insert(std::pair<std::string, shared_ptr<CEmsWorkerThreadPool> >(itr_map->first, itr_map->second));
					m_mapEmsWorkersPool.erase(itr_map);
				}
			}
		}
		catch(...){
			gLog->Write("[%s][%d][ERROR][Unknown Error Occurred:%s]", __FUNCTION__, __LINE__, itr_map->first.c_str());
		}
		Unlock();
	}
	
	if (bExist == false){
		// Proc 리스트를 요청
		sendReqProcList(iGroupNo);
		return;
	}

}

//--------------------------------------------
// Group Name 설정
//--------------------------------------------
void CEmsWorkerManager::bindGroupName(const char *pGroupName)
{
	if(pGroupName != NULL){
		if(m_CMDQueue != NULL){
			m_CMDQueue->Bind(RMQ_EXCHANGE_CMD, pGroupName);
			gLog->Write("[CMD][Set Group Name is %s]", pGroupName);
		}
	}
	else{
		gLog->Write("[%s][%s][%d][ERROR][CMD][Set Group Name is NULL]", __FILE__, __FUNCTION__, __LINE__);
	}
}

//--------------------------------------------
// Server Number 설정(char 자료형으로 저장한다)
//--------------------------------------------
//void CEmsWorkerManager::setServerNo(const char *pchServerNo)
//{ 
//	memset(m_strServerNo, '\0', SZ_INT_NUMBER+1);
//	strncpy(m_strServerNo, pchServerNo, SZ_INT_NUMBER);
//}

//--------------------------------------------
// 처리중인 Campaign 리스트의 비교 갱신
//--------------------------------------------
void CEmsWorkerManager::processSetServerConf(AMQPMessage *pMessage)
{
	bool        bServerStop  = false;
	const char *pGroupNo     = NULL;
	const char *pGroupName   = NULL;
	const char *pCommonQName = NULL;
	const char *pServerNo    = NULL;
	const char *pServerName  = NULL;
	const char *pServerIP    = NULL;

	try{
		int iGroupNo;
		pGroupNo     = pMessage->getHeader(CMD_SERVER_GROUPNUMBER).c_str();
		if(pGroupNo != NULL){
			iGroupNo = atoi(pGroupNo);
		}
		else{
			return;
		}
		pGroupName   = pMessage->getHeader(CMD_SERVER_GROUPNAME).c_str();
		pCommonQName = pMessage->getHeader(CMD_SERVER_COMMONQ).c_str();
		pServerNo    = pMessage->getHeader(CMD_SERVER_NUMBER).c_str();
		pServerName  = pMessage->getHeader(CMD_SERVER_NAME).c_str();
		pServerIP    = pMessage->getHeader(CMD_SERVER_IPADDR).c_str();
						
		gLog->Write("[RECV][CONFIG_INFO][Group_No:%s][Group_Name:%s][COMMONQ:%s][SERVER STATE %s]"
		       , pGroupNo, pGroupName, pCommonQName, pMessage->getHeader(CMD_SERVER_STATE).c_str());
		       	
		if((strcmp(pServerName, theEMSConfig()->getServerName())!=0)||(strcmp(pServerIP, theEMSConfig()->getServerIPAddr())!=0))
			return;
		
		bServerStop = strcmp(SERVER_STATE_ENABLE, pMessage->getHeader(CMD_SERVER_STATE).c_str())==0?false:true; //enable=false, disable=true
		setStopState(bServerStop);	
		
		//입수된 서버의 Group 정보를 리스트로 관리하도록 한다.
		shared_ptr<stGroupInfo> spGroupInfo = shared_ptr<stGroupInfo>(new stGroupInfo(iGroupNo, pGroupName, pCommonQName));
		if(spGroupInfo.get() != NULL){
			spGroupInfo.get()->getServerList()->insert(std::pair<int, stServerInfo>(atoi(pServerNo), stServerInfo(atoi(pServerNo), pServerName)));
			m_mapGroupInfo.insert(std::pair<int, shared_ptr<stGroupInfo> >(atoi(pGroupNo), spGroupInfo));
			bindGroupName(pGroupName);
			createCommonQueue(pCommonQName);
			setRunState(true);
		}
		else
			return ;
	}
	catch(...){
		setRunState(false);
	}
	
	if(isRun() == true){
		sendHeartBeat();
	}
	
	gLog->Write("[Set Config CMD Message][Group_No:%s][COMMONQ:%s][SERVER STATE %s]", pGroupNo, pCommonQName, bServerStop?"STOP":"RUN");
}

//--------------------------------------------
// 큐(Campaign)이름으로 스레드 풀 삭제/제거
//--------------------------------------------
void CEmsWorkerManager::processDeleteWorkerThread(const char * qname){

	if(qname != NULL){
		unordered_map< std::string, shared_ptr<CEmsWorkerThreadPool> >::iterator itr_map;
		itr_map = m_mapEmsWorkersPool.find(qname);

		Lock();
		try{
			if(itr_map != m_mapEmsWorkersPool.end()){
				itr_map->second.get()->setStopState(true);
				m_mapEmsWorkersPool_Del.insert(std::pair<std::string, shared_ptr<CEmsWorkerThreadPool> >(itr_map->first, itr_map->second));
				m_mapEmsWorkersPool.erase(itr_map);
			}
		}
		catch(...){
			gLog->Write("[%s][%d][ERROR][Unknown Error Occurred:%s]", __FUNCTION__, __LINE__, qname);
		}
		Unlock();
		gLog->Write("[DELETE Queue][Name:%s]", qname);
	}
	else{
		gLog->Write("[NO Queue][Name:%s]", qname);
	}
}


//--------------------------------------------
// 큐(Campaign)이름 스레드 프로세스 일시 정지
//--------------------------------------------
void CEmsWorkerManager::processPauseWorkerThread  (const char * qname)
{
	if(qname != NULL){
		unordered_map< std::string, shared_ptr<CEmsWorkerThreadPool> >::iterator itr_map;
		itr_map = m_mapEmsWorkersPool.find(string(qname));
		
		Lock();
		try{
			if(itr_map != m_mapEmsWorkersPool.end()){
				shared_ptr<CEmsWorkerThreadPool> tmpspEmsThreadPool = itr_map->second;
				tmpspEmsThreadPool.get()->pause();
				tmpspEmsThreadPool.get()->setStopState(true);
				m_mapEmsWorkersPool_Del.insert(std::pair<std::string, shared_ptr<CEmsWorkerThreadPool> >(itr_map->first, tmpspEmsThreadPool));
				m_mapEmsWorkersPool.erase(itr_map);
			}
		}
		catch(...){
			gLog->Write("[%s][%d][ERROR][Unknown Error Occurred:%s]", __FUNCTION__, __LINE__, qname);
		}
		Unlock();
		gLog->Write("[PAUSE Queue][Name:%s]", qname);
	}
	else{
		gLog->Write("[NO Queue][Name:%s]", qname);
	}
}


//--------------------------------------------
// 큐(Campaign)이름의 스레드 재시작 
//--------------------------------------------
void CEmsWorkerManager::processrestartWorkerThread (const char * qname)
{
	unordered_map< std::string, shared_ptr<CEmsWorkerThreadPool> >::iterator itr_map;
	itr_map = m_mapEmsWorkersPool.find(string(qname));
	if(itr_map == m_mapEmsWorkersPool.end()){
		//'qname'의 WorkerThreadPool이 존재하지 않음
	}
	else{
		shared_ptr<CEmsWorkerThreadPool> tmpEmsWorkerThreadPool = itr_map->second;
		tmpEmsWorkerThreadPool.get()->restart();
		gLog->Write("[RESTART Queue][Name:%s]", qname);
	}
}

//============================================
// EMS SERVER CMD QUEUE
//--------------------------------------------
// CMD Queue Event Setting 
//--------------------------------------------
int CEmsWorkerManager::onCancel(AMQPMessage * message ) {
	gLog->Write("[RabbitMQ Cancel Tag][Delivery TAG:%d]", message->getDeliveryTag());

	return 0;
}

//--------------------------------------------
// Event로 Message를 받아서 처리
//--------------------------------------------
int CEmsWorkerManager::onMessage( AMQPMessage * message ) {
	if(message != NULL){
		uint32_t j      = 0;
		char    *cmdmsg = message->getMessage(&j);
	
		if(cmdmsg != NULL){
			//---------[CMD Message 처리]------------
	   	theWorkerManager()->parseCMDMsg(message);
	    //---------------------------------------
		}
	}
	return 0;
};


//=========================
// SEND TO EMS AGENT SERVER
//=========================
//--------------------------------------------
// EmsServer Server HeartBeat Message
//--------------------------------------------
bool CEmsWorkerManager::getMsgHeaderInfo(int iMsgType, unordered_map<string, string> *pMapCmdHeaderInfo, int iGroupNo, int iServerNo)
{
	char strMsgType[10]={0,};
	sprintf(strMsgType, "%d", iMsgType);

	pMapCmdHeaderInfo->insert(std::pair<string, string>(MSGHEADER_TYPE, strMsgType));
	
	if( (iMsgType == MSG_TYPE_REQ_GET) 
		||(iMsgType == MSG_TYPE_HEARTBEAT)){
		const char *pServerName   = theEMSConfig()->getServerName();
		const char *pServerIPAddr = theEMSConfig()->getServerIPAddr();

		if(pServerName == NULL || pServerIPAddr == NULL){
			return false;
		}
		else{
			pMapCmdHeaderInfo->insert(std::pair<string, string>(MSGHEADER_SERVER_NAME,   pServerName));
			pMapCmdHeaderInfo->insert(std::pair<string, string>(MSGHEADER_SERVER_IP,     pServerIPAddr));
	
			if(iMsgType == MSG_TYPE_HEARTBEAT){
				char strNo[SZ_INT_NUMBER+1]={0,};
				snprintf(strNo, SZ_INT_NUMBER, "%d", iGroupNo);
				pMapCmdHeaderInfo->insert(std::pair<string, string>(MSGHEADER_GROUPNUMBER, string(strNo)));
				memset(strNo, 0, SZ_INT_NUMBER+1);
				snprintf(strNo, SZ_INT_NUMBER, "%d", iServerNo);
				pMapCmdHeaderInfo->insert(std::pair<string, string>(MSGHEADER_SERVER_NUMBER, string(strNo)));
			}
		}
	}
	
	return true;
}

//--------------------------------------------
//메시지 전송을 위해 Header정보를 입력
//--------------------------------------------
bool CEmsWorkerManager::setEACMDMsgHeader(unordered_map<string, string> *pInfomap)
{
	if(m_pEACMDExchange == NULL){
		gLog->Write("[%s][%s][%d][ERROR] Message Header Information is NULL!", __FILE__, __FUNCTION__, __LINE__);
		return false;
	}

	unordered_map<string, string>::iterator itr_map = pInfomap->begin();
	int iCount = 0;
	for(; itr_map != pInfomap->end(); itr_map++, iCount++){
		m_pEACMDExchange->setHeader(itr_map->first.c_str(), itr_map->second.c_str(), true);
	}
		
	if(iCount > 0 ){
		return true;
	}
	else{
		return false;
	}
}

//--------------------------------------------
//EACMD Queue에 메시지 전송 (메시지 생성하여 전송)
//--------------------------------------------
void CEmsWorkerManager::sendToEACMDMQ(unordered_map<string, string> *pMsgInfoMap, const char *l_pszFMT, ... )
{
	char strCmdMsg[MAX_LOG];

	va_list args;
	va_start(args, l_pszFMT);
	vsnprintf(strCmdMsg, MAX_LOG - 1, l_pszFMT, args);
	va_end(args);
	
	if(setEACMDMsgHeader(pMsgInfoMap) == true){
		m_pEACMDExchange->Publish(strCmdMsg, RMQ_EXCHANGE_EA_CMD);  
	}
}


//================================
// HEADER TYPE: MSG_TYPE_REQ_GET
//================================
//--------------------------------------------
// Config 설정 요청 -> EmsAgent CMD
//--------------------------------------------
bool CEmsWorkerManager::sendReqServerConfigInfo()
{
	if(getRMQUsable() == false)
		return false;

	unordered_map<string, string> mapMsgHeaderInfo;
	
	if(getMsgHeaderInfo(MSG_TYPE_REQ_GET, &mapMsgHeaderInfo) == true){
		char chType[16]={0,};
		sprintf(chType, "%d", GET_SERVER_CONF);
		mapMsgHeaderInfo.insert(std::pair<string, string>(MSGHEADER_GETMSG_TYPE, chType));
			
		try{
			sendToEACMDMQ(&mapMsgHeaderInfo, CMD_TYPE_SET_SERVER_CONF);
			return true;
		}
		catch(AMQPException ex){
			gLog->Write("[%s][%s][%d][ERROR][MSG:%s]", __FILE__, __FUNCTION__, __LINE__, ex.getMessage().c_str());
			return false;
		}
		catch(...){
			gLog->Write("[%s][%s][%d][ERROR][Unknown Error Occurred]", __FILE__, __FUNCTION__, __LINE__);
			return false;
		}
	}
	return true;
}

//--------------------------------------------
// EmsServer Send Start Signal Message to EmsAgent
//--------------------------------------------
bool CEmsWorkerManager::sendReqProcList(int iGroupNo)  // Group로 프로세스 리스트 요청
{
	if(getRMQUsable() == false)
		return false;

	unordered_map<string, string> mapMsgHeaderInfo;
	
	if(getMsgHeaderInfo(MSG_TYPE_REQ_GET, &mapMsgHeaderInfo, iGroupNo) == true){
		char chType[16]={0,};
		sprintf(chType, "%d", GET_PROC_LIST);
		mapMsgHeaderInfo.insert(std::pair<string, string>(MSGHEADER_GETMSG_TYPE, chType));
		//gLog->Write("[%s][%s][%d][MSGHEADER_TYPE:%s]", __FILE__, __FUNCTION__, __LINE__, chType);

		try{
			sendToEACMDMQ(&mapMsgHeaderInfo, CMD_TYPE_CHK_PROCESSLIST);
			return true;
		}
		catch(AMQPException ex){
			gLog->Write("[%s][%s][%d][ERROR][MSG:%s]", __FILE__, __FUNCTION__, __LINE__, ex.getMessage().c_str());
			return false;
		}
		catch(...){
			gLog->Write("[%s][%s][%d][ERROR][Unknown Error Occurred]", __FILE__, __FUNCTION__, __LINE__);
			return false;
		}
	}
	return true;
}

//================================
// HEADER TYPE: MSG_TYPE_HEARTBEAT
//================================
//--------------------------------------------
// EmsServer Send Heartbeat Message to EmsAgent
//--------------------------------------------
bool CEmsWorkerManager::sendHeartBeat()
{
	if(getRMQUsable() == false)
		return false;

	unordered_map<string, string> mapMsgHeaderInfo;
	
	//map 반복해서 GroupInfo에 들어있는 정보를 전달한다.
	unordered_map<int, shared_ptr<stGroupInfo> > *pGroupInfo = getGroupInfos();
	unordered_map<int, shared_ptr<stGroupInfo> >::iterator itr_gi =  pGroupInfo->begin();
	shared_ptr<stGroupInfo> spGroupInfo;
	unordered_map<int, stServerInfo> *getServerList      ();
	int iGroupNo;
	int iServerNo;

	for(; itr_gi!=pGroupInfo->end();itr_gi++){
		mapMsgHeaderInfo.clear();
		iGroupNo    = itr_gi->first;
		spGroupInfo = itr_gi->second;
		if(spGroupInfo.get()->getServerList()->size() > 0)
			iServerNo   = spGroupInfo.get()->getServerList()->begin()->first;
		else
			continue;
		if(getMsgHeaderInfo(MSG_TYPE_HEARTBEAT, &mapMsgHeaderInfo, iGroupNo, iServerNo) == true){
			try{
				sendToEACMDMQ(&mapMsgHeaderInfo, "Heartbeat");
			}
			catch(AMQPException ex){
				gLog->Write("[%s][%s][%d][ERROR][MSG:%s]", __FILE__, __FUNCTION__, __LINE__, ex.getMessage().c_str());
			}
			catch(...){
				gLog->Write("[%s][%s][%d][ERROR][Unknown Error Occurred]", __FILE__, __FUNCTION__, __LINE__);
			}
		}
	}
	return true;
}


//--------------------------------------------
// EmsServer CMD Queue를 설정/연결
//--------------------------------------------
bool CEmsWorkerManager::createCMDQueue()
{
	bool retval = false;

	if(m_CMDQueue != NULL){
		//이미 CMD 큐가 생성되어있는 상태
		retval = true;
	}
	else{ // m_CMDQueue가 NULL인 경우 새로 설정한다.
		
		const char * pServerName = getServerName();
		if(pServerName == NULL){
			gLog->Write("[%s][%s][%d][ERROR][Check Server Name][IEmsServer.ini]", __FILE__, __FUNCTION__, __LINE__);
			exit(-1);
		}
		else{  // SERVER_NAME 이 CMD를 처리하기위한 Queue 이름으로 사용된다.
			//CMD Queue 생성
			m_CMDQueue = theEmsRMQManager()->getQueue(RMQ_EXCHANGE_CMD, pServerName, false);

			if(m_CMDQueue != NULL){
				int ret = -1;
				do{
					if(isRun()==true)
						break;
						
					if((ret = checkServerConfigStart()) == 0)
						break;
					else
						sleep(1);
				}while(true);
				
				setRMQUsable(true);
			
				// CMD를 수신 Queue 연결
				// WorkerThread에 필요한 작업을 전달할 수 있다.
				m_CMDQueue->addEvent(AMQP_MESSAGE, onMessage);
				m_CMDQueue->addEvent(AMQP_CANCEL,  onCancel );
				m_CMDQueue->Consume(AMQP_NOACK);
				throw AMQPException("RMQConnection Error Exception Occurred");
			}
			else{ // if(retval==false){
				gLog->Write("[%s][%s][%d] CMD Queue Pointer Get Failed... ", __FILE__, __FUNCTION__,  __LINE__);
				exit(0);
			}
		}
	}

	return retval;
}


//--------------------------------------------
// EmsServer CMD Queue 연결
//--------------------------------------------
bool CEmsWorkerManager::connectEACMDQueue()
{
	m_pEACMDExchange = theEmsRMQManager()->getExchange(RMQ_EXCHANGE_EA_CMD, RMQ_ROUTE_KEY_EA_CMD, true);
	gLog->Write("[Create EACMD Queue %s]", (m_pEACMDExchange!=NULL)?"SUCCESS":"FAILED" );

	return (m_pEACMDExchange!=NULL)?true:false;
}

//--------------------------------------------
// EmsServer Queue 연결 리셋
//--------------------------------------------
void CEmsWorkerManager::resetRMQConn()
{
	theEmsRMQManager()->resetRMQConInfo();
	gLog->Write("[%s][%s][%d]", __FILE__, __FUNCTION__, __LINE__);
	theEmsRMQManager()->deleteRMQConn(RMQ_EXCHANGE_CMD,    getServerName());
	theEmsRMQManager()->deleteRMQConn(RMQ_EXCHANGE_EA_CMD, RMQ_ROUTE_KEY_EA_CMD, false);
	
	m_CMDQueue       = NULL;
	m_pEACMDExchange = NULL;

	setRMQUsable(false);
}

//--------------------------------------------
// 큐(Campaign)이름으로 스레드 풀 검색
//--------------------------------------------
shared_ptr<CEmsWorkerThreadPool> CEmsWorkerManager::getWorkersPool(const char * qname)
{
	shared_ptr<CEmsWorkerThreadPool> tmpEmsWorkerThreadPool;
	bool retval =  false;
	unordered_map< std::string, shared_ptr<CEmsWorkerThreadPool> >::iterator itr_map = m_mapEmsWorkersPool.find(string(qname));

	if(itr_map != m_mapEmsWorkersPool.end()){
		tmpEmsWorkerThreadPool = itr_map->second;
		retval = true;
	}

	return tmpEmsWorkerThreadPool;
}


//--------------------------------------------
// CMD Queue Process Function
//--------------------------------------------
// 큐(Campaign)이름으로 스레드 풀 생성/추가
void CEmsWorkerManager::addToWorkersPool  (const char * qname, int iThreadMode, int nThreads)
{
	shared_ptr<CEmsWorkerThreadPool> tmpEmsWorkerThreadPool = getWorkersPool(qname);
	if(tmpEmsWorkerThreadPool.get() != NULL){ //NULL이 아닌경우
		gLog->Write("[Exist][QName:%s][NODE:%d]", qname, iThreadMode);
	}
	else{ //없는 경우
		gLog->Write("[%s][%s][%d][WorkThreadPool is NULL][NODE:%d][QName:%s]", __FILE__, __FUNCTION__, __LINE__, iThreadMode, qname);

		// Rabbitmq 연결이 있는지 확인하고 없으면 새로 연결하여  ThreadPool 생성
		//set Rabbit MQ connection
		AMQPQueue * amqpQueue;
		if(iThreadMode == MODE_I){
			amqpQueue= theEmsRMQManager()->getQueue(RMQ_EXCHANGE_COMMON, qname);
		}
		else { //if(iThreadMode == MODE_M)
			amqpQueue= theEmsRMQManager()->getQueue(RMQ_EXCHANGE_MAIL, qname);
		}
		gLog->Write("[Add To Worker Pool][QName:%s][QMode:%s]", qname, (iThreadMode==MODE_I?"MODE_I":"MODE_M"));
				 
		if(amqpQueue != NULL){
			shared_ptr<CEmsWorkerThreadPool> tmpWorkerThreadPool = shared_ptr<CEmsWorkerThreadPool>( new CEmsWorkerThreadPool(qname, &m_mapCEmsDKIMList, iThreadMode, nThreads)); 
			m_mapEmsWorkersPool.insert(std::pair<std::string, shared_ptr<CEmsWorkerThreadPool> >(qname, tmpWorkerThreadPool));
			tmpWorkerThreadPool.get()->startThread();
		}
	}
}


//--------------------------------------------
// CEmsWorkServer 설정
//(epoll 처리되는 클라이언트 메시지 처리를 위한 큐를 공유 하고있다)
//--------------------------------------------
void CEmsWorkerManager::setWorkerServer (CEmsWorkServer *pServer)
{
	if(pServer != NULL){
		m_WorkServer = pServer;
	}
	else
		gLog->Write("[%s][%s][%d] WorkServer is NULL. Set Failed.", __FILE__, __FUNCTION__, __LINE__);
}


//--------------------------------------------
//CheckEmsWorkServer Client is Null
//--------------------------------------------
bool CEmsWorkerManager::IsNullWorkerServerClient()
{
	if(m_WorkServer == NULL)
		return true;
	return m_WorkServer->IsNullClientCount();
}


//--------------------------------------------
//EMS DKIM 초기화
//(Ems DKIM Setting Initialization)
//--------------------------------------------
void CEmsWorkerManager::InitEmsDKIM ()
{
	//설정파일에서 가져온 DKIM  리스트정보를 이용하여 EmsDKIM 초기화
	
	vector<stDKIMInfo>  *pEmsDKIMInfoList = theEMSConfig()->getDKIMList();
	shared_ptr<CEmsDKIM> spEmsDKIM;
	
	dkim_alg_t  signalg;
	string      pKeyPath;
	string      pSelector;
	string      pDomainName;
	string      pHeaderCanon;
	string      pBodyCanon;
	
	vector<stDKIMInfo>::iterator itr_info = pEmsDKIMInfoList->begin();
	for(; itr_info != pEmsDKIMInfoList->end(); itr_info++){
		signalg      = itr_info->_iDKIMSignAlg;
		pKeyPath     = itr_info->_chDKIMKeyPath;
		pSelector    = itr_info->_chDKIMSelector;
		pDomainName  = itr_info->_chDKIMDomainName;
		pHeaderCanon = itr_info->_chDKIMHeaderCanon;
		pBodyCanon   = itr_info->_chDKIMBodyCanon;
		
		spEmsDKIM = shared_ptr<CEmsDKIM>(new CEmsDKIM(pKeyPath.c_str()
		                                            , pSelector.c_str()
		                                            , pDomainName.c_str()
		                                            , pHeaderCanon.c_str()
		                                            , pBodyCanon.c_str()
		                                            , (dkim_alg_t)signalg));
		m_mapCEmsDKIMList.insert(std::pair<std::string, shared_ptr<CEmsDKIM> >(pDomainName, spEmsDKIM));

		gLog->Write("[DKIM_INIT][KeyPath:%s][Selector:%s][DomainName:%s][HCanon:%s][BCanon:%s][signalg:%d]"
		                       , pKeyPath.c_str(), pSelector.c_str(), pDomainName.c_str(), pHeaderCanon.c_str(), pBodyCanon.c_str(), signalg);
	}	
}


// Config 를 설정하는  thread를 생성하여 설정이 완료 될때까지 대기하도록 한다.
int CEmsWorkerManager::checkServerConfigStart()
{
	gLog->Write("CEmsWorkerManager Thread Create Success!");
	return pthread_create(&m_chkThread, NULL, &ChkThreadFunc, this);
}


//Config 설정 정보 요청을 주기적으로 보내는 Thread
void *CEmsWorkerManager::ChkThreadFunc(void *param)
{
	CEmsWorkerManager *pWorkerManager = (CEmsWorkerManager *)param;
	unordered_map<std::string, shared_ptr<CEmsWorkerThreadPool> > * pmapEmsWorkersPool_Del;

	time_t t_prev;
	time_t t_curr;
	double iDiffTime=-1;
	time(&t_prev);

	while(true){

		if(pWorkerManager->getRMQUsable() == false){
			sleep(1);
			continue;
		}

		try{
			time(&t_curr);
			iDiffTime = difftime(t_curr, t_prev);
			
			if(pWorkerManager->isRun()==false){
				if(iDiffTime <= REQ_CONFIG_SEND_PERIOD){
					sleep(1);
					continue;
				}
					
				time(&t_prev);
				if(pWorkerManager->sendReqServerConfigInfo() == false){
					sleep(1);
					continue;
				}
			}
			else {//send HeartBeat To IEmsAgent Server
				if(iDiffTime >= REQ_CONFIG_SEND_PERIOD){
					time(&t_prev);
					pWorkerManager->sendHeartBeat();
				}

				//check End EmsWorkerPool List
				if(pWorkerManager->getWorkersPoolMap_Del()->size() > 0 ){
					pmapEmsWorkersPool_Del = pWorkerManager->getWorkersPoolMap_Del();
					if(pmapEmsWorkersPool_Del->size() > 0 ){
						unordered_map<std::string, shared_ptr<CEmsWorkerThreadPool> >::iterator itr_end;

						pWorkerManager->Lock();
						try{
							itr_end = pmapEmsWorkersPool_Del->begin();
							gLog->Write("[%s][%s][%d][Delete QName][%s]", __FILE__, __FUNCTION__, __LINE__, itr_end->first.c_str());
							itr_end->second.get()->setStopState(true);
							pmapEmsWorkersPool_Del->erase(itr_end);
						}
						catch(...){
						}
						pWorkerManager->Unlock();
					}
				}
			}
			sleep(1);
		}
		catch(AMQPException ex){
			gLog->Write("[%s][%s][%d][ERROR][AMQPException][%s]", __FILE__, __FUNCTION__, __LINE__, ex.getMessage().c_str());
		}
		catch(...){
			gLog->Write("[%s][%s][%d][ERROR][Unknown Error Occurred]", __FILE__, __FUNCTION__, __LINE__);
		}
	}
	pthread_exit((void*)NULL);
}

