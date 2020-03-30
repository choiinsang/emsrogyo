#include "CDMARCXmlParserConfig.h"


CDMARCXmlParserConfig::CDMARCXmlParserConfig()
{
	//[DBINFO]
	memset(m_chDBName,   '\0', SZ_NAME+1);
	memset(m_chDBUser,   '\0', SZ_NAME+1);
	memset(m_chDBPasswd, '\0', SZ_PASSWD+1);
	memset(m_chDBIP,     '\0', SZ_IP4ADDRESS+1);
	m_iDBPort = 0;
	//[FILE_PATH]
	memset(m_chFileInputPath,    '\0', (SZ_NAME+1));
	memset(m_chFileInputTmpPath, '\0', (SZ_NAME+1));
	memset(m_chFileOutputPath,   '\0', (SZ_NAME+1));
	memset(m_chFileErrorPath,    '\0', (SZ_NAME+1));
	//[LOG]
	memset(m_chLogPath,     '\0', (SZ_NAME+1));
	memset(m_chLogFileName, '\0', (SZ_NAME+1));

}

CDMARCXmlParserConfig::~CDMARCXmlParserConfig()
{
}


CDMARCXmlParserConfig *CDMARCXmlParserConfig::m_pDMARCXmlParserConfig = NULL;

//-----------------------------------
// Get Config Singleton Pointer
CDMARCXmlParserConfig * CDMARCXmlParserConfig::getInstance()
{
	if ( m_pDMARCXmlParserConfig == NULL )
	{
		m_pDMARCXmlParserConfig = new CDMARCXmlParserConfig();
	}
	return m_pDMARCXmlParserConfig;
}

//-----------------------------------
//return error code to string
string CDMARCXmlParserConfig::errCodeToStr(errRESULT_CODE errCode)
{
	string retString = "Unknown Error Code";
	
	switch(errCode){
		case SUCCESS:               // 0
			retString="Success"; break;
		case FILE_NOT_EXIST:        // 1
			retString="File is not exist"; break;
		case FILE_INIT_FAILED:      // 2
			retString="File load and initialize failed"; break;
		case DB_INFO_INIT_FAILED:   // 3
			retString="DB information initialize failed"; break;
		case FILE_PATH_INIT_FAILED: // 4
			retString="File path information initialize failed"; break;
		case LOG_INIT_FAILED:       //5
			retString="Log information initialize failed"; break;
	}
	
	return retString;
}

//-----------------------------------
// Init Config with INI FileName('/etc/Ems.ini)
bool CDMARCXmlParserConfig::InitConfig(const char * l_pFileName)
{
	CIniFile INIFile;
	bool     retval = false;
	
	try{

		if(l_pFileName == NULL){
			throw FILE_NOT_EXIST;
		}
		else{
			if( (retval = INIFile.InitINIFile(l_pFileName)) ==  false){
				//init CIniFile failed
				throw FILE_INIT_FAILED;
			}
			else{
	
				//[DBINFO]
				char DBINFO[][30] = {
					  "DB_INFO"     //[0]
					, "DB_NAME"     //[1]
					, "DB_IP"       //[2]
					, "DB_USER"     //[3]
					, "DB_PASSWORD" //[4]  
					, "DB_PORT"     //[5]
				};
	
				//[FILE_PATH]
				char FILE_PATH[][50] = {
					  "FILE_PATH"           //[0]
					, "FILE_INPUT_PATH"     //[1]
					, "FILE_INPUT_TMP_PATH" //[2]
					, "FILE_OUTPUT_PATH"    //[3]
					, "FILE_ERROR_PATH"     //[4]
				};
				
				//[LOG]
				char LOG[][50]={
					  "LOG"            //[0]
					, "LOG_PATH"       //[1]
					, "LOG_FILENAME"   //[2]
				};
	
				//Default string
				char NULLSTRING  [4]  = {""};
	
				//[DBINFO]
				memset(m_chDBName,   '\0', SZ_NAME+1);
				memset(m_chDBUser,   '\0', SZ_NAME+1);
				memset(m_chDBPasswd, '\0', SZ_PASSWD+1);
				memset(m_chDBIP,     '\0', SZ_IP4ADDRESS+1);
				m_iDBPort = 0;
				//[FILE_PATH]
				memset(m_chFileInputPath,    '\0', (SZ_NAME+1));
				memset(m_chFileInputTmpPath, '\0', (SZ_NAME+1));
				memset(m_chFileOutputPath,   '\0', (SZ_NAME+1));
				memset(m_chFileErrorPath,    '\0', (SZ_NAME+1));
				//[LOG]
				memset(m_chLogPath,     '\0', (SZ_NAME+1));
				memset(m_chLogFileName, '\0', (SZ_NAME+1));

				//[DBINFO]
				do{
					retval = false;
					INIFile.GetProfileString(DBINFO[0], DBINFO[1], NULLSTRING, m_chDBName);
					if(strcmp(m_chDBName, NULLSTRING)==0) break;
				
					INIFile.GetProfileString(DBINFO[0], DBINFO[2], NULLSTRING,m_chDBIP);
					if(strcmp(m_chDBIP, NULLSTRING)==0) break;
				
					INIFile.GetProfileString(DBINFO[0], DBINFO[3], NULLSTRING, m_chDBUser);
					if(strcmp(m_chDBUser, NULLSTRING)==0) break;
				
					INIFile.GetProfileString(DBINFO[0], DBINFO[4], NULLSTRING, m_chDBPasswd);
					if(strcmp(m_chDBPasswd, NULLSTRING)==0) break;
				
					m_iDBPort  = INIFile.GetProfileInt(DBINFO[0], DBINFO[5], INIT_DEFAULT_DBPORT);
					if(m_iDBPort < 0) break;

					retval = true;
				}while(0);
				
				if(!retval){
					throw DB_INFO_INIT_FAILED;
				}

				//[FILE_PATH]
				do{
					retval = false;
					INIFile.GetProfileString(FILE_PATH[0], FILE_PATH[1], NULLSTRING, m_chFileInputPath);
					if(strcmp(m_chFileInputPath, NULLSTRING)==0) break;
					
					INIFile.GetProfileString(FILE_PATH[0], FILE_PATH[2], NULLSTRING, m_chFileInputTmpPath);
					if(strcmp(m_chFileInputTmpPath, NULLSTRING)==0) break;

					INIFile.GetProfileString(FILE_PATH[0], FILE_PATH[3], NULLSTRING, m_chFileOutputPath);
					if(strcmp(m_chFileOutputPath, NULLSTRING)==0) break;

					INIFile.GetProfileString(FILE_PATH[0], FILE_PATH[4], NULLSTRING, m_chFileErrorPath);
					if(strcmp(m_chFileErrorPath, NULLSTRING)==0) break;

					retval = true;
				}while(0);
				
				if(!retval){
					throw FILE_PATH_INIT_FAILED;
				}

				//[LOG]
				do{
					retval = false;
					INIFile.GetProfileString(LOG[0], LOG[1], NULLSTRING, m_chLogPath);
					if(strcmp(m_chLogPath, NULLSTRING)==0) break;
					
					INIFile.GetProfileString(LOG[0], LOG[2], NULLSTRING, m_chLogFileName);
					if(strcmp(m_chLogFileName, NULLSTRING)==0) break;

					retval = true;
				}while(0);
				
				if(!retval){
					throw LOG_INIT_FAILED;
				}



	
				INIFile.EndINIFile();
				return true;
			}
		}
	}
	catch(errRESULT_CODE ret_err_code){
		printf("Init Config Error Occurred![File:%s][Code:%d][Msg:%s]\n", l_pFileName, ret_err_code, errCodeToStr(ret_err_code).c_str());
		return false;		
	}
	catch(...){
		printf("Unknown Error Occurred!\n");
		return false;
	}
	
	
	return retval;
}

const char * CDMARCXmlParserConfig::checkString(char * pStr)
{
	if(strlen(pStr) == 0)
		return (const char*)NULL;
	else 
		return (const char*)pStr;	
}


//[DBINFO]
//======================================
const char *CDMARCXmlParserConfig::getDBName()
{
	return checkString(m_chDBName);
}

const char *CDMARCXmlParserConfig::getDBUser()
{
	return checkString(m_chDBUser);
}

const char *CDMARCXmlParserConfig::getDBPasswd()
{
	return checkString(m_chDBPasswd);
}

const char *CDMARCXmlParserConfig::getDBIP()
{
	return checkString(m_chDBIP);
}

//[FILE_PATH]
const char *CDMARCXmlParserConfig::getFileInputPath()
{
	return checkString(m_chFileInputPath);
}

const char *CDMARCXmlParserConfig::getFileInputTmpPath()
{
	return checkString(m_chFileInputTmpPath);
}

const char *CDMARCXmlParserConfig::getFileOutPutPath()
{
	return checkString(m_chFileOutputPath);
}

const char *CDMARCXmlParserConfig::getFileErrorPath()
{
	return checkString(m_chFileErrorPath);
}


//[LOG]
const char *CDMARCXmlParserConfig::getLogPath()
{
	return checkString(m_chLogPath);
}

const char *CDMARCXmlParserConfig::getLogFileName()
{
	return checkString(m_chLogFileName);
}


//-----------------------------------
// Print Config Values
void CDMARCXmlParserConfig::printFileName(){
	printf("Print Initialize Information: .....\n");
}

