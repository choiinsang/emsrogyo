#ifndef __CDMARC_LOG_HEADER__
#define __CDMARC_LOG_HEADER__

#include "../NetTemplate/LockObject.h"
#include "DMARCDefineHeader.h"

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <cstdarg>

using namespace std;

class CDMARCLog : public LockObject
{
// Construction
public:
	virtual ~CDMARCLog();
	       
	bool   InitLog     (const char *l_pszPath, const char *pLogName = "CMARCxmlParser");
	void   Write       (const char *l_pszFMT, ... );
	void   WriteLOG    (const char *l_pszData);
	
	int    remove_dir  (char *path);
	void   CloseInfo   (char *keyName);
	
	bool   isExistDir   (const char *l_pszPath);

	static CDMARCLog *getInstance();


private:
	       CDMARCLog();

// Implementation
private:
	static CDMARCLog *m_pLog;
	FILE             *m_FPLog;
	int               m_nLastDay;
	string            m_LogName;
	char              m_szPath[MAX_PATH + 1];

};

#define theDMARCLogger()  CDMARCLog::getInstance()


#endif //__CDMARC_LOG_HEADER__




