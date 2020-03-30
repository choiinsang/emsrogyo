/* ****************************************************************************
Filename	:	STH_syslog.c
Author		:	Eric
Date		:	2003/11/27
**************************************************************************** */
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>
#include "../inc/sth_syslog.h"

int STH_TR_SYSLOG=1;	// Syslog : Yes
int STH_TR_VERBOSE=0;	// Verbose : No
int STH_TR_LEVEL=1;		// Level : 1 ( Error, Warning )

void
InitInetdDaemon(const char *pname, int facility)
{
	openlog(pname, LOG_PID, facility);
}

// 현재 실행중인 프로세스인지 확인한다.
int
CheckAlive( const char *pname )
{
	char szLockPath[4096];
	FILE *fpLock;
	int nOldPid;
	char *pExeName;

	if( pExeName = strchr(pname,'/') )
		pExeName++;
	else
		pExeName=pname;

	snprintf(szLockPath,4096,PROCESS_LOCK_PATH,pExeName);

	if( (fpLock = fopen(szLockPath,"r")) != NULL )
	{
		if( feof(fpLock) || ferror(fpLock) )
		{
			fclose(fpLock);
			goto savePidInfo;
		}
		else
		{
			fread(&nOldPid, sizeof(int),1,fpLock);
			fclose(fpLock);
			snprintf(szLockPath,4096,PROCESS_CMDLINE_PATH,nOldPid);

			if( (fpLock = fopen(szLockPath,"r")) == NULL ) goto savePidInfo;
			if( !fgets(szLockPath,4095,fpLock) )
			{
				fclose(fpLock);
				goto savePidInfo;
			}
			fclose(fpLock);

			if( strstr( szLockPath, pExeName ) )
			{
				fprintf(stderr,"[%s] Already running\n",pExeName);
				return TRUE;
			}
		}
	}

savePidInfo:

	snprintf(szLockPath,4096,PROCESS_LOCK_PATH,pExeName);
	printf("[%s] save\n",szLockPath);
	if( (fpLock = fopen(szLockPath,"w")) == NULL )
	{
		fprintf(stderr,"warning: [%s] Lock create failed\n",pExeName);
		return FALSE;
	}

	nOldPid = getpid();
	
	fwrite(&nOldPid,sizeof(int),1,fpLock);
	fclose(fpLock);

	return FALSE;
}

void
InitDaemon(const char *pname, int facility)
{
	int		i;
	pid_t	pid;

	if ( (pid = fork()) != 0)
		exit(0);			/* parent terminates */

	/* 1st child continues */
	setsid();				/* become session leader */

	signal(SIGHUP, SIG_IGN);
	if ( (pid = fork()) != 0)
		exit(0);			/* 1st child terminates */

	// 프로세스가 기 실행주이면 종료한다.
	if( CheckAlive(pname ) )
		exit(0);

	/* 2nd child continues */
	chdir("/");				/* change working directory */
	umask(0);				/* clear our file mode creation mask */

	for (i = 0; i < 64; i++)
		close(i);

	openlog(pname, LOG_PID, facility);
}

void
SetDebugOption(int level, int tr_syslog, int tr_verbose)
{
	STH_TR_LEVEL = level;
	STH_TR_SYSLOG = tr_syslog;
	STH_TR_VERBOSE = tr_verbose;
}

void 
Trace(int level, char *szFormat, ...)
{
	va_list argp;
	
	if (level <= STH_TR_LEVEL)
	{
		if (STH_TR_VERBOSE)
		{ 
			va_start(argp, szFormat);
			vfprintf (stderr, szFormat, argp);
			if (szFormat[strlen(szFormat)]!='\n')
				fprintf (stderr,"\n");
			va_end(argp);
		}
		if (STH_TR_SYSLOG)
		{
			va_start(argp, szFormat);
			if (szFormat[strlen(szFormat)]=='\n')
				szFormat[strlen(szFormat)]='\0';
			if (level <= TR_WARNING) 
			{
				/* set LOG_ALERT at warnings */
				vsyslog (LOG_ALERT, szFormat, argp);
			}
			else 
				vsyslog (LOG_NOTICE, szFormat, argp);
			va_end(argp);
		}
	}
}
