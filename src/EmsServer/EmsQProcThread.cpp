//##############################################################################################
#include "EmsQProcThread.h"
#include "FuncString.h"
//#include <boost/shared_array.hpp>
#include "EmsLog.h"
//#include "EmsDefine.h"
#include "EmsConfig.h"

extern CEmsLog *gLog;
extern const char *getMsgHeaderStringFromKey(int hNumber);
extern const char *getQryStringFromKey(int hNumber, int iGroupNo, int iTaxGroupNo);

//--------------------------------------------------------------------
// CEmsQProcThread Instance
//--------------------------------------------------------------------
CEmsQProcThread *CEmsQProcThread::m_pEmsQProcThread = NULL;

CEmsQProcThread *CEmsQProcThread::getInstance()
{	
	if(m_pEmsQProcThread == NULL){
		do{
			m_pEmsQProcThread = new CEmsQProcThread();
			if(m_pEmsQProcThread != NULL) {
				if(m_pEmsQProcThread->getRunState()==TSTATE_PREREADY){
					delete m_pEmsQProcThread;
					m_pEmsQProcThread = NULL;
				}
				else{
					break;
				}
			}
			sleep(1);
		}while(true);
	}
	
	return m_pEmsQProcThread;
}
	

//--------------------------------------------------------------------
// CEmsQProcThread 
//--------------------------------------------------------------------
//
// �ʱ� �������� Get���� ������� Send�� ���� ���� 
// Get���� �� ��쿡�� Exchange �ʿ�
// �α׿� ���ؼ� ������� �����ؾ� �ϴ� ��찡 �����Ƿ�
//CEmsQProcThread::CEmsQProcThread(const char * pQName, const char *opt)
CEmsQProcThread::CEmsQProcThread()
: m_strQName       (RMQ_ROUTE_KEY_LOG)
, m_bIsStop        (false)
, m_iQProcOpt      (ILOG_OPT_SENDER)
, m_iRunState      (TSTATE_PREREADY)
, m_amqpQueue      (NULL)
, m_amqpExchange   (NULL)
, m_pEmsAgentServer(NULL)
{
	//m_EmsMysql.mIsUseLock    = true;
	m_EmsMysqlSub.mIsUseLock = true;

	if(m_strQName.length() == 0){
		setRunState(TSTATE_PREREADY);
	}	
	else
		setRunState(TSTATE_READY);
	
	m_iTaxGroupNo = theEMSConfig()->getTaxGroupNo();
	if(m_iTaxGroupNo <= 0){
		m_iTaxGroupNo = I_GROUP_TAX;
	}
}

//--------------------------------------------------------------------
// ~CEmsQProcThread 
//--------------------------------------------------------------------
CEmsQProcThread::~CEmsQProcThread()
{		
	if(m_bIsStop == false){
		m_bIsStop = true;
	}
	
	pthread_join(m_QMsgProcThread, NULL);
}

//--------------------------------------------------------------------
// StartThread 
//--------------------------------------------------------------------
void CEmsQProcThread::StartThread()
{
	//�޽��� ť ���� & ����
	if(connProcQueue() == false){
		gLog->Write("[%s][ERROR][Create Connection Process Queue Failed!]", __FUNCTION__);
		return;
	}
	

	pthread_create(&m_QMsgProcThread, NULL, &msgProcThreadFunc, this);
	gLog->Write("[%s][CEmsQProcThread Create Success!]", __FUNCTION__);
}

//--------------------------------------------------------------------
//  �ʱ� ���� option�� ���� Connection ����
//--------------------------------------------------------------------
bool CEmsQProcThread::connProcQueue()
{
	bool retval = true;	
	retval = setLogSender();

	if(m_iQProcOpt == ILOG_OPT_PROC){
		retval = setLogProcess();
	}
	
	return retval;
}


//--------------------------------------------------------------------
// Log Sender : Create Exchange Channel
//--------------------------------------------------------------------
bool CEmsQProcThread::setLogSender()
{	
	do{
		m_amqpExchange = theEmsRMQManager()->getExchange(RMQ_EXCHANGE_LOG, m_strQName.c_str());
		if(m_amqpExchange != NULL)
				return true;
		else
			sleep(1);
	}while(true);

	return true;
}

//--------------------------------------------------------------------
// Log Message Processer : Create Queue Channel
//--------------------------------------------------------------------
bool CEmsQProcThread::setLogProcess()
{	
	do{
		m_amqpQueue = theEmsRMQManager()->getQueue(RMQ_EXCHANGE_LOG, m_strQName.c_str());
		if(m_amqpQueue != NULL)
				return true;
		else
			sleep(THREAD_RMQ_WAIT_TIME);
	}while(true);
}

//--------------------------------------------------------------------
// Channel Recreate
//--------------------------------------------------------------------
bool CEmsQProcThread::reConnProcQueue()
{	
	theEmsRMQManager()->deleteRMQConn(RMQ_EXCHANGE_LOG, m_strQName.c_str(), false);
	sleep(1);
	m_amqpExchange = theEmsRMQManager()->getExchange(RMQ_EXCHANGE_LOG, m_strQName.c_str());

	if(m_amqpExchange != NULL)
		return true;
	else
		return false;
}


//--------------------------------------------------------------------
//  �޽��� ť�κ��� �޽����� ������
//--------------------------------------------------------------------
AMQPMessage *CEmsQProcThread::getMessage()
{
	AMQPMessage  * pTmpMsg = NULL;
	if(m_amqpQueue != NULL){
		m_amqpQueue->Get(AMQP_NOACK);
		pTmpMsg = m_amqpQueue->getMessage();
	}
	return pTmpMsg;	
}


//--------------------------------------------------------------------
//  �޽��� ó��: �α� ���(File)
//--------------------------------------------------------------------
void CEmsQProcThread::procMessage_debug(AMQPMessage *pMsg)
{
	uint32_t j         = -1;
	char    *pBodyData = pMsg->getMessage(&j);
	gLog->Write("[%s][RECV LOG] %s", __FUNCTION__, pBodyData );
		
}


//-------------------------------------
// procMessage_DBQueryProc 
//--------------------------------------
void CEmsQProcThread::procMessage_DBQueryProc(AMQPMessage *pMsg)
{

	//uint32_t    j           = -1;
	//char       *pBodyData   = pMsg->getMessage(&j);
	const char *pData       = NULL;
	const char *pCpNo       = NULL;
	const char *pMode       = NULL;
	const char *pGroupNo    = NULL;
	const char *pDBName     = NULL;
	const char *pMailDBName = NULL;
	int         iMsgMode    = MODE_NONE;
	string      strDBName;
	string      strMailDBName;

	pMode = pMsg->getHeader(MSGHEADER_TR_TYPE).c_str();
	if(pMode == NULL){
		gLog->Write("[%s][%s][%d][MODE]:[NULL]", __FILE__, __FUNCTION__, __LINE__);
		return;
	}
	else{
		iMsgMode = atoi(pMode);
		//gLog->Write("[%s][%s][%d][MODE]:[%s][%d]]", __FILE__, __FUNCTION__, __LINE__, pMode, iMsgMode);
	}
	//gLog->Write("[%s][%s][%d][MODE]:[%s][RECV][Len:%d][Body:%s][CampaignList:%d]", __FILE__, __FUNCTION__, __LINE__, pMode, j, pBodyData, m_pCampainList->size() );
	
	{
		pCpNo         = pMsg->getHeader(MSGHEADER_CPNUMBER).c_str();
		pData         = pMsg->getHeader(MSGHEADER_DBQUERY_SET).c_str();
		pGroupNo      = pMsg->getHeader(MSGHEADER_GROUPNUMBER).c_str();
		pDBName       = getDBName();
		pMailDBName   = getMailDBNameFromCpNo(pCpNo, iMsgMode);
		
		if((pDBName == NULL) || (pMailDBName == NULL)){
			//�ܹ������� Campaign Number�� ���� DB Name �����ͼ� ó���ϵ����Ѵ�.(DB Connection�� m_mapDBConnInfo���� ����)
			/* ischoi --���� �������� üũ�غ���
			gLog->Write("[%s][%s][%d][ERROR][CPNO:%s][DB Name:%s][MODE:%s]", __FILE__, __FUNCTION__, __LINE__, (pCpNo==NULL?"NULL":pCpNo), (pMailDBName==NULL?"NULL":pMailDBName), (iMsgMode==MODE_I?"I":"M"));
			return;
			*/
			//--ischoi pMailDBName�� ���� ��� ã�Ƽ� ó���ϵ��� �Ѵ�.
			//         ó�� �޽����� �����ϴ� ���� ������ ��
			pMailDBName = getMailDBName(pCpNo, iMsgMode);
			
			if(pMailDBName == NULL){
				gLog->Write("[%s][%s][%d][ERROR][CPNO:%s][DB Name:%s][MODE:%s]", __FILE__, __FUNCTION__, __LINE__, (pCpNo==NULL?"NULL":pCpNo), (pMailDBName==NULL?"NULL":pMailDBName), (iMsgMode==MODE_I?"I":"M"));
				return;
			}
			//else{
			//	gLog->Write("[%s][%s][%d][CPNO:%s][DB Name:%s]", __FILE__, __FUNCTION__, __LINE__, (pCpNo==NULL?"NULL":pCpNo), (pMailDBName==NULL?"NULL":pMailDBName));
			//}
		}
		
		strMailDBName = string(pMailDBName) ;
		pMailDBName = strMailDBName.c_str();
		if((pData != NULL) && (strlen(pData) > 0)){
			bool bQueryExist = true;
			int  iType       = atoi(pData);
			int  iGroupNo    = atoi(pGroupNo);
			char strLogBuf[Qry_StrlenBig]={0,};
			
			
			if((iType > iQryIns_Start)&&(iType < iQryIns_End)){
				const char *pQuery = getQryStringFromKey(iType, iGroupNo, getTaxGroupNo());
				//gLog->Write("[%s][%s][%d][TYPE:%s][DB Name:%s]\n[Query:%s]", __FILE__, __FUNCTION__, __LINE__, pData, pMailDBName, pQuery);
				if(pQuery != NULL){					
					switch(iType){
						case iQryUdt_Campaign_CpStep0:
						{
							snprintf(strLogBuf, Qry_StrlenBig, QryUdt_Campaign_CpStep0
							                  , pMsg->getHeader(MSGHEADER_CPNUMBER).c_str());
							break;
						}		
						case iQryUdt_Campaign_CpStep1:
						{
							snprintf(strLogBuf, Qry_StrlenBig, QryUdt_Campaign_CpStep1
							                  , pMsg->getHeader(MSGHEADER_CPNUMBER).c_str());
							break;
						}
						case iQryUdt_Campaign_CpStepComplete:
						{
							snprintf(strLogBuf, Qry_StrlenBig, QryUdt_Campaign_CpStepComplete
							                  , pMsg->getHeader(MSGHEADER_CPNUMBER).c_str());
							break;
						}
						case iQryUdt_Mail_SmtpStep0         :  //QryUdt_Mail_SmtpStep0
						{  //"UPDATE %s.mails SET smtp_step=0 WHERE no=%s AND smtp_step=-1 "
							snprintf(strLogBuf, Qry_StrlenBig, pQuery
																, pDBName
							                  , pMsg->getHeader(MSGHEADER_MAILNUMBER).c_str());
							break;
						}
						case iQryUdt_Mail_SmtpStep21        :  //QryUdt_Mail_SmtpStep21
						{  //"UPDATE %s.mails SET send_date=NOW(),smtp_step=21,smtp_code=%s,try_cnt=%s WHERE no=%s  "
							snprintf(strLogBuf, Qry_StrlenBig, pQuery
																, pMailDBName
							                  , pMsg->getHeader(MSGHEADER_SMTPCODE).c_str()
							                  , pMsg->getHeader(MSGHEADER_TRYCOUNT).c_str()
							                  , pMsg->getHeader(MSGHEADER_MAILNUMBER).c_str());
							break;
						}
						case iQryUdt_Mail_SmtpStep21Chk7    :  //QryUdt_Mail_SmtpStep21Chk7
						{  //"UPDATE %s.mails SET send_date=NOW(),smtp_step=21,smtp_code=%s,try_cnt=%s WHERE no=%s AND smtp_code<>7 "
							snprintf(strLogBuf, Qry_StrlenBig, pQuery
																, pMailDBName
							                  , pMsg->getHeader(MSGHEADER_SMTPCODE).c_str()
							                  , pMsg->getHeader(MSGHEADER_TRYCOUNT).c_str()
							                  , pMsg->getHeader(MSGHEADER_MAILNUMBER).c_str());
							break;
						}
						case iQryUdt_Mail_SmtpStepComplete  :  //QryUdt_Mail_SmtpStepComplete
						{  //"UPDATE %s.mails SET send_date=NOW(),smtp_step=7, smtp_code=%s,try_cnt=%s WHERE no=%s "
							snprintf(strLogBuf, Qry_StrlenBig, pQuery
																, pMailDBName
							                  , pMsg->getHeader(MSGHEADER_SMTPCODE).c_str()
							                  , pMsg->getHeader(MSGHEADER_TRYCOUNT).c_str()
							                  , pMsg->getHeader(MSGHEADER_MAILNUMBER).c_str());
							break;
						}
						case iQryUdt_Mail_SmtpQRfail        :  //QryUdt_Mail_SmtpQRfail
						{  //"UPDATE %s.mails SET send_date=NOW(),smtp_code=801,try_cnt=%s WHERE no=%s AND smtp_step=7 "
							snprintf(strLogBuf, Qry_StrlenBig, pQuery
																, pMailDBName
								                , pMsg->getHeader(MSGHEADER_TRYCOUNT).c_str()
								                , pMsg->getHeader(MSGHEADER_MAILNUMBER).c_str());
							break;
						}
						case iQryUdt_Mail_Smtp              :  //QryUdt_Mail_Smtp
						{ //"UPDATE %s.mails SET send_date=NOW(),smtp_step=%s,smtp_code=%s,try_cnt=%s WHERE no=%s "
							snprintf(strLogBuf, Qry_StrlenBig, pQuery
																, pMailDBName
							                  , pMsg->getHeader(MSGHEADER_SMTPSTEP).c_str()
							                  , pMsg->getHeader(MSGHEADER_SMTPCODE).c_str()
							                  , pMsg->getHeader(MSGHEADER_TRYCOUNT).c_str()
							                  , pMsg->getHeader(MSGHEADER_MAILNUMBER).c_str());
							break;
						}
						case iQryUdt_Mail_SmtpChk7          :  //QryUdt_Mail_SmtpChk7
						{ //"UPDATE %s.mails SET send_date=NOW(),smtp_step=%s,smtp_code=%s,try_cnt=%s WHERE no=%s AND smtp_code<>7 "
							snprintf(strLogBuf, Qry_StrlenBig, pQuery
																, pMailDBName
							                  , pMsg->getHeader(MSGHEADER_SMTPSTEP).c_str()
							                  , pMsg->getHeader(MSGHEADER_SMTPCODE).c_str()
							                  , pMsg->getHeader(MSGHEADER_TRYCOUNT).c_str()
							                  , pMsg->getHeader(MSGHEADER_MAILNUMBER).c_str());
							break;
						}
						case iQryIns_CodeExp: 
						{ //"INSERT INTO %s.mails_detail (`mail_no`,`try_cnt`,`smtp_step`,`smtp_code`,`explain`) VALUES (%s,%s,%s,%s,'%s') ON DUPLICATE KEY UPDATE `smtp_step`=%s,`smtp_code`=%s,`explain`='%s' "
							snprintf(strLogBuf, Qry_StrlenBig, QryIns_CodeExp
																, pMailDBName
							                  , pMsg->getHeader(MSGHEADER_MAILNUMBER).c_str()
							                  , pMsg->getHeader(MSGHEADER_TRYCOUNT).c_str()
							                  , pMsg->getHeader(MSGHEADER_SMTPSTEP).c_str()
							                  , pMsg->getHeader(MSGHEADER_SMTPCODE).c_str()
							                  , pMsg->getHeader(MSGHEADER_ERR_STR).c_str()
							                  , pMsg->getHeader(MSGHEADER_SMTPSTEP).c_str()
							                  , pMsg->getHeader(MSGHEADER_SMTPCODE).c_str()
							                  , pMsg->getHeader(MSGHEADER_ERR_STR).c_str());
							break;
						}
						case iQryIns_CodeExpResult:
						{ //"INSERT INTO %s.mails_detail (`mail_no`,`try_cnt`,`smtp_step`,`smtp_code`,`explain`,`result_code`) VALUES (%s,%s,%s,%s,'%s',%s) ON DUPLICATE KEY UPDATE `smtp_step`=%s,`smtp_code`=%s,`explain`='%s',`result_code`=%s "
							snprintf(strLogBuf, Qry_StrlenBig, QryIns_CodeExpResult
																, pMailDBName
							                  , pMsg->getHeader(MSGHEADER_MAILNUMBER).c_str()
							                  , pMsg->getHeader(MSGHEADER_TRYCOUNT).c_str()
							                  , pMsg->getHeader(MSGHEADER_SMTPSTEP).c_str()
							                  , pMsg->getHeader(MSGHEADER_SMTPCODE).c_str()
							                  , pMsg->getHeader(MSGHEADER_ERR_STR).c_str()
							                  , pMsg->getHeader(MSGHEADER_ERR_CODE).c_str()
							                  , pMsg->getHeader(MSGHEADER_SMTPSTEP).c_str()
							                  , pMsg->getHeader(MSGHEADER_SMTPCODE).c_str()
							                  , pMsg->getHeader(MSGHEADER_ERR_STR).c_str()
							                  , pMsg->getHeader(MSGHEADER_ERR_CODE).c_str());
							break;
						}
						default : 
							bQueryExist = false;
					}
					
					if(bQueryExist == true){
		
						if(m_EmsMysqlSub.QueryDB(strLogBuf) == true){
							gLog->Write("[Query: %s]", strLogBuf );
							//gLog->Write("[QueryType:%d][GroupNo:%d][CampaigNo:%s][MailNo:%s]", iType, iGroupNo, pCpNo, pMsg->getHeader(MSGHEADER_MAILNUMBER).c_str());
						}
						else
							gLog->Write("[%s][%s][%d][ERROR][UPDATE Query][%s]", __FILE__, __FUNCTION__, __LINE__, strLogBuf);
		
					}
				}
				else{
					gLog->Write("[%s][%s][%d][ERROR][Query Type is not validate][%s:%d]", __FILE__, __FUNCTION__, __LINE__, pData, iType);
				}
			}
		}
	}
}


//--------------------------------------------------------------------
// �޽���ť �α� �޽��� ó��
//--------------------------------------------------------------------
void *CEmsQProcThread::msgProcThreadFunc(void *Param)
{
	CEmsQProcThread *pThread = (CEmsQProcThread *)Param;

	do{
		if(pThread->getEmsAgentServer()->getspCommonQThread().get()->getRunState() == TSTATE_RUN)
			break;
		else
			sleep(1);
	}while(true);
	
	pThread->setRunState(TSTATE_RUN);
	
	while( true )
	{
		if(pThread->isStop())
			break;
			
		if(pThread->getRunState() == TSTATE_PAUSE){
			sleep(1);
			continue;
		}

		//CMD Queue ����
		AMQPQueue * pQueue = pThread->getQueueConn();
		
		if(pQueue == NULL){
			if(pThread->setLogProcess()==true){
				pQueue = pThread->getQueueConn();
			}
			else
				continue;
		}
		
		try{
			pThread->setLogProcessEvent();
			//pThread Waitting & EMSLOG Queue Process
			//End Thread....
		}
		catch(AMQPException ex){
			gLog->Write("[AMQPException] Error Occurred :%s", ex.getMessage().c_str());
			sleep(1);
			pThread->reConnProcQueue();
		}
		catch(...){
			gLog->Write("[Exception] Unknown Error Occurred :%s");
			sleep(1);
			pThread->reConnProcQueue();
		}
	}

	gLog->Write( "[%s][%s][%d] End Thread!!", __FILE__, __FUNCTION__, __LINE__);

	return NULL;
}


//--------------------------------------------------------------------
// RMQ Log Queue �޽��� ó�� ���μ���
//--------------------------------------------------------------------
int CEmsQProcThread::onMessage( AMQPMessage * message ) 
{
	try{
		if(message == NULL){
			return 0;
		}
					
    const char * pMsgType = message->getHeader(MSGHEADER_TYPE).c_str();
    //gLog->Write("[%s][%s][%d] Msg Header Type[%s]", __FILE__,  __FUNCTION__, __LINE__, pMsgType);

    if(pMsgType != NULL){
    	int iType = atoi(pMsgType);
    	switch(iType){
    		case MSG_TYPE_DEBUG:{
    			theEmsQProcThread()->procMessage_debug(message);
      		break;
    		}
    		case MSG_TYPE_DBUPDATE:
    		case MSG_TYPE_DBINSERT:{
    			theEmsQProcThread()->procMessage_DBQueryProc(message);
      		break;
    		}
    		default:{
    			//Default Log: Type�� ���ǵǾ� ���� ���� �α�
	      	gLog->Write("[%s][%s][%d] Unknown Log Type[0x%04X]", __FILE__,  __FUNCTION__, __LINE__, iType);
    		}
    	}
    }
    else{
    	//Type�� ���� �α�
    	gLog->Write("[%s][%s][%d] Log Type NULL[%s]", __FILE__,  __FUNCTION__, __LINE__, pMsgType);
    }
	}
	catch(AMQPException ex){
		gLog->Write("[%s][%s][%d] AMQP Connection is Error Occurred! Do Reconnection Process\n[%d][%s]!\n",  
		                __FILE__,  __FUNCTION__, __LINE__, (int)ex.getReplyCode(), ex.getMessage().c_str() );
		AMQPQueue *tmpQ = message->getQueue();
		tmpQ->Cancel( message->getConsumerTag() );
	}
	return 0;
}


//--------------------------------------------------------------------
// RMQ Log Queue �޽��� ó�� ���μ���
//--------------------------------------------------------------------
int CEmsQProcThread::onCancel(AMQPMessage * message )
{
	gLog->Write("cancel tag=%s", message->getDeliveryTag());
	return 0;
}

//--------------------------------------------------------------------
// RMQ Log Queue �޽��� ó�� ���μ���
//--------------------------------------------------------------------
void CEmsQProcThread::setLogProcessEvent()
{
	
	AMQPQueue * pQueue = getQueueConn();

		try{
			if(pQueue != NULL){
				pQueue->addEvent(AMQP_MESSAGE, onMessage);
				pQueue->addEvent(AMQP_CANCEL,  onCancel );
				pQueue->Consume (AMQP_NOACK);
			}
			else
				throw (AMQPException("[Queue Is NULL]"));
		}
		catch(AMQPException amqpEx){
			pQueue = NULL;
			try{
				theEmsRMQManager()->deleteRMQConn(RMQ_EXCHANGE_LOG, m_strQName.c_str(), false);
				sleep(THREAD_RMQ_WAIT_TIME);
				pQueue = theEmsRMQManager()->getQueue(RMQ_EXCHANGE_LOG, m_strQName.c_str());
				setQueueConn(pQueue);
				if(pQueue == NULL)
					sleep(THREAD_RMQ_WAIT_TIME);
			}
			catch(...){
				pQueue = NULL;
				setQueueConn(pQueue);
				gLog->Write("[%s][%s][%d][ERROR][MSG:%s]", __FILE__, __FUNCTION__, __LINE__, "RabbitMQ Manager Reset Error Occurred.");
			}
		}
		catch(...){
			gLog->Write("[%s][%s][%d][ERROR][MSG:%s]", __FILE__, __FUNCTION__, __LINE__, "Unknown Error Occurred.");
		}
}

//--------------------------------------------------------------------
// Init Ems Message Process
//--------------------------------------------------------------------
bool CEmsQProcThread::InitProc()
{
	if(m_pEmsAgentServer!=NULL){
		//Campaign ����
		m_pCampainList        = m_pEmsAgentServer->getCampaignList();
		m_pstCampaignInfoList = m_pEmsAgentServer->getspCommonQThread().get()->getstCampaignInfoList();
		return true;
	}
	else
		return false;
}


//--------------------------------------------------------------------
// ConnectDB 
//--------------------------------------------------------------------
bool CEmsQProcThread::ConnectDB (const char  *l_pszHost
	                             , const char  *l_pszUser
	                             , const char  *l_pszPasswd
	                             , const char  *l_pszDBName
	                             , unsigned int l_iPort)
{
	bool   bResult       = false;
	string strThreadName = "EmsLogProcThread";
	
	if( (l_pszHost==NULL)||(l_pszUser==NULL)||(l_pszPasswd==NULL)||(l_pszDBName==NULL)){
		gLog->Write("[%s][%s][%d][ERROR][Internal error occurred]", __FILE__, __FUNCTION__, __LINE__);
		return false;
	}
	
	memset(m_chDBIPAddr, 0, SZ_IP4ADDRESS+1);
	memset(m_chDBName,   0, SZ_NAME+1);
	memset(m_chDBUser,   0, SZ_NAME+1);
	memset(m_chDBPasswd, 0, SZ_PASSWD+1);

	strcpy(m_chDBIPAddr, l_pszHost  );
	strcpy(m_chDBName,   l_pszDBName);
	strcpy(m_chDBUser,   l_pszUser  );
	strcpy(m_chDBPasswd, l_pszPasswd);
	
	m_iDBPort = l_iPort;

	strThreadName += "Sub";
	m_EmsMysqlSub.SetOwnerName(strThreadName.c_str());
	m_EmsMysqlSub.SetHost(m_chDBIPAddr, m_chDBUser, m_chDBPasswd, m_chDBName, m_iDBPort);

	bResult = m_EmsMysqlSub.ConnectDB();

	if(!bResult)
	{
		gLog->Write("E F[CEmsQProcThread::ConnectDB] E[DB Sub Connect Fail] V[l_pszDBName:%s]", l_pszDBName);
		return false;
	}

	return bResult;
}


//-------------------------------------
// Queue�� �ԷµǾ��ִ� �޽��� Count
//--------------------------------------
int CEmsQProcThread::getEnqueueCount()
{
	if(m_amqpQueue == NULL)
		return -1;
	else
		return (int)m_amqpQueue->getCount();
}


//-------------------------------------
// Campaign Number�� Mail DB Name �˻�
//--------------------------------------
const char * CEmsQProcThread::getMailDBNameFromCpNo(const char *pCpNo, int iCpMode)
{
	const char *pMailDBName = NULL;
	//gLog->Write("[%s][%s][%d][CpNo:%s][CpMode:%d][CPSize:%d]", __FILE__, __FUNCTION__, __LINE__, pCpNo, iCpMode, m_pCampainList->size());
	if(m_pCampainList == NULL){
		gLog->Write("[%s][%s][%d][m_pCampainList is NULL]", __FILE__, __FUNCTION__, __LINE__);
		return NULL;
	}
	
	if(iCpMode == MODE_M){
		unordered_map<string, shared_ptr<CEmsDbThread> >::iterator itr_dt = m_pCampainList->find(pCpNo);
		if(itr_dt != m_pCampainList->end()){
			pMailDBName = itr_dt->second.get()->getMailDBName();
		}
	}
	else{ //if(iCpMode == MODE_I)
		unordered_map<string, shared_ptr<stCampaignInfo> >::iterator itr_cil = m_pstCampaignInfoList->find(pCpNo);
		if(itr_cil != m_pstCampaignInfoList->end()){
			pMailDBName = itr_cil->second.get()->getDBName();
		}
		//else{
		//	gLog->Write("[%s][%s][%d][MODE_I][Campaign is Nothing][CPNO:%s]", __FILE__, __FUNCTION__, __LINE__, pCpNo);
		//}
	}

	//gLog->Write("[%s][%s][%d][pMailDBName:%s]", __FILE__, __FUNCTION__, __LINE__, pMailDBName);
	return pMailDBName;
}


//-------------------------------------
// Campaign Number�� Mail DB Name �����´�. ������ ����
//--------------------------------------
const char * CEmsQProcThread::getMailDBName(const char *pCpNo, int iCpMode)
{
	const char *pMailDBName = NULL;
	if(pCpNo == NULL){
		return (const char *)(NULL);
	}
	
	//if((iCpMode==MODE_I)||(iCpMode==MODE_M)){
	switch(iCpMode){
		case MODE_I: {
			shared_ptr<stCampaignInfo> spCampaignInfo	= m_pEmsAgentServer->getspCommonQThread().get()->getspCampaignInfo(pCpNo);
			if(spCampaignInfo.get() != NULL){
				pMailDBName = spCampaignInfo.get()->getDBName();
			}
			break;
		}
		case MODE_M:{
			//CampaignList�� �˻��Ͽ� Campaign_No�� �ִ��� �˻��Ѵ�.
			//������ CampaignList�� �߰��ϰ�, DB Name�� �����Ѵ�
			unordered_map<string, shared_ptr<CEmsDbThread> >::iterator itr_cl = m_pCampainList->find(pCpNo);
			shared_ptr<CEmsDbThread> spDbThread;
			
			if(itr_cl == m_pCampainList->end()){
				//�����Ƿ� �߰��Ѵ�.
				spDbThread = m_pEmsAgentServer->getDbNameFromeTmpCpList(pCpNo);
				if(spDbThread.get() == NULL){
					return (const char *)(NULL);
				}
				else{
					pMailDBName = (const char*)(spDbThread.get()->getDBName());
				}
			}
			else{
				//�����Ѵٸ� DB Name�� �����Ѵ�.
				spDbThread = itr_cl->second;
				if(spDbThread.get() != NULL)
					pMailDBName = (const char*)(spDbThread.get()->getDBName());
				else
					return (const char *)(NULL);
			}
			break;
		}
		default: {
			gLog->Write("[%s][%d][ERROR][Unknown Campaign Mode][MODE:%d]", __FUNCTION__, __LINE__, iCpMode);
			return (const char *)(NULL);
		}
	}

	return pMailDBName;
}

