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

/*
class MyClient : public CClient
{
public:
	MyClient(unsigned long ulKey, int iMaxReadBuffer, int iMaxWriteBuffer, int pipe, shared_ptr<CRemoveList <unsigned long> > pRemoveList);
	~MyClient() {};
};

MyClient::MyClient(unsigned long ulKey, int iMaxReadBuffer, int iMaxWriteBuffer, int pipe, shared_ptr<CRemoveList <unsigned long> > pRemoveList)
: CClient(ulKey, iMaxReadBuffer, iMaxWriteBuffer, pipe, pRemoveList)
{
	printf("%ld\n", m_ulKey);
}

class CMyRequest : public CRequest<MyClient>
{
public:
};

// packet

class CMyThread : public CThread<CMyRequest>
{
public:
	CMyThread(unsigned long identifier, shared_ptr<pthread_mutex_t> pMutex, shared_ptr<pthread_cond_t>pCond, shared_ptr< queue < shared_ptr < CMyRequest > > > pQueue);
	void processRequest(shared_ptr<CMyRequest> pRequest);
};

CMyThread::CMyThread(unsigned long identifier, shared_ptr<pthread_mutex_t> pMutex, shared_ptr<pthread_cond_t>pCond, shared_ptr< queue < shared_ptr < CMyRequest > > > pQueue)
: CThread<CMyRequest>(identifier, pMutex, pCond, pQueue)
{
}

void CMyThread::processRequest(shared_ptr<CMyRequest> pRequest)
{
	PACKET_HEADER *pph = (PACKET_HEADER *)pRequest->getPacket().get();

	printf("CMyThread::processRequest\n", (char*)pph);

	switch(ntohl(pph->ulType))	
	{
	case MSG_CHAT:
		PACKET_CHAT *pc;
		pc = (PACKET_CHAT *)pph;
		printf("Client %ld said : %s\n", pRequest->getClient()->m_ulKey, pc->chat);
		break;
	case 11:
		printf("GGGGGGGGG\n");
		//pRequest->setPacket((char*)"Kabcd", 6);
		//pRequest->setClient(pRequest->getClient());
		//pRequest->setPacket((char*)pRequest->getClient()->m_ReadBuffer.get(), 5+1); // +1 '\0' Ãß°¡
		//pRequest->getPacket().get()[5] = '\0';
		//addQueue(pRequest);
		pRequest->getClient()->Send((void*)"Kabcd", 6);
		//pClient->Send((void*)"Kabcd", 6);
	default:
		break;
	}
}
*/

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
