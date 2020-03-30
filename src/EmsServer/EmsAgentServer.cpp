#include "EmsAgentServer.h"
#include "EmsConfig.h"
#include "EmsLog.h"
#include "FuncString.h"

extern CEmsLog * gLog;
extern char *itostr(int iNumber, char *pBuf, int iBufSize);
CEmsAgentServer * CEmsAgentServer::gEmsAgentServer=NULL;

CEmsAgentServer::CEmsAgentServer(int iMaxClientReadBuffer, int iMaxClientWriteBuffer, int iPort, int delay, int nMaxClients, int timer, int nWorkers, FILE *fp)
: CServer<CEmsClient >(iMaxClientReadBuffer, iMaxClientWriteBuffer, iPort, delay, nMaxClients, timer, fp)
, m_iMaxCampaignCount (nWorkers)
, m_bRMQUsable        (false)
, m_EACMDQueue        (NULL)
, m_bSendProcessList  (false)
, m_bCheckPauseFlag   (false)
{
	//Initialize of DB Configuration
	const char * dbIP     = theEMSConfig()->getDBIP();
	const char * dbName   = theEMSConfig()->getDBName();
	const char * dbUser   = theEMSConfig()->getDBUser();
	const char * dbPasswd = theEMSConfig()->getDBPasswd();

	memset(m_chDBIPAddr, 0, SZ_IP4ADDRESS+1);
	memset(m_chDBName,   0, SZ_NAME+1);
	memset(m_chDBUser,   0, SZ_NAME+1);
	memset(m_chDBPasswd, 0, SZ_PASSWD+1);
	
	strcpy(m_chDBIPAddr, dbIP);
	strcpy(m_chDBName,   dbName);
	strcpy(m_chDBUser,   dbUser);
	strcpy(m_chDBPasswd, dbPasswd);
	m_iDBPort = theEMSConfig()->getDBPort();

	//AMQPExchange *�� �����´�.
	reconnRMQConn();
	
	//Save Process Start Time
	time(&m_tLastCPTime);
	time(&m_tLastCMDHBTime);
	
	//onMessage���� ����� Class ��ü �ּ�
	gEmsAgentServer = this;
}

CEmsAgentServer::~CEmsAgentServer()
{
	if(m_bStopThread != true){
		m_bStopThread = true;
	}
	
	pthread_join(m_ThreadDBProc, NULL);
}

//-------------------------------------
// Reset RMQ Connection 
//-------------------------------------
void CEmsAgentServer::resetRMQConn()
{
	theEmsRMQManager()->resetRMQConInfo();
	gLog->Write("[%s][%s][%d][==>Exchange Disconnect]", __FILE__, __FUNCTION__, __LINE__);
	theEmsRMQManager()->deleteRMQConn(RMQ_EXCHANGE_CMD, RMQ_ROUTE_KEY_CMD);
	theEmsRMQManager()->deleteRMQConn(RMQ_EXCHANGE_EA_CMD, RMQ_ROUTE_KEY_EA_CMD, false);
	m_pamqpExchange = NULL;
	m_EACMDQueue    = NULL;
	setRMQUsable(false);
}


void CEmsAgentServer::reconnRMQConn()	//AMQPExchange *�� �����´�.
{
	setRMQUsable(false);
	m_pamqpExchange = NULL;
	resetRMQConn();
	do{
		gLog->Write("[%s][%s][%d][==>Exchange Reconnect]", __FILE__, __FUNCTION__, __LINE__);
		m_pamqpExchange = theEmsRMQManager()->getExchange(RMQ_EXCHANGE_CMD, RMQ_ROUTE_KEY_CMD, true);
		if(m_pamqpExchange==NULL){
			sleep(THREAD_RMQ_WAIT_TIME);
		}
		else
			break;
	}while(1);
	setRMQUsable(true);
}

void CEmsAgentServer::checkDB()
{
	MYSQL_RES *pMySQLResult;
	MYSQL_ROW  MySQLRow;
	int        RowCnt  = 0;
	bool       bResult = false;
	char       strQuery[Qry_StrlenBig+1]={0,};

	// DB Version Check!! - Campaign Field�� transmission_type�� �ִ��� Ȯ��
	snprintf(strQuery, Qry_StrlenBig, QrySel_CheckModeField, m_chDBName );
	
	bResult = m_EmsMysql.SelectDB_GetResult(strQuery, &RowCnt, 1);
	
	if((bResult == false)||(RowCnt==0)){
		gLog->Write("[%s][DB Name:%s][QUERY:%s]", __FUNCTION__, m_chDBName, strQuery);
		return;
	}
		
	pMySQLResult = m_EmsMysql.mpMySQLResult;
	MySQLRow     = mysql_fetch_row(pMySQLResult);
	
	if(atoi(RowStr(0)) == 0) {  // DB ���̺��� ���� ����
		m_EmsMysql.SelectDB_FreeResult();
		gLog->Write("[%s][DB Connecte Stop. Check DB Tables.]", __FUNCTION__);
		exit(-1);
	}
		
	m_EmsMysql.SelectDB_FreeResult();
}


//---------------------------------------------------
// WorkersThreadPool�� ������ m_spEmsQueue�� ó���Ѵ�.
// Thread�� ó��
//---------------------------------------------------
bool CEmsAgentServer::StartDBProcess()
{
	//EsmAgentServer���� ����� DB ����
	bool bResult = ConnectDB(m_chDBIPAddr, m_chDBUser, m_chDBPasswd, m_chDBName, m_iDBPort);
	if (bResult == false){
		gLog->Write("[%s][ERROR][DB Connect Failed][%s|[%s|[%s|[%s|%d]", 
		              __FUNCTION__, m_chDBIPAddr, m_chDBUser, m_chDBPasswd, m_chDBName, m_iDBPort);
		return false;
	}

	checkDB(); //DataBase�� ���� ���� ���  exit(-1) ó��
	
	//CEmsDbCommonQThread���� ���� ������ ó���ϵ��� Common Queue Processer ����
	m_spCommonQThread = shared_ptr<CEmsDbCommonQThread>( new CEmsDbCommonQThread());
	m_spCommonQThread.get()->ConnectDB(m_chDBIPAddr, m_chDBUser, m_chDBPasswd, m_chDBName, m_iDBPort);
	m_spCommonQThread.get()->setGroupInfoList(getGroupInfoList());
	m_spCommonQThread.get()->StartThread();

	m_bStopThread = false;
	pthread_create(&m_ThreadDBProc, NULL, &ThreadFuncDBProc, this);

	return true;
}

//---------------------------------------------------
// Set Message Header (value type string)
//---------------------------------------------------
void  CEmsAgentServer::setExHeader(AMQPExchange * ex, const char* hdrType, const char* hdrStr, bool bFlag)
{
	if(ex != NULL){
    ex->setHeader(hdrType, hdrStr, bFlag);
	}
}

//---------------------------------------------------
// Set Message Header (value type int)
//---------------------------------------------------
void  CEmsAgentServer::setExHeader(AMQPExchange * ex, const char* hdrType, int hdrData, bool bFlag)
{
	if(ex != NULL){
		char intBuf[SZ_INT_NUMBER]={0,};
		snprintf(intBuf, SZ_INT_NUMBER, "%d", hdrData);
    ex->setHeader(hdrType, intBuf, bFlag);
	}
}


//---------------------------------------------------
// Send CMD to Rabbitmq Server
//---------------------------------------------------
void  CEmsAgentServer::sendCMDtoRMQ(const char *pCmdmsg, const char* route_key)
{
	AMQPExchange * pCMDEx = getCMDExchange();
	
	if(theEmsRMQManager()->setDefaultExHeader(pCMDEx) == true){
		//gLog->Write("[%s][%s][%d][CMD_MSG]:[%s] [ROUTE_KEY]:[%s]", __FILE__, __FUNCTION__, __LINE__, pCmdmsg, route_key);
		pCMDEx->Publish(pCmdmsg, route_key);
	}
	else {
		gLog->Write("[%s][ERROR][Key:%s]", __FUNCTION__, route_key);
	}
}


//---------------------------------------------------
// Campaign Number�� ťó�� ���μ��� ������û
//---------------------------------------------------
bool  CEmsAgentServer::sendCMDCampaignCreate(const char *pCampaignNumber, int iGroupNo, const char *pGroupName)
{	//- RabbitMQ Connection���� �ش� �ε��� �޽��� ����
	bool bResult = false;
	Lock();
	try {
		AMQPExchange * pCMDEx = getCMDExchange();
			
		if(pCMDEx != NULL){
			//const char *pRouteKey = NULL;
			//set CMD type
			setExHeader(pCMDEx, CMD_TYPE,               CMD_TYPE_CREATE_QUEUE); //TYPE
			setExHeader(pCMDEx, CMD_INDEX_KEY,          pCampaignNumber);       //INDEX_KEY
			setExHeader(pCMDEx, CMD_SERVER_GROUPNUMBER, iGroupNo);              //GROUP_NUMBER
			setExHeader(pCMDEx, CMD_SERVER_GROUPNAME,   pGroupName);            //GROUP_NAME

			setExHeader(pCMDEx, MHEADER_CPNUMBER,       pCampaignNumber);       //CAMPAING_NUMBER
	
			sendCMDtoRMQ(CMD_TYPE_CREATE_QUEUE, pGroupName);
			bResult = true;
		}
	}
	catch(AMQPException ex){
		//Delete curent Exchange & Rollback Current job
		setRMQUsable(false);
	}
	catch (...){
		// error handling
	}
	Unlock();
	gLog->Write("[%s][Campaign No:%s]", __FUNCTION__, pCampaignNumber);
	
	return bResult;
}


//---------------------------------------------------
// ���μ��� ó���� ���� Mail �⺻���� ����
//---------------------------------------------------
bool  CEmsAgentServer::sendCMDCpMailInfo (shared_ptr<CEmsDbThread> spEmsDbThread
	                                      , MYSQL_ROW                &MySQLRow
	                                      , const char               *pCampaignNumber
	                                      , const char               *pServerName )
{
	//char chDBName[SZ_NAME+1]={0,};
	bool bResult = true;
	Lock();
	try {
		AMQPExchange * pCMDEx = getCMDExchange();
	
		if(pCMDEx != NULL){
			//set CMD type
			//-------------------
			//no          (0)
			//mailkey     (1)
			//sender_name (2)
			//sender_email(3)
			//mail_title  (4)
			//mail_body   (5)
			//user_wstr   (6)
			//reset_cnt   (7)
			//db_name     (8)
			//group_no    (9)
			//tr_type     (10)
			//-------------------

			setExHeader(pCMDEx, CMD_TYPE,      CMD_TYPE_INIT_CAMPAIGN);
			setExHeader(pCMDEx, CMD_INDEX_KEY, pCampaignNumber  );
	
			if(pServerName == NULL)
				setExHeader(pCMDEx, CMD_SERVER_NAME, ALL_SERVER);
			else
				setExHeader(pCMDEx, CMD_SERVER_NAME, pServerName);
	
			if(strlen(RowStr(0)) > 0)
				setExHeader(pCMDEx, MHEADER_CPNUMBER,    RowStr(0));
			if(strlen(RowStr(1)) > 0)
				setExHeader(pCMDEx, MHEADER_CPMAILKEY,   RowStr(1));
			if(strlen(RowStr(2)) > 0)
				setExHeader(pCMDEx, MHEADER_SENDERNAME,  RowStr(2));
			if(strlen(RowStr(3)) > 0) {
				setExHeader(pCMDEx, MHEADER_SENDEREMAIL, RowStr(3));
				char       *tmpDomain = NULL;
				const char *pDomain   = NULL;
				tmpDomain= strchr( const_cast<char*>(RowStr(3)), '@');
		
				if(tmpDomain != NULL)
					pDomain = tmpDomain+1;
				else
					pDomain = DEFAULT_DOMAIN_GABIA_COM;
	
				setExHeader(pCMDEx, MHEADER_SENDERDOMAIN, pDomain);
			}
			
			if(strlen(RowStr(4)) > 0)
				setExHeader(pCMDEx, MHEADER_MAILTITLE, RowStr(4));
				
			if(strlen(RowStr(6)) > 0) {
				setExHeader(pCMDEx, MHEADER_USEWSTR, RowStr(6));
				spEmsDbThread.get()->setUseWstr( (strcmp(RowStr(6), "1")==0)?true:false );
			}
			else {
				setExHeader(pCMDEx, MHEADER_USEWSTR, 0);
				spEmsDbThread.get()->setUseWstr( false );
			}
			
			if(strlen(RowStr(9)) > 0) { //group_no    [9]
				gLog->Write("[%s][%s][%d][GroupNo:%s]", __FILE__, __FUNCTION__, __LINE__, RowStr(9));
				setExHeader(pCMDEx, MHEADER_GROUPNUMBER, RowStr(9));
			}
			if(strlen(RowStr(10)) > 0) //transmission_type[10]
				setExHeader(pCMDEx, MHEADER_TR_TYPE, RowStr(10));
			gLog->Write("[%s][Group_Name:%s][Transmission Type : %s]", __FUNCTION__, spEmsDbThread.get()->getGroupName(), RowStr(10));


			//Send to RabbitMQ
			sendCMDtoRMQ(RowStr(5), spEmsDbThread.get()->getGroupName());
		}
	}
	catch(AMQPException ex){
		bResult = false;
		setRMQUsable(false);
	}
	catch (...) {
		// error handling
		bResult = false;
	}
	Unlock();
	
	return bResult;
}

//---------------------------------------------------
//- �ش� �ε����� �������� Campaign ���μ����� �����Ŵ
//---------------------------------------------------
void  CEmsAgentServer::sendCMDCampaignClose(const char *pCampaignNumber, int iGroup_no)
{
	const char *pGroupName=NULL;
	
	unordered_map<int, shared_ptr<stGroupInfo> >::iterator itr_gi = getGroupInfoList()->find(iGroup_no);
	if(itr_gi == getGroupInfoList()->end()){ //GroupInfo ����
		return ;
	}
	else{
		pGroupName=itr_gi->second->getGroupName();
	}
	
	
	Lock();
	try {
		AMQPExchange * pCMDEx = getCMDExchange();

		if(pCMDEx != NULL) {
			//set CMD type
			setExHeader(pCMDEx, CMD_TYPE,               CMD_TYPE_DELETE_QUEUE);
			setExHeader(pCMDEx, CMD_INDEX_KEY,          pCampaignNumber);
			setExHeader(pCMDEx, CMD_SERVER_GROUPNUMBER, iGroup_no);
			setExHeader(pCMDEx, CMD_SERVER_GROUPNAME,   pGroupName);

			sendCMDtoRMQ(CMD_TYPE_DELETE_QUEUE, pGroupName);
		}

		//Insert Campaign Number into the Delete List to delete EmsDbThread
		getEndCampaignList()->push_back(pCampaignNumber);
	}
	catch(...) {
		// error handling
	}
	Unlock();
	gLog->Write("[%s][CPSTEP2_SendComplete][%s][%d]", __FUNCTION__, pCampaignNumber, iGroup_no);
}

//---------------------------------------------------
//- �ش� �ε����� �������� Campaign ���μ����� ������Ŵ
//---------------------------------------------------
void  CEmsAgentServer::sendCMDCampaignPause(const char *pCampaignNumber, int iGroup_no)
{
	const char *pGroupName=NULL;
	
	unordered_map<int, shared_ptr<stGroupInfo> >::iterator itr_gi = getGroupInfoList()->find(iGroup_no);
	if(itr_gi == getGroupInfoList()->end()){ //GroupInfo ����
		return ;
	}
	else{
		pGroupName=itr_gi->second->getGroupName();
	}
	
	
	Lock();
	try {
		AMQPExchange * pCMDEx = getCMDExchange();

		if(pCMDEx != NULL) {
			//set CMD type
			setExHeader(pCMDEx, CMD_TYPE,               CMD_TYPE_PAUSE_QUEUE);
			setExHeader(pCMDEx, CMD_INDEX_KEY,          pCampaignNumber);
			setExHeader(pCMDEx, CMD_SERVER_GROUPNUMBER, iGroup_no);
			setExHeader(pCMDEx, CMD_SERVER_GROUPNAME,   pGroupName);

			sendCMDtoRMQ(CMD_TYPE_PAUSE_QUEUE, pGroupName);
		}
	}
	catch(...) {
		// error handling
	}
	Unlock();
}


//---------------------------------------------------
// Server Name�� �ش��ϴ� Server�� ���� ���� ��Ų��.
//---------------------------------------------------
void  CEmsAgentServer::sendCMDServerStop(const char *pCampaignNumber, const char *pServerName)
{	//- RabbitMQ Connection���� �ش� �ε��� �޽��� ����
	Lock();
	try {
		AMQPExchange * pCMDEx = getCMDExchange();

		if(pCMDEx != NULL){
		//set CMD type
			setExHeader(pCMDEx, CMD_TYPE,      CMD_TYPE_STOP_SERVER); //TYPE
			setExHeader(pCMDEx, CMD_INDEX_KEY, pCampaignNumber);      //INDEX_KEY
			
			if(pServerName == NULL)                                   //SERVER_NAME
				setExHeader(pCMDEx, CMD_SERVER_NAME, ALL_SERVER);
			else
				setExHeader(pCMDEx, CMD_SERVER_NAME, pServerName);
	
			sendCMDtoRMQ(CMD_TYPE_STOP_SERVER, RMQ_ROUTE_KEY_CMD);
	  }
	}
	catch(AMQPException ex){
		gLog->Write("[%s][%s][%d][ERROR][AMQP Error Occurred:%s][CpNo:%s]", __FILE__, __FUNCTION__, __LINE__, pCampaignNumber);
		setRMQUsable(false);
	}
	catch(...) {
		// error handling
	}
	Unlock();
	gLog->Write("[%s][%s][%d][CpNo:%s]", __FILE__, __FUNCTION__, __LINE__, pCampaignNumber);
}


//---------------------------------------------------
// ������ ����� Campaign ���μ����� �����Ѵ�.
//---------------------------------------------------
bool  CEmsAgentServer::checkPrevRunProcess()
{
	//DB Query : ���� �� ����Ǿ��� Campaign �˻�
	char       strQuery[Qry_StrlenBig+1]={0,};
	MYSQL_RES *pMySQLResult;
	MYSQL_ROW  MySQLRow;
	int        RowCnt  = 0;
	bool       bResult = false;
	
	unordered_map<int, stGroupInfo> tmpGroupInfoList;
	
	//Set Group Info
	memset(strQuery, 0, Qry_StrlenBig);
	snprintf(strQuery, Qry_StrlenBig, QrySel_GroupInfoALL);
	//gLog->Write("[%s][QUERY:%s]", __FUNCTION__, strQuery);
	bResult = m_EmsMysql.SelectDB_GetResult(strQuery, &RowCnt, FldCnt_GroupInfo);

	if((bResult != false) && (RowCnt > 0)){
		pMySQLResult = m_EmsMysql.mpMySQLResult;
	
		while((MySQLRow = mysql_fetch_row(pMySQLResult)) != NULL){
			//GroupInfo �߰�
			stGroupInfo tmpGroupInfo = stGroupInfo(atoi(RowStr(0)), RowStr(1), RowStr(2));
			tmpGroupInfoList.insert ( std::pair<int, stGroupInfo>(atoi(RowStr(0)), tmpGroupInfo) );
		}
		m_EmsMysql.SelectDB_FreeResult();
	}
		
	
	//DB���� Campaign Number �� ���¸� üũ
	memset(strQuery, 0, Qry_StrlenBig);
	snprintf(strQuery, Qry_StrlenBig, QrySel_Campaign_no_PrevRunStep, STR_MODE_M);
	//gLog->Write("[%s][QUERY:%s]", __FUNCTION__, strQuery);

	bResult = m_EmsMysql.SelectDB_GetResult(strQuery, &RowCnt, 1);
	
	//���� ó�� ���̾��� ����Ʈ�� ������
	vector<string> vecCampaignNoList;
	if((bResult != false) && (RowCnt > 0)){
		pMySQLResult = m_EmsMysql.mpMySQLResult;
	
		while((MySQLRow = mysql_fetch_row(pMySQLResult)) != NULL){
			vecCampaignNoList.push_back(RowStr(0));
		}
		m_EmsMysql.SelectDB_FreeResult();
	}
	
	gLog->Write("[%s][Check Previous Run Process][%d]", __FUNCTION__, vecCampaignNoList.size());
	//CampaignList�� ���� ��� ���μ��� �����Ͽ� ����Ʈ ����
	if(vecCampaignNoList.size() > 0) {
		vector<string>::iterator itr_il = vecCampaignNoList.begin();
		string  tmpProcName;
		for(; itr_il != vecCampaignNoList.end(); itr_il++){
			tmpProcName = *itr_il;

			memset(strQuery, 0, Qry_StrlenBig+1);
			snprintf(strQuery, Qry_StrlenBig, QrySel_Campaign_no_prev, tmpProcName.c_str(), STR_MODE_M );
			gLog->Write("[%s][QUERY:%s]", __FUNCTION__, strQuery);
			
			bResult = m_EmsMysql.SelectDB_GetResult(strQuery, &RowCnt, FldCnt_Campaign_Info);
			if( (bResult == false) || (RowCnt == 0) ){
				gLog->Write("[%s][Continue]", __FUNCTION__);
				continue ;
			}
			else{
				pMySQLResult = m_EmsMysql.mpMySQLResult;
				MySQLRow     = mysql_fetch_row(pMySQLResult);

				if(MySQLRow){
		
					bool        bRMQError  = false;
					int         iGroupNo   = atoi(RowStr(9));
					const char *pGroupName = NULL;
					unordered_map<int, stGroupInfo>::iterator itr_gi = tmpGroupInfoList.find(iGroupNo);

					if(itr_gi == tmpGroupInfoList.end()){ //GroupInfo ����
						//ó�������ʰ� ����Ų��. �ش� ������ ���ӵ� ��� �ش� �׷쿡 ���� ������ ������ �� �ִ�.
						m_EmsMysql.SelectDB_FreeResult();
						gLog->Write("[%s][Continue][None Group Information", __FUNCTION__);
						continue;
					}
					else{
						pGroupName=itr_gi->second.getGroupName();
					}

					shared_ptr<CEmsDbThread> tmpspEmsDbThread  = shared_ptr<CEmsDbThread>( new CEmsDbThread(RowStr(0)) );
					if(tmpspEmsDbThread.get() ==  NULL){
						m_EmsMysql.SelectDB_FreeResult();
						gLog->Write("[%s][Continue]", __FUNCTION__);
						continue;
					}

					tmpspEmsDbThread.get()->setMailDBName(RowStr(8));
					if(tmpspEmsDbThread.get()->ConnectDB(m_chDBIPAddr, m_chDBUser, m_chDBPasswd, m_chDBName, m_iDBPort) == false){
						m_EmsMysql.SelectDB_FreeResult();
						continue;
					}
					
					getCampaignList()->insert(std::pair<string, shared_ptr<CEmsDbThread> >(string(RowStr(0)), tmpspEmsDbThread) );

					tmpspEmsDbThread.get()->setGroupNo(RowStr(9));
					tmpspEmsDbThread.get()->setGroupName(pGroupName);
					gLog->Write("[%s][GroupNo:%d][GroupName:%s]", __FUNCTION__, iGroupNo, pGroupName);
					
					tmpspEmsDbThread.get()->setDelMailBody((strcmp(RowStr(12),"Y")==0)?true:false);
					gLog->Write("[%s][GroupNo:%d][GroupName:%s]", __FUNCTION__, iGroupNo, pGroupName);

					if(sendCMDCampaignCreate(RowStr(0), iGroupNo, pGroupName) == true){
						if(sendCMDCpMailInfo(tmpspEmsDbThread, MySQLRow, RowStr(0)) == false){
							bRMQError = true;
						}
						else{
							tmpspEmsDbThread.get()->setCpMailKey(RowStr(1));
							tmpspEmsDbThread.get()->StartThread();
						}
					}
					else{ //Error : sendCMDCampaignCreate
						bRMQError = true;
					}
					
					if(bRMQError == true){
						//AMQPException Occurred!
						unordered_map<string, shared_ptr<CEmsDbThread> >::iterator itr_dbThread;
						itr_dbThread = getCampaignList()->find(string(RowStr(0)));
						if(itr_dbThread != getCampaignList()->end()){
							getCampaignList()->erase(itr_dbThread);
						}
						m_EmsMysql.SelectDB_FreeResult();
						resetRMQConn();
						continue;;
					}
				}
				else{
					gLog->Write("[%s][DB Fetch Row Failed]", __FUNCTION__);
				}
				m_EmsMysql.SelectDB_FreeResult();
			}
		}
	} 
	else{  //vecCampaignNoList�� NULL�̸� 
		//gLog->Write("[%s][%s][%d] DO NOTHING", __FILE__, __FUNCTION__, __LINE__);
	}

	return true;
}  



//---------------------------------------------------
// CampaignList�� ���¸� üũ�ϰ�, RMQ�� CMD�� �����Ѵ�.
//---------------------------------------------------
void  CEmsAgentServer::checkCampaignState(const char * pCampaignNumber, shared_ptr<CEmsDbThread> &spEmsDbThread)
{
	//EmsDbThread�� CpStep�� CPSTEP2_SendComplete ���� üũ
	int iCpStep  = spEmsDbThread.get()->getCpStep();
	int iGroupNo = spEmsDbThread.get()->getGroupNo();
	
	//gLog->Write("[%s][%s][%d]-----[CpNo:%s][STEP:%d][GroupNo:%d]", __FILE__, __FUNCTION__, __LINE__, pCampaignNumber, iCpStep, iGroupNo);
	if(spEmsDbThread.get()->getCpStepFlag() == true){
		//update CpStep
		bool bResult = false;
		char strQuery[Qry_StrlenBig+1]={0,};
		const char *pQryStr = NULL;

		switch(iCpStep){
			case CPSTEP0_DbReadRunning:{
				pQryStr = QryUdt_Campaign_CpStep0; 
				break;
			}
			case CPSTEP1_DbReadComplete:{
				pQryStr = QryUdt_Campaign_CpStep1;
				gLog->Write("[%s][QryUdt_Campaign_CpStep1][%s][%d]", __FUNCTION__, pCampaignNumber, iGroupNo);
				break;
			}
			case CPSTEP2_SendComplete:{
				if(spEmsDbThread.get()->getDelMailBody() == true)
					pQryStr = QryUdt_Campaign_CpStepComplete_Del_mailbody;
				else
					pQryStr = QryUdt_Campaign_CpStepComplete; 
				break;
			}
			default:
				return;
		}
		snprintf(strQuery, Qry_StrlenBig, pQryStr, pCampaignNumber);
		bResult = m_EmsMysqlSub.QueryDB(strQuery);
	}
	
	if(iCpStep == CPSTEP2_SendComplete) { //�Ϸ�� ���μ����� ��쿡�� ������Ʈ�ϰ� ���μ��� ����
		sendCMDCampaignClose(pCampaignNumber, iGroupNo); //- RabbitMQ�� CMD �� �����Ͽ� �������� Campaign ���μ����� �����Ŵ
	}
}


//---------------------------------------------------
// CampaignList�� ���¸� üũ�ϰ�, RMQ�� CMD�� �����Ѵ�.
//---------------------------------------------------
bool CEmsAgentServer::checkGroupProcessCount(int iGroupNo)
{
	if(iGroupNo <= I_NONE_GROUP)
		return false;
	
	//GroupInfo ����Ʈ�� ���ӵ� Group Server�� ���� ��� return false
	unordered_map<int, shared_ptr<stGroupInfo> >::iterator itr_gi = getGroupInfoList()->find(iGroupNo);
	if(itr_gi == getGroupInfoList()->end())
		return false;
	
	if( getCampaignCount(iGroupNo) < MAX_GROUP_PROC_COUNT) 
		return true;
	else
		return false;

}


//---------------------------------------------------
// CampaignList�� ��û�� �� ��ŭ�� Campaign�� ó���ϵ��� �Ѵ�.
//---------------------------------------------------
void  CEmsAgentServer::addCampaignToList(int nCount)
{	
	//DB���� �߰��ؾ��� Campaign�� �ִ��� üũ&�߰�
	int  RowCnt  = 0;
	bool bResult = false;
	char chLastCpNumber[LEN_Campaign_no+1]={0,}; // Max Campaign Number Length : 19
	char strQuery      [Qry_StrlenBig+1]  ={0,};
	
	do{
		memset(strQuery, 0, Qry_StrlenBig+1);
		
		if(strlen(chLastCpNumber) == 0){
			snprintf(strQuery, Qry_StrlenBig, QrySel_Campaign_Info ADD_LIMIT, CPSTEPB_DbBeforeRunning, STR_MODE_M, 1);
		}
		else{
			snprintf(strQuery, Qry_StrlenBig, QrySel_Campaign_Info ADD_CHECK_NO ADD_LIMIT, CPSTEPB_DbBeforeRunning, STR_MODE_M, chLastCpNumber, 1);
		}

		bResult = m_EmsMysql.SelectDB_GetResult(strQuery, &RowCnt, FldCnt_Campaign_Info);
		//gLog->Write("[%s][%s][%d] [Query:%s][%d][RowCnt:%d]", __FILE__, __FUNCTION__, __LINE__, strQuery, bResult, RowCnt);
	
		if( (bResult == false) || (RowCnt == 0) ){
			return ;
		}
		else{
			MYSQL_RES *pMySQLResult = m_EmsMysql.mpMySQLResult;
			MYSQL_ROW  MySQLRow;
			
			//����Ʈ�� Campaign_no�� �ִ��� Ȯ���� ������ �߰��ϰ�, ������ �н�
			unordered_map<string, shared_ptr<CEmsDbThread> > *pCampaignList = getCampaignList();
			unordered_map<string, shared_ptr<CEmsDbThread> >::iterator  itr_cl;
			shared_ptr<CEmsDbThread> tmpspEmsDbThread;

			if((MySQLRow = mysql_fetch_row(pMySQLResult)) != NULL){
				memset(chLastCpNumber, 0, LEN_Campaign_no+1);
				snprintf(chLastCpNumber, LEN_Campaign_no, RowStr(0));
				
				//1. ���� ����Ʈ�� �����ϴ��� üũ
				itr_cl = pCampaignList->find(RowStr(0));
				
				if(itr_cl != pCampaignList->end()){ //����Ʈ�� ����
					//CMD �޽��� 						                                 
					gLog->Write("[%s][SEND COMMAND][EXIST][Campaign_no:%s][%d]"
		                  , __FUNCTION__, RowStr(0), m_iMaxCampaignCount);

		    	tmpspEmsDbThread = itr_cl->second;
				}
				else{ // ����Ʈ�� ���� ���
					// �뷮 ������ ��� Campaign_no�� ť�� �����Ͽ� �޽��� ó��						
					//1. Check Group
					//2. Insert into Campaign List
					//3. Send CMD Message (To Make Process Message by IEmsServer)
					
					// Group Campaign ������ üũ�Ͽ� Process�� �������� �����Ѵ�.					
					int  group_no = atoi(RowStr(9));
					bool bRet     = checkGroupProcessCount(group_no);  //RowStr(9) : group_no 

					if(bRet == true){ //�߰�
							
						gLog->Write("[%s][m_EmsMysql.SelectDB_GetResult][Not EXIST][DO INSERT][CPNO:%s][GROUPNO:%d]"
						                  , __FUNCTION__, RowStr(0), group_no);
						tmpspEmsDbThread = shared_ptr<CEmsDbThread>( new CEmsDbThread(RowStr(0)) );
						if(tmpspEmsDbThread.get() == NULL){
							m_EmsMysql.SelectDB_FreeResult();
							continue;
						}
						//�ӽ������� DB ��������(IP/ID/PASSWD)�� ���� ���Ͽ��� �о��: ���� ��������
						tmpspEmsDbThread.get()->setCpMailKey(RowStr(1));
						tmpspEmsDbThread.get()->setMailDBName(RowStr(8));
						tmpspEmsDbThread.get()->setDelMailBody( (strcmp(RowStr(12),"Y")==0)?true:false);
						tmpspEmsDbThread.get()->ConnectDB(m_chDBIPAddr, m_chDBUser, m_chDBPasswd, m_chDBName, m_iDBPort);
			
						pCampaignList->insert(std::pair<string, shared_ptr<CEmsDbThread> >(string(RowStr(0)), tmpspEmsDbThread) );
						nCount--;
					}
					else{ //�߰� �Ұ�
						m_EmsMysql.SelectDB_FreeResult();
						continue;
					}
				}

				//DBThread�� Group ���� �Է�
				bool        bError     = false;
				int         iGroupNo   = atoi(RowStr(9));
				const char *pGroupName = NULL;
				unordered_map<int, shared_ptr<stGroupInfo> >::iterator itr_gi = getGroupInfoList()->find(iGroupNo);

				if(itr_gi == getGroupInfoList()->end()) { //GroupInfo ����
					m_EmsMysql.SelectDB_FreeResult();
					continue;
				}
				else{ // �׷��� �ƴѰ�� GroupNo = 0( NONE_GROUP );
					pGroupName=itr_gi->second->getGroupName();
				}
				
				tmpspEmsDbThread.get()->setGroupNo(RowStr(9));
				tmpspEmsDbThread.get()->setGroupName(pGroupName);
				
				if(sendCMDCampaignCreate(RowStr(0), iGroupNo, pGroupName) == true){
					if(sendCMDCpMailInfo(tmpspEmsDbThread, MySQLRow, RowStr(0)) == true){
						tmpspEmsDbThread.get()->StartThread();
						snprintf(strQuery, Qry_StrlenBig, QryUdt_Campaign_CpStep0, RowStr(0));
						bResult = m_EmsMysqlSub.QueryDB(strQuery);
					}
					else{
						bError = true;
					}
				}
				else{
					bError = true;
				}
				
				if((bError == true)|| (bResult==false)){
					//sendCMDCampaignCreate() ���� �� AMQPException Occurred!
					unordered_map<string, shared_ptr<CEmsDbThread> >::iterator itr_dbThread;
					itr_dbThread = getCampaignList()->find(string(RowStr(0)));
					if(itr_dbThread != getCampaignList()->end()){
						getCampaignList()->erase(itr_dbThread);
					}
					m_EmsMysql.SelectDB_FreeResult();
					sleep(THREAD_RMQ_WAIT_TIME);
					break;
				}
			}
			else{
				m_EmsMysql.SelectDB_FreeResult();
				break;
			}			
			m_EmsMysql.SelectDB_FreeResult();
		}
		
	}while((bResult == true) && (nCount > 0) && (getRMQUsable()==true));
}



//---------------------------------------------------
// CampaignList�� Campaign Number�� Campaign �߰�.
//---------------------------------------------------
shared_ptr<CEmsDbThread> CEmsAgentServer::getEmsDbThread(const char* pCampaignNumber)
{
	unordered_map<string, shared_ptr<CEmsDbThread> > *pTmpCampaignList = getTmpCampaignList ();
	unordered_map<string, shared_ptr<CEmsDbThread> >::iterator itr_tmpcl;
	shared_ptr<CEmsDbThread> spDbThread;
	
	if(pTmpCampaignList->size() > 0){
		if((itr_tmpcl = pTmpCampaignList->find(pCampaignNumber)) != pTmpCampaignList->end()){
			//����Ʈ �ȿ� �����ϴ� ���
			spDbThread = itr_tmpcl->second;
			return spDbThread;
		}
	}
	
	//����Ʈ�� ���� ��� DB�� �˻��Ͽ� �־��ش�.
	try{
		int  RowCnt  = 0;
		bool bResult = false;
		char strQuery[Qry_StrlenBig+1] = {0,};
						
		memset(strQuery, 0, Qry_StrlenBig+1);
		//no[0],mailkey[1],sender_name[2],sender_email[3],mail_title[4],mail_body[5],use_wstr[6],reset_cnt[7],db_name[8],group_no[9],tr_type[10],step[11],del_mail_body[12]
		snprintf(strQuery, Qry_StrlenBig, QrySel_Campaign_Info_No ADD_LIMIT, pCampaignNumber, STR_MODE_M, 1);
		
		bResult = m_EmsMysql.SelectDB_GetResult(strQuery, &RowCnt, FldCnt_Campaign_Info);
		gLog->Write("[%s][%s][%d] [Query:%s][%d][RowCnt:%d]", __FILE__, __FUNCTION__, __LINE__, strQuery, bResult, RowCnt);
		
		if( (bResult == false) || (RowCnt == 0) ){
			return spDbThread;
		}
		else{
			MYSQL_RES *pMySQLResult = m_EmsMysql.mpMySQLResult;
			MYSQL_ROW  MySQLRow;
				
			//����Ʈ�� Campaign_no�� �ִ��� Ȯ���� ������ �߰��ϰ�, ������ �н�
			if((MySQLRow = mysql_fetch_row(pMySQLResult)) != NULL){
				gLog->Write("[%s][m_EmsMysql.SelectDB_GetResult][Not EXIST][DO INSERT][CPNO:%s][GROUPNO:%s]"
					          , __FUNCTION__, RowStr(0), RowStr(9));
				spDbThread = shared_ptr<CEmsDbThread>( new CEmsDbThread(TSTATE_PREREADY) );
				if(spDbThread.get() == NULL){
					m_EmsMysql.SelectDB_FreeResult();
					return spDbThread;
				}
				
				spDbThread.get()->setCampaignNoStr(RowStr(0));
				spDbThread.get()->setCpMailKey(RowStr(1));
				spDbThread.get()->setMailDBName(RowStr(8));
				spDbThread.get()->setGroupNo(RowStr(9));
				spDbThread.get()->setDelMailBody((strcmp(RowStr(12),"Y")==0)?true:false);
				
				pTmpCampaignList->insert(std::pair<string, shared_ptr<CEmsDbThread> >(string(RowStr(0)), spDbThread) );
				m_EmsMysql.SelectDB_FreeResult();
			}
		}
	}
	catch(...){
	}

	return spDbThread;
}

//---------------------------------------------------
// Pause ��û�� Campaign�� �˻��Ͽ� ���μ��� ���� ��û ó��
// 
//---------------------------------------------------
void  CEmsAgentServer::checkPauseCampaignList()
{	
	//DB���� �߰��ؾ��� Campaign�� �ִ��� üũ&�߰�
	int  RowCnt  = 0;
	bool bResult = false;
	char chLastCpNumber[LEN_Campaign_no+1]={0,};
	char strQuery      [Qry_StrlenBig+1]  ={0,};
	
	do{
		memset(strQuery, 0, Qry_StrlenBig+1);
		
		if(strlen(chLastCpNumber) == 0){
			snprintf(strQuery, Qry_StrlenBig, QrySel_Campaign_Pause  ADD_LIMIT, 1);
		}
		else{
			snprintf(strQuery, Qry_StrlenBig, QrySel_Campaign_Pause ADD_CHECK_NO  ADD_LIMIT, chLastCpNumber, 1);
		}

		bResult = m_EmsMysql.SelectDB_GetResult(strQuery, &RowCnt, FldCnt_Campaign_Pause);
		//gLog->Write("[%s][%s][%d] [Query:%s][%d][RowCnt:%d]", __FILE__, __FUNCTION__, __LINE__, strQuery, bResult, RowCnt);
	
		if( (bResult == false) || (RowCnt == 0) ){
			return ;
		}
		else{
			MYSQL_RES *pMySQLResult = m_EmsMysql.mpMySQLResult;
			MYSQL_ROW  MySQLRow;
			
			//���⿡�� ���� ��û ���� ����Ʈ�� ������Ų��.
			//���� ���� ����Ʈ�� �ش� ���� ó�� ���μ����� ���� ���, �ش� �׷쿡 �޽����� ����
			//���� ���μ����� �����ϴ� ���, �ش� �׷쿡 �޽����� ������, ���� ��Ű�� ���� �ش� ť(RMQ)�� ����ִ� �޽����� �����Ͽ� DB�� ������ �� ť�� �����ϵ��� �Ѵ�.
			//�߼ۼ��������� �ش� ť�� �޽��� �Լ��� ������Ű�� ������ �����Ѵ�->�߼����̴� �޽��� ���� �������� �ʴ´�

			unordered_map<string, shared_ptr<CEmsDbThread> > *pCampaignList = getCampaignList();
			unordered_map<string, shared_ptr<CEmsDbThread> >::iterator  itr_cl;
			shared_ptr<CEmsDbThread> tmpspEmsDbThread;

			//no[0],db_name[1],group_no[2],tr_type[3]
			if((MySQLRow = mysql_fetch_row(pMySQLResult)) != NULL){
				memset(chLastCpNumber, 0, LEN_Campaign_no+1);
				snprintf(chLastCpNumber, LEN_Campaign_no, RowStr(0));
				
				//UPDATE: campaign->step(-8)
				snprintf(strQuery, Qry_StrlenBig, QryUdt_Campaign_CpStepM8, RowStr(0));
				bResult = m_EmsMysqlSub.QueryDB(strQuery);
				
				if(strcmp(RowStr(0), STR_MODE_I)==0){
					m_EmsMysql.SelectDB_FreeResult();
					continue;
				}
				else{
					//1. ���� ����Ʈ�� �����ϴ��� üũ
					itr_cl = pCampaignList->find(RowStr(0));
					
					if(itr_cl != pCampaignList->end()){ //����Ʈ�� ����
	
						tmpspEmsDbThread = itr_cl->second;
						if(tmpspEmsDbThread.get() != NULL){
							//Pause DbThread
							tmpspEmsDbThread.get()->setRunState(TSTATE_PAUSE);
						}
					}
					else{ // ����Ʈ�� ����
					}
					
					//Group�� Pause CMD ����
					int         iGroupNo   = atoi(RowStr(2));
					const char *pGroupName = NULL;
					unordered_map<int, shared_ptr<stGroupInfo> >::iterator itr_gi = getGroupInfoList()->find(iGroupNo);
	
					if(itr_gi == getGroupInfoList()->end()) { //GroupInfo ����
						//Do nothing
					}
					else{
						//Send CMD Pause Message
						pGroupName=itr_gi->second->getGroupName();
						sendCMDCampaignPause(RowStr(0), iGroupNo);
	
						//DbThread�κ���  RMQ�� ����ִ� �޽����� �����´�.
						if(tmpspEmsDbThread.get() != NULL){
							tmpspEmsDbThread.get()->rollbackDBUpdate();
						}
	
						//insert into EndCampaignList
						getEndCampaignList()->push_back(RowStr(0));
					}
	
					//DB Result Free
					m_EmsMysql.SelectDB_FreeResult();				
				}
			}
			else{
				m_EmsMysql.SelectDB_FreeResult();
				break;
			}			
		}
		
	}while(bResult == true);
}


//---------------------------------------------------
// Ÿ�Կ� ���� ó������ Campaign ����
// bGroupType : 
// true �� ��� Group�� Campaign ���� ����
// false �� ��� �Ϲ� Campaign ���� ����
//---------------------------------------------------
int  CEmsAgentServer::getCampaignCount(int group_no)
{
	int cpCount = 0;

	if(group_no >= I_NONE_GROUP){ //group_no �� �ش��ϴ� Campaign ������ ����
		unordered_map<string, shared_ptr<CEmsDbThread> >::iterator itr_cplist = getCampaignList()->begin();
		for(;itr_cplist != getCampaignList()->end(); itr_cplist++){
			if(itr_cplist->second.get()->getGroupNo() == group_no)
				cpCount++;
		}
	}
	else {
		cpCount = m_CampainList.size();
	}
	
	return cpCount;
}


//---------------------------------------------------
// CampaignList�� ���¸� üũ�ϰ�, CampaignList�� �����Ѵ�.
//---------------------------------------------------
bool  CEmsAgentServer::checkProcState()
{
	int  campaignCount = getCampaignCount(); //Campaign ���� 

	//gLog->Write("[%s][%s][%d] [Check Process State]", __FILE__, __FUNCTION__, __LINE__);
	
	try{
		//Check Campaign Pause State
		if(m_bCheckPauseFlag == true){
			checkPauseCampaignList();
			m_bCheckPauseFlag = false;
		}
		else
			m_bCheckPauseFlag = true;
	}
	catch(...){
	}

	try{
		//Check Campaign List Process State
		if (campaignCount <= 0){ // Campaign ����
			//Do Nothing
		}
		else { //( campaignCount > 0 ) //�۾� ���� DB üũ
			unordered_map<string, shared_ptr<CEmsDbThread> >           *tmpCampaignlist;
			unordered_map<string, shared_ptr<CEmsDbThread> >::iterator  itr_cl;
	
			//string                   strCampaignNo;
			const char              *pCampaignNo;
			shared_ptr<CEmsDbThread> spEmsDbThread;
			
			tmpCampaignlist = getCampaignList();
			itr_cl          = tmpCampaignlist->begin();
			
			for(; itr_cl != tmpCampaignlist->end(); itr_cl++){
				//strCampaignNo = itr_cl->first;
				pCampaignNo   = itr_cl->first.c_str();
				spEmsDbThread = itr_cl->second;

				//Campaign Number�� �ش��ϴ� ���μ��� ���¸� üũ�ϰ� ����� CMD ���� �޽��� ����
				checkCampaignState(pCampaignNo, spEmsDbThread);
			}
	
			string strCpNo;
			while(getEndCampaignCount() > 0){

				strCpNo = getEndCampaignList()->front();
				getEndCampaignList()->pop_front();
				if(strCpNo.length() > 0){
					tmpCampaignlist->erase(strCpNo);
					gLog->Write("[%s][Close Campaign][Campaign No:%s]", __FUNCTION__, strCpNo.c_str());
				}
			}
		}
	}
	catch(...){
		gLog->Write("[%s][%s][%d][ERROR][%d]", __FILE__, __FUNCTION__, __LINE__, campaignCount);
		return false;
	}
	
	if( (campaignCount >= 0) && (campaignCount < m_iMaxCampaignCount) ){ // �ִ� Campaign ������ �����ʴ� ��� �����带 �߰��ϵ��� �Ѵ�.
		addCampaignToList(m_iMaxCampaignCount - campaignCount);            //Campaign �߰�
	}

	return true;
}


//--------------------------------------------------------------------
//  Server Heartbeat Time Check 
//--------------------------------------------------------------------
void CEmsAgentServer::checkServerHB(time_t curTime)
{
	//GroupInfo Heartbeat Time Update
	long tServerDie = REQ_CONFIG_SEND_PERIOD*3;
	unordered_map<int, shared_ptr<stGroupInfo> > *pGroupInfoList  = getGroupInfoList();
	unordered_map<int, shared_ptr<stGroupInfo> >::iterator itr_gi;
	shared_ptr<stGroupInfo>  spGroupInfo;
	unordered_map<int, stServerInfo> * pmapServerInfo;
	
	for(itr_gi = pGroupInfoList->begin(); itr_gi != pGroupInfoList->end(); ){
		spGroupInfo  = itr_gi->second;
		pmapServerInfo = spGroupInfo.get()->getServerList();
		
		if(pmapServerInfo->size() > 0){
			unordered_map<int, stServerInfo>::iterator itr_si = pmapServerInfo->begin();
			for(; itr_si != pmapServerInfo->end(); ){
				if(itr_si->second.diffHBTime(curTime) > tServerDie){
					itr_si = pmapServerInfo->erase(itr_si);
				}
				else{
					itr_si++;
				}
			}
		}
		if(pmapServerInfo->size() == 0){
			itr_gi = pGroupInfoList->erase(itr_gi);
		}
		else{
			itr_gi++;
		}
	}
}


//---------------------------------------------------
// ThreadFuncDBProc : Agent server Thread Function
//---------------------------------------------------
void * CEmsAgentServer::ThreadFuncDBProc(void * param)
{
	CEmsAgentServer * pEmsAS = (CEmsAgentServer *)param;

	time_t prevTime, curTime, tmpCpChkTime;
	time(&prevTime);
	time(&curTime);
	time(&tmpCpChkTime);
	
	int tCheckTmpGroupInfoInterval = 30;
	//Process Restore
	do{
		if(pEmsAS->getRMQUsable() == false){
			pEmsAS->reconnRMQConn();
			
			if(pEmsAS->getRMQUsable() == false){
				sleep(THREAD_RMQ_WAIT_TIME);
				continue;
			}
		}
		
		if(pEmsAS->checkPrevRunProcess() == false){
			sleep(THREAD_RMQ_WAIT_TIME);
		}
		else
			break;
	}while(true);
	
	while(!pEmsAS->m_bStopThread)	{
		
		if(pEmsAS->getRMQUsable() == false){
			pEmsAS->reconnRMQConn();
			
			if(pEmsAS->getRMQUsable() == false){
				sleep(THREAD_RMQ_WAIT_TIME);
				continue;
			}
		}

		if(pEmsAS->checkProcState() ==  false){
			gLog->Write("[%s][%s][%d][ERROR][RETURN FALSE]", __FILE__, __FUNCTION__, __LINE__);
			break;
		}

		time(&curTime);
		//Check Server HeartBeat Time.
		if(difftime(curTime, prevTime) > THREAD_RMQ_WAIT_TIME){
			pEmsAS->checkServerHB(curTime);
			prevTime = curTime;
		}
				
		//Campaign Process List Send 
		pEmsAS->checkSendCPList(curTime);
		//Send CMD Message To Keep IEmsServer CMD RMQ Connection
		pEmsAS->checkSendConnHBCMDQ(curTime);
		//Unused TmpGroupInfo Check
		if(pEmsAS->isCheckTimeOver(tmpCpChkTime, curTime, 60/*sec*/) == true){
			pEmsAS->checkTmpCampaignList(curTime);
		}

		usleep(THREAD_SLEEPTIME_1SEC);
	}

	gLog->Write("[==== EmsAgentServer END ====]");
	return (void *) NULL;
}


//---------------------------------------------------
// Connect DB
//---------------------------------------------------
bool CEmsAgentServer::ConnectDB(const char *l_pszHost, const char *l_pszUser, const char *l_pszPasswd, const char *l_pszDBName, unsigned int l_iPort)
{
	bool bResult = false;

	char EmsAgentServer[] ="EmsAgentServer";
	m_EmsMysql.mIsUseLock = true;
	m_EmsMysql.SetOwnerName(EmsAgentServer);
	m_EmsMysql.SetHost(l_pszHost, l_pszUser, l_pszPasswd, l_pszDBName, l_iPort);

	bResult = m_EmsMysql.ConnectDB();

	if(!bResult)
	{
		gLog->Write("[%s][%s][%d] DB Connection Failed!!!", __FILE__, __FUNCTION__, __LINE__);
		return false;
	}

	char EmsAgentServerSub[] ="EmsAgentServerSub";
	m_EmsMysqlSub.mIsUseLock = true;
	m_EmsMysqlSub.SetOwnerName(EmsAgentServerSub);
	m_EmsMysqlSub.SetHost(l_pszHost, l_pszUser, l_pszPasswd, l_pszDBName, l_iPort);

	bResult = m_EmsMysqlSub.ConnectDB();

	if(!bResult)
	{
		gLog->Write("[%s][%s][%d] DB Connection Failed!!", __FILE__, __FUNCTION__, __LINE__);
		return false;
	}

	char EmsAgentServerCMD[] ="EmsAgentServerSub";
	m_EmsCMDMysql.mIsUseLock = true;
	m_EmsCMDMysql.SetOwnerName(EmsAgentServerCMD);
	m_EmsCMDMysql.SetHost(l_pszHost, l_pszUser, l_pszPasswd, l_pszDBName, l_iPort);

	bResult = m_EmsCMDMysql.ConnectDB();

	if(!bResult)
	{
		gLog->Write("[%s][%s][%d] DB Connection Failed!!", __FILE__, __FUNCTION__, __LINE__);
		return false;
	}

	return bResult;
}


//---------------------------------------------------
// ���� �ð��� ���� �ð��� ���Ͽ�
//  timeInterval���� ũ�� true �׷��� �ʴٸ� false ����
//---------------------------------------------------
bool CEmsAgentServer::isCheckTimeOver(time_t &prevTime, time_t currTime, int timeInterval)
{
	int retval = difftime(currTime, prevTime);
	
	if(retval >= timeInterval){
		prevTime = currTime;
		return true;
	}
	else
		return false;	
}


//---------------------------------------------------
// Send Process List To CMD Queue(�ֱ������� ����)
//---------------------------------------------------
void CEmsAgentServer::checkSendCPList(time_t currTime)
{
	if(isCheckTimeOver(m_tLastCPTime, currTime, MAX_CPLIST_CHECK_WAIT_TIME) == false)
		return;
	
	if(getCampaignCount() == 0)
		return;
		
	//���� �������� ���μ��� ����Ʈ�� CMD�� ������.
	//GroupInfo List
	unordered_map<int, shared_ptr<stGroupInfo> > *pGroupInfoList  = getGroupInfoList();
	unordered_map<int, shared_ptr<stGroupInfo> >::iterator itr_gi = pGroupInfoList->begin();

	//CampaignList
	unordered_map<string, shared_ptr<CEmsDbThread> > *pCampaignList;
	unordered_map<string, shared_ptr<CEmsDbThread> >::iterator itr_cp;
	
	int     iGroupNo;
	bool    bFlag;
	string  strCpList;

	for(;itr_gi != pGroupInfoList->end(); itr_gi++){
		bFlag         = false;
		iGroupNo      = itr_gi->first;
		pCampaignList = getCampaignList();
		strCpList.clear();
		
		for(itr_cp=pCampaignList->begin(); itr_cp!=pCampaignList->end(); itr_cp++){
			if(itr_cp->second.get()->getGroupNo() == iGroupNo){
				if(bFlag == true)
					strCpList += CMD_DELIMITER;
				else
					bFlag = true;
					
				strCpList += itr_cp->first.c_str();
			}
		}
		
		if(strCpList.empty() == true)
			continue;
		else{
			//gLog->Write("[%s][LIST:%s]", __FUNCTION__, strCpList.c_str());
			
			strCpList += CMD_DELIMITER_END;
			Lock();
			try {
				//CpList Send to Ems Server
				AMQPExchange * pCMDEx = getCMDExchange();
				
				setExHeader(pCMDEx, CMD_TYPE,               CMD_TYPE_CHK_PROCESSLIST);       //TYPE PROCESS LIST
				setExHeader(pCMDEx, CMD_INDEX_KEY,          NULLSTRING);                     //CP INDEX NULL
				setExHeader(pCMDEx, CMD_SERVER_NAME,        NULLSTRING);                     //SERVER NAME "NULL"
				setExHeader(pCMDEx, CMD_SERVER_GROUPNUMBER, iGroupNo);                       //CMDSERVERGROUPNO
				setExHeader(pCMDEx, CMD_SERVER_GROUPNAME,   itr_gi->second->getGroupName()); //CMDSERVERGROUPNAME
		
				//Send to Rabbitmq
				sendCMDtoRMQ(strCpList.c_str(), itr_gi->second.get()->getGroupName());
			}
			catch (...) {
				// error handling
				gLog->Write("[%s][%s][%d][ERROR Occurred...][%s]", __FILE__, __FUNCTION__, __LINE__, strCpList.c_str());
			}
			Unlock();
		}
	}
}


//---------------------------------------------------
// Send IEmsAgent HeartBeat To CMD Queue(�ֱ������� ����)
//---------------------------------------------------
void CEmsAgentServer::checkSendConnHBCMDQ(time_t currTime)
{
	if(isCheckTimeOver(m_tLastCMDHBTime, currTime, MAX_CPLIST_CHECK_WAIT_TIME*6) == false)
		return;
		
	Lock();
	try {
		//CpList Send to Ems Server
		AMQPExchange * pCMDEx = getCMDExchange();
		
		setExHeader(pCMDEx, CMD_TYPE,               CMD_TYPE_CHK_CONN_HB);  //CMD_TYPE_CHK_CONN_HB
		setExHeader(pCMDEx, CMD_INDEX_KEY,          NULLSTRING);            //CP INDEX NULL
		setExHeader(pCMDEx, CMD_SERVER_NAME,        NULLSTRING);            //SERVER NAME "NULL"
		setExHeader(pCMDEx, CMD_SERVER_GROUPNUMBER, "0");                   //CMDSERVERGROUPNO
		setExHeader(pCMDEx, CMD_SERVER_GROUPNAME,   NULLSTRING);            //CMDSERVERGROUPNAME

		//Send to Rabbitmq
		sendCMDtoRMQ(CMD_TYPE_CHK_CONN_HB, RMQ_ROUTE_KEY_CMD);
	}
	catch (...) {
		// error handling
		gLog->Write("[%s][%s][%d][ERROR Occurred...][%s]", __FILE__, __FUNCTION__, __LINE__, CMD_TYPE_CHK_CONN_HB);
		setRMQUsable(false);
	}
	Unlock();
}


//---------------------------------------------------
// TmpCampaignList�� �˻��Ͽ� TimeOut �� ��ü ����
//---------------------------------------------------
void CEmsAgentServer::checkTmpCampaignList(time_t currTime)
{
	if(getTmpCampaignList()->size()==0)
		return;
		
	unordered_map<string, shared_ptr<CEmsDbThread> > *pTmpCampaignList = getTmpCampaignList();
	unordered_map<string, shared_ptr<CEmsDbThread> >::iterator itr_tmpcl;
	shared_ptr<CEmsDbThread> spEmsDbThread;
	
	for(itr_tmpcl = pTmpCampaignList->begin(); itr_tmpcl!=pTmpCampaignList->end();){
		spEmsDbThread = itr_tmpcl->second;
		if(spEmsDbThread.get() != NULL){
			if(difftime(currTime, spEmsDbThread.get()->getDBThreadTime()) > 60){
				itr_tmpcl = pTmpCampaignList->erase(itr_tmpcl);
				continue;
			}
		}
		else{
			itr_tmpcl = pTmpCampaignList->erase(itr_tmpcl);
			continue;
		}
		itr_tmpcl++;
	}
}


//---------------------------------------------------
// Process List Send to CMD Queue
// �ش� Group ���μ��� ����Ʈ�� �����Ͽ� EmsServer CMD ť�� ����ó��
//---------------------------------------------------
void CEmsAgentServer::sendProcessList(const char *pServerName, const char *pGroup_no)
{
	unordered_map<string, shared_ptr<CEmsDbThread> > *pCampaignList = getCampaignList();

	if((pCampaignList != NULL)&&(pCampaignList->size() > 0)){ // Campaign ����Ʈ ����....
		int    iGroupNo = atoi(pGroup_no);
		int    RowCnt   = 0;
		bool   bResult  = false;
		char   strQuery[Qry_StrlenBig+1] = {0,};

		//�ش� �׷��� ����Ʈ�� �����Ͽ� ����
		vector<string> sCampaignList;
		unordered_map<string, shared_ptr<CEmsDbThread> >::iterator  itr;

		for(itr=pCampaignList->begin(); itr!=pCampaignList->end(); itr++){
			if(itr->second.get()->getGroupNo()==iGroupNo){
				sCampaignList.push_back(itr->first);
			}
		}
		
		if(sCampaignList.size() == 0){
			return ;
		}
		else{
			//title & Body �����Ͱ� �����Ƿ� DB���� �����´�.
			string strCampaign_no;
			vector<string>::iterator  itr_il;
				
			for(itr_il = sCampaignList.begin(); itr_il != sCampaignList.end(); itr_il++){
				strCampaign_no = *itr_il;
				memset(strQuery, 0, Qry_StrlenBig+1);
				snprintf(strQuery, Qry_StrlenBig, QrySel_Campaign_no_prev, strCampaign_no.c_str(), STR_MODE_M);
				gLog->Write("[%s][QUERY:%s]", __FUNCTION__, strQuery);
				
				bResult = m_EmsMysql.SelectDB_GetResult(strQuery, &RowCnt, FldCnt_Campaign_Info);
				if( (bResult == false) || (RowCnt == 0) ){
					gLog->Write("[%s][Continue]", __FUNCTION__);
					continue ;
				}
				else{

					MYSQL_RES *pMySQLResult = m_EmsMysql.mpMySQLResult;
					MYSQL_ROW  MySQLRow     = mysql_fetch_row(pMySQLResult);

					if(MySQLRow){
						int         iGroupNo   = atoi(RowStr(9));
						const char *pGroupName = NULL;

						unordered_map<int, shared_ptr<stGroupInfo> >  *pGroupInfoList = getGroupInfoList();
						unordered_map<int, shared_ptr<stGroupInfo> >::iterator itr_gi = pGroupInfoList->find(iGroupNo);
						if(itr_gi == pGroupInfoList->end()){ //GroupInfo ����
							//ó�������ʰ� ����Ų��. �ش� ������ ���ӵ� ��� �ش� �׷쿡 ���� ������ ������ �� �ִ�.
							m_EmsMysql.SelectDB_FreeResult();
							gLog->Write("[%s][%s][%d][Continue][None Group Information", __FILE__, __FUNCTION__, __LINE__);
							continue;
						}
						else{
							pGroupName=itr_gi->second.get()->getGroupName();
						}

						itr = pCampaignList->find(strCampaign_no);
						if(itr != pCampaignList->end()){
							shared_ptr<CEmsDbThread>  spEmsDbThread = itr->second;
							if(spEmsDbThread.get() != NULL){
								if(sendCMDCampaignCreate(RowStr(0), iGroupNo, pGroupName) == true){
									if(sendCMDCpMailInfo(spEmsDbThread, MySQLRow, RowStr(0), pServerName) == false){
										gLog->Write("[%s][%s][%d][RMQ ERROR Occurred][Campaign No:%s]", __FILE__, __FUNCTION__, __LINE__, RowStr(0));
									}
								}
							}
						}
					}
					else{
						gLog->Write("[%s][%s][%d][DB Fetch Row Failed]", __FILE__, __FUNCTION__, __LINE__);
					}
					m_EmsMysql.SelectDB_FreeResult();
				}
			}
		}
	}
	else{
		//ó�� ���� Campaign�� ����
	}
}


//--------------------------------------------
// CMD Queue Event Setting 
//--------------------------------------------
int CEmsAgentServer::onCancel(AMQPMessage * message ) 
{
	gLog->Write("[%s][%s][%d]RabbitMQ getDeliveryTag():%d]", __FILE__, __FUNCTION__, __LINE__, message->getDeliveryTag());

	return 0;
}

//-------------------------------------
// EmsServer�� ������ ����Ʈ�� �߰�
//--------------------------------------
bool CEmsAgentServer::addToServerList(const char *pServerGroupNo, const char *pServerIP, const char *pServerName)
{
	if((pServerGroupNo == NULL)||(pServerIP == NULL)||(pServerName == NULL)){
		gLog->Write("IPAddr:(%s) | ServerName:(%s)", (pServerIP==NULL)?"NULL":pServerIP, (pServerName==NULL)?"NULL":pServerName);
		return false;
	}
	else{
		//DB Check
		MYSQL_RES *pMySQLResult;
		MYSQL_ROW  MySQLRow;
	
		int  RowCnt = 0;
		char strQuery[Qry_StrlenBig] = {0,};
		
		//snprintf(strQuery, Qry_StrlenBig, QrySel_ServerConfigInfo, pServerIP);
		snprintf(strQuery, Qry_StrlenBig, QrySel_ServerConfigInfo_GroupNoIP, pServerGroupNo, pServerIP);
		
		//gLog->Write("[%s][QUERY:%s]", __FUNCTION__, strQuery);
		bool bResult = m_EmsCMDMysql.SelectDB_GetResult(strQuery, &RowCnt, FldCnt_ServerConfigInfo);
		gLog->Write("[%s][GROUPNO:%s][IP:%s][SERVERNAME:%s]", __FUNCTION__, pServerGroupNo, pServerIP, pServerName);

		if( (bResult == false) || (RowCnt == 0) ){
			gLog->Write("[%s][Get Server Config Failed][%d][%d]", __FUNCTION__, bResult, RowCnt); 
			bResult = false;
		}
		else{
			pMySQLResult = m_EmsCMDMysql.mpMySQLResult;
			MySQLRow     = mysql_fetch_row(pMySQLResult);

			gLog->Write("[%s][SEND_CONF][IP:%s][%s|%s|%s|%s]", __FUNCTION__, pServerIP, RowStr(0), RowStr(1),RowStr(2),RowStr(3));

			Lock();
			try {
				//------------------
				//GroupInfo
				//------------------------------------------------
				int group_no = atoi(RowStr(1));
				unordered_map<int, shared_ptr<stGroupInfo> >::iterator itr_gi = getGroupInfoList()->find(group_no);
				if(itr_gi == getGroupInfoList()->end()){
					//GroupInfo �߰�
					shared_ptr<stGroupInfo> spGroupInfo = shared_ptr<stGroupInfo>(new stGroupInfo(group_no, RowStr(2), RowStr(3)));
					getGroupInfoList()->insert ( std::pair<int, shared_ptr<stGroupInfo> >(group_no, spGroupInfo) );
					itr_gi = getGroupInfoList()->find(group_no);
				}
				
			  //Insert into Server List
			  shared_ptr<stGroupInfo> tmpspGroupInfo  = itr_gi->second;
			  unordered_map<int, stServerInfo> *tmpServerList   = tmpspGroupInfo.get()->getServerList();
			  unordered_map<int, stServerInfo>::iterator itr_sl = tmpServerList->find(atoi(RowStr(0)));
			  if(itr_sl == tmpServerList->end()){
			  	//Server List�� �������� �ʴ� ���
			  	tmpServerList->insert(std::pair<int, stServerInfo>(atoi(RowStr(0)), stServerInfo(atoi(RowStr(0)), pServerName) ));
			  }
			  else{ //Server List�� �����ϴ� ��� Do Nothing!
			  	itr_sl->second.updateHB();
			  }
			  //------------------------------------------------
			}
			catch (...) {
				// error handling
			}
			Unlock();
			
			m_EmsCMDMysql.SelectDB_FreeResult();
			bResult = true;
		}
		return bResult;
	}
}


//--------------------------------------------------------------------
//  Server Connection State Update
//--------------------------------------------------------------------
void CEmsAgentServer::procMessage_heartbeat(AMQPMessage *pMsg)
{
	const char *pServerGroupNo = pMsg->getHeader(MSGHEADER_GROUPNUMBER).c_str();
	const char *pServerIPAddr  = pMsg->getHeader(MSGHEADER_SERVER_IP).c_str();
	const char *pServerNumber  = pMsg->getHeader(MSGHEADER_SERVER_NUMBER).c_str();
	const char *pServerName    = pMsg->getHeader(MSGHEADER_SERVER_NAME).c_str();
	//Server Heartbeat Update 
	char   tmpHBbuf[SZ_TMPBUFFER+1] = {0,};

	//GroupInfo Heartbeat Time Update
	unordered_map<int, shared_ptr<stGroupInfo> > *pGroupInfoList  = getGroupInfoList();
	unordered_map<int, shared_ptr<stGroupInfo> >::iterator itr_gi = pGroupInfoList->find(atoi(pServerGroupNo));
		
	if(itr_gi != pGroupInfoList->end()){
		shared_ptr<stGroupInfo>                    spGroupInfo = itr_gi->second;
		unordered_map<int, stServerInfo>          *pServerList = spGroupInfo.get()->getServerList();
		unordered_map<int, stServerInfo>::iterator itr_sl      = pServerList->find(atoi(pServerNumber));
		if(itr_sl != pServerList->end()){
			itr_sl->second.updateHB();
		}
		else{
			pServerList->insert(std::pair<int, stServerInfo>(atoi(pServerNumber), stServerInfo(atoi(pServerNumber), pServerName) ));
		}
	}
	else{
		//���� ���������� ���µ� ������ ����Ǿ��ִٸ�
		//IEmsAgent�� ��������� ����̹Ƿ� ���� ������ ������Ʈ�ϵ����Ѵ�.
		addToServerList(pServerGroupNo, pServerIPAddr, pServerName);		
	}
	
	snprintf(tmpHBbuf, SZ_TMPBUFFER, QryUdt_ServerHB, pServerGroupNo, pServerIPAddr);
	if(m_EmsCMDMysql.QueryDB(tmpHBbuf) == false){
		//m_EmsCMDMysql DB ���� ������ �õ��� �ϵ��� �Ѵ�.
	}
}


//-------------------------------------
// EmsServer�� ���� ������ ����
//--------------------------------------
bool CEmsAgentServer::sendServerConfig(const char *pServerIP, const char *pServerName)
{
	if((pServerIP == NULL)|| (pServerName == NULL)){
		gLog->Write("IPAddr:(%s) | ServerName:(%s)", (pServerIP==NULL)?"NULL":pServerIP, (pServerName==NULL)?"NULL":pServerName);
		return false;
	}
	else{
		//DB Check
		MYSQL_RES *pMySQLResult;
		MYSQL_ROW  MySQLRow;
	
		int  RowCnt = 0;
		char strQuery[Qry_StrlenBig] = {0,};
		
		//"SELECT si.server_no,si.group_no,gi.group_name,gi.group_commonQname,si.server_usable FROM server_info si,group_info gi WHERE si.group_no=gi.group_no AND si.server_ip='%s' "
		snprintf(strQuery, Qry_StrlenBig, QrySel_ServerConfigInfo, pServerIP);
		bool bResult = m_EmsCMDMysql.SelectDB_GetResult(strQuery, &RowCnt, FldCnt_ServerConfigInfo);

		if( (bResult == false) || (RowCnt == 0) ){
			gLog->Write("Get Server Config Failed[%d][%d]", bResult, RowCnt); 
			bResult = false;
		}
		else{
			pMySQLResult = m_EmsCMDMysql.mpMySQLResult;
			while( (MySQLRow = mysql_fetch_row(pMySQLResult)) != NULL){
				gLog->Write("[%s][Get DB Config Send To EmsServer][%s|%s|%s|%s]", __FUNCTION__, RowStr(0), RowStr(1),RowStr(2),RowStr(3));
	
				Lock();
				try {
					//CpList Send to Ems Server
					AMQPExchange * pCMDEx = getCMDExchange();
					
					setExHeader(pCMDEx, CMD_TYPE,               CMD_TYPE_SET_SERVER_CONF); //TYPE SET EMS SERVER CONFIGURATION
					setExHeader(pCMDEx, CMD_INDEX_KEY,          NULLSTRING);     //CP INDEX NULL
					setExHeader(pCMDEx, CMD_SERVER_NAME,        pServerName);    //Server Name
					setExHeader(pCMDEx, CMD_SERVER_IPADDR,      pServerIP);      //Server IP Address
					setExHeader(pCMDEx, CMD_SERVER_NUMBER,      RowStr(0));      //Server Server Number
					if(RowCnt > 1){
						setExHeader(pCMDEx, CMD_SERVER_MULTI_GROUP, CMD_YES);      //Server Multi-Group (Yes)
					}
					else{
						setExHeader(pCMDEx, CMD_SERVER_MULTI_GROUP, CMD_NO);      //Server Multi-Group (No)
					}
					setExHeader(pCMDEx, CMD_SERVER_GROUPNUMBER, RowStr(1));      //Server Group Number
					setExHeader(pCMDEx, CMD_SERVER_GROUPNAME,   RowStr(2));      //Server Group Name
					setExHeader(pCMDEx, CMD_SERVER_COMMONQ,     RowStr(3));      //Server (Group) Common Queue
					setExHeader(pCMDEx, CMD_SERVER_STATE,       RowStr(4));      //Server 'enable' or 'disable'
				
					//Send to Rabbitmq
					sendCMDtoRMQ("CMD_SET_CONFIGURATION", RMQ_ROUTE_KEY_CMD);  //RMQ_ROUTE_KEY_CMD�� ��ü ������ �޽��� ����
					
					//------------------
					//GroupInfo
					//------------------------------------------------
					int group_no = atoi(RowStr(1));
					unordered_map<int, shared_ptr<stGroupInfo> >::iterator itr_gi = getGroupInfoList()->find(group_no);
					if(itr_gi == getGroupInfoList()->end()){
						//GroupInfo �߰�
						shared_ptr<stGroupInfo> spGroupInfo = shared_ptr<stGroupInfo>(new stGroupInfo(group_no, RowStr(2), RowStr(3)));
						getGroupInfoList()->insert ( std::pair<int, shared_ptr<stGroupInfo> >(group_no, spGroupInfo) );
						itr_gi = getGroupInfoList()->find(group_no);
					}
					
				  //Insert into Server List
				  shared_ptr<stGroupInfo> tmpspGroupInfo  = itr_gi->second;
				  unordered_map<int, stServerInfo> *tmpServerList   = tmpspGroupInfo.get()->getServerList();
				  unordered_map<int, stServerInfo>::iterator itr_sl = tmpServerList->find(atoi(RowStr(0)));
				  if(itr_sl == tmpServerList->end()){
				  	//Server List�� �������� �ʴ� ���
				  	tmpServerList->insert(std::pair<int, stServerInfo>(atoi(RowStr(0)), stServerInfo(atoi(RowStr(0)), pServerName)));
				  }
				  else{ //Server List�� �����ϴ� ��� Do Nothing!
				  	itr_sl->second.updateHB();
				  }
				  //------------------------------------------------
				}
				catch (AMQPException ex) {
					// error handling
					gLog->Write("[%s][%s][%d][ERROR][MSG:%s]", __FILE__, __FUNCTION__, __LINE__, ex.getMessage().c_str());
					setRMQUsable(false);
				}
				catch (...) {
					// error handling
				}
				Unlock();
				
			}

			m_EmsCMDMysql.SelectDB_FreeResult();
			bResult = true;
		}
		return bResult;
	}
}


//-------------------------------------
// Request ó���� ���� �޽��� ���μ��� �Լ�
//--------------------------------------
void CEmsAgentServer::procMessage_ReqGet(AMQPMessage *pMsg)
{
	//MSGINFO_STATE
	uint32_t j        = -1;
	char    *pMsgData = NULL;
	
	const char *pMsgInfoType = pMsg->getHeader(MSGHEADER_GETMSG_TYPE).c_str();
	const char *pServerName  = pMsg->getHeader(MSGHEADER_SERVER_NAME).c_str();

	int iMsgType = atoi(pMsgInfoType);

	if( iMsgType > -1){
		pMsgData = pMsg->getMessage(&j);
		
		switch(iMsgType){
			case GET_SERVER_CONF:{
				const char *pServerIPAddr  = pMsg->getHeader(MSGHEADER_SERVER_IP).c_str();
				gLog->Write("[RECV][GET_SERVER_CONF:%d][IP:%s][Name:%s][Data:%s]", iMsgType, pServerIPAddr, pServerName, pMsgData);
				//���� ��������=���� ���ӿ�û
				sendServerConfig(pServerIPAddr, pServerName);
				break;
			}
			case GET_PROC_LIST:{
				const char *pGroup_no = pMsg->getHeader(MSGHEADER_GROUPNUMBER).c_str();
				//gLog->Write("[%s][RECV][GET_PROC_LIST:%d][Name:%s][Group_Num:%s][Data:%s]", __FUNCTION__, iMsgType, pServerName, pGroup_no, pMsgData);
				sendProcessList(pServerName, pGroup_no);
			}
		}
	}
}


//--------------------------------------------
// Event�� Message�� �޾Ƽ� ó��
//--------------------------------------------
int CEmsAgentServer::onMessage( AMQPMessage * message ) {
		
	if(message == NULL){
		return 0;
	}
				
  const char *tmpLogType    = message->getHeader(MSGHEADER_TYPE).c_str();

  if(tmpLogType != NULL){
  	int iType = atoi(tmpLogType);
  	switch(iType){
  		case MSG_TYPE_REQ_GET:{
  			getEmsAgentServer()->procMessage_ReqGet(message);
    		break;
  		}
  		case MSG_TYPE_HEARTBEAT:{
  			getEmsAgentServer()->procMessage_heartbeat(message);
  			break;
  		}
  		default:{
      	gLog->Write("[%s][%s][%d] Unknown Log Type[0x%04X]", __FILE__,  __FUNCTION__, __LINE__, iType);
  		}
  	}
  }
  else{
  	//Type�� ���� �α�
  	gLog->Write("[%s][%s][%d] Log Type NULL[%s]", __FILE__,  __FUNCTION__, __LINE__, tmpLogType);
  }
	
	return 0;
}


//--------------------------------------------
// EMS AGENT CMD Queue�� ����/����
//--------------------------------------------
bool CEmsAgentServer::createEACMDQueue()
{
	//CMD Queue Event ó���� ���� Static ����
	gEmsAgentServer = this;

	//CMD Queue ����
	m_EACMDQueue = theEmsRMQManager()->getQueue(RMQ_EXCHANGE_EA_CMD, RMQ_ROUTE_KEY_EA_CMD, false);

	try{
		if(m_EACMDQueue != NULL){
			gLog->Write("[%s][MSG:EA CMD Queue][%s][%s] ", __FUNCTION__, RMQ_EXCHANGE_EA_CMD, RMQ_ROUTE_KEY_EA_CMD);
			
			// CMD�� ���� Queue ����
			m_EACMDQueue->addEvent(AMQP_MESSAGE, CEmsAgentServer::onMessage);
			m_EACMDQueue->addEvent(AMQP_CANCEL,  CEmsAgentServer::onCancel );
			m_EACMDQueue->Consume(AMQP_NOACK);
			throw AMQPException("RMQ Connection Error Occurred.");
		}
		else{
			throw AMQPException("CMD Queue Pointer Get Failed...");
		}
	}
	catch(AMQPException amqpEx){
		gLog->Write("[%s][%s][%d][ERROR][MSG:%s]", __FILE__, __FUNCTION__, __LINE__, amqpEx.getMessage().c_str());
		setRMQUsable(false);
		//theEmsRMQManager()->deleteRMQConn(RMQ_EXCHANGE_EA_CMD, RMQ_ROUTE_KEY_EA_CMD, false);
	}
	catch(...){
		gLog->Write("[%s][%s][%d][ERROR][Unknown Error Occurred]", __FILE__, __FUNCTION__, __LINE__);
	}

	setRMQUsable(false);
	m_EACMDQueue = NULL;
	
	return true;
}


//--------------------------------------------
// EMS AGENT CMD Queue�� ����/����
//--------------------------------------------
shared_ptr<CEmsDbThread> CEmsAgentServer::getDbNameFromeTmpCpList (const char* pCampaignNumber)
{
	shared_ptr<CEmsDbThread> spDbThread;
	unordered_map<string, shared_ptr<CEmsDbThread> > *pTmpCampaignList = getTmpCampaignList ();
	unordered_map<string, shared_ptr<CEmsDbThread> >::iterator itr_tmpcl;
	if(pTmpCampaignList->size() > 0){
		//�켱������ m_TmpCampaignList�� �ش� Campaign Number�� Ȯ���Ѵ�
		itr_tmpcl = pTmpCampaignList->find(pCampaignNumber);
		
		if(itr_tmpcl != pTmpCampaignList->end()){ //����
			spDbThread = itr_tmpcl->second;
			return spDbThread;
		}			
	}
	//�ش� Campaign Number�� ���� ���
	// m_TmpCamapaignList�� �߰��Ѵ�

	//EmsDbThread  ���� ���⿡ �߰��Ѵ�.
	spDbThread = getEmsDbThread(pCampaignNumber);
	
	return spDbThread;
}
