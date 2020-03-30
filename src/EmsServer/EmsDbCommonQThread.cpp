#include "FuncString.h"
#include <boost/shared_array.hpp>
#include "EmsDbCommonQThread.h"
#include "EmsConfig.h"
#include "EmsLog.h"

extern CEmsLog * gLog;
//--------------------------------------------------------------------
// CEmsDbCommonQThread :
// 개인메일('P') 이 있는 경우 메시지를 생성 후
// RabbitMQ에 전송하여 메일 처리
//--------------------------------------------------------------------
CEmsDbCommonQThread::CEmsDbCommonQThread()
:	m_Thread               (0)
,	m_iRunState            (TSTATE_READY)
,	m_bIsStop              (false)
{
	// DB 설정 정보 초기화
	memset(m_chDBIPAddr, 0, SZ_IP4ADDRESS +1);
	memset(m_chDBName,   0, SZ_NAME+1);
	memset(m_chDBUser,   0, SZ_NAME+1);
	memset(m_chDBPasswd, 0, SZ_PASSWD+1);

	m_EmsMysql.mIsUseLock    = true;
	m_EmsMysqlSub.mIsUseLock = true;
	m_iMaxRetryCount         = theEMSConfig()->getMailRetryCount();

	m_spCommonQueue = shared_ptr<CEmsQueue>( new CEmsQueue() );

	time(&m_chkCompleteTime);
}

//--------------------------------------------------------------------
// ~CEmsDbCommonQThread 
//--------------------------------------------------------------------
CEmsDbCommonQThread::~CEmsDbCommonQThread()
{
	if(m_bIsStop == false){
		m_bIsStop = true;
	}
	
	pthread_join(m_Thread, NULL);
}


//--------------------------------------------------------------------
//메시지를 전송하기위해 COMMON Queue 리스트에 routeKey가 있는지 확인
//--------------------------------------------------------------------
AMQPQueue * CEmsDbCommonQThread::getAMQPQueue (const char *routeKey)
{
	AMQPQueue* pAMQPQueue = NULL;
	//routeKey를 이용하여 AMQPQueue를 리턴
	unordered_map<string, AMQPQueue* >::iterator itr_q = m_amqpQueueList.find(routeKey);
	if(itr_q != m_amqpQueueList.end()){ //존재
		pAMQPQueue = itr_q->second;
	}
	else{
		pAMQPQueue = theEmsRMQManager()->getQueue(RMQ_EXCHANGE_COMMON, routeKey);
		m_amqpQueueList.insert(std::pair<string, AMQPQueue *>(routeKey, pAMQPQueue));
	}
	return pAMQPQueue;
}

//--------------------------------------------------------------------
// 에러가 발생한 RMQ Queue Connection을 COMMON Queue 리스트에서 routeKey로 삭제
//--------------------------------------------------------------------
void CEmsDbCommonQThread::deleteAMQPQueue (const char *routeKey)
{
	unordered_map<string, AMQPQueue* >::iterator itr_q = m_amqpQueueList.find(routeKey);
	if(itr_q != m_amqpQueueList.end()){ //존재
		m_amqpQueueList.erase(itr_q);
	}
}

//--------------------------------------------------------------------
//메시지를 전송하기위해 COMMON Exchange 리스트에 routeKey가 있는지 확인
//--------------------------------------------------------------------
AMQPExchange * CEmsDbCommonQThread::getAMQPExchange (const char *routeKey)
{ 
	AMQPExchange* pAMQPExchange = NULL;
	unordered_map<string, AMQPExchange* >::iterator itr_ex = m_amqpExchangeList.find(routeKey);
	if(itr_ex != m_amqpExchangeList.end()){ //존재
		pAMQPExchange = itr_ex->second;
	}
	else{
		pAMQPExchange = theEmsRMQManager()->getExchange (RMQ_EXCHANGE_COMMON, routeKey);
		if(pAMQPExchange != NULL){
			m_amqpExchangeList.insert(std::pair<string, AMQPExchange* >(routeKey, pAMQPExchange));
		}
	}
	return pAMQPExchange;
}

//--------------------------------------------------------------------
// 에러가 발생한 RMQ Exchange Connection을 COMMON Exchange 리스트에서 routeKey로 삭제
//--------------------------------------------------------------------
void CEmsDbCommonQThread::deleteAMQPEx (const char *routeKey)
{ 
	unordered_map<string, AMQPExchange* >::iterator itr_ex = m_amqpExchangeList.find(routeKey);
	if(itr_ex != m_amqpExchangeList.end()){ //존재
		m_amqpExchangeList.erase(itr_ex);
	}
}



//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
void CEmsDbCommonQThread::deleteQ(const char *routeKey)
{
	theEmsRMQManager()->deleteRMQConn(RMQ_EXCHANGE_COMMON, routeKey, false);
}



//--------------------------------------------------------------------
// StartThread 
//--------------------------------------------------------------------
void CEmsDbCommonQThread::StartThread()
{		
	int retval = pthread_create(&m_Thread, NULL, &ThreadFunc, this);
	if(retval != 0){
		gLog->Write("[%s][%s][%d][ERROR] DB Thread Create Failed! ", __FILE__, __FUNCTION__, __LINE__);
	}	
}


//--------------------------------------------------------------------
// 비정상 종료된 Campaign 프로세스를 복구한다.
//--------------------------------------------------------------------
void CEmsDbCommonQThread::checkPrevCampaign()
{
	//DB Query : 진행 중 종료되었던 Campaign 검색
	char       strQuery[Qry_StrlenBig+1]={0,};
	MYSQL_RES *pMySQLResult;
	MYSQL_ROW  MySQLRow;
	int        RowCnt  = 0;
	bool       bResult = false;
	
	shared_ptr<CEmsQueue>  spCommonQueue = getspCommonQueue();

	//DB로부터 완료되지 않은 Campaign 정보를 수집
	for(int i=CPSTEP0_DbReadRunning; i<CPSTEP2_SendComplete; i++){
		memset(strQuery, 0, Qry_StrlenBig+1);

		snprintf(strQuery, Qry_StrlenBig, QrySel_Campaign_Info, i, STR_MODE_I);
		//gLog->Write("[%s][QRY:%s]", __FUNCTION__, strQuery);
		bResult = m_EmsMysql.SelectDB_GetResult(strQuery, &RowCnt, FldCnt_Campaign_Info);
		
		if( (bResult == false) || (RowCnt == 0) ){
			return;
		}
		else{  // 서버 시작전 실행 중이던 메시지 처리 상태가 0또는 1인 Campaign 리스트
			pMySQLResult = m_EmsMysql.mpMySQLResult;
			shared_ptr<CCpMail> spCpMail;
			while((MySQLRow = mysql_fetch_row(pMySQLResult)) != NULL ){
				
				//처리해야할 Campaign이 존재하므로 리스트에 넣어둔다.
				spCpMail = shared_ptr<CCpMail>(new CCpMail);
				//-------------------------------------------
				// no(0),mailkey(1),sender_name(2),sender_email(3),mail_title(4),mail_body(5),use_wstr(6),reset_cnt(7),db_name(8),group_no(9),tr_type(10),step(11), del_mail_body(12)
		
				if(strlen(RowStr(0)) > 0)
					spCpMail.get()->setMailInfo(MHEADER_CPNUMBER,    RowStr(0));
				if(strlen(RowStr(1)) > 0){
					spCpMail.get()->setMailInfo(MHEADER_CPMAILKEY,   RowStr(1));
				}
				if(strlen(RowStr(2)) > 0)
					spCpMail.get()->setMailInfo(MHEADER_SENDERNAME,  RowStr(2));
				if(strlen(RowStr(3)) > 0) {
					spCpMail.get()->setMailInfo(MHEADER_SENDEREMAIL, RowStr(3));
					char       *ptmpDomain = NULL;
					const char *pDomain    = NULL;
					ptmpDomain= strchr( const_cast<char*>(RowStr(3)), '@');
				
					if(ptmpDomain != NULL){
						pDomain = ptmpDomain+1;
					}
					else{
						pDomain = DEFAULT_DOMAIN_GABIA_COM;
					}
					spCpMail.get()->setMailInfo(MHEADER_SENDERDOMAIN, pDomain);
				}
		
				if(strlen(RowStr(4)) > 0)
					spCpMail.get()->setMailInfo(MHEADER_MAILTITLE,   RowStr(4));
				if(strlen(RowStr(6)) > 0)
					spCpMail.get()->setMailInfo(MHEADER_USEWSTR,     RowStr(6));
				if(strlen(RowStr(8)) > 0)
					spCpMail.get()->setMailInfo(MHEADER_DBNAME,      RowStr(8));
				if(strlen(RowStr(9)) > 0)
					spCpMail.get()->setMailInfo(MHEADER_GROUPNUMBER, RowStr(9));
				if(strlen(RowStr(10)) > 0)
					spCpMail.get()->setMailInfo(MHEADER_TR_TYPE,     RowStr(10));
	
				if(strlen(RowStr(5)) > 0)
					spCpMail.get()->setMailInfo(MAIL_MSG_BODY, RowStr(5));
				//-------------------------------------------
				
				int iStep = atoi(RowStr(11));
				if(iStep == CPSTEP0_DbReadRunning)
					spCommonQueue.get()->addCpMail(spCpMail);
	
				bool bDelMailBody = false;
				if(strcmp(RowStr(12), "Y")==0)
					bDelMailBody = true;
					
				shared_ptr<stCampaignInfo> spstCampaignInfo = shared_ptr<stCampaignInfo>(new stCampaignInfo(RowStr(0), atoi(RowStr(9)), RowStr(8), bDelMailBody));
				
				if(spstCampaignInfo.get() != NULL){
					getstCampaignInfoList()->insert(std::pair<string, shared_ptr<stCampaignInfo> >(RowStr(0), spstCampaignInfo));
					unordered_map<string, shared_ptr<vector<string> > >::iterator itr_dbpl = getDBProcList()->find(RowStr(8));
					if(itr_dbpl != getDBProcList()->end()){
						itr_dbpl->second.get()->push_back(RowStr(0));
					}
					else{
						//DB를 삽입하고, 해당하는 shared_ptr<vector<string> >를 생성해서 삽입한다.
						shared_ptr<vector<string> > spVecStr = shared_ptr<vector<string> >(new vector<string>());
						if(spVecStr.get() != NULL){
							spVecStr.get()->push_back(RowStr(0));
							getDBProcList()->insert(std::pair<string, shared_ptr<vector<string> > >(RowStr(8), spVecStr));
						}
					}
				}
				else{ //if(spstCampaignInfo.get()==NULL)
				}
			}
			m_EmsMysql.SelectDB_FreeResult();
		}	
	}

	//checkCompleteCampaignList() 체크를 위한 TIme Check
	time(&m_chkCompleteTime);
} 


//--------------------------------------------------------------------
// CampaignList 완료 체크
// 여러개의 Campaign을 관리하게 됨. 
// 효율적인 데이터 관리를 위해서 어떻게 처리해야할지 정의
// 개별적으로 보냄
//--------------------------------------------------------------------
bool CEmsDbCommonQThread::checkCompleteCampaignList()
{
	char  strQuery[Qry_StrlenBig+1] = {0,};
	int   RowCnt  = 0;
	bool  bResult = false;
	
	unordered_map<string, shared_ptr<stCampaignInfo> >  *pstCampaignInfoList = getstCampaignInfoList();
	//unordered_map<string, shared_ptr<vector<string> > > *pEndCampaignList    = getEndCampaignList();

	if(pstCampaignInfoList->size() <= 0)
		return true;
	else{
		int     iCpNoCount    = 0;
		string  strDBName;
		
		vector<string>              tmpCampaignList;
		vector<string>              recvCampaignList;
		shared_ptr<vector<string> > spCpList; 
		vector<string>::iterator    itr_CpList;
			
		vector<string>              spCpEndList;

		unordered_map<string, shared_ptr<vector<string> > >           *dbCpList     = getDBProcList();
		unordered_map<string, shared_ptr<vector<string> > >::iterator  itr_dbCpList = dbCpList->begin();
		
		for(; itr_dbCpList != dbCpList->end(); itr_dbCpList++){
			
			iCpNoCount = 0;
			strDBName  = itr_dbCpList->first;
			spCpList   = itr_dbCpList->second;
			
			if(spCpList.get() != NULL && spCpList.get()->size() > 0){
				// 1.Campaign List를 적당한 개수로 나누어 메일 전송이 완료 되었는지 체크하고, 
				// 2.메일 전송이 완료 된 경우 완료 리스트에 삽입하여 
				// 3.Campaign cpStep을 Update하도록 한다.
				//---------------------------------------
				vector<string>           removedCampaignList;
				vector<string>::iterator itr_rm;
				vector<string>::iterator itr_tmp;
				//---------------------------------------
				bool   bFlag;
				string tmpStrCpNo;
				//---------------------------------------
				//DB map에 연결된  Campaign Number 리스트를 검색하여 완료된 항목 처리
				//---------------------------------------
				itr_CpList = spCpList.get()->begin();
					
				while(itr_CpList != spCpList.get()->end()){
					bFlag      = false;
					iCpNoCount = 0;
					tmpCampaignList.clear();
					recvCampaignList.clear();
					tmpStrCpNo.clear();
					
					for(; itr_CpList != spCpList.get()->end(); itr_CpList++){
						tmpCampaignList.push_back(itr_CpList->c_str());
						//gLog->Write("[%s][%s][%d][Campaign_no: %s]", __FILE__, __FUNCTION__, __LINE__, itr_CpList->c_str());

						if(bFlag == false){
							tmpStrCpNo = (*itr_CpList);
							bFlag = true;
						}
						else{
							tmpStrCpNo += ",";
							tmpStrCpNo += (*itr_CpList);
						}
						
						if((++iCpNoCount) >= MAX_CAMPAIGNLIST_COUNT)
							break;   //for() break;
					}
					
					if(bFlag == false){
						break;  //while() break; No Campaign List
					}
					else{
						snprintf(strQuery, Qry_StrlenBig, QrySel_Mail_StepComplete, strDBName.c_str(), m_iMaxRetryCount, tmpStrCpNo.c_str());
						bResult = m_EmsMysql.SelectDB_GetResult(strQuery, &RowCnt, FldCnt_Mail_StepComplete);
						
						if((bResult == false) || (RowCnt == 0)){
							//남은 리스트없음(리스트 삭제) - 전체리스트를 삭제 리스트에 추가
							//pEmsSelectDB->SelectDB_FreeResult();
							m_EmsMysql.SelectDB_FreeResult();
						}
						else{
							//리스트 있는 경우 리스트를 빼고 삭제 리스트에 추가
							MYSQL_RES  *pMySQLResult = m_EmsMysql.mpMySQLResult;
							MYSQL_ROW   MySQLRow;
							
							while((MySQLRow = mysql_fetch_row(pMySQLResult)) != NULL){
								recvCampaignList.push_back(RowStr(0)); //Campaign index <= 끝나지 않은 Campaign
								//gLog->Write("[%s][%s][%d][Uncomplete Campaign_no:%s][Count:%s]", __FILE__, __FUNCTION__, __LINE__, RowStr(0), RowStr(1));
							}
							
							//pEmsSelectDB->SelectDB_FreeResult();
							m_EmsMysql.SelectDB_FreeResult();
						}
						
						vector<string>::iterator itr = tmpCampaignList.begin();

						vector<string>::iterator itr_cpl;
						vector<string>::iterator itr_recv = recvCampaignList.begin();
						for(; itr_recv != recvCampaignList.end(); itr_recv++){
							if(tmpCampaignList.size()==0)
								break;
							itr_cpl = tmpCampaignList.begin();
							
							for(; itr_cpl!=tmpCampaignList.end(); ){
								if(strcmp(itr_cpl->c_str(), itr_recv->c_str())==0){
									itr_cpl = tmpCampaignList.erase(itr_cpl);
								}
								else
									itr_cpl++;
							}
						}
						
						//프로세스 리스트에서 임시 Campaign 리스트를 제거한다
						vector<string>::iterator itr_tmp = tmpCampaignList.begin();
						for(; itr_tmp != tmpCampaignList.end(); itr_tmp++){
							removedCampaignList.push_back(*itr_tmp);
							//gLog->Write("[%s][%s][%d][Remove:%s]", __FILE__, __FUNCTION__, __LINE__, itr_tmp->c_str());
						}
					}  // if(bFlag) - End
				} // while() - End
				
				//프로세스 리스트에서 CampaignNo 삭제
				itr_rm     = removedCampaignList.begin();
				itr_tmp    = removedCampaignList.end();
				//gLog->Write("[%s][%d][removedCampaignList][%d]", __FUNCTION__, __LINE__, removedCampaignList.size());

				for(; itr_rm != removedCampaignList.end(); itr_rm++){
					for(itr_CpList=spCpList.get()->begin(); itr_CpList != spCpList.get()->end(); itr_CpList++){
						if(*itr_CpList== *itr_rm){
							//gLog->Write("[%s][%s][%d][Removed Campaign no][%s][%s]", __FILE__, __FUNCTION__, __LINE__, (*itr_CpList).c_str(), (*itr_rm).c_str());
							//DB List에 연결된 Campaign List에서 삭제
							spCpList.get()->erase(itr_CpList);
							break;
						}
					}
						
					spCpEndList.push_back(*itr_rm);
					//gLog->Write("[Insert Into spCpEndList][%s][%d][%s]", __FUNCTION__, __LINE__, itr_rm->c_str());
				}
			}  //END_if()
		}   //END_for()

		
		//Update DB Complete List
		if(spCpEndList.size()>0){

			unordered_map<string, shared_ptr<stCampaignInfo> >::iterator itr_cpl;
			vector<string>::iterator itr_endcplist  = spCpEndList.begin();
			bool bDelMailBody = false;
								
			for(; itr_endcplist != spCpEndList.end(); itr_endcplist++){
				bDelMailBody = false;

				//'mail_body' 삭제
				itr_cpl = pstCampaignInfoList->find(*itr_endcplist);
				if(itr_cpl != pstCampaignInfoList->end()){
					//Delete if Exist, Check `del_mail_body`
					bDelMailBody = itr_cpl->second.get()->getDelMailBody();
					setCampaignStep(itr_endcplist->c_str(), CPSTEP2_SendComplete, bDelMailBody);
					pstCampaignInfoList->erase(itr_cpl);
				}
				else{
					//Camapign Step Info List 삭제
					setCampaignStep(itr_endcplist->c_str(), CPSTEP2_SendComplete, bDelMailBody);
				}
			}
		}
		else{
			//[Do Nothing!]
		}
	}
	return true;
}

//--------------------------------------------------------------------
// deleteRQMConn : 연결이 끊긴 Common Queue AMQP 연결 리스트에서 삭제 
//--------------------------------------------------------------------
void  CEmsDbCommonQThread::deleteRQMConn(shared_ptr<CCpMail> spCpMail)
{
	const char   *pGroupCommonQName     = NULL;
	const char   *pCampaignGroupNo      = spCpMail.get()->getMailInfo(MHEADER_GROUPNUMBER);
	int           group_no              = atoi(pCampaignGroupNo);
	shared_ptr<stGroupInfo> spGroupInfo = getspGroupInfo(group_no);
	
	if(spGroupInfo.get() == NULL){
		gLog->Write("[%s][%s][%d][ERROR][spGroupInfo is NULL][%d]", __LINE__, __FUNCTION__, __LINE__, group_no);
	}
	else{
		pGroupCommonQName = spGroupInfo.get()->getGroupCommonQName(); //Common Queue 이름을 RouteKey로 사용함
		deleteAMQPEx(pGroupCommonQName);
		deleteAMQPQueue(pGroupCommonQName);
		deleteQ(pGroupCommonQName);
	}
}

//--------------------------------------------------------------------
// sendMailMsgProcess : 
// DB에서 IndexKey(=Campaign_ng)에 해당하는 CpMail을 가져와서 Rabbitmq에 메시지 전송
// GROUP인 경우와  NONE_GROUP인 경우 확인하여 해당 큐에 메시지 전송 가능한지 체크 후 전송하도록 한다
//--------------------------------------------------------------------
int CEmsDbCommonQThread::sendMailMsgProcess(shared_ptr<CCpMail> spCpMail)
{
	char        strQuery[Qry_StrlenBig+1] = {0,};
	const char *pCampaignNo               = spCpMail.get()->getMailInfo(MHEADER_CPNUMBER);
	const char *pCampaignGroupNo          = spCpMail.get()->getMailInfo(MHEADER_GROUPNUMBER);
	const char *pCpMailDBName             = spCpMail.get()->getMailInfo(MHEADER_DBNAME);

	//gLog->Write("[%s][%s][%d] [Send Mail Process]", __FILE__, __FUNCTION__, __LINE__);
	
	if( (pCampaignNo == NULL) || (pCampaignGroupNo == NULL) ){
		if(pCampaignGroupNo != NULL){
			shared_ptr<stGroupInfo> spGroupInfo = getspGroupInfo(atoi(pCampaignGroupNo));
			gLog->Write("[%s][%s][%d][ERROR][Internal error occurred][Campaign_no:%s][Group_no:%s][CommonQName:%s]", __FILE__, __FUNCTION__, __LINE__, pCampaignNo, pCampaignGroupNo, spGroupInfo.get()->getGroupCommonQName());
		}
		else{
			gLog->Write("[%s][%s][%d][ERROR][Internal error occurred][Campaign_no:%s][Group_no:NULL]", __FILE__, __FUNCTION__, __LINE__, pCampaignNo);
		}
		setCampaignStep(pCampaignNo, CPSTEP0_DbReadRunning);
		return -1;
	}
	else{
		int LoopCnt  = 0;
		int group_no = atoi(pCampaignGroupNo);
		const char *pGroupCommonQName       = NULL;		
		shared_ptr<stGroupInfo> spGroupInfo = getspGroupInfo(group_no);

		if(spGroupInfo.get() == NULL){
			//gLog->Write("[%s][%s][%d][ERROR][NO Send Group][%d]", __FILE__, __FUNCTION__, __LINE__, group_no);
			setCampaignStep(pCampaignNo, CPSTEP0_DbReadRunning);
			return -1;
		}

		pGroupCommonQName = spGroupInfo.get()->getGroupCommonQName(); //Common Queue 이름을 RouteKey로 사용함
		
		//Check Queue Size
		int qsize = getEnqueueCount(pGroupCommonQName);
		if( (pGroupCommonQName!=NULL) && (qsize < MAX_ENQUEUE_SIZE_I) ){
			//RMQ에 메시지 전송 가능한 상태

			bool bMQisFull = false;
			bool bResult   = false;
			int  RowCnt    = 0;
			bool bWstr     = (atoi(spCpMail.get()->getMailInfo(MHEADER_USEWSTR))>0)? true : false;

	
			snprintf(strQuery, Qry_StrlenBig, QrySel_Mail_Info,  pCpMailDBName, pCampaignNo);
			bResult = m_EmsMysql.SelectDB_GetResult(strQuery, &RowCnt, FldCnt_Mail_Info); 
			
			if(bResult == false){
				return -1;
			}
			else if(RowCnt == 0){
				//gLog->Write("[%s][None CpMail Data][Campaign_no:%s][QUERY:%s]", __FUNCTION__, pCampaignNo, strQuery);
				setCampaignStep(pCampaignNo, CPSTEP1_DbReadComplete);
				return 1;
			}
			else{
				//CpMail의 메시지를 RMQ에 전달한다.
				//------------------------------------------------------//
				// 전송할 메일 정보를 가져와서 큐에 메시지를 보냄
				//------------------------------------------------------//
				MYSQL_RES  *pMySQLResult = m_EmsMysql.mpMySQLResult;
				MYSQL_ROW   MySQLRow;
				const char *pMailNo ;
				int         iErrorCode=0;
				
				//no[0],mailidx[1],campaign_no[2],to_name[3],to_id[4],to_domain[5],try_cnt[6],email[7],wide_str[8]
				while((MySQLRow = mysql_fetch_row(pMySQLResult)) != NULL)
				{
					bResult = false;
					pMailNo = RowStr(0);    //MailNo
					
					try{
						bResult = sendToRMQCpMail(MySQLRow, spCpMail, bWstr, false);
					}
					catch(AMQPException ex){
						gLog->Write("[%s][%s][%d][ERROR][AMQPException Error Occurred!][MSG:%s]"
						              , __FILE__, __FUNCTION__, __LINE__
						              , ex.getMessage().c_str());
						iErrorCode = -1;
						try{
							deleteRQMConn(spCpMail); 
						}
						catch(AMQPException ex){
							gLog->Write("[%s][%s][%d][ERROR][AMQPException Error Occurred!][MSG:%s]"
							              , __FILE__, __FUNCTION__, __LINE__
							              , ex.getMessage().c_str());
						}
						catch(...){
							gLog->Write("[%s][%s][%d][ERROR] Unknown Error Occurred!"
						                , __FILE__, __FUNCTION__, __LINE__);
						}
						sleep(THREAD_RMQ_WAIT_TIME);
						break;
					}
					catch(...){
						gLog->Write("[%s][%s][%d][ERROR] Unknown Error Occurred!"
						                , __FILE__, __FUNCTION__, __LINE__);
						iErrorCode = -2;
						break;
					}
				
					if(bResult == true){
				
						LoopCnt++;
						memset(strQuery, 0, Qry_StrlenBig+1);
						//UPDATE %s.mails SET smtp_step=0 WHERE no=%s AND smtp_step=-1
						snprintf(strQuery, Qry_StrlenBig, QryUdt_Mail_SmtpStep0, pCpMailDBName, pMailNo);
						bResult = m_EmsMysqlSub.QueryDB(strQuery);
				
						if(bResult == false)
						{
							gLog->Write("[%s][%s][%d][ERROR][QryUdt_Mail_SmtpStep0] Failed[Query:%s]", __FILE__, __FUNCTION__, __LINE__, strQuery);
							iErrorCode=1;
							break;
						}
						else{
							//gLog->Write("[%s][%s][%d][SUCCESS][QryUdt_Mail_SmtpStep0][Query:%s]", __FILE__, __FUNCTION__, __LINE__, strQuery);
						}
					}
					else {
						snprintf(strQuery, Qry_StrlenBig, QryUdt_Mail_SmtpStepM2, pCpMailDBName, pMailNo);
						bResult = m_EmsMysqlSub.QueryDB(strQuery);
					}
					
					//if(getEnqueueCount(pGroupCommonQName) >= MAX_ENQUEUE_SIZE_I){
					int iQSize = getEnqueueCount(pGroupCommonQName);
					if(iQSize >= MAX_ENQUEUE_SIZE_I){
						bMQisFull = true;
						break;
					}
					else{
						//gLog->Write("[%s][%s][%d]*****>[%s Queue Size:%d]", __FILE__, __FUNCTION__, __LINE__, pGroupCommonQName, iQSize);
					}
				} //  END - while();

				m_EmsMysql.SelectDB_FreeResult();
				if(bMQisFull){
					setCampaignStep(pCampaignNo, CPSTEP0_DbReadRunning);
					return -1;
				}
				
				if((LoopCnt != RowCnt) ||(iErrorCode < 0))	{
					gLog->Write("[%s][%s][%d][ERROR][CpMail Send Count Difference][RowCnt:%d][SendCnt:%d]"
					              , __FILE__, __FUNCTION__, __LINE__, RowCnt, LoopCnt);
					setCampaignStep(pCampaignNo, CPSTEP0_DbReadRunning);
					return -1;
				}
				else{
					//정상적으로 메시지를 RMQ에 전송처리 하였을때 Campaign DB를 업데이트한다.
					setCampaignStep(pCampaignNo, CPSTEP1_DbReadComplete);
					return 1;
				}
			}
		}
		else{
			setCampaignStep(pCampaignNo, CPSTEP0_DbReadRunning);
			return -1;
		}
	}
	return 0;
}


//--------------------------------------------------------------------
// Check Campaign
//--------------------------------------------------------------------
int CEmsDbCommonQThread::checkCampaign()
{
	int   RowCnt  = 0;
	bool  bResult = false;
	
	char chLastCpNumber[LEN_Campaign_no+1]={0,}; // Max Campaign Number Length : 19
	char strQuery      [Qry_StrlenBig+1]  ={0,};
	
	//***************************************************
	// MAX_ENQUEUE_SIZE_I 만큼 DB에서 데이터를 추출하여 담는다
	//***************************************************
	shared_ptr<CEmsQueue>  spCommonQueue = getspCommonQueue();
	int cpCount = 0;

	do{
		int job_count = MAX_ENQUEUE_SIZE_I - spCommonQueue.get()->getQueueSize();
		if(job_count <= 0)
			break;

			
		if(spCommonQueue.get()->getQueueSize() > 0)
			gLog->Write("[%s][Check Campaign Process][%d][%d]", __FUNCTION__, spCommonQueue.get()->getQueueSize(), MAX_ENQUEUE_SIZE_I);

		memset(strQuery, 0, Qry_StrlenBig+1);
		
		if(strlen(chLastCpNumber) == 0){
			snprintf(strQuery, Qry_StrlenBig, QrySel_Campaign_Info ADD_LIMIT, CPSTEPB_DbBeforeRunning, STR_MODE_I, job_count);
		}
		else{
			snprintf(strQuery, Qry_StrlenBig, QrySel_Campaign_Info ADD_CHECK_NO ADD_LIMIT, CPSTEPB_DbBeforeRunning, STR_MODE_I, chLastCpNumber, job_count);
		}
		
		bResult = m_EmsMysql.SelectDB_GetResult(strQuery, &RowCnt, FldCnt_Campaign_Info);
		
		if( (bResult == false) || (RowCnt == 0) ){
			//gLog->Write("[%s][%s][%d] Personal Mail Message is Not Exists ", __FILE__, __FUNCTION__, __LINE__);
			return 0;
		}
		else{
			
			MYSQL_RES *pMySQLResult = m_EmsMysql.mpMySQLResult;
			MYSQL_ROW  MySQLRow;
			
			shared_ptr<CCpMail>   spCpMail;
			unordered_map<string, shared_ptr<stCampaignInfo> >::iterator itr_cps;
		
			while((MySQLRow = mysql_fetch_row(pMySQLResult)) != NULL ){
				
				int                      iGroupNo   = atoi(RowStr(9));
				shared_ptr<stGroupInfo> spGroupInfo = getspGroupInfo(iGroupNo);
				
				memset(chLastCpNumber, '\0', LEN_Campaign_no+1);
				strncpy(chLastCpNumber, RowStr(0), LEN_Campaign_no);

				if( spGroupInfo.get() == NULL) { //GroupInfo 없음
					continue;
				}
				
				//처리해야할 Campaign이 존재하므로 리스트에 넣어둔다.
				spCpMail = shared_ptr<CCpMail>(new CCpMail);
				//-------------------------------------------
				//spCpMail에 Campaign  정보 저장
				
				try{
					if(strlen(RowStr(0)) > 0)
						spCpMail.get()->setMailInfo(MHEADER_CPNUMBER,    RowStr(0));
					if(strlen(RowStr(1)) > 0){
						spCpMail.get()->setMailInfo(MHEADER_CPMAILKEY,   RowStr(1));
					}
					if(strlen(RowStr(2)) > 0)
						spCpMail.get()->setMailInfo(MHEADER_SENDERNAME,  RowStr(2));
					if(strlen(RowStr(3)) > 0) {
						spCpMail.get()->setMailInfo(MHEADER_SENDEREMAIL, RowStr(3));
						char       *ptmpDomain = NULL;
						const char *pDomain    = NULL;
						ptmpDomain= strchr( const_cast<char*>(RowStr(3)), '@');
					
						if(ptmpDomain != NULL){
							pDomain = ptmpDomain+1;
						}
						else{
							pDomain = DEFAULT_DOMAIN_GABIA_COM;
						}
						spCpMail.get()->setMailInfo(MHEADER_SENDERDOMAIN, pDomain);
					}
			
					if(strlen(RowStr(4)) > 0)
						spCpMail.get()->setMailInfo(MHEADER_MAILTITLE,   RowStr(4));
					else
						throw MHEADER_MAILTITLE;
						
					if(strlen(RowStr(6)) > 0)
						spCpMail.get()->setMailInfo(MHEADER_USEWSTR,     RowStr(6));
						
					if(strlen(RowStr(8)) > 0)
						spCpMail.get()->setMailInfo(MHEADER_DBNAME,      RowStr(8));
					else
						throw MHEADER_DBNAME;
						
					if(strlen(RowStr(9)) > 0)
						spCpMail.get()->setMailInfo(MHEADER_GROUPNUMBER, RowStr(9));
					else
						throw MHEADER_GROUPNUMBER;
						
					if(strlen(RowStr(10)) > 0)
						spCpMail.get()->setMailInfo(MHEADER_TR_TYPE,     RowStr(10));
	
					if(strlen(RowStr(5)) > 0)
						spCpMail.get()->setMailInfo(MAIL_MSG_BODY, RowStr(5));
					else
						throw MAIL_MSG_BODY;
				}
				catch(const char* errVal){
						//내용이 없는 경우 에러처리하여 -3으로 업데이트 처리함.
						gLog->Write("[%s][%s][%d][ERROR][CPNO:%s][%s is NULL]", __FILE__, __FUNCTION__, __LINE__, RowStr(0), errVal);
						setCampaignStep(RowStr(0), CPSTEPM3_DbException);
						continue;
				}
				catch(...){
						gLog->Write("[%s][%s][%d][ERROR][Unknown Error Occurred]", __FILE__, __FUNCTION__, __LINE__);
						setCampaignStep(RowStr(0), CPSTEPM3_DbException);
						continue;
				}

				//-------------------------------------------
				spCommonQueue.get()->addCpMail(spCpMail);
				
				//Campaign  List 관리를 위해  넣어준다.
				itr_cps = getstCampaignInfoList()->find(RowStr(0));
				if(itr_cps == getstCampaignInfoList()->end()){
					//insert Campaign Info
					bool bDelMailBody = false;
					if(strcmp(RowStr(12), "Y")==0)
						bDelMailBody = true;

					shared_ptr<stCampaignInfo> spstCampaignInfo = shared_ptr<stCampaignInfo>(new stCampaignInfo(RowStr(0), atoi(RowStr(9)), RowStr(8), bDelMailBody));
				
					if(spstCampaignInfo.get() != NULL){
						getstCampaignInfoList()->insert(std::pair<string, shared_ptr<stCampaignInfo> >(RowStr(0), spstCampaignInfo));
						unordered_map<string, shared_ptr<vector<string> > >::iterator itr_dbpl = getDBProcList()->find(RowStr(8));
						if(itr_dbpl != getDBProcList()->end()){
							itr_dbpl->second.get()->push_back(RowStr(0));
						}
						else{
							//DB를 삽입하고, 해당하는 shared_ptr<vector<string> >를 생성해서 삽입한다.
							shared_ptr<vector<string> > spVecStr = shared_ptr<vector<string> >(new vector<string>());
							if(spVecStr.get() != NULL){
								spVecStr.get()->push_back(RowStr(0));
								getDBProcList()->insert(std::pair<string, shared_ptr<vector<string> > >(RowStr(8), spVecStr));
							}
						}
						setCampaignStep(RowStr(0), CPSTEP0_DbReadRunning);
						cpCount++;
					}
					else{ //if(spstCampaignInfo.get()==NULL)
					}
				}
				else{
					//pass
				}
			}
			m_EmsMysql.SelectDB_FreeResult();
		}
	}while(0);

	return cpCount;
}


//--------------------------------------------------------------------
// EmsQueue Message Process 
// 1) Campaign->transmission_type='I'인 리스트를 가지고 메일 메시지를 생성
// 2) DB에서 가져온 데이터를 이용하여 RabbitMQ에 메시지 전송
// 3) 서버 Main에서 메시지 있을때마다 전송해주면 좋겠지만 폴링 방식으로 체크하자
// 4) 한번에 여러개를 가져오도록 한다.
//--------------------------------------------------------------------
int CEmsDbCommonQThread::sendToCampaignCommonQ()
{
	int  iCount      = 0;
	int  iSendResult = 0;
	shared_ptr<CCpMail>          spCpMail;
	vector<shared_ptr<CCpMail> > vecCpMail;

	//Send Campaign Mail Message to Rabbitmq Common Queue
	while(m_spCommonQueue.get()->getQueueSize() > 0){

		setisfullMQ(false);
		spCpMail = m_spCommonQueue.get()->getCpMail();
		if(spCpMail.get() != NULL){
			//CpMail을 DB에서 가져와서 Rabbitmq Common Queue에 전달한다(CpMail을 업데이트!)
			iSendResult = sendMailMsgProcess(spCpMail);
			if(iSendResult >= 0){//정상 처리됨
				iCount++;
			}
			else{ //비정상 처리됨
				//Do Nothing
				vecCpMail.push_back(spCpMail);
			}
		}
		if(getisfullMQ()==true){
			usleep(THREAD_SLEEPTIME_100MSEC);
		}
	}
	
	if(vecCpMail.size() > 0){
		//gLog->Write("[%s][%s][%d][Vector CpMail Size:%d]", __FILE__, __FUNCTION__, __LINE__, vecCpMail.size());
		vector<shared_ptr<CCpMail> >::iterator itr_cp = vecCpMail.begin();
		for(; itr_cp != vecCpMail.end(); itr_cp++){
			m_spCommonQueue.get()->addCpMail(*itr_cp);
		}
	}

	return iCount;
}


bool CEmsDbCommonQThread::isCompleteCheckTime(int iPeriod)
{
	time_t currtime;
	time(&currtime);
	
	if((int)difftime(currtime, m_chkCompleteTime) > iPeriod){
		m_chkCompleteTime = currtime;
		return true;
	}
	else{
		return false;
	}
}

//--------------------------------------------------------------------
// ThreadFunc :CEmsDbCommonQThread 객체 내부적으로
// EmsQueue에 메시지 입력/처리에 대한 부분만을 담당한다.
// 
//--------------------------------------------------------------------
void *CEmsDbCommonQThread::ThreadFunc(void *Param)
{
	CEmsDbCommonQThread *pDbThread = (CEmsDbCommonQThread *)Param;

	pDbThread->checkPrevCampaign();
	pDbThread->setRunState(TSTATE_RUN);

	while( true )
	{
		if(pDbThread->isStop()==true)
			break;
		if(pDbThread->getRunState() == TSTATE_PAUSE){
			sleep(1);
			continue;
		}
		
		//메일 전송된 Campaign List 완료 체크
		if(pDbThread->getstCampaignInfoList()->size() > 0){
			if(pDbThread->isCompleteCheckTime(DEFAULT_COMPLETECHECKTIME) == true){
				pDbThread->checkCompleteCampaignList();
			}
		}
		//작업대기 큐에 대기중인 작업이 있는 경우라면 먼저 다 처리한 후에 디비에서 읽어오도록 한다.
		//큐에다가 Campaign 리스트를 넣어 둠
		if(pDbThread->getspCommonQueue().get()->getQueueSize() == 0 ){
			//DB->EmsAgentServer Queue
			pDbThread->checkCampaign();
		}

		//대기큐의 확인 후 처리
		if( (pDbThread->getspCommonQueue().get()->getQueueSize()==0)
			  && (pDbThread->getstCampaignInfoList()->size()==0) ){
			sleep(2);  //대기 큐에 작업이 없는 경우 
		}
		else{
			int retval = pDbThread->sendToCampaignCommonQ();
			if(retval > 0){ 
				//EmsQueue에 있는  CCpMail 을 Rabbitmq에 보낸다 
				//Process Campaign List
				usleep(THREAD_SLEEPTIME_1MSEC);
			}
			else{ //보낸 메시지가 없거나 보낼 수 없는 경우 sleep(1)
				if(retval == 0){
					sleep(1);
				}
				else{
					sleep(1);
				}
			}
		}
	}

	gLog->Write("[%s][%s][%d][End CEmsDbCommonQThread!!]", __FILE__, __FUNCTION__, __LINE__);
	return NULL;
}


//--------------------------------------------------------------------
// ConnectDB 
//--------------------------------------------------------------------
bool CEmsDbCommonQThread::ConnectDB(const char *l_pszHost, const char *l_pszUser, const char *l_pszPasswd, const char *l_pszDBName, unsigned int l_iPort)
{
	bool   bResult = false;
	string strThreadName = "CEmsDbCommonQThread";

	strcpy(m_chDBIPAddr, l_pszHost);
	strcpy(m_chDBUser,   l_pszUser);
	strcpy(m_chDBPasswd, l_pszPasswd);
	strcpy(m_chDBName,   l_pszDBName);
	m_iDBPort = l_iPort;

	m_EmsMysql.SetOwnerName(strThreadName.c_str());
	m_EmsMysql.SetHost(l_pszHost, l_pszUser, l_pszPasswd, l_pszDBName, l_iPort);

	bResult = m_EmsMysql.ConnectDB();

	if(!bResult)
	{
		return false;
	}

	strThreadName += "Sub";
	m_EmsMysqlSub.SetOwnerName(strThreadName.c_str());
	m_EmsMysqlSub.SetHost(l_pszHost, l_pszUser, l_pszPasswd, l_pszDBName, l_iPort);

	bResult = m_EmsMysqlSub.ConnectDB();

	if(!bResult)
	{
		return false;
	}

	return bResult;
}


//-------------------------------------
// sendToRMQCpMail - Rabbitmq로 CpMail 메시지를 전송
//--------------------------------------
bool CEmsDbCommonQThread::sendToRMQCpMail(MYSQL_ROW &MySQLRow, shared_ptr<CCpMail> spCpMail, bool bUseWstr, bool bUsenderEmail)
{
	//[mails] no(0),mailidx(1),campaign_no(2),to_name(3),to_id(4),to_domain(5),try_cnt(6),wide_str(7)
	bool          bResult = false;
	AMQPExchange *pEx;
	
	const char   *pGroupCommonQName     = NULL;
	const char   *pCampaignGroupNo      = spCpMail.get()->getMailInfo(MHEADER_GROUPNUMBER);
	int           group_no              = atoi(pCampaignGroupNo);
	shared_ptr<stGroupInfo> spGroupInfo = getspGroupInfo(group_no);
	
	if(spGroupInfo.get() == NULL){
		gLog->Write("[%s][%d][ERROR][NO Send Group][%d]", __FUNCTION__, __LINE__, group_no);
		return bResult;
	}
	else{
		pGroupCommonQName = spGroupInfo.get()->getGroupCommonQName(); //Common Queue 이름을 RouteKey로 사용함
		pEx               = getAMQPExchange(pGroupCommonQName);
	}
	             
	if(pEx == NULL){
		return bResult;
	}
	
	try{
		int ichknum = 0;
		pEx->setHeader("Delivery-mode", 2);
		pEx->setHeader("Content-type", "text/plain");
		pEx->setHeader(MHEADER_GROUPNUMBER,  pCampaignGroupNo,   true);  //"Group Number"
		pEx->setHeader(MHEADER_QINDEX,       pGroupCommonQName,  true);  //"Group Common Queue Name"
		pEx->setHeader(MHEADER_CPMAILKEY,    spCpMail.get()->getMailInfo(MHEADER_CPMAILKEY), true);  //"campaign_mailkey"
		
		try{
			if(strlen(RowStr(0)) == 0)
				throw ichknum;
			else{
				pEx->setHeader(MHEADER_MAILNUMBER,   RowStr(0),  true);  //"mail_no"
				ichknum++;
			}

			if(strlen(RowStr(1)) == 0)
				throw ichknum;
			else{
				pEx->setHeader(MHEADER_MAILINDEX,    RowStr(1),  true);  //"mailidx"
		  	ichknum++;
			}

			if(strlen(RowStr(2)) == 0)
				throw ichknum;
			else{
				 pEx->setHeader(MHEADER_CPNUMBER,     RowStr(2),  true);  //"campaign_no"
				ichknum+=2;
			}

			pEx->setHeader(MHEADER_TONAME,       RowStr(3),  true);  //"to_name"

			if(strlen(RowStr(4)) == 0)
				throw ichknum;
			else{
				pEx->setHeader(MHEADER_TOID,         RowStr(4),  true);  //"to_id"
				ichknum++;
			}

			if(strlen(RowStr(5)) == 0)
				throw ichknum;
			else{
				pEx->setHeader(MHEADER_TODOMAIN,     RowStr(5),  true);  //"to_domain"
				ichknum++;
			}

			pEx->setHeader(MHEADER_TRYCOUNT,     RowStr(6),  true);  //"try_cnt"
		}
		catch(int chknum){
			gLog->Write("[%s][ERROR][SEND_M][Error Pos:%d]", __FUNCTION__, chknum);
			return false;
		}
		catch(...){
			gLog->Write("[%s][ERROR][SEND_M][Unknown Error Occurred]", __FUNCTION__);
			return false;
		}
		
		if(bUseWstr == true){
			pEx->setHeader(MHEADER_USEWSTR,          "1",  true);  //"use_wstr"
			pEx->setHeader(MHEADER_WIDESTR,    RowStr(8),  true);  //"wide_str"
		}
		else{
			pEx->setHeader(MHEADER_USEWSTR,          "0",  true);  //"use_wstr"
			pEx->setHeader(MHEADER_WIDESTR,           "",  true);  //"wide_str"
		}
		
		pEx->setHeader(MHEADER_SENDEREMAIL,  spCpMail.get()->getMailInfo(MHEADER_SENDEREMAIL),  true);
		pEx->setHeader(MHEADER_SENDERNAME,   spCpMail.get()->getMailInfo(MHEADER_SENDERNAME),   true);
		pEx->setHeader(MHEADER_SENDERDOMAIN, spCpMail.get()->getMailInfo(MHEADER_SENDERDOMAIN), true);
		pEx->setHeader(MHEADER_MAILTITLE,    spCpMail.get()->getMailInfo(MHEADER_MAILTITLE),    true);
		pEx->setHeader(MHEADER_TR_TYPE,      spCpMail.get()->getMailInfo(MHEADER_TR_TYPE),      true);
	
		pEx->Publish(spCpMail.get()->getMailInfo(MAIL_MSG_BODY), pGroupCommonQName);
		gLog->Write("[SEND_I][%s][%s][%s][%s][%s][%s][%s][%s]"
	             , RowStr(0), RowStr(1), RowStr(2), RowStr(3), RowStr(4), RowStr(5), RowStr(6), RowStr(7) );

		
		bResult = true;
	}
	catch(AMQPException ex){
		deleteRQMConn(spCpMail);
		gLog->Write("[%s][%s][%d][ERROR][AMQPException Error Occurred!][MSG:%s]"
						              , __FILE__, __FUNCTION__, __LINE__, ex.getMessage().c_str());
		exit(1);//AMQP Connection에 문제가 있는 경우 프로그램 재시작
	}
	catch(...){
		gLog->Write("[%s][%s][%d][ERROR][Unknown Error Occurred!]"
						              , __FILE__, __FUNCTION__, __LINE__);
	}
	
	return bResult;
}

//-----------------------------------------------------
// getEnqueueCount : Queue에 저장된 Count
//-----------------------------------------------------
int CEmsDbCommonQThread::getEnqueueCount(const char *routeKey)
{
	if(routeKey == NULL){
		routeKey = RMQ_EXCHANGE_COMMON;
	}
	
	AMQPQueue * tmpAMQPQueue = getAMQPQueue(routeKey);
	
	try{
		if(tmpAMQPQueue == NULL)
			return -1;
		else{
			tmpAMQPQueue->Declare(routeKey, AMQP_DURABLE);
			return tmpAMQPQueue->getCount();
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
// CampaignStep (Campaign No 추가 및 Step 설정)
//-----------------------------------------------------
bool CEmsDbCommonQThread::isCampaignStepUpdate(const char *pCampaignNumber, int iCpStep)
{
	bool retval = false;
	unordered_map<string, int>::iterator itr_cpStep;
	itr_cpStep = m_mapCampaignStep.find(string(pCampaignNumber));
	
	if(itr_cpStep != m_mapCampaignStep.end()){
		if(itr_cpStep->second != iCpStep){
			retval = true;
		}
	}
	else{
		m_mapCampaignStep.insert(std::pair<string, int >(string(pCampaignNumber), iCpStep));
		retval = true;
	}
	
	return retval;
}

//-----------------------------------------------------
// CampaignStep (Campaign No 삭제)
//-----------------------------------------------------
void CEmsDbCommonQThread::delCampaignStep(const char *pCampaignNumber)
{
	unordered_map<string, int>::iterator itr_cpStep;
	itr_cpStep = m_mapCampaignStep.find(string(pCampaignNumber));
	if(itr_cpStep != m_mapCampaignStep.end()){
		m_mapCampaignStep.erase(itr_cpStep);
	}
}

//-----------------------------------------------------
// setCampaignStep (Campaign Step 상태 설정)
//-----------------------------------------------------
bool CEmsDbCommonQThread::setCampaignStep(const char *pCampaignNumber, int iCpStep, bool bDelFlag)
{
	if((isCampaignStepUpdate(pCampaignNumber, iCpStep) == true)||(CPSTEP2_SendComplete == iCpStep)){
		char  strQuery[Qry_Strlen+1] = {0,};
		memset(strQuery, 0, Qry_Strlen+1);
	
		//"UPDATE Campaign SET CpStep=0 WHERE CpIdx='%s'"
		const char * tmpStep = NULL;
		switch(iCpStep){
			case CPSTEP0_DbReadRunning   : //0
			{
				tmpStep = QryUdt_Campaign_CpStep0;
				break;
			}
			case CPSTEP1_DbReadComplete  : //1
			{
				tmpStep = QryUdt_Campaign_CpStep1;
				break;
			}
			case CPSTEP2_SendComplete    : //2
			{
				if(bDelFlag == false)
					tmpStep = QryUdt_Campaign_CpStepComplete;
				else
					tmpStep = QryUdt_Campaign_CpStepComplete_Del_mailbody;
				delCampaignStep(pCampaignNumber);
				break;
			}
			case CPSTEPM3_DbException    : //-3
			{
				tmpStep = QryUdt_Campaign_CpStepM3;
				delCampaignStep(pCampaignNumber);
				break;
			}
			default :
				tmpStep = NULL;
		}
	
		if(tmpStep == NULL){
			return false;
		}
		else{
			bool bResult = false;
			snprintf(strQuery, Qry_Strlen, tmpStep, pCampaignNumber);
			//gLog->Write("[%s][%s][%d][STEP:%d][QUERY:%s]", __FILE__, __FUNCTION__, __LINE__, iCpStep, strQuery);
			bResult = m_EmsMysqlSub.QueryDB(strQuery);
			
			if(bResult == false){
				//Log Update Campaign iCpStep!! Campaign Step설정 변경 로그 남기자!!
				gLog->Write("[UPDATE][FAILED]");
				gLog->Write(strQuery);
				return false;
			}
			else{
				//gLog->Write("[UPDATE][SUCCESS][Step:%d][Query:%s]", iCpStep, strQuery);
				gLog->Write("[UPDATE][Step:%d][Campaign_No:%s][%s]", iCpStep, pCampaignNumber, (bDelFlag?"Y":"N"));
				return true;
			}
		}
	}
	return true;
}


//-----------------------------------------------------
// Group Number 전달 시 GroupInfo 리턴
//-----------------------------------------------------
shared_ptr<stGroupInfo> CEmsDbCommonQThread::getspGroupInfo(int group_no)
{
	shared_ptr<stGroupInfo> spGroupInfo;
	unordered_map<int, shared_ptr<stGroupInfo> >::iterator itr_gi = m_pGroupInfoList->find(group_no);
	if(itr_gi != m_pGroupInfoList->end()){
		spGroupInfo = itr_gi->second;
	}
	else{
		return spGroupInfo;
	}
	return spGroupInfo;
}


//-----------------------------------------------------
// stCampaignInfo 를 생성하여 m_stCampaignInfoList에 추가한다.
//-----------------------------------------------------
shared_ptr<stCampaignInfo> CEmsDbCommonQThread::getspCampaignInfo(const char* pCPNo)
{
	shared_ptr<stCampaignInfo> spCpInfo;
	
	if(pCPNo != NULL){
		//pCPNo를 이용하여 stCampaignInfo를 생성한다.
		int   RowCnt  = 0;
		bool  bResult = false;
	
		char chLastCpNumber[LEN_Campaign_no+1]={0,}; // Max Campaign Number Length : 19
		char strQuery      [Qry_StrlenBig+1]  ={0,};
				
		snprintf(strQuery, Qry_StrlenBig, QrySel_Campaign_Info_No ADD_LIMIT, pCPNo, STR_MODE_I, 1);
		gLog->Write("[%s][%s][%d][Query:%s]", __FILE__, __FUNCTION__, __LINE__, strQuery);
		
		bResult = m_EmsMysql.SelectDB_GetResult(strQuery, &RowCnt, FldCnt_Campaign_Info);
		
		if( (bResult == false) || (RowCnt == 0) ){
			gLog->Write("[%s][%s][%d][Campaign is not exist][CpNo:%s]", __FILE__, __FUNCTION__, __LINE__, pCPNo);
			return spCpInfo;
		}
		else{
			
			MYSQL_RES *pMySQLResult = m_EmsMysql.mpMySQLResult;
			MYSQL_ROW  MySQLRow;
			
			shared_ptr<CCpMail>   spCpMail;
			unordered_map<string, shared_ptr<stCampaignInfo> >::iterator itr_cps;
			
			if(pMySQLResult == NULL)
				return spCpInfo;
				
			if((MySQLRow = mysql_fetch_row(pMySQLResult)) != NULL ){
				
				int                      iGroupNo   = atoi(RowStr(9));
				shared_ptr<stGroupInfo> spGroupInfo = getspGroupInfo(iGroupNo);
				
				memset(chLastCpNumber, '\0', LEN_Campaign_no+1);
				strncpy(chLastCpNumber, RowStr(0), LEN_Campaign_no);

				//Campaign  List 관리를 위해  넣어준다.
				itr_cps = getstCampaignInfoList()->find(RowStr(0));
				if(itr_cps == getstCampaignInfoList()->end()){
					//insert Campaign Info
					bool bDelMailBody = false;
					if(strcmp(RowStr(12), "Y")==0)
						bDelMailBody = true;

					spCpInfo = shared_ptr<stCampaignInfo>(new stCampaignInfo(RowStr(0), atoi(RowStr(9)), RowStr(8), bDelMailBody));
				
					if(spCpInfo.get() != NULL){
						getstCampaignInfoList()->insert(std::pair<string, shared_ptr<stCampaignInfo> >(RowStr(0), spCpInfo));
						unordered_map<string, shared_ptr<vector<string> > >::iterator itr_dbpl = getDBProcList()->find(RowStr(8));
						if(itr_dbpl != getDBProcList()->end()){
							itr_dbpl->second.get()->push_back(RowStr(0));
						}
						else{
							//DB를 삽입하고, 해당하는 shared_ptr<vector<string> >를 생성해서 삽입한다.
							shared_ptr<vector<string> > spVecStr = shared_ptr<vector<string> >(new vector<string>());
							if(spVecStr.get() != NULL){
								spVecStr.get()->push_back(RowStr(0));
								getDBProcList()->insert(std::pair<string, shared_ptr<vector<string> > >(RowStr(8), spVecStr));
							}
						}
						setCampaignStep(RowStr(0), CPSTEP0_DbReadRunning);
					}
					else{ //if(spCpInfo.get()==NULL)
					}
				}
				else{
					//pass
				}
			}
			m_EmsMysql.SelectDB_FreeResult();
		}
	}
	
	return spCpInfo;
}
