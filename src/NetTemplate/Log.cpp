#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <dirent.h>
#include "Log.h"

#include "sth_misc.h"

CLog *CLog::m_pLog = NULL;

CLog::CLog():m_FPLog(NULL), m_nLastDay(-1)
{
	memset(m_szPath, 0x00, MAX_PATH + 1);

	mCheckerCh = ' ';
}

CLog::~CLog()
{
}

void CLog::Set_CheckerCh(char _ch)
{
	mCheckerCh = _ch;
}

CLog &CLog::GetInstance(char _ch)
{
	if ( !m_pLog )
	{
#ifdef VERSION_TEST
		RemoveDirFile("LOG");
#endif
		//rmdir("./LOG /s/q");		

		m_pLog = new CLog();
	}

	return *m_pLog;
}

bool CLog::InitLog(char *l_pszPath)
{
	CLog* pLog = &GetInstance();
	DIR *pDir;
	memcpy(pLog->m_szPath, l_pszPath, MAX_PATH);

	if ((pDir = opendir(pLog->m_szPath)) == NULL)
	{
		if (mkdir(pLog->m_szPath, 00777) < 0)
			return false;
	}
	closedir(pDir);
	return true;
}

void CLog::Write(const char *l_pszFMT, ... )
{
	CLog* pLog = &GetInstance();
	if (pLog->m_szPath[0] == 0x00)
	{
#ifdef VERSION_TEST
		if (false == InitLog("./LOG/"))
		{
			//            printf("Error InitLog!\n");
			return;
		}
#else //VERSION_SERVICE
		if (false == InitLog("/etc/LOG/252/"))
		{
			//            printf("Error InitLog!\n");
			return;
		}
#endif
	}

	char szFile[MAX_LOG];

	time_t Time;
	struct tm *CurrTime;
	time(&Time);
	CurrTime = localtime(&Time);

	if (CurrTime->tm_mday != pLog->m_nLastDay)
	{
		pLog->Lock();
		if (pLog->m_FPLog != NULL)
		{
			fclose(pLog->m_FPLog);
			pLog->m_FPLog = NULL;
		}
		sprintf(szFile, "%s%4d%02d%02d.log", pLog->m_szPath, CurrTime->tm_year + 1900,
			CurrTime->tm_mon + 1, CurrTime->tm_mday);
		//printf("File:%s\n", szFile); //

		pLog->m_FPLog = fopen(szFile, "a+");
		pLog->Unlock();
		pLog->m_nLastDay = CurrTime->tm_mday;
	}

	va_list args;
	va_start(args, l_pszFMT);
	vsnprintf(szFile, MAX_LOG - 1, l_pszFMT, args);
	//    vsyslog(LOG_DEBUG, l_pszFMT, args );
	va_end(args);

	if (pLog->m_FPLog != NULL)
	{
		pLog->Lock();

		//if(szFile[0] == mCheckerCh || szFile[0] == 'M'/* || szFile[0] == 'E'*/)
		//if(szFile[0] == mCheckerCh)
		//if(szFile[0] == mCheckerCh || szFile[0] == 'E' || szFile[0] == 'Q' || szFile[0] == 'M' || szFile[0] == 'C' || szFile[0] == 'S' || szFile[0] == 'D' || szFile[0] == 'N' || szFile[0] == 'A')
		//if(szFile[0] == 'E' || szFile[0] == 'A')
			fprintf(pLog->m_FPLog, "%02d:%02d:%02d %s\n", CurrTime->tm_hour, CurrTime->tm_min, CurrTime->tm_sec, szFile);
		fflush(pLog->m_FPLog);

#ifdef VERSION_TEST
		if(mCheckerCh !=' ')
		{
			//if(szFile[0] == mCheckerCh || szFile[0] == 'M' || szFile[0] == 'E')
			if(szFile[0] == mCheckerCh || szFile[0] == 'E' || szFile[0] == 'A')
			//if(szFile[0] == mCheckerCh || szFile[0] == 'E' || szFile[0] == 'Q' || szFile[0] == 'M' || szFile[0] == 'C')
				printf("%s\n", szFile);
		}
		else
		{
			printf("%s\n", szFile);
		}
#endif

		pLog->Unlock();
	}
}


void CLog::WriteLOG(const char *l_pszData)
{
	CLog* pLog = &GetInstance();
	if (pLog->m_szPath[0] == 0x00)
	{
		if (false == InitLog("./LOG/"))
		{
			//            printf("Error InitLog!\n");
			return;
		}
	}

	char szFile[MAX_LOG];

	time_t Time;
	struct tm *CurrTime;
	time(&Time);
	CurrTime = localtime(&Time);

	if (CurrTime->tm_mday != pLog->m_nLastDay)
	{
		pLog->Lock();
		if (pLog->m_FPLog != NULL)
		{
			fclose(pLog->m_FPLog);
			pLog->m_FPLog = NULL;
		}
		sprintf(szFile, "%s%4d%02d%02d.log", pLog->m_szPath, CurrTime->tm_year + 1900, CurrTime->tm_mon + 1,
			CurrTime->tm_mday);

		pLog->m_FPLog = fopen(szFile, "a+");
		pLog->Unlock();
		pLog->m_nLastDay = CurrTime->tm_mday;
	}	

	if (pLog->m_FPLog != NULL)
	{
		pLog->Lock();
		fprintf(pLog->m_FPLog, "%02d:%02d:%02d %s", CurrTime->tm_hour, CurrTime->tm_min, CurrTime->tm_sec,
			l_pszData);
		fflush(pLog->m_FPLog);
		pLog->Unlock();
	}
}


