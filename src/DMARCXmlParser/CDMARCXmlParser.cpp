#include "CDMARCXmlParser.h"
#include "CDMARCLog.h"
#include "CDMARCFunctions.h"
#include "CDMARCXmlParserConfig.h"
#include <zip.h>
#include <boost/foreach.hpp>
//.gz Extraction Headers
#include <zlib.h>


using namespace std;

extern CDMARCLog             *gLog;
extern CDMARCXmlParserConfig *gTheConfig;


using namespace std;
//***********************************************
// XML Parser 생성자
//***********************************************
CDMARCXmlParser::CDMARCXmlParser()
{
	m_bXmlLoadState = false;
	m_spFeedbackRecord = shared_ptr<stFeedbackRecord>(new stFeedbackRecord) ;
	//printf("m_spFeedbackRecord Created.....\n");
}


//***********************************************
// XML Parser 소멸자
//***********************************************
CDMARCXmlParser::~CDMARCXmlParser()
{
}


//***********************************************
// XML Load
//***********************************************
bool CDMARCXmlParser::LoadXml(std::string xmlFilePath)
{
	const char *pFileName = strrchr(xmlFilePath.c_str(), '/');
	if( pFileName != NULL ){
		pFileName++;
		m_strXMLFileName = pFileName;
	}
	else{
		return false;
	}
	
	return ParseXml(xmlFilePath);
}

//***********************************************
// 파일 압축을 풀고 새로운 파일을 임시 폴더에 생성
// orgFilePath: 'xxx.zip' 파일 경로
// retPath    : 파일 압축을 해제하여 생성된 'xxx.xml' 파일 경로
//***********************************************
int CDMARCXmlParser::fileExtract(std::string orgFilePath, std::string &retPath)
{
	//Open the ZIP archive
	int err        = 0;
	int file_total = -1;
	int sz_buf     = SZ_DEFAULTBUFFER*2;
	
	char buffer[SZ_DEFAULTBUFFER*2] = {0,};
	
	std::string errStr;
	int iExt = checkFileExtention (orgFilePath.c_str(), errStr);

	switch(iExt){ //확장자 체크
		case EXT_XML:{ //xml 파일인 경우
			retPath = orgFilePath;
			return iExt;
			break;
		}
		case EXT_ZIP:
		case EXT_GZ:{ //.zip or .gz 파일인 경우
			break;
		}
		default:{ //xml 혹은 zip 파일이 아닌 경우
			return iExt;
		}
	}
	
	if(iExt == EXT_ZIP){
		zip *pzFile = zip_open(orgFilePath.c_str(), 0, &err);

		if(pzFile == NULL){
			return UNKNOWN_ERROR;
		}
		else{
		//-------------------------------
			struct zip_file *file_in_zip = NULL;
			const char      *fname       = NULL;
	
			file_total = zip_get_num_files(pzFile);
			
			for(int i=0; i<file_total; i++){
				
				file_in_zip = zip_fopen_index(pzFile, i, 0);
				
				if(file_in_zip == NULL){
					continue;
				}
				else{
					fname = zip_get_name (pzFile, i, 0);
				
					//일단 압축을 풀어 저장한다.
					if(fname == NULL){
						zip_fclose(file_in_zip);
						continue;
					}
					else{
						//압축을 풀어 저장한다.
						retPath =  gTheConfig->getFileInputTmpPath();
						retPath += "/";
						retPath += fname;
						
						FILE *pf = fopen(retPath.c_str(), "w+");
						if(pf != NULL){
							if(file_in_zip){
					    	int  rdata = 0;
					    	uint ipos  = 0;

								while( (rdata = zip_fread(file_in_zip, buffer, sz_buf)) > 0 ){
									fwrite(buffer, 1, rdata, pf);
									ipos += rdata;
									memset(buffer, 0, sz_buf);
								}
							}
							fclose(pf);
						}
						zip_fclose(file_in_zip);
						//zip 디스크립터를 닫음, 파일을 저장됨
					}
					iExt = checkFileExtention (orgFilePath.c_str(), errStr);
				}
			}
		}
		zip_close(pzFile);
	}
	else if(iExt == EXT_GZ){		// orgFilePath Extract .gz file
		
		FILE   *pf     = NULL;
		gzFile  gzfile = gzopen (orgFilePath.c_str(), "r");
		
		if (!gzfile) { //.gz 파일 열기 실패
			gLog->Write("gzopen of '%s' failed: %s", orgFilePath.c_str(), strerror (errno));
			return FILE_OPEN_ERROR;
		}
		else{
			string newFilePath("");

			char *tmpbuf = strdup(orgFilePath.c_str());
			if(tmpbuf!=NULL){
				char *gzPos  = strrchr(tmpbuf, '.');
				
				if(gzPos != NULL){
					tmpbuf[gzPos - tmpbuf] = '\0';
					newFilePath = tmpbuf;
				}
				else{ //gzPos is NULL: '.gz'이 없다
					gLog->Write("Error Occurred: '.gz' is Not Exist[filename:%s]", tmpbuf);
					free(tmpbuf);
					return FILE_OPEN_ERROR;
				}
				free(tmpbuf);
			}
			else{
				gLog->Write("Error:[%s]", strerror(errno));
				return MEM_ERROR;
			}
			
			if(newFilePath.size() == 0){
				gLog->Write("newFilePath is NULL");
				iExt = UNKNOWN_ERROR;
				return iExt;
			}
			
			if( (pf = fopen(newFilePath.c_str(), "w+")) == NULL){ // File Open Failed!
				gLog->Write("Error Occurred: %s", strerror(errno) );
				return FILE_OPEN_ERROR;
			}
			else{ // File Open OK!
				//-----------------------------------
				//.gz File Extract and Save New File
				//-----------------------------------
				int  err         = -1;
				int  bytes_read  = 0;
				int  bytes_write = 0;
				int  LENGTH_M1   = LENGTH - 1;
				bool bIsEND      = false;
				
				unsigned char tmpBuf[LENGTH] = {0,};
				
				while(1){
					err = -1;
					bytes_read  = 0;
					bytes_write = 0;
					memset(tmpBuf, 0, LENGTH);
					
					bytes_read = gzread (gzfile, tmpBuf, LENGTH_M1);
					buffer[bytes_read] = '\0';
					gLog->Write("%s", buffer);
					fwrite(tmpBuf, 1, bytes_read, pf);
					
					if (bytes_read < LENGTH_M1) {
						if (gzeof (gzfile)) {
							bIsEND=true;
							break;
						}
						else {
							const char * error_string;
							error_string = gzerror (gzfile, & err);
							if (err) {
								fprintf (stderr, "Error: %s.\n", error_string);
								exit (EXIT_FAILURE);
							}
						}
					}
				}
				fclose(pf);
				gzclose (gzfile);
			}
			retPath = newFilePath;
			iExt    = checkFileExtention (retPath.c_str(), errStr);

			return iExt;
		}
	}
	else{
		gLog->Write("Check Extention...:%d", iExt);
		return iExt;
	}

	return iExt;
}


//***********************************************
// 파일을 Parsing 하여 xml 파일을 생성
// orgPath: 'xxx.zip' 원본 파일 경로
// retPath: 'xxx.xml' 파일
//***********************************************
bool CDMARCXmlParser::fileParse(std::string orgPath, std::string &retPath)
{
	//create 'retPath': orgPath를 extract하여 retPath 파일을 생성
	int retState = fileExtract(orgPath, retPath);
	
	if(retState == EXT_XML){
		return true;
	}
	else if( (retState == EXT_ZIP) || (retState == EXT_GZ)){
		string tmpOrgPath = retPath;
		bool   bRet       = fileParse (tmpOrgPath, retPath);
		
		if(bRet == true){
			if(tmpOrgPath != retPath){
				unlink( tmpOrgPath.c_str() );
			}
		}
		else{
			unlink( tmpOrgPath.c_str() );
		}
		return bRet;
	}
	else{
		return false;
	}
}

//***********************************************
// XML Parse (xxx.zip)
//***********************************************
bool CDMARCXmlParser::ParseZip(std::string zipFilePath)
{
	string lastFilePath;
	bool   bRet = fileParse(zipFilePath, lastFilePath);
	gLog->Write("[Input File:%s][Result File:%s][%s]", zipFilePath.c_str(), lastFilePath.c_str(), (bRet?"TRUE":"FALSE") );

	if( (bRet!= false) && (lastFilePath.empty() != true)){
		bRet = ParseXml(lastFilePath);
		unlink( lastFilePath.c_str());
	}
	
	return bRet;
}

//***********************************************
// XML Parse (xxx.xml)
//***********************************************
bool CDMARCXmlParser::ParseXml(std::string xmlFilePath)
{
	//======================================================
		//Config 파일에서 설정 읽음: 
		//폴더 경로:Input/Output(Process/Done/Error)
		//디비 정보:DB Name/USER/PASSWORD
		
	  std::stringstream ss;
	  FILE             *fp;
	  char             *pFileBuf=NULL;
	  size_t            tmpsz   =0;
	  struct stat       statBuf;
	  
	  gLog->Write("[xmlFilePath: %s]", xmlFilePath.c_str());

	  if( !stat(xmlFilePath.c_str(), &statBuf) ){
	  	tmpsz    = statBuf.st_size;
	  	pFileBuf = (char*)malloc(tmpsz+1);
	  	memset(pFileBuf, 0, tmpsz+1);
	  	
	  	//파일 저장
	  	int ipos=0;
	  	int iret=0;
	  	
	  	if( (fp=fopen(xmlFilePath.c_str(), "r")) != NULL ){
	  		//-- 파일로부터 데이터를 저장
	  		while(!feof(fp)){
	  			iret = fread(pFileBuf+ipos, 1, SZ_TEMP, fp);
	  			ipos +=iret;
	  		}
	  		
	  		//-- 데이터 파싱
	  		boost::property_tree::ptree pt;
	  		ss << pFileBuf;
	  		boost::property_tree::xml_parser::read_xml(ss, pt);

				//shared_ptr<stFeedbackRecord> m_spFeedbackRecord;  getFeedbackRecord()
				
				//DMARC_FEEDBACK - "feedback"
				std::string s(pt.get<std::string>(DMARC_FEEDBACK));

				BOOST_FOREACH( boost::property_tree::ptree::value_type const& v, pt.get_child(DMARC_FEEDBACK) ) {

					to_lowercase(const_cast<char*>(v.first.c_str()));
					
					//DMARC_REPORT_METADATA - "report_metadata"
					if( v.first == DMARC_REPORT_METADATA ) {

						getFeedbackRecord()->getReportMetaData()->m_strOrgName = v.second.get<std::string>(DMARC_REPORT_METADATA_ORG_NAME);
						//gLog->Write("[%s][%d][%s]----[%s][%s]", __FUNCTION__, __LINE__, DMARC_REPORT_METADATA, DMARC_REPORT_METADATA_ORG_NAME, getFeedbackRecord()->getReportMetaData()->m_strOrgName.c_str());
						getFeedbackRecord()->getReportMetaData()->m_strEmail   = v.second.get<std::string>(DMARC_REPORT_METADATA_EMAIL);
						//gLog->Write("[%s][%d][%s]----[%s][%s]", __FUNCTION__, __LINE__, DMARC_REPORT_METADATA, DMARC_REPORT_METADATA_EMAIL, getFeedbackRecord()->getReportMetaData()->m_strEmail.c_str());
						
						//DMARC_REPORT_METADATA_EXTRA_CONTACT_INFO[] - "extra_contact_info"
						try{
							getFeedbackRecord()->getReportMetaData()->m_strExtraContactInfo = v.second.get<std::string>(DMARC_REPORT_METADATA_EXTRA_CONTACT_INFO);
							//gLog->Write("[%s][%d][%s]----[%s][%s]", __FUNCTION__, __LINE__, DMARC_REPORT_METADATA, DMARC_REPORT_METADATA_EXTRA_CONTACT_INFO, getFeedbackRecord()->getReportMetaData()->m_strExtraContactInfo.c_str());
						}
						catch(...){
						}
						
						try{
							getFeedbackRecord()->getReportMetaData()->m_strReport_id = v.second.get<std::string>(DMARC_REPORT_METADATA_REPORT_ID);
							//gLog->Write("[%s][%d][%s]----[%s][%s]", __FUNCTION__, __LINE__, DMARC_REPORT_METADATA, DMARC_REPORT_METADATA_REPORT_ID, getFeedbackRecord()->getReportMetaData()->m_strReport_id.c_str());
						}
						catch(...){
							gLog->Write("[%s][%d] Report_ID Is Not Exist", __FUNCTION__, __LINE__);
							return false;
						}
							
						try{
							boost::property_tree::ptree pt1 = v.second;
							BOOST_FOREACH( boost::property_tree::ptree::value_type const& vf, pt1.get_child(DMARC_REPORT_METADATA_DATE_RANGE) ){
								to_lowercase(const_cast<char*>(vf.first.c_str()));
								if(vf.first == DMARC_REPORT_METADATA_DATE_RANGE_BEGIN){
									getFeedbackRecord()->getReportMetaData()->m_strBegin = vf.second.data();
									//gLog->Write("[%s][%d][%s]----[%s][%s]", __FUNCTION__, __LINE__, DMARC_REPORT_METADATA_DATE_RANGE, DMARC_REPORT_METADATA_DATE_RANGE_BEGIN, getFeedbackRecord()->getReportMetaData()->m_strBegin.c_str());
								}
								else if(vf.first == DMARC_REPORT_METADATA_DATE_RANGE_END){
									getFeedbackRecord()->getReportMetaData()->m_strEnd = vf.second.data();
									//gLog->Write("[%s][%d][%s]----[%s][%s]", __FUNCTION__, __LINE__, DMARC_REPORT_METADATA_DATE_RANGE, DMARC_REPORT_METADATA_DATE_RANGE_END, getFeedbackRecord()->getReportMetaData()->m_strEnd.c_str());
								}
							}
						}
						catch(...){
						}
					}
					
					//DMARC_POLICY_PUBLISHED - "policy_published"
					else if ( v.first == DMARC_POLICY_PUBLISHED ) {
						
						try{//DMARC_POLICY_PUBLISHED_DOMAIN - "domain"
						getFeedbackRecord()->getPolicyPublished()->m_strDomain = v.second.get<std::string>(DMARC_POLICY_PUBLISHED_DOMAIN);
							//gLog->Write("[%s][%d][%s]----[%s][%s]", __FUNCTION__, __LINE__, DMARC_POLICY_PUBLISHED, DMARC_POLICY_PUBLISHED_DOMAIN, getFeedbackRecord()->getPolicyPublished()->m_strDomain.c_str());
						}catch(...){}
						try{//DMARC_POLICY_PUBLISHED_ADKIM - "adkim"
						getFeedbackRecord()->getPolicyPublished()->m_strAdkim  = v.second.get<std::string>(DMARC_POLICY_PUBLISHED_ADKIM);
							//gLog->Write("[%s][%d][%s]----[%s][%s]", __FUNCTION__, __LINE__, DMARC_POLICY_PUBLISHED, DMARC_POLICY_PUBLISHED_ADKIM, getFeedbackRecord()->getPolicyPublished()->m_strAdkim.c_str());
						}catch(...){}
						try{//DMARC_POLICY_PUBLISHED_ASPF - "aspf"
						getFeedbackRecord()->getPolicyPublished()->m_strAspf   = v.second.get<std::string>(DMARC_POLICY_PUBLISHED_ASPF);
							//gLog->Write("[%s][%d][%s]----[%s][%s]", __FUNCTION__, __LINE__, DMARC_POLICY_PUBLISHED, DMARC_POLICY_PUBLISHED_ASPF, getFeedbackRecord()->getPolicyPublished()->m_strAspf.c_str());
						}catch(...){}
						try{//DMARC_POLICY_PUBLISHED_P - "p"
						getFeedbackRecord()->getPolicyPublished()->m_strP      = v.second.get<std::string>(DMARC_POLICY_PUBLISHED_P);
							//gLog->Write("[%s][%d][%s]----[%s][%s]", __FUNCTION__, __LINE__, DMARC_POLICY_PUBLISHED, DMARC_POLICY_PUBLISHED_P, getFeedbackRecord()->getPolicyPublished()->m_strP.c_str());
						}catch(...){}
						try{//DMARC_POLICY_PUBLISHED_SP - "sp"
						getFeedbackRecord()->getPolicyPublished()->m_strSP     = v.second.get<std::string>(DMARC_POLICY_PUBLISHED_SP);
							//gLog->Write("[%s][%d][%s]----[%s][%s]", __FUNCTION__, __LINE__, DMARC_POLICY_PUBLISHED, DMARC_POLICY_PUBLISHED_SP, getFeedbackRecord()->getPolicyPublished()->m_strSP.c_str());
						}catch(...){}
						try{//DMARC_POLICY_PUBLISHED_PCT - "pct"
						getFeedbackRecord()->getPolicyPublished()->m_strPct    = v.second.get<std::string>(DMARC_POLICY_PUBLISHED_PCT);
						//gLog->Write("[%s][%d][%s]----[%s][%s]", __FUNCTION__, __LINE__, DMARC_POLICY_PUBLISHED, DMARC_POLICY_PUBLISHED_PCT, getFeedbackRecord()->getPolicyPublished()->m_strPct.c_str());
						}catch(...){}
					}//-- [END] DMARC_POLICY_PUBLISHED - "policy_published"					
					
					//DMARC_RECORD - "record"
					else if( v.first == DMARC_RECORD ) {
						//getRecords() <== 요기에 push_back 할꺼다.	
						bool bRet = true;
						shared_ptr<stRecord> spstRecord =  shared_ptr<stRecord>(new stRecord);

						{//DMARC_RECORD_ROW -"row"
							boost::property_tree::ptree pt1 = v.second;
							BOOST_FOREACH( boost::property_tree::ptree::value_type const& vf, pt1.get_child(DMARC_RECORD_ROW) ){
							try{
							
									to_lowercase(const_cast<char*>(vf.first.c_str()));
									
									//DMARC_RECORD_ROW_SOURCE_IP - "source_ip"
									if( vf.first == DMARC_RECORD_ROW_SOURCE_IP ){
										spstRecord->getRow()->m_strSourceIP = vf.second.data();
										//gLog->Write("[%s][%d][%s]----[%s][%s]", __FUNCTION__, __LINE__, DMARC_RECORD_ROW, DMARC_RECORD_ROW_SOURCE_IP, spstRecord->getRow()->m_strSourceIP.c_str());
									}
									
									//DMARC_RECORD_ROW_COUNT - "count"
									else if( vf.first == DMARC_RECORD_ROW_COUNT ){
										string strCount = vf.second.data();
										spstRecord->getRow()->m_uiCount = atoi( strCount.c_str() );
										//gLog->Write("[%s][%d][%s]----[%s][%s]", __FUNCTION__, __LINE__, DMARC_RECORD_ROW, DMARC_RECORD_ROW_COUNT, strCount.c_str());
									}
									
									//DMARC_RECORD_ROW_POLICY_EVALUATED - "policy_evaluated"
									else if( vf.first == DMARC_RECORD_ROW_POLICY_EVALUATED ){
										//gLog->Write("[DMARC_RECORD_ROW_POLICY_EVALUATED = %s ] [vf.first = %s]", DMARC_RECORD_ROW_POLICY_EVALUATED, vf.first.c_str());
	
										boost::property_tree::ptree pt2 = vf.second;
										for(boost::property_tree::ptree::iterator itr_ve = pt2.begin(); itr_ve != pt2.end(); itr_ve++){
	
											try{
												to_lowercase(const_cast<char*>(itr_ve->first.c_str()));
												//DMARC_RECORD_ROW_P_DISPOSITION - "disposition"
												if( itr_ve->first == DMARC_RECORD_ROW_P_DISPOSITION ){
													spstRecord->getRow()->getpPolicyEvaluated()->m_strDisposition = itr_ve->second.data();
													//gLog->Write("[%s][%d][%s][%s]----[%s][%s]", __FUNCTION__, __LINE__, DMARC_RECORD_ROW, DMARC_RECORD_ROW_POLICY_EVALUATED, DMARC_RECORD_ROW_P_DISPOSITION, spstRecord->getRow()->getpPolicyEvaluated()->m_strDisposition.c_str());
												}
												//DMARC_RECORD_ROW_P_DKIM - "dkim"
												else if( itr_ve->first == DMARC_RECORD_ROW_P_DKIM ){
													spstRecord->getRow()->getpPolicyEvaluated()->m_strDkim = itr_ve->second.data();
													//gLog->Write("[%s][%d][%s][%s]----[%s][%s]", __FUNCTION__, __LINE__, DMARC_RECORD_ROW, DMARC_RECORD_ROW_POLICY_EVALUATED, DMARC_RECORD_ROW_P_DKIM, spstRecord->getRow()->getpPolicyEvaluated()->m_strDkim.c_str());
												}
												//DMARC_RECORD_ROW_P_SPF - "spf"
												else if( itr_ve->first == DMARC_RECORD_ROW_P_SPF ){
													spstRecord->getRow()->getpPolicyEvaluated()->m_strSpf = itr_ve->second.data();
													//gLog->Write("[%s][%d][%s][%s]----[%s][%s]", __FUNCTION__, __LINE__, DMARC_RECORD_ROW, DMARC_RECORD_ROW_POLICY_EVALUATED, DMARC_RECORD_ROW_P_SPF, spstRecord->getRow()->getpPolicyEvaluated()->m_strSpf.c_str());
												}
											}
											catch(...){
											}
										}
									}
								}
								catch(...){
									gLog->Write("[%s][%d]DMARC_RECORD_ROW", __FUNCTION__, __LINE__);
								}
							}
						}
						
						{ //DMARC_RECORD_IDENTIFIRES - "identifires"
							boost::property_tree::ptree pt1 = v.second;
							BOOST_FOREACH( boost::property_tree::ptree::value_type const& vf, pt1.get_child(DMARC_RECORD_IDENTIFIRES) ){

								try{
									to_lowercase(const_cast<char*>(vf.first.c_str()));
								
									//DMARC_RECORD_I_HEADER_FROM - "header_from"
									if( vf.first == DMARC_RECORD_I_HEADER_FROM ){
										spstRecord->getIdentifires()->m_strHeaderFrom = vf.second.data();
									}
								}
								catch(...){
									gLog->Write("[%s][%d]DMARC_RECORD_IDENTIFIRES", __FUNCTION__, __LINE__);
								}
							}
						}
						
						{ //DMARC_RECORD_AUTH_RESULTS - "auth_results"
							boost::property_tree::ptree pt1 = v.second;
							BOOST_FOREACH( boost::property_tree::ptree::value_type const& vf, pt1.get_child( DMARC_RECORD_AUTH_RESULTS ) ){
									
								try{
									to_lowercase(const_cast<char*>(vf.first.c_str()));
									
									//DMARC_RECORD_AUTH_RESULTS_SPF - "spf"
									if( vf.first == DMARC_RECORD_AUTH_RESULTS_SPF ){
										spstRecord->getAuthResults()->getAuthResultSpf()->setExist(true);
										spstRecord->getAuthResults()->getAuthResultSpf()->m_strDomain = vf.second.get<std::string>(DMARC_RECORD_AUTH_RESULTS_SPF_DOMAIN);
										spstRecord->getAuthResults()->getAuthResultSpf()->m_strResult = vf.second.get<std::string>(DMARC_RECORD_AUTH_RESULTS_SPF_RESULT);
									}
									
									//DMARC_RECORD_AUTH_RESULTS_DKIM - "dkim"
									else if( vf.first == DMARC_RECORD_AUTH_RESULTS_DKIM ){
										spstRecord->getAuthResults()->getAuthResultDkim()->setExist(true);
										spstRecord->getAuthResults()->getAuthResultDkim()->m_strDomain = vf.second.get<std::string>(DMARC_RECORD_AUTH_RESULTS_DKIM_DOMAIN); 
										spstRecord->getAuthResults()->getAuthResultDkim()->m_strResult = vf.second.get<std::string>(DMARC_RECORD_AUTH_RESULTS_DKIM_RESULT); 
									}
								}
								catch(...){
									gLog->Write("[%s][%d]DMARC_RECORD_AUTH_RESULTS", __FUNCTION__, __LINE__);
								}
							}
						}
						
						if(bRet == true){
							shared_ptr<stFeedbackRecord> pstFeedbackRecords = getFeedbackRecord();
							shared_ptr<vector<shared_ptr<stRecord> > > pstRecords = pstFeedbackRecords->getRecords();

							getFeedbackRecord()->getRecords()->push_back(spstRecord);
						}
					}
				}
			}
			else{
				return false;
			}
	  	
	  	if(fp != NULL){
	  		fclose(fp);
	  	}
	  }
	  else{//-- 실패(-1)
	  	//-- 파싱 실패
	  }

	  if(pFileBuf != NULL){
	  	free(pFileBuf);
	  	pFileBuf = NULL;
	  }
	//======================================================

	return true;
}



