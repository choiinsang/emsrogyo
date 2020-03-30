//##############################################################################################
#include "EmsDbThread.h"
#include "FuncString.h"
#include <boost/shared_array.hpp>
#include "EmsConfig.h"
#include "EmsLog.h"

extern CEmsLog * gLog;
//-------------------------------------
// CEmsDbThread
//-------------------------------------
CEmsDbThread::CEmsDbThread(const char *pCampaign_no)
:	m_amqpQueue            (NULL)
,	m_amqpExchange         (NULL)
,	m_iRunState            (TSTATE_READY)
,	m_bIsStop              (false)
, m_iDBPort              (INIT_DEFAULT_DBPORT)
, m_iGroupNo             (I_NONE_GROUP)
,	m_CpStep               (CPSTEPB_DbBeforeRunning)
, m_bCpStepFlag          (false)
,	m_IsStoped             (-1)
,	m_UseWstr              (false)
, m_iSCount              (0)
, m_bDelMailBody        (false)
{
	m_iMaxRetryCount = theEMSConfig()->getMailRetryCount();
	
	// DB 설정 정보 초기화
	memset(m_chDBIPAddr, 0, SZ_IP4ADDRESS +1);
	memset(m_chDBName,   0, SZ_NAME+1);
	memset(m_chDBUser,   0, SZ_NAME+1);
	memset(m_chDBPasswd, 0, SZ_PASSWD+1);
	
	//mails DB Name
	memset(m_chMailDBName,   0, SZ_NAME+1);

	// 메일 기본정보 초기화
	if(pCampaign_no == NULL){
		gLog->Write("[%s][%s][%d][ERROR] CampaignIndex is NULL!!", __FILE__, __FUNCTION__, __LINE__);
		return ;
	}
	else{
		memset(m_CampaignNo,            0, LEN_Campaign_no+1);
		memcpy(m_CampaignNo, pCampaign_no, LEN_Campaign_no+1);
	}

	memset(m_CpMailKey, 0, LEN_CpMailKey+1);

	m_EmsMysql.mIsUseLock    = true;
	m_EmsMysqlSub.mIsUseLock = true;
	
	//메시지 큐 생성 & 연결
	//Queue를 생성하지 않고 Exchange만 생성하여 메시지를 전송하도록 한다.
	setRMQUsable(false);
	do{
		m_amqpQueue = theEmsRMQManager()->getQueue(RMQ_EXCHANGE_MAIL, pCampaign_no);
		if(m_amqpQueue == NULL){
			sleep(THREAD_RMQ_WAIT_TIME);
			continue;
		}

		m_amqpExchange = theEmsRMQManager()->getExchange(RMQ_EXCHANGE_MAIL, pCampaign_no);
		if(m_amqpExchange == NULL){
			sleep(THREAD_RMQ_WAIT_TIME);
			continue;
		}
		break;
	}while(true);
	setRMQUsable(true);
}


//-------------------------------------
// CEmsDbThread(Tempary Used Mode iState(TSTATE_PREREADY)
//-------------------------------------
CEmsDbThread::CEmsDbThread(int iState)
:	m_amqpQueue            (NULL)
,	m_amqpExchange         (NULL)
,	m_iRunState            (iState)
,	m_bIsStop              (false)
, m_iDBPort              (INIT_DEFAULT_DBPORT)
, m_iGroupNo             (I_NONE_GROUP)
,	m_CpStep               (CPSTEPB_DbBeforeRunning)
, m_bCpStepFlag          (false)
,	m_IsStoped             (-1)
,	m_UseWstr              (false)
, m_iSCount              (0)
{
}

//-------------------------------------
// ~CEmsDbThread 
//-------------------------------------
CEmsDbThread::~CEmsDbThread()
{
	if(m_iRunState == TSTATE_PREREADY){
		//Do Nothing
	}
	else{
		//Thread 종료
		if(m_bIsStop == false){
			m_bIsStop = true;
		}
		pthread_join(m_Thread, NULL);
	
		try{
			theEmsRMQManager()->deleteRMQConn(RMQ_EXCHANGE_MAIL, m_CampaignNo, true);
			m_amqpQueue    = NULL;
			m_amqpExchange = NULL;
			setRMQUsable(false);
		}
		catch(...){
			gLog->Write("[ERROR]RabbitMQ [%s] Queue Name Delete Failed.", m_CampaignNo);
		}
	}
}


//-------------------------------------
// Set Campaign Number:
//-------------------------------------
void CEmsDbThread::setCampaignNoStr(const char *pCampaign_no)
{
	memset(m_CampaignNo,            0, LEN_Campaign_no+1);
	memcpy(m_CampaignNo, pCampaign_no, LEN_Campaign_no+1);
}
	
//-------------------------------------
// Set Campaign Mail Key DB: cpMailKey value
//-------------------------------------
void CEmsDbThread::setCpMailKey(const char* cpMailKey)
{ 
	if(cpMailKey != NULL){
		sprintf(m_CpMailKey, "%s", cpMailKey);
	}
}

//-------------------------------------
// Set Campaign Group Number
//-------------------------------------
void CEmsDbThread::setGroupNo(const char *pGroupNo)
{
	if(pGroupNo != NULL){
		m_strGroupNo = pGroupNo;
		m_iGroupNo   = atoi(pGroupNo);
	}
}

//-------------------------------------
// Reset RMQ Connection 
//-------------------------------------
void CEmsDbThread::resetRMQConn()
{
	theEmsRMQManager()->deleteRMQConn(RMQ_EXCHANGE_MAIL, m_CampaignNo);
	m_amqpQueue    = NULL;
	m_amqpExchange = NULL;
	setRMQUsable(false);
}

//-------------------------------------
// Reconnect RMQ Connection 
//-------------------------------------
void CEmsDbThread::reconnRMQConn()
{
	do{
		theEmsRMQManager()->resetRMQConInfo();
		m_amqpQueue = theEmsRMQManager()->getQueue(RMQ_EXCHANGE_MAIL, m_CampaignNo);
		if(m_amqpQueue == NULL){
			sleep(THREAD_RMQ_WAIT_TIME);
			gLog->Write("[%s][%s][%d][Reconnect Queue Failed]", __FILE__, __FUNCTION__, __LINE__);
			continue;
		}
		gLog->Write("[%s][%s][%d][Reconnect Queue]", __FILE__, __FUNCTION__, __LINE__);

		m_amqpExchange = theEmsRMQManager()->getExchange(RMQ_EXCHANGE_MAIL, m_CampaignNo);
		if(m_amqpExchange == NULL){
			sleep(THREAD_RMQ_WAIT_TIME);
			gLog->Write("[%s][%s][%d][Reconnect Exchange Failed]", __FILE__, __FUNCTION__, __LINE__);
			continue;
		}
		gLog->Write("[%s][%s][%d][Reconnect Exchange]", __FILE__, __FUNCTION__, __LINE__);
		
		break;
	}while(true);
	setRMQUsable(true);
}

//-------------------------------------
// StartThread 
//-------------------------------------
void CEmsDbThread::StartThread()
{
	if ( (m_Thread != 0) && (m_CpStep >= CPSTEP0_DbReadRunning) ){ // Running State
		gLog->Write("[EMS DbThread Restart!!][%s]", getCampaignNoStr());
		setCampaignStep(CPSTEP0_DbReadRunning);
	}
	else{
		int retval = pthread_create(&m_Thread, NULL, &GetDbThreadFunc, this);
		if(retval != 0){
			gLog->Write("[%s][%s][%d][ERROR] DB Thread Create Failed! ", __FILE__, __FUNCTION__, __LINE__);
		}
	}	
}


//-------------------------------------
// GetDbThreadFunc 
//-------------------------------------
void *CEmsDbThread::GetDbThreadFunc(void *Param)
{
	CEmsDbThread *pDbThread = (CEmsDbThread *)Param;

	pDbThread->setRunState(TSTATE_RUN);

	while( true )
	{
		if(pDbThread->isStop()==true){
			gLog->Write("[ DbThread is Stop!][%s]", pDbThread->getCampaignNoStr());

			break;
		}
		if(pDbThread->getRunState() == TSTATE_PAUSE){
			sleep(1);
			continue;
		}
		
		if(pDbThread->getRMQUsable() == false){
			//RMQ 연결을 시도한다.
			//성공한 경우 
			pDbThread->reconnRMQConn();
			if(pDbThread->getRMQUsable() == false){
				sleep(THREAD_RMQ_WAIT_TIME);
				continue;
			}
		}
		
		if(pDbThread->getCpStep() == CPSTEP1_DbReadComplete){
			//등록된 전송될 메일을 전부 큐에 보냈다면 전송상태 체크하여 스레드 대기처리
			pDbThread->checkCpMailSendComplete();
			sleep(2);
		}
		else if(pDbThread->getCpStep() == CPSTEP2_SendComplete){
			//[Do Nothing!]
			sleep(1);
			continue;
		}
		else{
			pDbThread->GetCampaignCpMail();
		}

		if(pDbThread->m_iSCount > MAX_DB_CHECK_WAIT_COUNT){ 
			sleep(1);
		}
		else{
			usleep(THREAD_SLEEPTIME_1MSEC*(10 + pDbThread->m_iSCount*300));
		}
	}

	return NULL;
}


//-------------------------------------
// checkCpMailSendComplete
//-------------------------------------
bool CEmsDbThread::checkCpMailSendComplete()
{
	bool bResult = false;

	if(getCpStep() != CPSTEP2_SendComplete){
		
		//DB Check
		MYSQL_RES *pMySQLResult;
		MYSQL_ROW  MySQLRow;
	
		int  RowCnt = 0;
	
		//DB 체크하여 CpMail 전송 상태가 전송 대기 혹은 전송중인 것이 없는 경우 전송 완료로 처리한다.
		//차후 재전송 로직이 필요한 경우에러 항목에 대해서 재전송 리스트 구성하여 주기적으로 재전송 하는 로직이 필요함
		
		snprintf(m_strQuery, Qry_StrlenBig, QrySel_Mail_UnComplete, getMailDBName(), m_CampaignNo, m_iMaxRetryCount);
		bResult = m_EmsMysql.SelectDB_GetResult(m_strQuery, &RowCnt, 1);
		
		pMySQLResult = m_EmsMysql.mpMySQLResult;
		MySQLRow     = mysql_fetch_row(pMySQLResult);
		
		if( (bResult == false) || (RowCnt == 0) ){
			return false;
		}
		
		int iRetCount = atoi(RowStr(0));
	
		m_EmsMysql.SelectDB_FreeResult();
	
		if(iRetCount > 0) {  //0보다 큰 경우 아직 처리중이거나 처리해야할 메일이 남아있음
			return false;
		}
		else {
			// 처리해야할 메일이 더 이상 존재하지 않음
			setCampaignStep(CPSTEP2_SendComplete);
			bResult = true;
		}
	}
	else{ //(getCpStep() == CPSTEP2_SendComplete)
		gLog->Write("[CPSTEP2_SendComplete][Campaign No:%s][%s]", m_CampaignNo, (getDelMailBody()==true?"Y":"N"));
		bResult = true;
	}

	return bResult;
}

//-------------------------------------
// Set Mail DB Name 
//-------------------------------------
void CEmsDbThread::setMailDBName(const char *pMailDBName)
{
	memset(m_chMailDBName,   0, SZ_NAME+1);
	strcpy(m_chMailDBName, pMailDBName);
}


//-------------------------------------
// ConnectDB 
//-------------------------------------
bool CEmsDbThread::ConnectDB(const char *l_pszHost, const char *l_pszUser, const char *l_pszPasswd, const char *l_pszDBName, unsigned int l_iPort)
{
	char strConnName[SZ_NAME+1] = {0,};
	
	try
	{
		strcpy(m_chDBIPAddr, l_pszHost);
		strcpy(m_chDBUser,   l_pszUser);
		strcpy(m_chDBPasswd, l_pszPasswd);
		strcpy(m_chDBName,   l_pszDBName);
		m_iDBPort = l_iPort;
		
		if(strlen(m_chMailDBName)==0){
			strcpy(m_chMailDBName, l_pszDBName);
		}

		//m_EmsMysql.SetOwnerName("EmsDbThread");
		snprintf(strConnName, SZ_NAME, "%s", "EmsDbThread");
		m_EmsMysql.SetOwnerName(strConnName);
		m_EmsMysql.SetHost(m_chDBIPAddr, m_chDBUser, m_chDBPasswd, m_chDBName, m_iDBPort);
		
		if(m_EmsMysql.ConnectDB() == false)
			throw 0;
			
		memset(strConnName, 0, SZ_NAME+1);
		snprintf(strConnName, SZ_NAME, "%s_Sub", "EmsDbThread");
		m_EmsMysqlSub.SetOwnerName(strConnName);
		m_EmsMysqlSub.SetHost(m_chDBIPAddr, m_chDBUser, m_chDBPasswd, m_chDBName, m_iDBPort);
	
		if(m_EmsMysqlSub.ConnectDB() == false)
			throw 1;

	}
	catch (int iErr) {
		gLog->Write("[%s][%s][%d][ERROR][DB Connect Fail][Key:%s][IP:%s][DBName:%s][User:%s][Password:%s][Port:%d]", __FILE__, __FUNCTION__, __LINE__, strConnName, m_chDBIPAddr, m_chDBUser, m_chDBPasswd, m_chDBName, m_iDBPort);
		assert(false); //ischoi - 테스트
		return false;
	}

	catch (...)
	{
		gLog->Write("[%s][%d][ERROR][Key:%s][IP:%s][DBName:%s][User:%s][Password:%s][Port:%d]", __FUNCTION__, __LINE__, strConnName, m_chDBIPAddr, m_chDBUser, m_chDBPasswd, m_chDBName, m_iDBPort);
		assert(false); //ischoi - 테스트
		return false;
	}

	return true;
}


//-------------------------------------
// sendToRMQCpMail - Rabbitmq로 CpMail 메시지를 전송
//--------------------------------------
bool CEmsDbThread::sendToRMQCpMail(MYSQL_ROW &MySQLRow)
{
	//no(0),mailidx(1),campaign_no(2),to_name(3),to_id(4),to_domain(5),try_cnt(6),wide_str(7)
	//no(0),mailidx(0),campaign_no(0),to_name(0),to_id(0),to_domain(0),try_cnt(0),email(0),wide_str(0)
	bool  bResult     = false;
	bool  bWstrState  = false;
	char *pCampaignNo = getCampaignNoStr();
	AMQPExchange * pEx = getExchange();
	if(pEx == NULL){
		return bResult;
	}
	try{
		int ichknum=0;
		pEx->setHeader("Delivery-mode", 2);
	  pEx->setHeader("Content-type",     "text/plain");
	  pEx->setHeader(MHEADER_GROUPNUMBER,getGroupNoToStr(), true);
	  pEx->setHeader(MHEADER_QINDEX,     pCampaignNo,true);
		pEx->setHeader(MHEADER_CPNUMBER,   pCampaignNo,true);
		pEx->setHeader(MHEADER_CPMAILKEY,  m_CpMailKey,true);

		//mail_no
		if(strlen(RowStr(0)) == 0)
			throw ichknum;
		else{
			pEx->setHeader(MHEADER_MAILNUMBER, RowStr(0),  true);
			ichknum++;
		}
    //mailidx
		if(strlen(RowStr(1)) == 0)
			throw ichknum;
		else{
			pEx->setHeader(MHEADER_MAILINDEX,  RowStr(1),  true);
			ichknum+=2;
		}
		//to_name
		pEx->setHeader(MHEADER_TONAME,     RowStr(3),  true);
		ichknum++;
		//to_id
		if(strlen(RowStr(4)) == 0)
			throw ichknum;
		else{
			pEx->setHeader(MHEADER_TOID,       RowStr(4),  true);
			ichknum++;
		}
		//to_domain
		if(strlen(RowStr(5)) == 0)
			throw ichknum;
		else{
			pEx->setHeader(MHEADER_TODOMAIN,   RowStr(5),  true);
			ichknum++;
		}
		//try_cnt
		pEx->setHeader(MHEADER_TRYCOUNT,   RowStr(6),  true);
	}
	catch(int chknum){
		gLog->Write("[%s][ERROR][SEND_M][Error Pos:%d]", __FUNCTION__, chknum);
		return false;
	}
	catch(...){
		gLog->Write("[%s][ERROR][SEND_M][Unknown Error Occurred]", __FUNCTION__);
		return false;
	}

	if(strlen(RowStr(8)) > 0){
		pEx->setHeader(MHEADER_USEWSTR,        "1",  true);
		pEx->setHeader(MHEADER_WIDESTR,  RowStr(8),  true);
		bWstrState = true;
	}
	else{
		pEx->setHeader(MHEADER_USEWSTR,        "0",  true);
		pEx->setHeader(MHEADER_WIDESTR,         "",  true);
		bWstrState = false;
	}

	pEx->setHeader(MHEADER_TR_TYPE,   STR_MODE_M,  true);
	
	// [mail_msg_body] route_key를 Campaign_no로 사용함
	// 메시지 전송에 필요한 데이터는 헤더에 실어서 보냄. 
	// 메시지 body 부분은 일단 임시로 인덱스키를 넣어 전송.	pEx->Publish(m_CampaignNo, m_CampaignNo);  
	pEx->Publish(m_CampaignNo, m_CampaignNo);  //[mail_msg_body] route_key를 m_CampaignNo로 사용함

	gLog->Write("[%s][SEND_M][%s][%s][%s][%s][%s][%s][%s][%s]"
	           , __FUNCTION__
	           , RowStr(0), RowStr(1), RowStr(2), RowStr(3), RowStr(4), RowStr(5), RowStr(6), RowStr(7) );
	bResult = true;
	
	return bResult;
}


//-------------------------------------
// GetCpMail - campaign 별로 쓰레드가 실행됨
// CpMail을 검색하여 전송하고, 전송된 개수를 리턴
//--------------------------------------
int CEmsDbThread::GetCpMail( int rCount)  //rCount : DB에서 읽어올 CpMail 개수
{
	bool bError   = false;
	bool bResult  = false;
	int  RowCnt   = 0;
	int  LoopCnt  = 0;
	
	snprintf(m_strQuery, Qry_StrlenBig, QrySel_Mail_Info ADD_LIMIT, getMailDBName(), m_CampaignNo, rCount);
	bResult = m_EmsMysql.SelectDB_GetResult(m_strQuery, &RowCnt, FldCnt_Mail_Info);

	if(bResult == false)
		return 0;

	if(RowCnt == 0){
		if(m_iSCount <= MAX_DB_CHECK_WAIT_COUNT)
			m_iSCount++;
		return 0;
	}
	else{
		if(m_iSCount > MAX_DB_CHECK_WAIT_COUNT){ //이전의 상태가 CPSTEP1_DbReadComplete 인 경우
			setCampaignStep(CPSTEP0_DbReadRunning);
		}
		m_iSCount=0;
	}

	//------------------------------------------------------//
	//--ischoi 전송할 메일 정보를 가져와서 큐에 메시지를 보냄
	//------------------------------------------------------//
	MYSQL_ROW   MySQLRow;
	MYSQL_RES  *pMySQLResult = m_EmsMysql.mpMySQLResult;
	const char *pMailNo ;
	
	while((MySQLRow = mysql_fetch_row(pMySQLResult)) != NULL)
	{
		//no(0),mailidx(1),campaign_no(2),to_name(3),to_id(4),to_domain(5),try_cnt(6),wide_str(7)
		pMailNo    = RowStr(0);    //mail_no
				
		try{
			bResult = sendToRMQCpMail(MySQLRow);
		}
		catch(AMQPException ex){
			gLog->Write("[%s][%s][%d][ERROR] AMQPException Error Occurred![ErrMsg:%s]", __FILE__, __FUNCTION__, __LINE__, ex.getMessage().c_str());
			resetRMQConn();
			bError = true;
			break;
		}
		catch(...){
			gLog->Write("[%s][%s][%d][ERROR] Unknown Error Occurred!", __FILE__, __FUNCTION__, __LINE__);
			bError = true;
			break;
		}

		if(bResult == true){
			snprintf(m_strQuery, Qry_StrlenBig, QryUdt_Mail_SmtpStep0, getMailDBName(), pMailNo);
			bResult = m_EmsMysqlSub.QueryDB(m_strQuery);

			if(bResult == false)
			{
				gLog->Write("[%s][%s][%d][ERROR] Mail Update SmtpStep Failed[Index:%s]", __FILE__, __FUNCTION__, __LINE__, RowStr(1));
				usleep(100);
				continue;
			}
			else 
				LoopCnt++;
		}
		else {
			snprintf(m_strQuery, Qry_StrlenBig, QryUdt_Mail_SmtpStepM2, getMailDBName(), pMailNo);
			bResult = m_EmsMysqlSub.QueryDB(m_strQuery);
		}
		
		usleep(100);
	}

	m_EmsMysql.SelectDB_FreeResult();

	if(bError == true)
		return -1;
	else
		return LoopCnt;
}

//-------------------------------------
// getEnqueueCount : Queue에 저장된 Count
//-------------------------------------
int CEmsDbThread::getEnqueueCount()
{
	try{
		if(m_amqpQueue == NULL)
			return -1;
		else{
			m_amqpQueue->Declare(m_CampaignNo, AMQP_DURABLE);
			return m_amqpQueue->getCount();
		}
	}
	catch(AMQPException ex){
		gLog->Write("[%s][%s][%d][ERROR][MSG:%s]", __FILE__, __FUNCTION__, __LINE__, ex.getMessage().c_str());
		return -3;
	}
	catch(...){
		gLog->Write("[%s][%s][%d][ERROR][Unknown Error Occurred]", __FILE__, __FUNCTION__, __LINE__);
		return -2;
	}
}

//-----------------------------------------------------
// GetCampaignCpMail (쓰레드 실행함수)
//-----------------------------------------------------
void CEmsDbThread::GetCampaignCpMail()
{
	int enqueue_count = getEnqueueCount();
	if(enqueue_count >= MAX_ENQUEUE_SIZE){
		usleep(100*THREAD_SLEEPTIME_1MSEC);
		return;
	}
	else if(enqueue_count < 0 ){
		//Error Occurred
		resetRMQConn();
		return;
	}
	
	//<= CpStep이 1이면 GetCpMail할필요 없음, 2이면 vec에서 삭제해야하고, 일정 타임에만 Check_CampaignComplete 하면됨, 
	//Check_CampaignComplete_ByDb();
	//save Check_CampaignComplete_ByCampaignCompleteCnt(); //[By Complete]

	//CampaignIndex별로 처리를 하므로 Campaign 리스트는 필요 없음
	int RowCnt = GetCpMail(MAX_ENQUEUE_SIZE - enqueue_count);
	
	if(RowCnt == 0) //CpMail이 없는 경우 읽기 완료 상태로 Campaign 상태 변경
	{
		if( m_iSCount > MAX_DB_CHECK_WAIT_COUNT){ //Row가 없는 상태에서 일정시간이 지난 경우
			if(getCpStep() < CPSTEP1_DbReadComplete){
				setCampaignStep(CPSTEP1_DbReadComplete);
			}
		}
	}
	else
	{  //(입력된  Row가 있다면)
		if(getCpStep() > CPSTEP0_DbReadRunning){
			setCampaignStep(CPSTEP0_DbReadRunning);
		}
	}
}


//-------------------------------------
// CpStep을 변경할 경우 m_bCpStepFlag 변경
//-------------------------------------
void CEmsDbThread::setCpStep(int stepState)
{
	m_CpStep = stepState; 
	m_bCpStepFlag = true; 
}

//-------------------------------------
// CpStep의 변경 사항을 가져간 경우 m_bCpStepFlag를 0으로 변경
//-------------------------------------
bool CEmsDbThread::getCpStepFlag()
{
	bool bRet = m_bCpStepFlag;
	m_bCpStepFlag = false;
	return bRet; 
}

//-------------------------------------
// Set Campaign Step Function
//-------------------------------------
bool CEmsDbThread::setCampaignStep(int iCpStep)
{
	//"UPDATE Campaign SET CpStep=0 WHERE mailkey='%s'"
	//Campaign의 Update를 처리해야하기 때문에 Update는 여기서 하지 않고  EmsDbThread의 CpStep 값만 변경한다.
	
	bool        bResult  = true;
	const char *strQuery = NULL;
		
	if((iCpStep >= CPSTEP0_DbReadRunning) && (iCpStep <= CPSTEP2_SendComplete)){
		if(iCpStep != getCpStep()){
			switch(iCpStep){
				case CPSTEP0_DbReadRunning   : //0
					strQuery = QryUdt_Campaign_CpStep0;
					break;
				case CPSTEP1_DbReadComplete  : //1
					strQuery = QryUdt_Campaign_CpStep1;
					break;
				case CPSTEP2_SendComplete    : //2
				{
					if(getDelMailBody()==true)
						strQuery = QryUdt_Campaign_CpStepComplete_Del_mailbody;
					else
						strQuery = QryUdt_Campaign_CpStepComplete;
					break;
				}
			}
			
			if(strQuery != NULL){
				snprintf(m_strQuery, Qry_StrlenBig, strQuery, m_CampaignNo);
				bResult = m_EmsMysqlSub.QueryDB(m_strQuery);
				setCpStep(iCpStep);
			}
		}
	}
	else
		bResult = false;
	
	return bResult;
}

//-------------------------------------
// DB업데이트 시간 비교 함수
//-------------------------------------
long CEmsDbThread::getTimeDiff(char *timestamp)
{
  struct timeval curTime;        
  struct timeval lastTime;       
  struct tm      lastTm;
  
  long  rtn        = 0; 
  int   ipos       = 0;
  char  tmp_year[5]={0,};
  char  tmp_mon [3]={0,};
  char  tmp_day [3]={0,};
  char  tmp_hour[3]={0,};
  char  tmp_min [3]={0,};
  char  tmp_sec [3]={0,};

  gettimeofday(&curTime, NULL);

  memcpy(&tmp_year,  timestamp, 4);
  tmp_year[4]= '\0'; ipos += 5;
  memcpy(&tmp_mon,   timestamp+ipos, 2);
  tmp_mon[2]= '\0';  ipos += 3;
  memcpy(&tmp_day,   timestamp+ipos, 2);
  tmp_day[2]= '\0';  ipos += 3;
  memcpy(&tmp_hour,  timestamp+ipos, 2);
  tmp_hour[2]= '\0'; ipos += 3;
  memcpy(&tmp_min,   timestamp+ipos, 2);
  tmp_min[2]= '\0';  ipos += 3;
  memcpy(&tmp_sec,   timestamp+ipos, 2);
  tmp_sec[2]= '\0';  

  lastTm.tm_year  = atoi(tmp_year) - 1900;
  lastTm.tm_mon   = atoi(tmp_mon) -1;
  lastTm.tm_mday  = atoi(tmp_day);
  lastTm.tm_hour  = atoi(tmp_hour);
  lastTm.tm_min   = atoi(tmp_min);
  lastTm.tm_sec   = atoi(tmp_sec);
  lastTime.tv_sec = mktime(&lastTm);
  
  rtn = curTime.tv_sec - lastTime.tv_sec;
  return rtn;
}

void CEmsDbThread::rollbackDBUpdate()
{
	shared_ptr<CEmsQueue> msgBackupQ = procGetMessagesFromQueue();
	if(msgBackupQ.get() != NULL){
		gLog->Write("[%s][%d][QueueSize:%d]", __FUNCTION__, __LINE__, msgBackupQ.get()->getQueueSize());
		
		//QryUdt_Mail_SmtpStepM1           "UPDATE %s.mails SET smtp_step=-1, send_date='0000-00-00 00:00:00' WHERE no IN ( %s )  "
		shared_ptr<CCpMail> spCpMail;
		string              strMailNoList;
		bool                bFlag       = false;
		bool                bResult     = false;
		int                 iMaxCount   = 30;
		char               *pMailDBName = getMailDBName();
		char                strQuery[Qry_StrlenBig+1] = {0,};
		
		while(msgBackupQ.get()->getQueueSize() > 0){
			memset(strQuery, 0, Qry_StrlenBig+1);
			strMailNoList.clear();
			bFlag = false;

			for(int iCount = 0; msgBackupQ.get()->getQueueSize() > 0;){

				spCpMail = msgBackupQ.get()->getCpMail();
				if(spCpMail.get() == NULL)
					continue;

				if(bFlag == false){
					strMailNoList = spCpMail.get()->getMailInfo(MHEADER_MAILNUMBER);
					bFlag = true;
				}
				else{
					strMailNoList += ",";
					strMailNoList += spCpMail.get()->getMailInfo(MHEADER_MAILNUMBER);
				}
				
				if((++iCount) >= iMaxCount)
					break;   //for() break;
			}

			snprintf(strQuery, Qry_StrlenBig, QryUdt_Mail_SmtpStepM1, pMailDBName, strMailNoList.c_str());
			bResult = m_EmsMysqlSub.QueryDB(strQuery);
			gLog->Write("[%s][%d][Query:%s]", __FUNCTION__, __LINE__, strQuery);
		}
	}
}


shared_ptr<CEmsQueue> CEmsDbThread::procGetMessagesFromQueue()
{
	shared_ptr<CEmsQueue> spEmsQueue     = shared_ptr<CEmsQueue>(new CEmsQueue);
	AMQPQueue            *pAMQPMsgQueue  = m_amqpQueue;
	
	if(pAMQPMsgQueue != NULL){
		while(true){
			try{
				pAMQPMsgQueue->Get(AMQP_NOACK);
				AMQPMessage *msg = pAMQPMsgQueue->getMessage();
				
				if(msg == NULL){
					break; // Message is Null
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
								gLog->Write("[%s][%s][%d] Header[%s]:[%s]", __FILE__, __FUNCTION__, __LINE__, tmpType, tmpData );
							}
						}
						//------------------------------------
						
						spEmsQueue.get()->addCpMail(tmpspCpMail);
					}
					else{
						gLog->Write("[%s][%s][%d]Create CpMail Failed", __FILE__, __FUNCTION__, __LINE__);
					}
				}
				else
					break;
		
			}
			catch(AMQPException ex){
				//Error Occurred :  AMQPException::AMQP_RESPONSE_SERVER_EXCEPTION
				gLog->Write("[%s][%s][%d][ERROR] AMQP Connection Error Occurred![CODE:%d][%s]!",
				            __FILE__,  __FUNCTION__, __LINE__, (int)ex.getReplyCode(), ex.getMessage().c_str() );
			}
			catch(...){
				gLog->Write("Unknown ERROR Occurred!");
			}
		}
	}
	else{
		string strErrMsg = "AMQP Message Queue Connection is null";
		gLog->Write("[ERROR][%s][GroupNo:%s][QName:%s]", strErrMsg.c_str(), getGroupNoToStr(), getCpMailKey());
	}
	
	return spEmsQueue;
}

