#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include "CServerEpoll.h"
#include <string>
#include "packet_header.h"
#include "packets.h"
#include "CClientEpoll.h"
#include <iostream>

using namespace std;

// client

class MyClient : public CClientEpoll
{
public:
        MyClient(unsigned long ulKey, int iMaxReadBuffer, int iMaxWriteBuffer, int epoll_fd, shared_ptr<CRemoveList <unsigned long> > m_pRemoveList);
        ~MyClient() {};
};

MyClient::MyClient(unsigned long ulKey, int iMaxReadBuffer, int iMaxWriteBuffer, int epoll_fd, shared_ptr<CRemoveList <unsigned long> > m_pRemoveList)
: CClientEpoll(ulKey, iMaxReadBuffer, iMaxWriteBuffer, epoll_fd, m_pRemoveList)
{
	printf("%ld\n", m_ulKey);
}

// server

typedef class CServer<MyClient> CVanilaServer;

class CMyServer : public CVanilaServer
{
public:
	CMyServer(int iMaxClientReadBuffer, int iMaxClientWriteBuffer, int iPort, int delay, int nMaxClients, int timer, FILE *fp);
	void ParserMain(shared_ptr<MyClient> pClient);
	void TimerFunc();
};

CMyServer::CMyServer(int iMaxClientReadBuffer, int iMaxClientWriteBuffer, int iPort, int delay, int nMaxClients, int timer, FILE *fp)
: CVanilaServer(iMaxClientReadBuffer, iMaxClientWriteBuffer, iPort, delay, nMaxClients, timer, fp)
{
}

void CMyServer::TimerFunc()
{
	printf("Timer function invoke. m_nCurrClients = %d\n", m_nCurrClients);
	unordered_map<unsigned long, shared_ptr<MyClient> >::iterator I;

	shared_ptr<MyClient> pClient;
	for(I = m_ClientList.Begin(); I != m_ClientList.End(); I++)
	{
		pClient = (*I).second;
		cout << "IP : " << pClient->m_IP << "\t" << "Port : " << pClient->m_nPort << endl;
	}
}

void CMyServer::ParserMain(shared_ptr<MyClient> pClient)
{
	PACKET_HEADER *pph = (PACKET_HEADER *)(pClient->m_ReadBuffer.get());

	switch(ntohl(pph->ulType))	
	{
	case MSG_CHAT:
		PACKET_CHAT *pc;
		pc = (PACKET_CHAT *)pph;
		printf("Client %ld said : %s\n", pClient->m_ulKey, pc->chat);

		pClient->Send(pc, ntohl(pph->ulSize));
		break;
	case 11:
		printf("GGGGGGGGG\n");
		pClient->Send((void*)"Kabcd", 6);
	default:
		break;
	}
}

int main(int argc, char* argv[])
{
FILE* stream = fopen( "data2", "a+");
	//shared_ptr<CMyServer>pServer = shared_ptr<CMyServer>(new CMyServer(102400, 102400, 5096, 100, 10000, 3000, stream));
	shared_ptr<CMyServer>pServer = shared_ptr<CMyServer>(new CMyServer(102400, 102400, 10020, 1000, 10000, 3000, stream));

	pServer->InitNetwork();
	pServer->Start();
//	pServer->Connect("222.231.60.234", 5555, 5000);

#ifdef	WIN32
	Sleep(30000);
#endif
#ifdef	__LINUX__
	sleep(100000);	
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
