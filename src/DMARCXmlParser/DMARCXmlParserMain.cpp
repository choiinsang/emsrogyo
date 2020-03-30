//************************************************
//
// 	Program : DMARC_XML_PARSER 
//
//************************************************

#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

#include "CDMARCXmlParserConfig.h"
#include "CDMARCFunctions.h"
#include "CDMARCLog.h"
#include "CDMARCXmlParserThread.h"

extern int  file_is_modified(const char *path, time_t &oldMTime, string &errStr);
extern bool getFilesFromDir (const char *pDirPath, vector<std::string> &vecFiles, std::string &errStr);

using namespace std;

const char            *strTITLE   = "DMARCXmlParser";
CDMARCLog             *gLog       = NULL;
CDMARCXmlParserConfig *gTheConfig = NULL;

bool	     IsALive = true ;
pid_t	     g_pid;

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
				fprintf(stderr,"KILL CHILD [%d][%d]\n", signo, g_pid);
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
	fprintf(stderr,"close Process [%d][%d]\n", signo, g_pid);
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
	fprintf(stderr, "%s: usage: %s [options]\n"
	        "\t-[H,h] Help\tDisplay Help Manual of DMARCXmlParser Daemon Process.\n"
	        "\t-[V,v] Display DMARCXmlParser Daemon Version.\n",
	        progname, progname);

	return 0;
}

void printLine()
{
	fprintf (stdout, "-------------------------------------------------\n");
}

int main(int argc, char *argv[])
{
	bool bFlag = false;
	bool bOut  = false;
	int  c;
	char *p;
	char *progname = (p = strrchr(argv[0], '/')) == NULL ? argv[0] : p + 1;

	while ((c = getopt(argc, argv, "VvHh")) != -1)
	{
		bFlag = true;
		switch (c)
		{
		  case 'V':
		  case 'v':
		  	printLine();
		  	fprintf (stdout, "%s %s (Release Version)\n\n", strTITLE, DMARCXMLPARSER_VERSION ) ;
		  	if(c == 'V')
		  	{
		  		fprintf (stdout, "Release Date : %s \n(Compile DateTime:%s %s) \n", DMARCXMLPARSER_RELEASE_DATE, __DATE__, __TIME__);
		  		fprintf (stdout, "Release Subversion : %s \n", DMARCXMLPARSER_SUB_VERSION);
		  	}
		  	fprintf (stdout, "Released by Gabia Inc. All Rights Reserved\n" ) ;
		  	fprintf (stdout, "Copyright Gabia Inc. All Rights Reserved\n" ) ;
		  	printLine();

		  	bOut = true;
		  	break;

		  case 'H':
		  case 'h':
		  	printLine();
		  	fprintf ( stderr, "Usage] %s -[h|H|v|V] -\n", progname );
		  	usage(progname);
		  	printLine();

		  	bOut = true;
		  	return 0;
		  	break;

		  case 'C':
		  case 'c':
		  	printLine();
		  	fprintf (stdout, "%s %s (Release Version)\n", strTITLE, DMARCXMLPARSER_VERSION ) ;
		  	fprintf (stdout, "Compile DateTime: %s %s \n", __DATE__, __TIME__);
		  	printLine();

		  	bOut = true;
		  	break;
				
		  default:
		  	printLine();
		  	usage(progname);
		  	printLine();
		  	bOut = true;
		}
		
		if(bOut == true)
			break;
	}
	
	if(bFlag == true)
		return 0;
	
	
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

	//------------[DMARC XML PARSER START]---------------
	gTheConfig = theDMARCXmlParserConfig();
	
	if(gTheConfig->InitConfig(DMARCXMLPARSER_INI) == true){
		//fprintf( stdout, "[%s] Read Config Success...[%s]\n", argv[0], DMARCXMLPARSER_INI);
	}
	else {
		fprintf( stderr, "[%s] Read Config Failed...[%s]\n", argv[0], DMARCXMLPARSER_INI);
		exit(0);
		//assert(false);
	}

	gLog = theDMARCLogger();
	gLog->InitLog(gTheConfig->getLogPath(), gTheConfig->getLogFileName());
	gLog->Write("=============[DMARC_XML_PARSER START]=============");

	//------------------------
	// Input Path Process
	// 1. 파일 리스트 수집-> 상위 하나만 가져옴. 정렬해서 하나만 처리->반복 처리
	//------------------------
	//printf("gTheConfig->getFileInputPath():%s\n", gTheConfig->getFileInputPath());
	
	//-- CDMARCXmlParserThread Start
	//CDMARCXmlParserThread *pXmlParserThread = theXmlParserThread();
	theXmlParserThread()->startThread();
	
	//------------------------
	while(true){
		sleep(2);
		//printf("While Loop Check....\n");
	}
	//------------------------
	gLog->Write("=============[DMARC_XML_PARSER CLOSED]=============");

	return 0;

}

