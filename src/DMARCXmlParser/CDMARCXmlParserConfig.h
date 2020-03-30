#ifndef __CDMARCXMLPARSER_CONFIG_HEADER__
#define __CDMARCXMLPARSER_CONFIG_HEADER__


#include <iostream>
#include <stdio.h>
#include <vector>
#include <string.h>
#include <sys/time.h>

#include "../Common/INIFile.h"
#include "../NetTemplate/LockObject.h"
#include "DMARCDefineHeader.h"

using namespace std;


class CDMARCXmlParserConfig : public LockObject
{
	// Construction
public:
	~CDMARCXmlParserConfig();


private:
	
	CDMARCXmlParserConfig();
	// Overrides

public:
	// Implementation
	bool        InitConfig         (const char * l_pFileName);
	const char *checkString        (char * pStr);

	//[DBINFO]                      
	const char *getDBName          ();
	const char *getDBUser          ();
	const char *getDBPasswd        ();
	const char *getDBIP            ();
	int         getDBPort          () { return m_iDBPort; };

	//[FILE_PATH]
	const char *getFileInputPath   ();
	const char *getFileInputTmpPath();
	const char *getFileOutPutPath  ();
	const char *getFileErrorPath   ();

	//[LOG]
	const char *getLogPath         ();
	const char *getLogFileName     ();

	//Print Info
	void         printFileName     ();
	
	static CDMARCXmlParserConfig * getInstance();

	//error code
	enum errRESULT_CODE{
		SUCCESS = 0,
		FILE_NOT_EXIST,
		FILE_INIT_FAILED,
		DB_INFO_INIT_FAILED,
		FILE_PATH_INIT_FAILED,
		LOG_INIT_FAILED
	};
	//return error code to string
	string errCodeToStr(errRESULT_CODE errCode);

private:
	static CDMARCXmlParserConfig *m_pDMARCXmlParserConfig;

	//[DBINFO]                
	char   m_chDBName          [SZ_NAME+1];
	char   m_chDBUser          [SZ_NAME+1];
	char   m_chDBPasswd        [SZ_PASSWD+1];
	char   m_chDBIP            [SZ_IP4ADDRESS+1];
	int    m_iDBPort;         
	//[FILE_PATH]
	char   m_chFileInputPath   [SZ_NAME+1];
	char   m_chFileInputTmpPath[SZ_NAME+1];
	char   m_chFileOutputPath  [SZ_NAME+1];
	char   m_chFileErrorPath   [SZ_NAME+1];
	//[LOG]
	char   m_chLogPath         [SZ_NAME+1];
	char   m_chLogFileName     [SZ_NAME+1];

};

#define theDMARCXmlParserConfig() CDMARCXmlParserConfig::getInstance()

#endif // __CDMARCXMLPARSER_CONFIG_HEADER__
