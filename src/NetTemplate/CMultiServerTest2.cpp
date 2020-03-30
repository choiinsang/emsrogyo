#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include "CThreadPool.h"
#include "CServer.h"
#include <string>
#include "packet_header.h"
#include "packets.h"
#include "CClient.h"
#include "CMultiServer.h"
#include "CRequest.h"

#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>

#include <boost/unordered_map.hpp>
#define unordered_map boost::unordered_map


using namespace std;

// client

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
	
	switch(ntohl(pph->ulType))	{
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

// server

typedef class CMultiServer<MyClient, CMyRequest, CMyThread> CVanilaServer;

class CMyServer : public CVanilaServer
{
public:
	CMyServer(int iMaxClientReadBuffer, int iMaxClientWriteBuffer, int iPort, int delay, int nMaxClients, int timer, int nWorkers, FILE *fp);
	void TimerFunc();

	bool Connect(char *szAddr, int iPort, int mdelay);
};

bool CMyServer::Connect(char *szAddr, int iPort, int mdelay)
{	
	CVanilaServer::Connect(szAddr, iPort, mdelay);

	/*
	{
		unordered_map <unsigned int, shared_ptr< MyClient > >::iterator R;

		R = m_ClientList.find(CVanilaServer::msave_ulCurrKey);	

		if( m_ClientList.end() != R )
		{
			printf("TTTTTTTTTKKKKKK\n");

			//MyClient aaa;
			shared_ptr<MyClient> pClient = (shared_ptr<MyClient>)(*R).second;

			pClient->Send((void*)("1234\r\n"), 6);
		}
	}
	*/

	return true;
}

CMyServer::CMyServer(int iMaxClientReadBuffer, int iMaxClientWriteBuffer, int iPort, int delay, int nMaxClients, int timer, int nWorkers, FILE *fp)
: CVanilaServer(iMaxClientReadBuffer, iMaxClientWriteBuffer, iPort, delay, nMaxClients, timer, nWorkers, fp)
{
}

void CMyServer::TimerFunc()
{
//	printf("Timer function invoke. -> m_nCurrClients = %d\n", m_nCurrClients);
}

#define	MAX_WORKERS	5	

int main(int argc, char* argv[])
{
	FILE* stream = fopen( "data2", "a+");
	//shared_ptr<CMyServer>pServer = shared_ptr<CMyServer>(new CMyServer(102400, 102400, 5096, 1000, 5000, 3000, MAX_WORKERS, stream));
	shared_ptr<CMyServer>pServer = shared_ptr<CMyServer>(new CMyServer(102400, 102400, 10020, 1000, 1, 3000, MAX_WORKERS, stream));

	pServer->InitNetwork();
	pServer->Start();
	//	pServer->Connect("222.231.60.234", 5555, 5000);
//	pServer->Connect("127.0.0.1", 10020, 5000);
	//pServer->Connect("58.145.101.25", 7878, 5000);

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


