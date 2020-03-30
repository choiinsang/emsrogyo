#include "EmsLog.h"

#include <iostream>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include "../Libgabia/inc/sth_misc.h"
#include "EmsRMQManager.h"
#include "EmsDefineString.h"
#include "EmsConfig.h"
#include "FuncString.h"

using namespace std;

//extern char *itostr(int iNumber, char *pBuf, int iBufSize);
//==================================================
//
//==================================================
CEmsLog::CEmsLog()
: m_FPLog           (NULL)
, m_nLastDay        (-1)
, m_bUseQueue       (false)
, m_strLogName      ("")
, m_bCheckThreadStop(false)
, m_bRMQUsable      (false)
, m_pMsgExchange    (NULL)
, m_maxRetryCount   (INIT_DEFAULT_MAXRETRYCOUNT)
{
	memset(m_szPath, 0x00, MAX_PATH+1);
}

//==================================================
//
//==================================================
CEmsLog::~CEmsLog()
{
}

//==================================================
//
//==================================================
CEmsLog *CEmsLog::m_pLog = NULL;

//==================================================
//
//==================================================
CEmsLog *CEmsLog::getInstance()
{
	if ( m_pLog == NULL )
	{
		m_pLog = new CEmsLog();
	}

	return m_pLog;
}

//==================================================
//
//==================================================
bool CEmsLog::isExistDir(const char *l_pszPath)
{
	DIR *pDir;
	if ((pDir = opendir(l_pszPath)) == NULL)
	{
		return false;
	}
	closedir(pDir);
	return true;
}


//==================================================
//
//==================================================
bool CEmsLog::InitLog(const char *l_pszPath, const char *pLogName)
{
	if(l_pszPath == NULL)
		return false;
	
	if(pLogName != NULL)
		m_strLogName = pLogName;
	else
		exit(0);

	memcpy(m_szPath, l_pszPath, MAX_PATH);

	if(isExistDir(m_szPath) == false){
		if (mkdir(m_szPath, 00777) < 0){
			//fprintf(stdout, "Make Directory Failed[ECODE:%d]\n", strerror(errno));
			exit(0);
		}
	}
	
	m_maxRetryCount = theEMSConfig()->getMailRetryCount();
	
	return true;
}
		
		
//==================================================
//
//==================================================
bool CEmsLog::InitLogQueue(const char * pQueueName, bool bQueueUse)
{
	if( (pQueueName == NULL)||(strlen(pQueueName)==0) ){
		return false;
	}

	//Set Log Queue Name and Queue Connection
	m_bUseQueue    = true;
	m_strMsgQueueName = pQueueName;
	do{
		m_pMsgExchange = theEmsRMQManager()->getExchange(RMQ_EXCHANGE_LOG, m_strMsgQueueName.c_str());
		if(m_pMsgExchange != NULL){
			setRMQUsable(true);
			break;
		}
		else
			sleep(THREAD_RMQ_WAIT_TIME);
	}while(true);
	
	//RMQ Connection Thread
	if(bQueueUse == true)
		checkRMQConnStart();
		
	return true;
}


//==================================================
//
//==================================================
int CEmsLog::checkRMQConnStart()
{
	return pthread_create(&m_chkThread, NULL, &chkThreadFunc, this);
	Write("[%s][EMS Log Thread Create Success!]", __FUNCTION__);
}

//==================================================
//
//==================================================
void CEmsLog::resetRMQConn()
{
	theEmsRMQManager()->deleteRMQConn(RMQ_EXCHANGE_LOG, m_strMsgQueueName.c_str(), false);
	Write("[%s][Delete RMQ Connections]", __FUNCTION__);
	m_pMsgExchange = NULL;
	setRMQUsable(false);
}

//==================================================
//
//==================================================
void CEmsLog::reconnRMQConn()
{
	do{
		theEmsRMQManager()->resetRMQConInfo();
		Write("[%s][%s][%d][--------------------------------]", __FILE__, __FUNCTION__, __LINE__);
		m_pMsgExchange = theEmsRMQManager()->getExchange(RMQ_EXCHANGE_LOG, m_strMsgQueueName.c_str());
		Write("[%s][%s][%d][--------------------------------]", __FILE__, __FUNCTION__, __LINE__);
		if(m_pMsgExchange == NULL){
			sleep(THREAD_RMQ_WAIT_TIME);
			Write("[%s][%s][%d][Reconnect Queue Failed]", __FILE__, __FUNCTION__, __LINE__);
			continue;
		}
		Write("[%s][%s][%d][Reconnect Queue]", __FILE__, __FUNCTION__, __LINE__);
		break;
	}while(true);
	setRMQUsable(true);
	procMsgInfoQueue();
}


//==================================================
//
//==================================================
void CEmsLog::procMsgInfoQueue()
{
	std::deque<shared_ptr<stMsgInfo> > * pMsgInfoQueue = getMsgInfoQueue();
	shared_ptr<stMsgInfo> spMsgInfo;
	try{
		while(pMsgInfoQueue->size() > 0){
			Lock();
			spMsgInfo = pMsgInfoQueue->front();
			pMsgInfoQueue->pop_front();
			Unlock();
			
			if(spMsgInfo.get()->_iMsg > NULL_TYPE){
				sendMsgToQueue(spMsgInfo.get()->_iLogType, spMsgInfo.get()->_iMsgType, spMsgInfo.get()->_spCpMail, spMsgInfo.get()->_iMsg);
			}
			else{
				sendMsgToQueue(spMsgInfo.get()->_iLogType, spMsgInfo.get()->_iMsgType, spMsgInfo.get()->_spCpMail, spMsgInfo.get()->_strMsg.c_str());
			}
		}
		
	}
	catch(...){
		if(spMsgInfo.get() != NULL)
			pMsgInfoQueue->push_front(spMsgInfo);
	}

}


//==================================================
// Rabbitmq Connection을 주기적으로 체크하는 쓰레드
//==================================================
void *CEmsLog::chkThreadFunc(void *param)
{
	CEmsLog *pEmsLog = (CEmsLog *)param;
	
	while(pEmsLog->isStopRMQChecker()==false){
		try{
			if(pEmsLog->getRMQUsable() == false){
				pEmsLog->reconnRMQConn();
			}
			else{
				sleep(1);
			}
		}
		catch(AMQPException ex){
			pEmsLog->resetRMQConn();
		}
		catch(...){
			pEmsLog->resetRMQConn();
		}
	}
	pthread_exit(NULL);
}


//==================================================
// 
//==================================================
void CEmsLog::Write(const char *l_pszFMT, ... )
{
	if (m_szPath[0] == 0x00)
	{
		fprintf(stderr, "[%s][%s][%d] Error LogFile Path!\n", __FILE__, __FUNCTION__, __LINE__);
		return;
	}

	char szFile[MAX_LOG];

	time_t Time;
	struct tm *CurrTime;
	time(&Time);
	CurrTime = localtime(&Time);

	if (CurrTime->tm_mday != m_nLastDay)
	{
		Lock();
		if (m_FPLog != NULL)
		{
			fclose(m_FPLog);
			m_FPLog = NULL;
		}
		
		sprintf(szFile, "%s/%s_%4d%02d%02d.log", m_szPath, m_strLogName.c_str(), CurrTime->tm_year + 1900,
			CurrTime->tm_mon + 1, CurrTime->tm_mday);

		m_FPLog = fopen(szFile, "a+");
		Unlock();
		m_nLastDay = CurrTime->tm_mday;
	}

	va_list args;
	va_start(args, l_pszFMT);
	vsnprintf(szFile, MAX_LOG - 1, l_pszFMT, args);
	//    vsyslog(LOG_DEBUG, l_pszFMT, args );
	va_end(args);

	if (m_FPLog != NULL)
	{
		Lock();

		fprintf(m_FPLog, "[%02d:%02d:%02d] %s\n", CurrTime->tm_hour, CurrTime->tm_min, CurrTime->tm_sec, szFile);
		fflush(m_FPLog);

		Unlock();
	}
}


//==================================================
//--ischoi 사용하는 코드 없음
//==================================================
void CEmsLog::WriteLOG(const char *l_pszData)
{
	if (m_FPLog == NULL)
	{
		fprintf(stderr, "[%s][%s][%d]%s\n", __FILE__, __FUNCTION__, __LINE__, l_pszData);
		return;
	}

	char szFile[MAX_LOG];

	time_t Time;
	struct tm *CurrTime;
	time(&Time);
	CurrTime = localtime(&Time);

	if (CurrTime->tm_mday != m_nLastDay)
	{
		Lock();
		if (m_FPLog != NULL)
		{
			fclose(m_FPLog);
			m_FPLog = NULL;
		}

		sprintf(szFile, "%s/%s_%4d%02d%02d.log", m_szPath, m_strLogName.c_str(), CurrTime->tm_year + 1900,
			CurrTime->tm_mon + 1, CurrTime->tm_mday);			

		m_FPLog = fopen(szFile, "a+");
		Unlock();
		m_nLastDay = CurrTime->tm_mday;
	}	

	if (m_FPLog != NULL)
	{
		Lock();
		fprintf(m_FPLog, "[%02d:%02d:%02d] %s", CurrTime->tm_hour, CurrTime->tm_min, CurrTime->tm_sec,
			l_pszData);
		fflush(m_FPLog);
		Unlock();
	}
}


//==================================================
//
//==================================================
bool CEmsLog::setMsgHeader(unordered_map<string, string> *pInfomap, char *strTime)
{
	if(m_pMsgExchange == NULL){
		if(m_FPLog != NULL){
			fprintf(m_FPLog, "[%s][%s][%d] [Error] Message Header Information is NULL!\n", __FILE__, __FUNCTION__, __LINE__);
		}
		return false;
	}

	unordered_map<string, string>::iterator itr_map = pInfomap->begin();
	int iCount = 0;
	for(; itr_map != pInfomap->end(); itr_map++, iCount++){

		if( (strcmp(itr_map->first.c_str(), MAIL_MSG_BODY)==0 ) 
			    || (strcmp(itr_map->first.c_str(), MHEADER_MAILTITLE)==0 ) 
			    || (strcmp(itr_map->first.c_str(), MHEADER_WIDESTR)==0 ) )
			    continue;
			    
		m_pMsgExchange->setHeader(itr_map->first.c_str(), itr_map->second.c_str(), true);
	}
	
	if(strTime != NULL){
		m_pMsgExchange->setHeader(MSGHEADER_TIMESTAMP, strTime, true);
	}
	else{
		m_pMsgExchange->setHeader(MSGHEADER_TIMESTAMP, "[--:--:--]", true);
	}
		
	if(iCount > 0 ){
		return true;
	}
	else{
		return false;
	}
}


//==================================================
//Queue를 이용해서 메시지 처리
//==================================================
void CEmsLog::WriteToMQ(int logType, unordered_map<string, string> &I_infomap, const char *l_pszFMT, ... )
{
	if (m_FPLog == NULL)
	{
		Write("[%s][%s][%d] [Error] LogFile Connection is NULL!\n", __FILE__, __FUNCTION__, __LINE__);
		return;
	}
	
	if(logType == LOG_NONE){
		Write("[%s][%s][%d][MSG_TYPE is LOG_NONE[%d]]", __FILE__, __FUNCTION__, __LINE__, LOG_NONE);
		return;
	}
	
	char strLog[MAX_LOG];

	time_t Time;
	struct tm *CurrTime;
	time(&Time);
	CurrTime = localtime(&Time);

	if (CurrTime->tm_mday != m_nLastDay)
	{
		Lock();
		if (m_FPLog != NULL)
		{
			fclose(m_FPLog);
			m_FPLog = NULL;
		}
		
		sprintf(strLog, "%s/%s_%4d%02d%02d.log", m_szPath, m_strLogName.c_str(), CurrTime->tm_year + 1900,
			CurrTime->tm_mon + 1, CurrTime->tm_mday);

		m_FPLog = fopen(strLog, "a+");
		Unlock();
		
		m_nLastDay = CurrTime->tm_mday;
	}

	va_list args;
	va_start(args, l_pszFMT);
	vsnprintf(strLog, MAX_LOG - 1, l_pszFMT, args);
	va_end(args);
	
	char strTime[16] = {0,};
	sprintf(strTime, "[%02d:%02d:%02d]", CurrTime->tm_hour, CurrTime->tm_min, CurrTime->tm_sec);

	if((logType & LOG_WQ) == LOG_WQ){
		Lock();
		try{
			if(setMsgHeader(&I_infomap, strTime) == true){
				m_pMsgExchange->Publish(strLog, RMQ_ROUTE_KEY_LOG);
			}
		}
		catch(AMQPException ex){
			Unlock();
			throw AMQPException("WriteToMQ Error Occurred");
		}
		catch(...){
			Unlock();
			throw "Error";
		}
		Unlock();
	}

	if((logType & LOG_WF)==LOG_WF){
		Lock();
		fprintf(m_FPLog, "%s %s\n", strTime, strLog);
		fflush(m_FPLog);	
		Unlock();
	}	
}


//==================================================
// Mail Message Information을 파일로 저장
//==================================================
void CEmsLog::WriteInfo(shared_ptr<CCpMail> spCpMail, bool bMsgFromMQ)
{
	//Log 경로 아래에 Key를 폴더로하는 file 정보를 넣는다.
	char tmpPath[MAX_PATH]={0,};
	sprintf(tmpPath, "%s/%s", m_szPath, spCpMail.get()->getWorkerName());

	//폴더 체크
	if(isExistDir(tmpPath) == false){
		if (mkdir(tmpPath, 00777) < 0)
			return;
	}


	//파일 오픈
	FILE *pfd = NULL;
	char  tmpInfoPath[SZ_NAME] ={0,};

	sprintf(tmpInfoPath, "%s/%s_%s.info", tmpPath, spCpMail.get()->getMailInfo(MHEADER_CPNUMBER), spCpMail.get()->getMailInfo(MHEADER_MAILNUMBER) );
	try{
		if(!(pfd = fopen(tmpInfoPath, "w"))){
			pfd = NULL;
			return;
		}
		else{
			char  tmpstr[SZ_MAXBUFFER];
			const char *tmpType = NULL;
			const char *tmpData = NULL;
			
			//Header Information
			for(int i=0; i < CP_MAILINFO_END; i++){
				memset(tmpstr, 0, SZ_MAXBUFFER);
				tmpType = getHeaderStringFromKey(i);
	  		if(tmpType == NULL)   // header type 없는 경우
	  			continue;
	
				if(bMsgFromMQ==false){
					switch(i){
						case MAILINFO_SMTPCODE:
							sprintf(tmpstr, "%s:%d", tmpType, spCpMail.get()->getSmtpCode());
							break;
						case MAILINFO_SMTPSTEP:
							sprintf(tmpstr, "%s:%d", tmpType, spCpMail.get()->getSmtpStep());
							break;
						case MAILINFO_TRYCOUNT:
							sprintf(tmpstr, "%s:%d", tmpType, spCpMail.get()->getRetryCount());
							break;
						default:{
							tmpData = spCpMail.get()->getMailInfo(tmpType);
			  			if((tmpData != NULL)&&(strlen(tmpData) >0)){
			  				sprintf(tmpstr, "%s:%s", tmpType, tmpData);
			  			}
			  			else 
			  				continue;
						}
					}
				}
				else{
					tmpData = spCpMail.get()->getMailInfo(tmpType);
					if((tmpData != NULL)&&(strlen(tmpData) >0)){
						sprintf(tmpstr, "%s:%s", tmpType, tmpData);
					}
					else
						continue;
				}
				//Write("[%s][%s][%d][TYPE:%s][DATA:%s]", __FILE__, __FUNCTION__, __LINE__, tmpType, tmpData);
				
				if(i > 0)
					fputs("\n", pfd);

				fputs(tmpstr, pfd);
			}
			
			if(bMsgFromMQ==true){
				if (strcmp(spCpMail.get()->getMailInfo(MHEADER_TR_TYPE), STR_MODE_I)==0){
					spCpMail.get()->setMode(MODE_I);
				}
				else{
					spCpMail.get()->setMode(MODE_M);
				}
			}

			//Write("[%s][%s][%d][sTR_MODE:%s][iTR_MODE:%d]", __FILE__, __FUNCTION__, __LINE__, spCpMail.get()->getMailInfo(MHEADER_TR_TYPE), spCpMail.get()->getMode());

			if(spCpMail.get()->getMode()==MODE_I){//Body

				tmpData = spCpMail.get()->getMailInfo(MAIL_MSG_BODY);
				if(tmpData != NULL){
					fputs("\n", pfd);
					fputs(MAIL_MSG_BODY, pfd);
					fputs(":", pfd);
					fputs(tmpData, pfd);
				}
			}
		}

		if(pfd != NULL){
			fclose(pfd);
		}
	}
	catch(...){
		unlink(tmpInfoPath);
	}
}


//==================================================
//완료 메시지인 경우
//Mail Message Information 파일을 삭제.
//==================================================
void CEmsLog::DeleteInfo(shared_ptr<CCpMail> spCpMail)
{
	//Log 경로 체크
	char tmpPath[MAX_PATH]={0,};
	sprintf(tmpPath, "%s/%s", m_szPath, spCpMail.get()->getWorkerName());

	//폴더 체크
	if(isExistDir(tmpPath) == false){
			return;
	}

	//파일 오픈
	char  tmpInfoPath[SZ_NAME] ={0,};
	sprintf(tmpInfoPath, "%s/%s_%s.info", tmpPath, spCpMail.get()->getMailInfo(MHEADER_CPNUMBER), spCpMail.get()->getMailInfo(MHEADER_MAILNUMBER) );

	struct stat fileInfo;
	if( stat(tmpInfoPath, &fileInfo) == 0 ){
		unlink(tmpInfoPath);
		return;
	}
}


//==================================================
// KeyName에 해당하는 Campaign 파일 경로에 
// 처리중인 메시지 정보 저장
//==================================================
void CEmsLog::ReadInfo(char *keyName, shared_ptr<CEmsQueue> &spEmsQueue)
{
	//Log 경로 아래에 Key가 존재하는지 체크
	char tmpPath[MAX_PATH]= {0,};
	sprintf(tmpPath, "%s/%s", m_szPath, keyName);

	DIR *pDir;
	if ((pDir = opendir(tmpPath)) == NULL){
			return;
	}
	else{
		//Read File List 
		struct stat    file_stat;
		vector<string> vecFileList;
		struct dirent *pdirent;
		int            err;
		char           filename[SZ_NAME];
		long           maxfilesize=-1;
		
		while((pdirent=readdir(pDir)) != NULL){
			if(strstr(pdirent->d_name, "info") != NULL){
				vecFileList.push_back(pdirent->d_name);
				memset(filename, 0, SZ_NAME);
				snprintf(filename, SZ_NAME, "%s/%s", tmpPath, pdirent->d_name);
				if ((err=stat(filename, &file_stat)) == 0) {
					if(file_stat.st_size > maxfilesize)
						maxfilesize = file_stat.st_size;
				}
			}
		}
		
		//Insert message into EmsQueue
		if(vecFileList.size() != 0){
			//Read File
			char  msg_delimiter = ':';
			char  tmpFilePath[MAX_PATH]={0,};
			shared_array< char > spReadBuf;
			shared_array< char > spReadBuf2;
			char   *tmpBuf  = NULL;
			char   *tmpBuf2 = NULL;
			FILE   *pfd     = NULL;
			fpos_t fpos;

			if(maxfilesize > SZ_MAXBUFFER)
				maxfilesize += 1;
			else 
				maxfilesize = SZ_MAXBUFFER+1;
				
			spReadBuf  = shared_array< char >(new char[maxfilesize]);
			tmpBuf     = spReadBuf.get();
			spReadBuf2 = shared_array< char >(new char[maxfilesize]);
			tmpBuf2    = spReadBuf2.get();
			
			if(tmpBuf==NULL){
				Write("[%s][ERROR][Create Shared Buffer Failed]", __FUNCTION__);
				return;
			}
			
			char *pHType   = NULL;
			char *pMsgData = NULL;
			size_t ipos = 0;
			size_t hpos = 0;
			size_t dpos = 0;
			
			for(vector<string>::iterator itr_fname = vecFileList.begin();
				   itr_fname != vecFileList.end(); itr_fname++)
			{
				memset(tmpFilePath, 0, MAX_PATH);
				sprintf(tmpFilePath, "%s/%s", tmpPath, itr_fname->c_str());

				if( (pfd = fopen(tmpFilePath, "r")) != NULL){
					//Make Message
					boost::shared_ptr<CCpMail> tmpspCpMail = boost::shared_ptr<CCpMail>(new CCpMail);
					if (tmpspCpMail.get() == NULL)
						continue;

					try{
						int iMaxHeaderLen = 30;
						
						while(!feof(pfd)){
							memset(tmpBuf, 0, maxfilesize);
							fgets(tmpBuf, maxfilesize, pfd);
							ipos = strlen(tmpBuf);
							if(ipos == 0){
								continue;
							}
							tmpBuf[ipos]='\0';
	
							{  //Line Parsing :tmpBuf = Header_Type:Mag_Data
								bool   bRet   = false;
								size_t buflen = strlen(tmpBuf);
	
								if(tmpBuf[buflen-1] == '\n')
									tmpBuf[buflen-1] = '\0';
									
								//find msg_delimiter ':'
								for(ipos = 0; ipos < iMaxHeaderLen; ipos++){
									if(tmpBuf[ipos] == msg_delimiter){
										bRet        = true;
										tmpBuf[ipos]= '\0';
										ipos++;
										break;
									}
									else if(tmpBuf[ipos]=='\0'){
										break;
									}
								}
								
								if(bRet == false){
									continue;  // whlie(!feof(pfd))
								}
								else{
									pHType   = tmpBuf;
									pMsgData = &tmpBuf[ipos];
									hpos = 0;
									dpos = strlen(pMsgData);

									fgetpos(pfd, &fpos);
									
									while(!feof(pfd)){
										memset(tmpBuf2, 0, maxfilesize);
										fgets(tmpBuf2, maxfilesize, pfd);
										
										bRet = false;
										//find msg_delimiter ':'
										for(ipos = 0; ipos < iMaxHeaderLen; ipos++){
											if(tmpBuf2[ipos] == msg_delimiter){
												bRet = true;
												//ipos++;
												break;
											}
											else if(tmpBuf2[ipos]=='\0'){
												break;
											}
										}
										
										if(bRet == true){ //Msg Header Type 발견
											for(int i=0; i<CP_MAILINFO_END; i++){
												
												if((strncmp(tmpBuf2, getHeaderStringFromKey(i), ipos)==0)||(strncmp(tmpBuf2, MAIL_MSG_BODY, ipos)==0)){
													fsetpos(pfd, &fpos);
													bRet = true;
													break;
												}
												else{
													bRet = false;
												}
											}
											
											if(bRet == true){
												break;
											}
										}
										
										//if(bRet == false)
										memcpy(&pMsgData[dpos], tmpBuf2, strlen(tmpBuf2));
										dpos += strlen(tmpBuf2);
										pMsgData[dpos]='\0';
									}
									tmpspCpMail.get()->setMailInfo(pHType, pMsgData);
								}
							}
						}
					}
					catch(...){
						Write("[%s][%s][%d][ERROR][%s file read error]", __FILE__, __LINE__, tmpFilePath);
					}

					fclose(pfd);
					
					const char *tmpdata2 = NULL;
					if( (tmpdata2 = tmpspCpMail.get()->getMailInfo(MHEADER_TRYCOUNT) ) != NULL ){
						if( atoi(tmpdata2) < m_maxRetryCount ){
							if(strcmp(tmpspCpMail.get()->getMailInfo(MHEADER_TR_TYPE), STR_MODE_M)==0){
								tmpspCpMail.get()->setWorkerName(tmpspCpMail.get()->getMailInfo(MHEADER_QINDEX));
							}
							else{ //if (STR_MODE_I)
								tmpspCpMail.get()->setWorkerName(keyName);
							}
							spEmsQueue.get()->addCpMail(tmpspCpMail);
						}
					}
					else{
						spEmsQueue.get()->addCpMail(tmpspCpMail);
					}
				}      //if(fopen(File))
			}      //for(vecFileList Parsing)
		}      // if(vecFileList.size() != 0){
	}
	closedir(pDir);
}


//==================================================
//
//==================================================
int CEmsLog::remove_dir(char *path)
{
	DIR *d = opendir(path);
	size_t path_len = strlen(path);
	int r = -1;
	
	if(d){
		struct dirent *p;
		
		r = 0;
		
		while(!r && (p=readdir(d))){
			int r2 = -1;
			char *buf;
			size_t len;
			
			/* Skip the names "." and ".." as we don't want to recurse on them. */
			if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, "..")){
				continue;
			}
			
			len = path_len + strlen(p->d_name) + 2; 
			buf = (char*)malloc(len);
			
			if(buf){
				struct stat statbuf;
				
				snprintf(buf, len, "%s/%s", path, p->d_name);
				
				if (!stat(buf, &statbuf)) {
					if (S_ISDIR(statbuf.st_mode)) {
						r2 = remove_dir(buf);
					}
					else {
						r2 = unlink(buf);
					}
				}
				free(buf);
			}
			r = r2;
		}
		
		closedir(d);
	}

	if(!r) {
		Write("[rmdir path]:%s", path);
		r = rmdir(path);
	}
	
	return r;
}


//==================================================
//
//==================================================
void CEmsLog::CloseInfo(char *keyName)
{
	//Log 경로 아래에 Key를 폴더로하는 file 정보를 넣는다.
	char tmpPath[MAX_PATH]={0,};
	sprintf(tmpPath, "%s/%s", m_szPath, keyName);

	Write("[DONE][%s]", tmpPath);
	//Check Folder
	if(isExistDir(tmpPath) == true){
		remove_dir(tmpPath);
	}
}


//==================================================
//
//==================================================
void CEmsLog::getMsgHeaderInfo(int iMsgType, shared_ptr<CCpMail> &spCpMail, unordered_map<string, string> *pMapLogHeaderInfo)
{
	if(pMapLogHeaderInfo == NULL){
		return;
	}
	else{
		char strMsgType[10]={0,};
		sprintf(strMsgType, "%d", iMsgType);
		pMapLogHeaderInfo->insert(std::pair<string, string>(MSGHEADER_TYPE, strMsgType));
		pMapLogHeaderInfo->insert(std::pair<string, string>(MSGHEADER_CPNUMBER,   spCpMail.get()->getMailInfo(MHEADER_CPNUMBER)));
		pMapLogHeaderInfo->insert(std::pair<string, string>(MSGHEADER_MAILNUMBER, spCpMail.get()->getMailInfo(MHEADER_MAILNUMBER)));
		pMapLogHeaderInfo->insert(std::pair<string, string>(MSGHEADER_TOID,       spCpMail.get()->getMailInfo(MHEADER_TOID)));
		pMapLogHeaderInfo->insert(std::pair<string, string>(MSGHEADER_TODOMAIN,   spCpMail.get()->getMailInfo(MHEADER_TODOMAIN)));
		pMapLogHeaderInfo->insert(std::pair<string, string>(MSGHEADER_GROUPNUMBER,spCpMail.get()->getMailInfo(MHEADER_GROUPNUMBER)));
	}
}


//==================================================
//기존에 사용중인 로그 정보는 Function 으로 변경 혹은 API 형태로 분리하여 메시지 처리에 대한 방법을 강구한다.
//실질적인 메시지 데이터 이동 보다는 프로토콜을 이용한 전달 방식이 좋을 듯함. <= <TODO> 수정 예정 -ischoi
//==================================================
void CEmsLog::sendMsgToQueue(int iLogType, int iMsgType, shared_ptr<CCpMail> spCpMail, int iMsg)
{
	if(getRMQUsable()==false ){
		//Disconnect  상태이므로 MessageQueue에 일단 넣어서 데이터를 보관한다.
		
		Write("[%s][RMQUsable:%d][MsgInfoQueue Size:%d]", __FUNCTION__, getRMQUsable(), getMsgInfoQueue()->size());
		shared_ptr<stMsgInfo> spMsgInfoQueue = shared_ptr<stMsgInfo>(new stMsgInfo(iLogType, iMsgType, spCpMail, iMsg));
		Lock();
		getMsgInfoQueue()->push_back(spMsgInfoQueue);
		Unlock();
		return;
	}
	
	char     iBuffer[Qry_Strlen]={0,};
	char    *pBuf  = NULL;
	string   strTmp=__FUNCTION__;

	//iLogType : LOG_NONE(0)/WF(1)/WQ(2)/WA(3)
	unordered_map<string, string> mapLogHeaderInfo;

	char strMsgType[10]={0,};
	sprintf(strMsgType, "%d", iMsgType);
	mapLogHeaderInfo.insert(std::pair<string, string>(MSGHEADER_TYPE,        strMsgType));
	mapLogHeaderInfo.insert(std::pair<string, string>(MSGHEADER_CPNUMBER,    spCpMail.get()->getMailInfo(MHEADER_CPNUMBER)));
	mapLogHeaderInfo.insert(std::pair<string, string>(MSGHEADER_MAILNUMBER,  spCpMail.get()->getMailInfo(MHEADER_MAILNUMBER)));
	mapLogHeaderInfo.insert(std::pair<string, string>(MSGHEADER_MAILINDEX,   spCpMail.get()->getMailInfo(MHEADER_MAILINDEX)));
	mapLogHeaderInfo.insert(std::pair<string, string>(MSGHEADER_TOID,        spCpMail.get()->getMailInfo(MHEADER_TOID)));
	mapLogHeaderInfo.insert(std::pair<string, string>(MSGHEADER_TODOMAIN,    spCpMail.get()->getMailInfo(MHEADER_TODOMAIN)));
	mapLogHeaderInfo.insert(std::pair<string, string>(MSGHEADER_GROUPNUMBER, spCpMail.get()->getMailInfo(MHEADER_GROUPNUMBER)));
	
	pBuf = itostr(iMsg, iBuffer, Qry_Strlen);
	mapLogHeaderInfo.insert(std::pair<string, string>(MSGHEADER_DBQUERY_SET, pBuf));
	pBuf = itostr(spCpMail.get()->getMode(), iBuffer, Qry_Strlen);
	mapLogHeaderInfo.insert(std::pair<string, string>(MSGHEADER_TR_TYPE,     pBuf));
	
	//Write("[%s][%s][%d][MSG TYEP:%d]", __FILE__, __FUNCTION__, __LINE__, iMsg);
	try{
		switch(iMsg){
	
			case iQryUdt_Mail_SmtpStep0 :
				mapLogHeaderInfo.insert(std::pair<string, string>(MSGHEADER_ERR_STR,""));
				break;
			case iQryUdt_Mail_SmtpStep21 : {
				pBuf = itostr(spCpMail.get()->getSmtpCode(), iBuffer, Qry_Strlen);
				mapLogHeaderInfo.insert(std::pair<string, string>(MSGHEADER_SMTPCODE, pBuf));
				pBuf = itostr(spCpMail.get()->getRetryCount(), iBuffer, Qry_Strlen);
				mapLogHeaderInfo.insert(std::pair<string, string>(MSGHEADER_TRYCOUNT, pBuf));
				mapLogHeaderInfo.insert(std::pair<string, string>(MSGHEADER_ERR_STR,""));
				break;
			}
			case iQryUdt_Mail_SmtpStep21Chk7 : {
				pBuf = itostr(spCpMail.get()->getSmtpStep(), iBuffer, Qry_Strlen);
				mapLogHeaderInfo.insert(std::pair<string, string>(MSGHEADER_SMTPSTEP, pBuf));
				pBuf = itostr(spCpMail.get()->getRetryCount(), iBuffer, Qry_Strlen);
				mapLogHeaderInfo.insert(std::pair<string, string>(MSGHEADER_TRYCOUNT, pBuf));
				mapLogHeaderInfo.insert(std::pair<string, string>(MSGHEADER_ERR_STR,""));
				break;
			}
			case iQryUdt_Mail_SmtpStepComplete : {
				pBuf = itostr(spCpMail.get()->getSmtpCode(), iBuffer, Qry_Strlen);
				mapLogHeaderInfo.insert(std::pair<string, string>(MSGHEADER_SMTPCODE, pBuf));
				pBuf = itostr(spCpMail.get()->getRetryCount(), iBuffer, Qry_Strlen);
				mapLogHeaderInfo.insert(std::pair<string, string>(MSGHEADER_TRYCOUNT, pBuf));
				mapLogHeaderInfo.insert(std::pair<string, string>(MSGHEADER_ERR_STR,""));
				break;
			}
			case iQryUdt_Mail_SmtpQRfail : { 
				pBuf = itostr(spCpMail.get()->getRetryCount(), iBuffer, Qry_Strlen);
				mapLogHeaderInfo.insert(std::pair<string, string>(MSGHEADER_TRYCOUNT, pBuf));
				mapLogHeaderInfo.insert(std::pair<string, string>(MSGHEADER_ERR_STR,""));
				break;
			}
			case iQryUdt_Mail_Smtp : {
				pBuf = itostr(spCpMail.get()->getSmtpStep(), iBuffer, Qry_Strlen);
				mapLogHeaderInfo.insert(std::pair<string, string>(MSGHEADER_SMTPSTEP, pBuf));
				pBuf = itostr(spCpMail.get()->getSmtpCode(), iBuffer, Qry_Strlen);
				mapLogHeaderInfo.insert(std::pair<string, string>(MSGHEADER_SMTPCODE, pBuf));
				pBuf = itostr(spCpMail.get()->getRetryCount(), iBuffer, Qry_Strlen);
				mapLogHeaderInfo.insert(std::pair<string, string>(MSGHEADER_TRYCOUNT, pBuf));
				mapLogHeaderInfo.insert(std::pair<string, string>(MSGHEADER_ERR_STR,""));
				break;
			}
			case iQryUdt_Mail_SmtpChk7 : {
				pBuf = itostr(spCpMail.get()->getSmtpStep(), iBuffer, Qry_Strlen);
				mapLogHeaderInfo.insert(std::pair<string, string>(MSGHEADER_SMTPSTEP, pBuf));
				pBuf = itostr(spCpMail.get()->getSmtpCode(), iBuffer, Qry_Strlen);
				mapLogHeaderInfo.insert(std::pair<string, string>(MSGHEADER_SMTPCODE, pBuf));
				pBuf = itostr(spCpMail.get()->getRetryCount(), iBuffer, Qry_Strlen);
				mapLogHeaderInfo.insert(std::pair<string, string>(MSGHEADER_TRYCOUNT, pBuf));
				mapLogHeaderInfo.insert(std::pair<string, string>(MSGHEADER_ERR_STR,""));
				break;
			}
			case iQryIns_CodeExp : {
				pBuf = itostr(spCpMail.get()->getSmtpStep(), iBuffer, Qry_Strlen);
				mapLogHeaderInfo.insert(std::pair<string, string>(MSGHEADER_SMTPSTEP, pBuf));
				pBuf = itostr(spCpMail.get()->getSmtpCode(), iBuffer, Qry_Strlen);
				mapLogHeaderInfo.insert(std::pair<string, string>(MSGHEADER_SMTPCODE, pBuf));
				pBuf = itostr(spCpMail.get()->getRetryCount(), iBuffer, Qry_Strlen);
				mapLogHeaderInfo.insert(std::pair<string, string>(MSGHEADER_TRYCOUNT, pBuf));
				mapLogHeaderInfo.insert(std::pair<string, string>(MSGHEADER_ERR_STR,  spCpMail.get()->getExpResultStr()));
				break;
			}
			case iQryIns_CodeExpResult : {
				pBuf = itostr(spCpMail.get()->getSmtpStep(), iBuffer, Qry_Strlen);
				mapLogHeaderInfo.insert(std::pair<string, string>(MSGHEADER_SMTPSTEP,   pBuf));
				pBuf = itostr(spCpMail.get()->getSmtpCode(), iBuffer, Qry_Strlen);
				mapLogHeaderInfo.insert(std::pair<string, string>(MSGHEADER_SMTPCODE,   pBuf));
				pBuf = itostr(spCpMail.get()->getRetryCount(), iBuffer, Qry_Strlen);
				mapLogHeaderInfo.insert(std::pair<string, string>(MSGHEADER_TRYCOUNT,   pBuf));
				pBuf = itostr(spCpMail.get()->getExpResultCode(), iBuffer, Qry_Strlen);
				mapLogHeaderInfo.insert(std::pair<string, string>(MSGHEADER_ERR_CODE,   pBuf));
				mapLogHeaderInfo.insert(std::pair<string, string>(MSGHEADER_ERR_STR,    spCpMail.get()->getExpResultStr()));
				break;
			}
		}
	}
	catch(...){
		Write("[%s][%s][%d][ERROR][MsgType:%d][CPNO:%s][CPMAILNUMBER:%s]"
		             , __FILE__, __FUNCTION__, __LINE__
		             , iMsg
		             , spCpMail.get()->getMailInfo(MSGHEADER_CPNUMBER)
		             , spCpMail.get()->getMailInfo(MSGHEADER_MAILNUMBER));
	}

	//Check Mesage is Last or Continue...
	if( (spCpMail.get()->getRetryCount() >= m_maxRetryCount) || (spCpMail.get()->getStepComplete() == true) ){
		DeleteInfo(spCpMail);
	}
	else{
		WriteInfo(spCpMail);
	}
	
	try{
		WriteToMQ(iLogType, mapLogHeaderInfo, strTmp.c_str() );
	}
	catch(AMQPException ex){
		Write("[%s][%s][%d][ERROR][MSG:%s][CPNO:%s][MAILNO:%s]"
		, __FILE__, __FUNCTION__, __LINE__
		, ex.getMessage().c_str(), spCpMail.get()->getMailInfo(MSGHEADER_CPNUMBER), spCpMail.get()->getMailInfo(MSGHEADER_MAILNUMBER));
		
		shared_ptr<stMsgInfo> spMsgInfoQueue = shared_ptr<stMsgInfo>(new stMsgInfo(iLogType, iMsgType, spCpMail, iMsg));
		Lock();
		getMsgInfoQueue()->push_back(spMsgInfoQueue);
		Unlock();
		resetRMQConn();
	}
	catch(...){
		Write("[%s][%s][%d][ERROR][Unknown Error Occurred]", __FILE__, __FUNCTION__, __LINE__);
		shared_ptr<stMsgInfo> spMsgInfoQueue = shared_ptr<stMsgInfo>(new stMsgInfo(iLogType, iMsgType, spCpMail, iMsg));
		Lock();
		getMsgInfoQueue()->push_back(spMsgInfoQueue);
		Unlock();
		resetRMQConn();
	}
}

//==================================================
//
//==================================================
void CEmsLog::sendMsgToQueue(int iLogType, int iMsgType, shared_ptr<CCpMail> spCpMail, const char *pLogStr)
{
	//iLogType : LOG_NONE(0)/WF(1)/WQ(2)/WA(3)
	unordered_map<string, string> mapLogHeaderInfo;
	getMsgHeaderInfo(iMsgType, spCpMail, &mapLogHeaderInfo);
	
	WriteToMQ(iLogType, mapLogHeaderInfo, pLogStr );
}

