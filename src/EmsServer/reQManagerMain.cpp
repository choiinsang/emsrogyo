////////////// ====================================================================== ////////////////////
//
// 	Program : EMS 
//
////////////// ====================================================================== ////////////////////
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

#include <ctype.h>

#include "../Common/INIFile.h"
#include "EmsConfig.h"
#include "EmsLog.h"
#include "EmsRMQManager.h"
#include "reQManagerProc.h"


using namespace std;

bool        IsALive         = true ;
pid_t       g_pid;
CEmsLog    *gLog            = NULL;

const char *strTITLE        = "reQManager";
const char *strRELEASE_DATE = "20160527";
const char *strVERSION      = "0.0.1";
const char *strSUB_VERSION  = "QR1";

void sig_proc ( int signo ) 
{
	switch ( signo )
	{
		case SIGQUIT:
		case SIGTERM:
		case SIGABRT:
		case SIGKILL:
		case SIGPIPE:
		case SIGINT : // Ctrl + C
		{
			IsALive = false ;

			if (g_pid > 0)
			{
				fprintf(stderr,"KILL CHILD %d\n", g_pid);
				kill(g_pid, SIGINT);
			}
			exit(0);
		}
		break ;

		default :
		{
			// do nothing ...
		}
		break ;
	}
}


/*
**  USAGE -- print usage message
**
**  Parameters:
**  	None.
**
**  Return value:
**  	EX_USAGE
*/

int usage(char *progname)
{
	fprintf(stderr, "usage: %s [options]\n"
	        "\t-[H,h] Help    Display Help Manual of Re-Enqueue Messages To RabbitMQ Daemon Process.\n"
	        "\t-[V,v] Version Display %s Daemon Version.\n"
	        "\t-[C,c] Compile Display %s Compile Information.\n"
	        "set:\n"
	        "\t-[F,f] Set Configuration File Path Name.\n\tex) %s -f %s\n",
	        progname, progname, progname, progname, IEMS_SERVER_INI);

	return 0;
}

void printLine()
{
	fprintf (stdout, "-------------------------------------------------\n");
}

int main(int argc, char *argv[])
{
	bool bFlag = false;
	int  c;
	char chConfFileName[SZ_TMPBUFFER] = {0,};
	char *p;
	char *progname = (p = strrchr(argv[0], '/')) == NULL ? argv[0] : p + 1;
	
	strcpy(chConfFileName, IEMS_SERVER_INI);

	while ((c = getopt(argc, argv, "VvHhCcFf")) != -1)
	{
		switch (c)
		{
		  case 'V':
		  case 'v':
		  	printLine();
		  	fprintf (stdout, "%s %s (Release Version)\n\n", strTITLE, strVERSION ) ;
		  	if(c == 'V')
		  	{
		  		fprintf (stdout, "Release Date : %s \n(Compile DateTime:%s %s) \n", strRELEASE_DATE, __DATE__, __TIME__);
		  		fprintf (stdout, "Release Subversion : %s \n",                      strSUB_VERSION);
		  	}
		  	fprintf (stdout, "Released by Gabia Inc. All Rights Reserved\n" ) ;
		  	fprintf (stdout, "Copyright Gabia Inc. All Rights Reserved\n" ) ;
		  	printLine();

		  	bFlag = true;
		  	break;

		  case 'H':
		  case 'h':
		  	printLine();
		  	fprintf ( stderr, "Usage] %s -[h|H|V|v|C|c|F|f] \n", progname );
		  	usage(progname);
		  	printLine();

		  	bFlag = true;
		  	break;

		  case 'C':
		  case 'c':
		  	printLine();
		  	fprintf (stdout, "%s %s (Release Version)\n", strTITLE, strVERSION ) ;
		  	fprintf (stdout, "Compile DateTime: %s %s \n", __DATE__, __TIME__);
		  	printLine();

		  	bFlag = true;
		  	break;
				
		  case 'F':
		  case 'f':{
		  	memset(chConfFileName, 0, SZ_TMPBUFFER);
				strcpy(chConfFileName, argv[optind]);
				break;
		  }

		  default:
		  	printLine();
		  	usage(progname);
		  	printLine();
		  	bFlag = true;
		}
		
		if(bFlag == true){
			return 0;
		}
	}
	
	if(_DAEMON_VERSION_ > 0)
		fprintf(stdout, "_DAEMON_VERSION_ : %d \n", _DAEMON_VERSION_);
		
#if( _DAEMON_VERSION_ )
	g_pid = fork();
	if ( g_pid == -1 )
	{
		fprintf ( stderr, "fork() is failed.. \n" ) ;
		return -1 ;
	}
	else if ( g_pid != 0 )
	{
		return 0;
	}

	setpgrp();

	while (IsALive)
	{
		g_pid = fork();
		if ( g_pid == -1 )
		{
			fprintf ( stderr, "fork() is failed.. \n" ) ;
			return -1 ;
		}
		else if ( g_pid == 0 )
		{
			break;
		}
		else if ( g_pid != 0 )
		{
			signal ( SIGINT , sig_proc ) ;
			signal ( SIGQUIT, sig_proc ) ;
			signal ( SIGTERM, sig_proc ) ;
			signal ( SIGABRT, sig_proc ) ;
			signal ( SIGKILL, sig_proc ) ;
			signal ( SIGPIPE, sig_proc ) ;
			waitpid( g_pid  , NULL,  0 ) ;
			sleep(3);
		}
	}

	setpgrp();
#endif

 
	//------------[REENQUEUE-MANAGER START]---------------
	CEMSConfig      *pEmsCfg          = theEMSConfig();
	CEmsRMQManager  *pEmsRMQManager   = NULL;
	CReQManagerProc *pCReQManagerProc = NULL;

	printLine();
	if(pEmsCfg->InitConfig(chConfFileName) == true){
		fprintf( stdout, "[CONFIG] Read Config Success...[%s]\n", chConfFileName);
	}
	else {
		fprintf( stderr, "[CONFIG] Read Config Failed...[%s]\n", chConfFileName);
		assert(false);
	}
	printLine();

	pEmsRMQManager   = theEmsRMQManager();
	gLog             = theEmsLogger();
	pCReQManagerProc = theReQManager();
		
	//gLog->InitLog("./", argv[0]);
	gLog->InitLog(pEmsCfg->getServerLogPath(), argv[0]);
	gLog->Write("- [%s] Start -", progname);
	
	try{
		int iLoopCnt = 0;
		int iMENU    = 0;
		
		while(IsALive==true){
			pCReQManagerProc->showMenu();
			iMENU = 0;
			
			fprintf(stdout, "[%d][INPUT]:", iLoopCnt);
			scanf("%d", &iMENU);
			
			if(iMENU == 0)
				IsALive = false;
			else
				pCReQManagerProc->procMenu(iMENU);
				
			iLoopCnt++;
		}
	}
	catch(...){
		gLog->Write("Some Kind of Error Occurred");
	}

	gLog->Write("- [%s] Closed -", progname);

	return 0;
}

