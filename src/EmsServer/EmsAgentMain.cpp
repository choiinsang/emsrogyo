//=================================//
// 	Program : IEMS Agent            //
//=================================//
#include <iostream>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include "CServerEpoll.h"
#include "packet_header.h"
#include "packets.h"
#include "CClientEpoll.h"
#include "CRequest.h"
#include "EmsCommon.h"
#include "../Common/INIFile.h"
#include "EmsLog.h"
#include "EmsConfig.h"
#include "EmsAgentServer.h"
#include "EmsQProcThread.h"

using namespace std;

#define	MAX_WORKERS	5

const char *strTITLE  = "IEMS Agent";

CEmsLog    *gLog      = NULL;
bool	      IsALive   = true ;
pid_t	      g_pid ;



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
			else{
				fprintf(stderr,"===( END IEMS Agent Main Thread! )===%d\n", g_pid);
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

int usage(char *progname)
{
	fprintf(stderr, "usage: %s [options]\n"
	        "\t-[H,h] Help    Display Help Manual of Integrated EmsAgent Daemon Process.\n"
	        "\t-[V,v] Version Display %s Integrated EmsAgent Daemon Version.\n"
	        "\t-[C,c] Compile Display %s Integrated EmsAgent Compile Information.\n"
	        "set:\n"
	        "\t-[F,f] Set Configuration File Path Name.\n\tex) %s -f %s\n",
	        progname, progname, progname, progname, IEMS_AGENT_INI);

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

	strcpy(chConfFileName, IEMS_AGENT_INI);

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
		  	fprintf ( stderr, "Usage] %s -[H|h|V|v|C|c|F|f] \n", progname );
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

//------------[IEMS AGENT SERVER START]---------------
	CEMSConfig     * pEmsCfg        = theEMSConfig();
	CEmsRMQManager * pEmsRMQManager = NULL;
	
	if(pEmsCfg->InitConfig(chConfFileName) == true)
		fprintf(stdout, "[IEMS AGENT] Read Config Success...[%s]\n", chConfFileName);
	else {
		fprintf(stderr, "[IEMS AGENT] Read Config Failed...[%s]\n", chConfFileName);
		assert(false);
	}

	pEmsRMQManager = theEmsRMQManager();
	gLog           = theEmsLogger();
		
	//Log Initialization
	gLog->InitLog(pEmsCfg->getServerLogPath(), argv[0]);
	gLog->InitLogQueue(RMQ_ROUTE_KEY_LOG);
	gLog->Write("[Log Path:%s][RKEY_LOG:%s]", pEmsCfg->getServerLogPath(), RMQ_EXCHANGE_LOG);
	gLog->Write("=============[IEMS AGENT SERVER START!]=============");

	FILE *stream = NULL; 
	CEmsAgentServer theAgentServer(SZ_MAX_SENDBUFFER, SZ_MAX_RECVBUFFER, pEmsCfg->getServerPort(), 1000, SZ_MAX_CLIENT, SZ_MAX_TIMERVALUE, MAX_WORKERS, stream);
	
	int checkCount=0;
	
	do{
		if(theAgentServer.StartDBProcess() == true)
			break;

		if(checkCount++ > MAX_DB_CHECK_WAIT_COUNT){
			gLog->Write("EmsAgent Server DB Connection failed");
		}
		sleep(1);
	}while(true);

	theAgentServer.Start();
	
	//Rabbitmq Log Queue Processor
	CEmsQProcThread * pMsgProcThread = theEmsQProcThread();
	
	bool bResult = false;
	pMsgProcThread->setOption ( ILOG_OPT_PROC );
	bResult = pMsgProcThread->ConnectDB ( theEMSConfig()->getDBIP()
	                          , theEMSConfig()->getDBUser()
	                          , theEMSConfig()->getDBPasswd()
	                          , theEMSConfig()->getDBName()
	                          , theEMSConfig()->getDBPort());
	if (bResult == false){
		gLog->Write("[%s][%s][%d] DB Connect Failed [%s|[%s|[%s|[%s|%d]\n"
		            , __FILE__, __FUNCTION__, __LINE__, theEMSConfig()->getDBIP()
	              , theEMSConfig()->getDBUser(), theEMSConfig()->getDBPasswd()
	              , theEMSConfig()->getDBName(), theEMSConfig()->getDBPort());
		//exit(-1);
		return false;
	}

	pMsgProcThread->setEmsAgentServer(&theAgentServer);
	if(pMsgProcThread->InitProc() == false){
		gLog->Write("[%s][%s][%d][ERROR][theEmsQProcThread() Init Failed]", __FILE__, __FUNCTION__, __LINE__);
		return 0;
	}
	sleep(2);
	pMsgProcThread->StartThread();

	///*
	while (IsALive)
	{
		//EmsAgent 서버 CMD Queue 실행
		theAgentServer.createEACMDQueue();	//Running and MQ Message Processing State 
		sleep(THREAD_RMQ_WAIT_TIME);
	}
	//*/

	gLog->Write("EmsAgent Server Thread END!!!");

	return 0;
}

