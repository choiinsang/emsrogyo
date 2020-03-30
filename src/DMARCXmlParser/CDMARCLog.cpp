#include "CDMARCLog.h"

#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include "../Libgabia/inc/sth_misc.h"

CDMARCLog::CDMARCLog()
: m_FPLog        (NULL)
, m_nLastDay     (-1)
, m_LogName      ("")
{
	memset(m_szPath, 0x00, MAX_PATH+1);
}

CDMARCLog::~CDMARCLog()
{
}


CDMARCLog *CDMARCLog::m_pLog = NULL;

CDMARCLog *CDMARCLog::getInstance()
{
	if ( m_pLog == NULL )
	{
		m_pLog = new CDMARCLog();
	}

	return m_pLog;
}


bool CDMARCLog::isExistDir(const char *l_pszPath)
{
	DIR *pDir;
	if ((pDir = opendir(l_pszPath)) == NULL)
	{
			return false;
	}
	closedir(pDir);
	return true;
}

bool CDMARCLog::InitLog(const char *l_pszPath, const char *pLogName)
{
	if(l_pszPath == NULL)
		return false;
	
	if(pLogName != NULL)
		m_LogName = pLogName;
	else
		exit(0);

	memcpy(this->m_szPath, l_pszPath, MAX_PATH);

	if(isExistDir(this->m_szPath) == false){
		if (mkdir(this->m_szPath, 00777) < 0){
			//fprintf(stdout, "Make Directory Failed[ECODE:%d]\n", strerror(errno));
			exit(0);
		}
	}
	
	return true;
}


//==================================================
//--ischoi 차후  Queue를 이용해서 메시지 처리하는 방안 검토
void CDMARCLog::Write(const char *l_pszFMT, ... )
{
	if (this->m_szPath[0] == 0x00)
	{
		fprintf(stderr, "[%s][%s][%d] Error LogFile Path!\n", __FILE__, __FUNCTION__, __LINE__);
		return;
	}

	char szFile[MAX_LOG];

	time_t Time;
	struct tm *CurrTime;
	time(&Time);
	CurrTime = localtime(&Time);

	if (CurrTime->tm_mday != this->m_nLastDay)
	{
		this->Lock();
		if (this->m_FPLog != NULL)
		{
			fclose(this->m_FPLog);
			this->m_FPLog = NULL;
		}
		
		sprintf(szFile, "%s/%s_%4d%02d%02d.log", this->m_szPath, this->m_LogName.c_str(), CurrTime->tm_year + 1900,
			CurrTime->tm_mon + 1, CurrTime->tm_mday);

		//printf("Log File:[%s]\n", szFile); //

		this->m_FPLog = fopen(szFile, "a+");
		this->Unlock();
		this->m_nLastDay = CurrTime->tm_mday;
	}

	va_list args;
	va_start(args, l_pszFMT);
	vsnprintf(szFile, MAX_LOG - 1, l_pszFMT, args);
	//    vsyslog(LOG_DEBUG, l_pszFMT, args );
	va_end(args);

	if (this->m_FPLog != NULL)
	{
		this->Lock();

		fprintf(this->m_FPLog, "[%02d:%02d:%02d] %s\n", CurrTime->tm_hour, CurrTime->tm_min, CurrTime->tm_sec, szFile);
		fflush(this->m_FPLog);

		this->Unlock();
	}
}


//==================================================
//--ischoi 사용하는 코드 없음
void CDMARCLog::WriteLOG(const char *l_pszData)
{
	if (this->m_FPLog == NULL)
	{
		fprintf(stderr, "[%s][%s][%d]%s\n", __FILE__, __FUNCTION__, __LINE__, l_pszData);
		return;
	}

	char szFile[MAX_LOG];

	time_t Time;
	struct tm *CurrTime;
	time(&Time);
	CurrTime = localtime(&Time);

	if (CurrTime->tm_mday != this->m_nLastDay)
	{
		this->Lock();
		if (this->m_FPLog != NULL)
		{
			fclose(this->m_FPLog);
			this->m_FPLog = NULL;
		}

		sprintf(szFile, "%s/%s_%4d%02d%02d.log", this->m_szPath, this->m_LogName.c_str(), CurrTime->tm_year + 1900,
			CurrTime->tm_mon + 1, CurrTime->tm_mday);			

		this->m_FPLog = fopen(szFile, "a+");
		this->Unlock();
		this->m_nLastDay = CurrTime->tm_mday;
	}	

	if (this->m_FPLog != NULL)
	{
		this->Lock();
		fprintf(this->m_FPLog, "[%02d:%02d:%02d] %s", CurrTime->tm_hour, CurrTime->tm_min, CurrTime->tm_sec,
			l_pszData);
		fflush(this->m_FPLog);
		this->Unlock();
	}
}


int CDMARCLog::remove_dir(char *path)
{
   DIR *d = opendir(path);
   size_t path_len = strlen(path);
   int r = -1;

   if (d)
   {
      struct dirent *p;

      r = 0;

      while (!r && (p=readdir(d)))
      {
          int r2 = -1;
          char *buf;
          size_t len;

          /* Skip the names "." and ".." as we don't want to recurse on them. */
          if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
          {
             continue;
          }

          len = path_len + strlen(p->d_name) + 2; 
          buf = (char*)malloc(len);

          if (buf)
          {
             struct stat statbuf;

             snprintf(buf, len, "%s/%s", path, p->d_name);

             if (!stat(buf, &statbuf))
             {
                if (S_ISDIR(statbuf.st_mode))
                {
                   r2 = remove_dir(buf);
                }
                else
                {
                   r2 = unlink(buf);
                }
             }

             free(buf);
          }

          r = r2;
      }

      closedir(d);
   }

   if (!r)
   {
      r = rmdir(path);
   }

   return r;
}


void CDMARCLog::CloseInfo(char *keyName)
{
	//Log 경로 아래에 Key를 폴더로하는 file 정보를 넣는다.
	char tmpPath[MAX_PATH]={0,};
	sprintf(tmpPath, "%s/%s", this->m_szPath, keyName);

	Write("[%s]==>[%s]", __FUNCTION__, tmpPath);
	//폴더 체크
	if(isExistDir(tmpPath) == true){
		remove_dir(tmpPath);
	}
}


