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
//CEmsWorkerManager �����ϱ� ���� ������ ����
//--------------------------------------------
void CEmsWorkerManager::startThread()
{
	// Message Queue ��� ���·� ��ȯ(CMD ���) - Wait Point!!
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
//Common Queue ó���� ���� ���μ��� ����
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
		cmd_IndexKey   = message->getHeader(CMD_INDEX_KEY).c_str();         // IEmsSerer������ Campaign_no�� IndexKey�� ����.
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
				// IEmsAgent���� ���μ��� ����Ʈ�� ��û����
				// ���� ����Ʈ�� ���μ����� ������ �� ���μ��� ó��
				int iGroupNo = atoi(cmd_GroupNo);
				sendReqProcList(iGroupNo);
			}
			return true;
		}
		
		//���� ���� �׷켳���� ���
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
// ť(Campaign)�̸����� ������ Ǯ ����/�߰�
//--------------------------------------------
void CEmsWorkerManager::processCreateWorkerThread(const char * qname, int iThreadMode, int nThreads){
	gLog->Write("[QName:%s][MODE:%d]", qname, iThreadMode);
	addToWorkersPool(qname, iThreadMode, nThreads);
	m_WorkServer->bindQueue(qname);
}

//--------------------------------------------
// ť(Campaign)�̸��� Campaign ���� �ʱ�ȭ
//--------------------------------------------
void CEmsWorkerManager::processInitMailCampaign(const char * qname, AMQPMessage *pMessage) //qname = Index Key
{
	gLog->Write("[CMD][INIT Campaign][IndexKey:%s]", qname);
	unordered_map< std::string, shared_ptr<CEmsWorkerThreadPool> >::iterator itr_map;
	itr_map = m_mapEmsWorkersPool.find(string(qname));
	
	if(itr_map == m_mapEmsWorkersPool.end()){         //'qname'�� WorkerThreadPool�� ����
		processCreateWorkerThread(qname, MODE_M);	

		itr_map = m_mapEmsWorkersPool.find(string(qname));  //���� �� ��˻�
		if(itr_map == m_mapEmsWorkersPool.end()){
			gLog->Write("[CMD]Mail Queue Create Failed[KeyIndex:%s]", qname);
			return;  // 'qname'�� WorkerThreadPool ���� ����
		}
	}

	shared_ptr<CEmsWorkerThreadPool> spWorkerThreadPool = itr_map->second;
	if(spWorkerThreadPool.get()->checkMailInfo() == false)
		spWorkerThreadPool.get()->setCpMailInfo(pMessage);  

}

//--------------------------------------------
// ���μ��� ����(qname�� CMD_ALL_INDEX �� ��� ��� ���μ��� ����)
// processDeleteWorkerThread() �� ��� �����带 �׳� ���� ���ѹ����Ƿ� 
// �������� �޽��� ���̻� �������� ���ϵ��� ó��
// ���� ó�����̴� �޽����� ���� ����ǵ��� ó��
// Ŭ���̾�Ʈ ť�� �� �̻� ó���� �޽���(Client Connection)�� ���ٸ� ���μ��� (���)����
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
// ó������ Campaign ����Ʈ�� �� ����
//--------------------------------------------
void CEmsWorkerManager::processCheckList(AMQPMessage *pMessage)
{
	//Message���� ���μ��� ����Ʈ�� ���� ����
	//�������� ���μ��� ����Ʈ�� ���� �ʴ´ٸ� Ems Agent�� �������� ����Ʈ ������ �ٽ� ��û�Ѵ�.
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
	
	//Process List ����
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
	
	//ó������ Process List ����
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
	
	//����Ʈ ���ؼ� recvProcList���� ������ curProcList���� ���� ��� ����,
	//�� ����Ʈ�� ���� �ʴٸ� ���μ��� ����Ʈ ��û
	vector<string>::iterator itr_list1 = recvProcList.begin();
	vector<string>::iterator itr_list2;
	bool bExist = true;
	bool bCheck = false;

	//���ŵ� ����Ʈ�� ���� ������ �ִ� ����Ʈ�� ��ȸ�ϸ鼭 
	//�ߺ��� ������ �����Ͽ� ���� ó�� ���� ����Ʈ�� ������.
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
		// ó�� �Ϸ� �Ǿ��ų� ���� �۾������� �ʴ� ����Ʈ�� �۾� ����Ʈ���� �����ϵ��� ��
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
		// Proc ����Ʈ�� ��û
		sendReqProcList(iGroupNo);
		return;
	}

}

//--------------------------------------------
// Group Name ����
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
// Server Number ����(char �ڷ������� �����Ѵ�)
//--------------------------------------------
//void CEmsWorkerManager::setServerNo(const char *pchServerNo)
//{ 
//	memset(m_strServerNo, '\0', SZ_INT_NUMBER+1);
//	strncpy(m_strServerNo, pchServerNo, SZ_INT_NUMBER);
//}

//--------------------------------------------
// ó������ Campaign ����Ʈ�� �� ����
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
		
		//�Լ��� ������ Group ������ ����Ʈ�� �����ϵ��� �Ѵ�.
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
// ť(Campaign)�̸����� ������ Ǯ ����/����
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
// ť(Campaign)�̸� ������ ���μ��� �Ͻ� ����
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
// ť(Campaign)�̸��� ������ ����� 
//--------------------------------------------
void CEmsWorkerManager::processrestartWorkerThread (const char * qname)
{
	unordered_map< std::string, shared_ptr<CEmsWorkerThreadPool> >::iterator itr_map;
	itr_map = m_mapEmsWorkersPool.find(string(qname));
	if(itr_map == m_mapEmsWorkersPool.end()){
		//'qname'�� WorkerThreadPool�� �������� ����
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
// Event�� Message�� �޾Ƽ� ó��
//--------------------------------------------
int CEmsWorkerManager::onMessage( AMQPMessage * message ) {
	if(message != NULL){
		uint32_t j      = 0;
		char    *cmdmsg = message->getMessage(&j);
	
		if(cmdmsg != NULL){
			//---------[CMD Message ó��]------------
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
//�޽��� ������ ���� Header������ �Է�
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
//EACMD Queue�� �޽��� ���� (�޽��� �����Ͽ� ����)
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
// Config ���� ��û -> EmsAgent CMD
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
bool CEmsWorkerManager::sendReqProcList(int iGroupNo)  // Group�� ���μ��� ����Ʈ ��û
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
	
	//map �ݺ��ؼ� GroupInfo�� ����ִ� ������ �����Ѵ�.
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
// EmsServer CMD Queue�� ����/����
//--------------------------------------------
bool CEmsWorkerManager::createCMDQueue()
{
	bool retval = false;

	if(m_CMDQueue != NULL){
		//�̹� CMD ť�� �����Ǿ��ִ� ����
		retval = true;
	}
	else{ // m_CMDQueue�� NULL�� ��� ���� �����Ѵ�.
		
		const char * pServerName = getServerName();
		if(pServerName == NULL){
			gLog->Write("[%s][%s][%d][ERROR][Check Server Name][IEmsServer.ini]", __FILE__, __FUNCTION__, __LINE__);
			exit(-1);
		}
		else{  // SERVER_NAME �� CMD�� ó���ϱ����� Queue �̸����� ���ȴ�.
			//CMD Queue ����
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
			
				// CMD�� ���� Queue ����
				// WorkerThread�� �ʿ��� �۾��� ������ �� �ִ�.
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
// EmsServer CMD Queue ����
//--------------------------------------------
bool CEmsWorkerManager::connectEACMDQueue()
{
	m_pEACMDExchange = theEmsRMQManager()->getExchange(RMQ_EXCHANGE_EA_CMD, RMQ_ROUTE_KEY_EA_CMD, true);
	gLog->Write("[Create EACMD Queue %s]", (m_pEACMDExchange!=NULL)?"SUCCESS":"FAILED" );

	return (m_pEACMDExchange!=NULL)?true:false;
}

//--------------------------------------------
// EmsServer Queue ���� ����
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
// ť(Campaign)�̸����� ������ Ǯ �˻�
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
// ť(Campaign)�̸����� ������ Ǯ ����/�߰�
void CEmsWorkerManager::addToWorkersPool  (const char * qname, int iThreadMode, int nThreads)
{
	shared_ptr<CEmsWorkerThreadPool> tmpEmsWorkerThreadPool = getWorkersPool(qname);
	if(tmpEmsWorkerThreadPool.get() != NULL){ //NULL�� �ƴѰ��
		gLog->Write("[Exist][QName:%s][NODE:%d]", qname, iThreadMode);
	}
	else{ //���� ���
		gLog->Write("[%s][%s][%d][WorkThreadPool is NULL][NODE:%d][QName:%s]", __FILE__, __FUNCTION__, __LINE__, iThreadMode, qname);

		// Rabbitmq ������ �ִ��� Ȯ���ϰ� ������ ���� �����Ͽ�  ThreadPool ����
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
// CEmsWorkServer ����
//(epoll ó���Ǵ� Ŭ���̾�Ʈ �޽��� ó���� ���� ť�� ���� �ϰ��ִ�)
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
//EMS DKIM �ʱ�ȭ
//(Ems DKIM Setting Initialization)
//--------------------------------------------
void CEmsWorkerManager::InitEmsDKIM ()
{
	//�������Ͽ��� ������ DKIM  ����Ʈ������ �̿��Ͽ� EmsDKIM �ʱ�ȭ
	
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


// Config �� �����ϴ�  thread�� �����Ͽ� ������ �Ϸ� �ɶ����� ����ϵ��� �Ѵ�.
int CEmsWorkerManager::checkServerConfigStart()
{
	gLog->Write("CEmsWorkerManager Thread Create Success!");
	return pthread_create(&m_chkThread, NULL, &ChkThreadFunc, this);
}


//Config ���� ���� ��û�� �ֱ������� ������ Thread
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

