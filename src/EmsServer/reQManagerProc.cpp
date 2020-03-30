#include "reQManagerProc.h"
#include "manual.h"

extern CEmsLog *gLog;

const char *CMD = "CMD";

//private
CReQManagerProc::CReQManagerProc()
{
	m_iManual_HELP_Cnt = sizeof(manual_help)/(SZ_STRING+1);
	m_iManual_MENU_Cnt = sizeof(manual_menu)/(SZ_STRING+1);
	m_iManual_CMD_Cnt  = sizeof(manual_cmd)/(SZ_STRING+1);
	m_iManual_BASE_Cnt = sizeof(manual_base)/(SZ_STRING+1);
}


//public
CReQManagerProc::~CReQManagerProc()
{
}


CReQManagerProc *CReQManagerProc::m_pReQManager = NULL;


CReQManagerProc *CReQManagerProc::getInstance()
{
	if(m_pReQManager == NULL){
		m_pReQManager = new CReQManagerProc();
	}

	return m_pReQManager;
}



//--------------------------------------
// 문자열을 소문자로 변경
//--------------------------------------
void CReQManagerProc::strLower(char *strSrc)
{
	int iLen = strlen(strSrc);
	for(int i=0; i<iLen; i++){
		if(strSrc[i]=='\0'){
			break;
		}
		else{
			strSrc[i] = tolower(strSrc[i]);
		}
	}
}


//--------------------------------------
// 라인 화면 출력
//--------------------------------------
void CReQManagerProc::showLine()
{
	fprintf(stdout, "-----------------------------\n");
}



//--------------------------------------
// Menu 화면 출력
//--------------------------------------
void CReQManagerProc::showMenu()
{
	for(int i=0; i<m_iManual_MENU_Cnt ; i++){
		fprintf(stdout, "[%d]%s\n", i, manual_menu[i]);
	}
	showLine();
}


//--------------------------------------
// 입력 Menu 파싱
//--------------------------------------
int  CReQManagerProc::parseMenu(char *strMenu)
{
	int iRet    = -1;
	int iMenuNo = -1;
	
	if(strMenu != NULL){
		strLower(strMenu);
	}
	else
		return -1;

	for(int i=0; i<m_iManual_MENU_Cnt; i++){
		if(strcmp(manual_menu[i], strMenu)==0){
			iMenuNo = i;
			break;
		}
	}
	
	if(iMenuNo == 0)
		iRet = -1;
	else{
		iRet = procMenu(iMenuNo);
	}
	
	return iRet;
}

bool CReQManagerProc::procMenu(int iMenu)
{
	bool bRet = true;
	//Do Switch Proces 
	switch(iMenu){
		case 	iEXIT: { //0
			bRet = false;
			break;
		}
		case 	iENQUEUE_FROM_FILES: {//1
			procEnQueueFromFiles();
			break;
		}
		case 	iGET_FILES_FROM_QUEUE: {//2
			procGetFilesFromQueue();
			break;
		}
		default:
			bRet = true;

	}
	return bRet;
}

//--------------------------------------
// 문자열 입력
//--------------------------------------
int CReQManagerProc::getData(char *pBuffer)
{
	int iRet    = -1;
	scanf("%s", pBuffer);
	iRet = strlen(pBuffer);
	return iRet;
}



//************************
// Message Queue Process
//***********************/
void process(int cmdNumber)
{
	if(cmdNumber > 0){
		switch(cmdNumber){
			case 1: 
				break;
			case 2: 
				break;
			case 3: 
				break;
			case 4: 
				break;
			default:
				printf("Check CMD...\n");
		}
	}
	else{
		printf("CMD: %d\n", cmdNumber);
	}
}



//--------------------------------------
// Directory Files -> EnQueue
//--------------------------------------
void CReQManagerProc::procEnQueueFromFiles()
{
	//-. queueName으로 큐접속
	//1. dirPath/queueName 메일 디렉토리를 dirPath/tmp 폴더를 생성하여 목적폴더를 rename 시킴
	//2. dirPath/tmp/queueName 폴더 안의 리스트를 검색하여 구조체로 생성하여 리스트 생성
	//3. queueName 큐에 메시지 생성하여 전달.
	char                  strQName[SZ_STRING+1]={0,};
	shared_ptr<CEmsQueue> spEmsMsgQueue;
	int                   iRet = -1;
	
	fprintf(stdout, "[INPUT][QName]: ");
	iRet = getData(strQName);
	
	if(iRet > 0){
		spEmsMsgQueue = procGetMessagesFromDir(strQName);
		//fprintf(stdout, "[QName:%s]\n", strQName);
		showLine();

		if(spEmsMsgQueue.get() != NULL){
			char strExchangeName[SZ_STRING+1]={0,};
			fprintf(stdout, "[INPUT][Mail Send Queue Type](Group Share Queue:[I:0] else [M:1]) ");
			iRet = getData(strExchangeName);
			if(iRet > 0){
				int iExMode = atoi(strExchangeName);
				const char *pExchange;
				if ( iExMode == 0){
					pExchange = RMQ_EXCHANGE_COMMON;
				}
				else{
					pExchange = RMQ_EXCHANGE_MAIL;
				}
				procSendToQueue(pExchange, strQName, spEmsMsgQueue);
				//[TEST]//procSendToQueue(pExchange, "1_test", spEmsMsgQueue);
			}
			else{
				string strErrMsg = "Exchange Name Get Failed";
				fprintf(stdout, "[QName:%s] %s\n", strQName, strErrMsg.c_str());
				gLog->Write("[QName:%s] %s", strQName, strErrMsg.c_str());
			}
		}
		else{
			string strErrMsg = "Ems Message is NULL";
			fprintf(stdout, "[QName:%s] %s\n", strQName, strErrMsg.c_str());
			gLog->Write("[QName:%s] %s", strQName, strErrMsg.c_str());
		}
		showLine();
	}
}


//--------------------------------------
// Directory에서 파일의 데이터를 메시지 큐에 저장
//--------------------------------------
shared_ptr<CEmsQueue> CReQManagerProc::procGetMessagesFromDir(const char *pDirName)
{
	shared_ptr<CEmsQueue> spEmsQueue = shared_ptr<CEmsQueue>(new CEmsQueue);
	char DirName[SZ_STRING+1]={0,};
	strncpy(DirName, pDirName, SZ_STRING);

	//Uncomplete Previous Message Process
	gLog->ReadInfo(DirName, spEmsQueue);
	
	return spEmsQueue;
}


//--------------------------------------
// Send Message File Messages -> Queue
//--------------------------------------
void CReQManagerProc::procSendToQueue(const char *pExchange, const char *pQName, shared_ptr<CEmsQueue> spEmsQueue)
{
	if(spEmsQueue.get() != NULL){
		
		AMQPExchange   *pMsgExchange   = NULL;
		CEmsRMQManager *pEmsRMQManager = theEmsRMQManager();
		int             iMaxRetyrCnt   = 3;
		int             iRetryCnt      = 0;
		
		while(iRetryCnt < iMaxRetyrCnt){
			if( (pMsgExchange = pEmsRMQManager->getExchange(pExchange, pQName))==NULL ){
				iRetryCnt++;
				sleep(3);
			}
			else
				break;
		}
		
		if(pMsgExchange != NULL){
			
			boost::shared_ptr<CCpMail>             spCpMail;
			unordered_map<string, char*>          *pEmsMsgs;
			unordered_map<string, char*>::iterator itr_em;

			bool bBodyFlag = false;
			int  iMailCnt  = 0;
				
			while(true){
				spCpMail = spEmsQueue.get()->getCpMail();
				if(spCpMail.get() == NULL)
					break;
				else{
					iMailCnt++;
					bBodyFlag = false;
					pEmsMsgs  = spCpMail.get()->getCpMailInfo();
					itr_em    = pEmsMsgs->begin();
					
					for(; itr_em != pEmsMsgs->end(); itr_em++){
						if(strcmp(itr_em->first.c_str(), MAIL_MSG_BODY)==0){
							bBodyFlag = true;
						}
						else{
							pMsgExchange->setHeader(itr_em->first.c_str(), itr_em->second, true);
						}
					}
					
					if(bBodyFlag == true){
						pMsgExchange->Publish( spCpMail.get()->getMailInfo(MAIL_MSG_BODY), pQName);
						//fprintf(stdout, "1)Mail Send:(Ex:%s, QName:%s, Count:%d)\n", pExchange, pQName, iMailCnt);
					}
					else{
						pMsgExchange->Publish("Msg From File", pQName);
						//fprintf(stdout, "2)Mail Send:(Ex:%s, QName:%s, Count:%d)\n", pExchange, pQName, iMailCnt);
					}
				}
			}
			fprintf(stdout, "Send Messages To RMQ(Ex:%s, QName:%s, Msg Count:%d)\n", pExchange, pQName, iMailCnt);
		}
		else{
			fprintf(stdout, "RMQ Exchange Get Failed(Ex:%s, QName:%s)\n", pExchange, pQName);
			return;
		}
	}
	else{
		fprintf(stdout, "Ems Message Info Queue is Empty\n");
	}
}


//--------------------------------------
// Queue -> Directory Files
//--------------------------------------
void CReQManagerProc::procGetFilesFromQueue()
{
	//-. queueName으로 큐접속
	//1. dirPath/queueName 메일 디렉토리를 생성(기존 폴더 체크 후 없으면 생성하고 있으면 재사용)
	//2. queueName의 큐에서 파일들을 가져와서 파일 리스트로 구성
	
	char                  strQName[SZ_STRING+1]={0,};
	shared_ptr<CEmsQueue> spEmsMsgQueue;
	int                   iRet = -1;
	
	fprintf(stdout, "[INPUT][QName]: ");
	iRet = getData(strQName);

	if(iRet > 0){
		char strExchangeName[SZ_STRING+1]={0,};
		fprintf(stdout, "[INPUT][Mail Send Queue Type](Group Share Queue:[I:0] else [M:1]) ");
		iRet = getData(strExchangeName);
		
		if(iRet > 0){
			int iExMode = atoi(strExchangeName);
			const char *pExchange;
			if ( iExMode == 0){
				pExchange = RMQ_EXCHANGE_COMMON;
			}
			else{
				pExchange = RMQ_EXCHANGE_MAIL;
			}

			spEmsMsgQueue = procGetMessagesFromQueue(pExchange, strQName);
			//큐로부터 수신된 메시지가 있는지 확인
			if(spEmsMsgQueue.get() != NULL){//있다면 파일로 쓰기
				fprintf(stdout, "Write Ems Mail Message To File\n");
			}
			else{ //없다면 Pass
				fprintf(stdout, "Ems Message Queue is Empty\n");
			}
		}
		else{
			fprintf(stdout, "RMQ Exchange Name Get Failed(QName:%s)\n", strQName);
		}
	}
	else{
		fprintf(stdout, "RMQ Queue Name Get Failed\n");
	}
}
                                      
//--------------------------------------
// Queue에서 메시지를 파일로 저장
//--------------------------------------
shared_ptr<CEmsQueue> CReQManagerProc::procGetMessagesFromQueue(const char *pExchange, const char *pQName)
{
	shared_ptr<CEmsQueue> spEmsQueue     = shared_ptr<CEmsQueue>(new CEmsQueue);
	AMQPQueue            *pAMQPMsgQueue  = NULL;
	CEmsRMQManager       *pEmsRMQManager = theEmsRMQManager();

	int iMaxRetyrCnt = 3;
	int iRetryCnt    = 0;
	
	while(iRetryCnt < iMaxRetyrCnt){
		if( (pAMQPMsgQueue= pEmsRMQManager->getQueue(pExchange, pQName))==NULL ){
			iRetryCnt++;
			sleep(3);
		}
		else
			break;
	}
	
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
		
						tmpspCpMail.get()->setWorkerName(pQName); // CCpMail에 Queue 이름을 설정한다.
						
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
		
		
						//Mail Body Info
						//------------------------------------
						uint32_t j = -1;
						tmpData = msg->getMessage(&j);
						if(j > 0){
							tmpspCpMail.get()->setMailInfo(MAIL_MSG_BODY, tmpData);
						}
						else{
							gLog->Write("[%s][%s][%d][WARN]MESSAGE Body is NULL[%s]", 
							__FILE__, __FUNCTION__, __LINE__, msg->getHeader(MHEADER_CPMAILKEY).c_str());
						}
		
						//헤더 정보들을 파일에 저장
						gLog->WriteInfo(tmpspCpMail, true);
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
		string strErrMsg = "AMQP Message Queue Connection Failed";
		fprintf(stdout, "[ERROR][%s][Exchange:%s][QName:%s]\n", strErrMsg.c_str(), pExchange, pQName);
		gLog->Write("[ERROR][%s][Exchange:%s][QName:%s]", strErrMsg.c_str(), pExchange, pQName);
		return spEmsQueue;
	}
	
	return spEmsQueue;
}



void CReQManagerProc::procSendToDir(const char *pDirName,  shared_ptr<CEmsQueue> spEmsQueue)
{
}

