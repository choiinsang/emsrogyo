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

#include "EmsConfig.h"
#include "EmsMXManager.h"
#include "EmsWorkerManager.h"

#include "../Common/INIFile.h"
#include "EmsLog.h"
#include "EmsDomainListSet.h"

extern void Get_SizeCodeRes();
extern void Set_MessageKey(char *_pStr);

using namespace std;

const char *strTITLE           = "IEMS Server";

CEmsLog           *gLog        = NULL;
CEmsDomainListSet *gDomainList = NULL;

bool	 IsALive = true ;
pid_t	 g_pid;

void sig_proc ( int signo ) 
{
	//printf ( "\n **** signal(%d) is occured !! ****\n", signo ) ;

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
	        "\t-[H,h] Help    Display Help Manual of IEmsServer Daemon Process.\n"
	        "\t-[V,v] Version Display %s IEmsServer Daemon Version.\n"
	        "\t-[C,c] Compile Display %s IEmsServer Compile Information.\n"
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
		  	fprintf (stdout, "%s %s (Release Version)\n\n", strTITLE, IEMS_VERSION ) ;
		  	if(c == 'V')
		  	{
		  		fprintf (stdout, "Release Date : %s \n(Compile DateTime:%s %s) \n", IEMS_RELEASE_DATE, __DATE__, __TIME__);
		  		fprintf (stdout, "Release Subversion : %s \n",                      IEMS_SUB_VERSION);
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
		  	fprintf (stdout, "%s %s (Release Version)\n", strTITLE, IEMS_VERSION ) ;
		  	fprintf (stdout, "Compile DateTime: %s %s \n", __DATE__, __TIME__);
		  	printLine();

		  	bFlag = true;
		  	break;
				
		  case 'F':
		  case 'f':{
		  	printLine();
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
		
		if(bFlag == true)
			return 0;
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

 
//------------[EMS SERVER START]---------------
	CEMSConfig     * pEmsCfg        = theEMSConfig();
	CEmsRMQManager * pEmsRMQManager = NULL;
	char           * pServerName    = NULL;
	
	if(pEmsCfg->InitConfig(chConfFileName) == true){
		fprintf( stdout, "[IEMS Server] Read Config Success...[%s]\n", chConfFileName);
	}
	else {
		fprintf( stderr, "[IEMS Server] Read Config Failed...[%s]\n", chConfFileName);
		assert(false);
	}

	Get_SizeCodeRes();
	pServerName =  const_cast<char *>(theEMSConfig()->getServerName());
	Set_MessageKey(pServerName);

	pEmsRMQManager = theEmsRMQManager();
	gLog           = theEmsLogger();
		
	gLog->InitLog(pEmsCfg->getServerLogPath(), argv[0]);
	gLog->InitLogQueue(RMQ_ROUTE_KEY_LOG, true);	
	gLog->Write("=============[EMS SERVER START]=============");

	gDomainList = theDomainList();
	gDomainList->setDomainListInfo(pEmsCfg->getDNSFileName());
	//gDomainList->printListInfo();
		
	// MX(Mail Exchanger) Information  Manager
	theMXManager();
	CEmsWorkerManager * pWorkerManager = theWorkerManager();


	//Client Thread Manager Server
	FILE *stream = NULL; 
	CEmsWorkServer theServer(SZ_MAX_SENDBUFFER, SZ_MAX_RECVBUFFER, pEmsCfg->getServerPort(), 1000, SZ_MAX_CLIENT, SZ_MAX_TIMERVALUE, stream);
	
	theServer.setWorkersPool(pWorkerManager->getWorkersPoolMap());
	theServer.StartProcessQueue();
	pWorkerManager->setWorkerServer(&theServer);
	
	//------------------------
	//EmsWorkServer Thread Start!!
	//------------------------
	theServer.Start();	
	//------------------------
	//EmsWorkerManager Thread Start!!
	//------------------------
	// Event 처리 동작이 시작되고 대기모드로 동작한다.
	pWorkerManager->startThread();
	//------------------------
	//Running pWorkerThread!
	//------------------------
	theServer.Stop();
	gLog->Write("=============[EMS SERVER CLOSED]=============");
	theServer.CleanUpNetwork();

	return 0;

}

