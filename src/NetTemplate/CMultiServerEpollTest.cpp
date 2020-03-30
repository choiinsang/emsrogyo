#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include "CThreadPool.h"
#include "CServerEpoll.h"
#include <string>
#include "packet_header.h"
#include "packets.h"
#include "CClientEpoll.h"
#include "CMultiServerEpoll.h"
#include "CRequest.h"
using namespace std;

#include "CRemoveList.h"
#include <boost/shared_ptr.hpp>
using namespace boost; 

#include "MyClient.h"
#include "CMyRequest.h"
#include "CMyThread.h"
#include "CMyServer.h"

#define	MAX_WORKERS	5	

int main(int argc, char* argv[])
{
	FILE* stream = fopen( "data2", "a+");
	//shared_ptr<CMyServer>pServer = shared_ptr<CMyServer>(new CMyServer(102400, 102400, 5096, 1000, 5000, 3000, MAX_WORKERS, stream));
	shared_ptr<CMyServer>pServer = shared_ptr<CMyServer>(new CMyServer(102400, 102400, 10020, 1000, 5000, 3000, MAX_WORKERS, stream));

	pServer->InitNetwork();
	pServer->Start();
//	pServer->Connect("222.231.60.234", 5555, 5000);

#ifdef	WIN32
	Sleep(30000);
#endif
#ifdef	__LINUX__
	sleep(30000);	
#endif

	printf("Stopping ...\n");
	pServer->Stop();
	printf("Stopped\n");
	pServer->CleanUpNetwork();
        fclose(stream);

	char c;

	fgets(&c, 1, stdin);
	return 0;
}
