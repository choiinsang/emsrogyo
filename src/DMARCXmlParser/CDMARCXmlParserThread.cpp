//##############################################################################################
#include "CDMARCXmlParserThread.h"
#include "CDMARCXmlParserConfig.h"
#include "CDMARCLog.h"
#include "CDMARCFunctions.h"
#include "DMARCDefineString.h"


extern CDMARCLog             *gLog;
extern CDMARCXmlParserConfig *gTheConfig ;

//--------------------------------------------------------------------
// CDMARCXmlParserThread 
//--------------------------------------------------------------------
CDMARCXmlParserThread::CDMARCXmlParserThread()
{
	m_bIsStop = false;
	
	memset(m_chOrgDirPath,    0, SZ_NAME+1);
	memset(m_chTmpDirPath,    0, SZ_NAME+1);
	memset(m_chTargetDirPath, 0, SZ_NAME+1);
	memset(m_chErrorDirPath,  0, SZ_NAME+1);
	

	memcpy(m_chOrgDirPath,    gTheConfig->getFileInputPath(),    strlen(gTheConfig->getFileInputPath()));
	memcpy(m_chTmpDirPath,    gTheConfig->getFileInputTmpPath(), strlen(gTheConfig->getFileInputTmpPath()));
	memcpy(m_chTargetDirPath, gTheConfig->getFileOutPutPath(),   strlen(gTheConfig->getFileOutPutPath()));
	memcpy(m_chErrorDirPath,  gTheConfig->getFileErrorPath(),    strlen(gTheConfig->getFileErrorPath()));
	
	try{
		if(checkDir (m_chOrgDirPath,    true) == false) throw "FILE_INPUT_PATH";
		if(checkDir (m_chTmpDirPath,    true) == false) throw "FILE_INPUT_TMP_PATH";
		if(checkDir (m_chTargetDirPath, true) == false) throw "FILE_OUTPUT_PATH";
		if(checkDir (m_chErrorDirPath,  true) == false) throw "FILE_ERROR_PATH";
	}
	catch(string strNotExistDir){
		gLog->Write("Directory Is Not Exist:[%s]", strNotExistDir.c_str());
		exit(0);
	}

	//Init Mysql DB Connection
	if(ConnectDB() == false){
		exit(-1);
	}

	setRunState(TSTATE_READY);
}


//--------------------------------------------------------------------
// Check & Set DB Host Info & Init Mysql DB Connection
//--------------------------------------------------------------------
bool CDMARCXmlParserThread::ConnectDB()
{	
	if( (gTheConfig->getDBIP() == NULL)
		||(gTheConfig->getDBUser() == NULL)
		||(gTheConfig->getDBPasswd() == NULL)
		||(gTheConfig->getDBName() == NULL)
		||(gTheConfig->getDBPort() <= 0) ){
			gLog->Write("DB Config Error!");
			exit(0);
	}
	else{
		char strThreadName   [] = "DMARCXmlParserThread";
		char strThreadNameSub[] = "DMARCXmlParserThreadSub";
	
		try{
			bool bResult = false;
			
			m_DmarcMysql.SetHost(gTheConfig->getDBIP(), gTheConfig->getDBUser(), gTheConfig->getDBPasswd(), gTheConfig->getDBName(), gTheConfig->getDBPort());
			m_DmarcMysql.SetOwnerName(strThreadName);
			
			bResult = m_DmarcMysql.ConnectDB();
			
			
			if(!bResult){
				gLog->Write("[%s] E[DB Connect Fail] V[DBName:%s]", __FUNCTION__, gTheConfig->getDBName());
				throw string("DB Connection Errorr Occurred(Main db connection");
			}

			bResult = false;
			m_DmarcMysqlSub.SetHost(gTheConfig->getDBIP(), gTheConfig->getDBUser(), gTheConfig->getDBPasswd(), gTheConfig->getDBName(), gTheConfig->getDBPort());
			m_DmarcMysqlSub.SetOwnerName(strThreadNameSub);
			
			bResult = m_DmarcMysqlSub.ConnectDB();
			
			if(!bResult){
				gLog->Write("[%s] E[DB Connect Fail] V[DBName:%s]", __FUNCTION__, gTheConfig->getDBName());
				throw string("DB Connection Errorr Occurred(Sub db connection");
			}

			return true;
		}
		catch(std::string errStr){
			gLog->Write("[%s][%d] E[DB Connect Error Occurred] V[DBName:%s][Msg:%s]", __FUNCTION__, __LINE__, gTheConfig->getDBName(), errStr.c_str());
			return false;
		}
		catch(...){
			gLog->Write("[%s][%d] E[DB Connect Unknown Error Occurred] V[DBName:%s]", __FUNCTION__, __LINE__, gTheConfig->getDBName());
			return false;
		}
	}

}

//--------------------------------------------------------------------
// ~CDMARCXmlParserThread 
//--------------------------------------------------------------------
CDMARCXmlParserThread::~CDMARCXmlParserThread()
{		
	if(m_bIsStop == false){
		m_bIsStop = true;
	}
	
	pthread_join(m_tParserThread, NULL);
}


//--------------------------------------------------------------------
// Init CDMARCXmlParserThread Static Variable
//--------------------------------------------------------------------
CDMARCXmlParserThread *CDMARCXmlParserThread::m_pDMARCXmlParserThread = NULL;


//--------------------------------------------------------------------
// Get CDMARCXmlParserThread Static Variable
//--------------------------------------------------------------------
CDMARCXmlParserThread *CDMARCXmlParserThread::getInstance()
{	
	if(m_pDMARCXmlParserThread == NULL){
		
		do{
			m_pDMARCXmlParserThread = new CDMARCXmlParserThread();
			
			if(m_pDMARCXmlParserThread != NULL) {
				break;
			}
			sleep(1);
		}while(true);
	}
	
	return m_pDMARCXmlParserThread;
}
	
//--------------------------------------------------------------------
// StartThread 
//--------------------------------------------------------------------
void CDMARCXmlParserThread::startThread()
{
	pthread_create(&m_tParserThread, NULL, &xmlParserThreadFunc, this);
}


//--------------------------------------------------------------------
// XML Parsing & Return (shared_ptr) CDMARCXmlParser
//--------------------------------------------------------------------
shared_ptr<CDMARCXmlParser> CDMARCXmlParserThread::parseFile(const char *filePath, int extNo)
{
	shared_ptr<CDMARCXmlParser> spXmlParser;

	if(filePath != NULL){
		
		spXmlParser = shared_ptr<CDMARCXmlParser>(new CDMARCXmlParser);
		switch(extNo){
			case EXT_XML:{
				spXmlParser.get()->ParseXml(filePath);
				break;
			}
			case EXT_ZIP:
			case EXT_GZ:{
				if(spXmlParser.get()->ParseZip(filePath) == false){
					shared_ptr<CDMARCXmlParser> tmpspXmlParser;
					return tmpspXmlParser;
				}
				
				break;
			}
			default:{
				shared_ptr<CDMARCXmlParser> tmpspXmlParser;
				return tmpspXmlParser;
			}
		}
	}
	
	return spXmlParser;
}


//--------------------------------------------------------------------
// Move File from Original Path to Target Path
//--------------------------------------------------------------------
bool CDMARCXmlParserThread::moveFile(const char *fromPath, const char *toPath, const char *fileName)
{
	if(fromPath == NULL || toPath == NULL){
		return false;
	}
	else{
		int iRet=0;
		std::string strFromPath = fromPath;
		std::string strToPath   = toPath;
			
		if(fileName != NULL){
			strFromPath += "/";
			strFromPath += fileName;
			strToPath   += "/";
			strToPath   += fileName;
		}
		

		if( (iRet = rename(strFromPath.c_str(), strToPath.c_str())) == -1){
			gLog->Write("[%s]Error Occurred[CODE:%d][MSG:%s]", __FUNCTION__, errno, strerror(errno) );
			return false;
		}
		else
			return true;
	} 
}

//--------------------------------------------------------------------
// 디렉토리 검색 및 XML 파일 파싱 처리
//--------------------------------------------------------------------
void *CDMARCXmlParserThread::xmlParserThreadFunc(void *Param)
{
	CDMARCXmlParserThread *pThread = (CDMARCXmlParserThread *)Param;

	pThread->setRunState(TSTATE_RUN);

	while( true )
	{
		if(pThread->isStop())
			break;
			
		if(pThread->getRunState() == TSTATE_PAUSE){
			sleep(1);
			continue;
		}
		else{
			//-- File Exist In the Directory
			std::string         errStr;
			vector<std::string> vecFiles;

			if(getFilesFromDir(pThread->getOrgDirPath(), vecFiles, errStr) == true){
				
				if(vecFiles.size() > 0){ //파일이 존재하는 경우
					int          chkResult;
					std::string  errStr;
					string       strPath("");

					for(vector<std::string>::iterator itr = vecFiles.begin(); itr != vecFiles.end(); itr++){
						
						chkResult = checkFileExtention(itr->c_str(), errStr);
						
						strPath = pThread->getOrgDirPath();
						strPath += "/";
						strPath += itr->c_str();
						
						switch(chkResult){
							case EXT_XML:
							case EXT_ZIP:
							case EXT_GZ :{
								//-- Paras & Save Data
								shared_ptr<CDMARCXmlParser> spDMARCXmlParser = pThread->parseFile(strPath.c_str(), chkResult);
								if(spDMARCXmlParser.get() == NULL){
									pThread->moveFile(pThread->getOrgDirPath(), pThread->getErrorDirPath(), itr->c_str());
								}
								else{
									if(pThread->InsertIntoDB(spDMARCXmlParser) == true){
										gLog->Write("InsertIntoDB Success[%s]", pThread->getTargetDirPath());
										pThread->moveFile(pThread->getOrgDirPath(), pThread->getTargetDirPath(), itr->c_str());
									}
									else{
										gLog->Write("InsertIntoDB Failed[%s]", pThread->getErrorDirPath());
										pThread->moveFile(pThread->getOrgDirPath(), pThread->getErrorDirPath(), itr->c_str());
									}
								}
								break;
							}
							case EXT_NOT_EXIST:  //-- 확장자가 맞지 않은 경우
							default           :{ //-- Error
								//-- 파일을 에러 폴더로 이동
								pThread->moveFile(pThread->getOrgDirPath(), pThread->getErrorDirPath(), itr->c_str());
							}
						} // switch
					} // for
				} // if()
			}
			else{
				//Do Nothing
			}
	
			sleep(2);
		}
	}

	gLog->Write( "[%s][%s] End Thread!!", __FILE__, __FUNCTION__);

	return NULL;
}



bool CDMARCXmlParserThread::Start_Transaction()
{
	return m_DmarcMysql.QueryDB(START_TRANSACTION);
}

bool CDMARCXmlParserThread::Commit_Transaction()
{
	return m_DmarcMysql.QueryDB(CALL_COMMIT);
}

bool CDMARCXmlParserThread::Rollback_Transaction()
{
	return m_DmarcMysql.QueryDB(CALL_ROLLBACK);
}


//--------------------------------------------------------------------
// InsertIntoDB 
//--------------------------------------------------------------------
bool  CDMARCXmlParserThread::InsertIntoDB(shared_ptr<CDMARCXmlParser> xmlInfo)
{
	//--------------------------
	//shared_ptr<CDMARCXmlParser> xmlInfo에 데이터가 모두 담겨있다.
	//--------------------------
	bool bRet = true;
	
	if(xmlInfo.get() == NULL){
		bRet = false;
	}
	else{

		Start_Transaction();
		try{
	
			shared_ptr<stFeedbackRecord> spFeedbackRecord = xmlInfo->getFeedbackRecord();
			if(spFeedbackRecord.get() != NULL){
				//레코드가 있다면, 값들을 채워서 DB에 삽입 하도록 한다. 혹시나 모를 중복 처리되는 파일이 있을 수 있으므로, 이미 존재하는 경우에는 업데이트
				//INSERT report_info -> success
				//  INSERT  report_record -> success(OK)
				//  INSERT  report_record -> fail
				//    UPDATE report_record -> success(OK)
				//    UPDATE report_record -> fail(Data Fail)
				
				//INSERT report_info -> fail
				//    UPDATE report_info -> success
				//        INSERT  report_record -> success(OK)
				//        INSERT  report_record -> fail
				//            UPDATE report_record -> success(OK)
				//            UPDATE report_record -> fail(Data Fail)
				//    UPDATE report_info -> fail

				char strQry[SZ_DEFAULTBUFFER] = {0,};
				stReportMetaData  *pReprotMetaData  = spFeedbackRecord->getReportMetaData();
				stPolicyPublished *pPolicyPublished = spFeedbackRecord->getPolicyPublished();
				//[report_id][policy_domain][report_email][report_org_name][report_date_begin][report_date_end][policy_adkim][policy_aspf][policy_p][policy_sp][policy_pct]
				snprintf(strQry, SZ_DEFAULTBUFFER, QueryInsert_info,
				                 pReprotMetaData->m_strReport_id.c_str(), 
				                 pPolicyPublished->m_strDomain.c_str(),
				                 pReprotMetaData->m_strOrgName.c_str(), 
				                 pReprotMetaData->m_strEmail.c_str(),
				                 pReprotMetaData->m_strBegin.c_str(),
				                 pReprotMetaData->m_strEnd.c_str(),
				                 pPolicyPublished->m_strAdkim.c_str(),
				                 pPolicyPublished->m_strAspf.c_str(),
				                 pPolicyPublished->m_strP.c_str(),
				                 pPolicyPublished->m_strSP.c_str(),
				                 pPolicyPublished->m_strPct.c_str());
				
				bRet = m_DmarcMysql.QueryDB(strQry);

				if(bRet == false){
					//UPDATE 'report_info"
					memset(strQry, 0, SZ_DEFAULTBUFFER);
					//[report_email][report_org_name][report_date_begin][report_date_end][policy_adkim][policy_aspf][policy_p][policy_sp][policy_pct]
					snprintf(strQry, SZ_DEFAULTBUFFER, QueryUpdate_info,
					                 pReprotMetaData->m_strEmail.c_str(),
					                 pReprotMetaData->m_strOrgName.c_str(), 
					                 pReprotMetaData->m_strBegin.c_str(),
					                 pReprotMetaData->m_strEnd.c_str(),
					                 pPolicyPublished->m_strAdkim.c_str(),
					                 pPolicyPublished->m_strAspf.c_str(),
					                 pPolicyPublished->m_strP.c_str(),
					                 pPolicyPublished->m_strSP.c_str(),
					                 pPolicyPublished->m_strPct.c_str(),
					                 pReprotMetaData->m_strReport_id.c_str(), 
					                 pPolicyPublished->m_strDomain.c_str());
					
					bRet = m_DmarcMysql.QueryDB(strQry);
				}
				
				if(bRet == true){ //'report_info' TABLE INSERT or UPDATE Success

					shared_ptr<vector<shared_ptr<stRecord> > > spRecords = spFeedbackRecord->getRecords();
					if(spRecords.get() == NULL){ //record Data is Nothing
						//End Update : Error Exception Occurred
						gLog->Write("[%s][%s][%d][ERROR] Record Data is NULL", __FILE__, __FUNCTION__, __LINE__, strQry);
						throw string("Error Occurred: Record Data is NULL");
					}
					else{
						//DO 'report_record' TABLE INSERT
						for(vector<shared_ptr<stRecord> >::iterator itr_r = spRecords->begin(); itr_r != spRecords->end(); itr_r++ ){
							memset(strQry, 0, SZ_DEFAULTBUFFER);
							shared_ptr<stRecord> spRecord = *itr_r;
							//[report_id][source_ip][count][identifiers_header_from][policy_evaluated_disposition][policy_evaluated_dkim][policy_evaluated_spf][auth_results_spf_domain][auth_results_spf_result][auth_results_dkim_domain][auth_results_dkim_result]
							snprintf(strQry, SZ_DEFAULTBUFFER, QueryInsert_record,
							                 pReprotMetaData->m_strReport_id.c_str(), 
							                 spRecord->getRow()->m_strSourceIP.c_str(), 
							                 spRecord->getRow()->m_uiCount,
							                 spRecord->getIdentifires()->m_strHeaderFrom.c_str(),
							                 spRecord->getRow()->getpPolicyEvaluated()->m_strDisposition.c_str(),
							                 spRecord->getRow()->getpPolicyEvaluated()->m_strDkim.c_str(),
							                 spRecord->getRow()->getpPolicyEvaluated()->m_strSpf.c_str(),
							                 spRecord->getAuthResults()->getAuthResultSpf()->m_strDomain.c_str(),
							                 spRecord->getAuthResults()->getAuthResultSpf()->m_strResult.c_str(),
							                 spRecord->getAuthResults()->getAuthResultDkim()->m_strDomain.c_str(),
							                 spRecord->getAuthResults()->getAuthResultDkim()->m_strResult.c_str());
		
							bRet = m_DmarcMysql.QueryDB(strQry);
							
							if(bRet == false){ //INSERT fail
								//DO 'report_record' TABLE UPDATE
								memset(strQry, 0, SZ_DEFAULTBUFFER);
								//[count][identifiers_header_from][policy_evaluated_disposition][policy_evaluated_dkim][policy_evaluated_spf][auth_results_spf_domain][auth_results_spf_result][auth_results_dkim_domain][auth_results_dkim_result]
								snprintf(strQry, SZ_DEFAULTBUFFER, QueryUpdate_record,
								                 spRecord->getRow()->m_uiCount,
								                 spRecord->getIdentifires()->m_strHeaderFrom.c_str(),
								                 spRecord->getRow()->getpPolicyEvaluated()->m_strDisposition.c_str(),
								                 spRecord->getRow()->getpPolicyEvaluated()->m_strDkim.c_str(),
								                 spRecord->getRow()->getpPolicyEvaluated()->m_strSpf.c_str(),
								                 spRecord->getAuthResults()->getAuthResultSpf()->m_strDomain.c_str(),
								                 spRecord->getAuthResults()->getAuthResultSpf()->m_strResult.c_str(),
								                 spRecord->getAuthResults()->getAuthResultDkim()->m_strDomain.c_str(),
								                 spRecord->getAuthResults()->getAuthResultDkim()->m_strResult.c_str(),
								                 pReprotMetaData->m_strReport_id.c_str(), 
								                 spRecord->getRow()->m_strSourceIP.c_str());
			
								bRet = m_DmarcMysql.QueryDB(strQry);
								if(bRet == false){
									gLog->Write("[%s][%s][%d][ERROR] INSERT INTO report_record Failed[Query:%s]", __FILE__, __FUNCTION__, __LINE__, strQry);
									continue;
								}
							}
						}
					}
				}
				else{ //'report_info' INSERT or UPDATE Failed
					//End DB INSERT/UPDATE Process
				}
			}
			else{
				gLog->Write("[%s][%s][%d][ERROR] Error Occurred: FeedbackRecord is Not Exist!", __FILE__, __FUNCTION__, __LINE__);
				throw string("Error Occurred: FeedbackRecord is Not Exist!");
			}
			
			Commit_Transaction();
		}
		catch(string errStr){
			gLog->Write(errStr.c_str());
			Rollback_Transaction();
			return false;
		}
		catch(...){
			gLog->Write("Unknown Error Occurred!");
			Rollback_Transaction();
			return false;
		}
	}
	
	return bRet;
}

