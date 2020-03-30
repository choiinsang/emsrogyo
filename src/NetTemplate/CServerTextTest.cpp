#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include "CServer.h"
#include "CTextServer.h"
#include <string>
#include "packet_header.h"
#include "packets.h"
#include "CClient.h"

using namespace std;

// client

class MyClient : public CClient
{
public:
        MyClient(unsigned long ulKey, int iMaxReadBuffer, int iMaxWriteBuffer, int pipe, shared_ptr<CRemoveList <unsigned long > > pRemoveList);
        ~MyClient() {};
};

MyClient::MyClient(unsigned long ulKey, int iMaxReadBuffer, int iMaxWriteBuffer, int pipe, shared_ptr<CRemoveList <unsigned long> > pRemoveList)
: CClient(ulKey, iMaxReadBuffer, iMaxWriteBuffer, pipe, pRemoveList)
{
	printf("%ld\n", m_ulKey);
}

// server

typedef class CTextServer<MyClient> CVanilaServer;

class CMyServer : public CVanilaServer
{
public:
	CMyServer(int iMaxClientReadBuffer, int iMaxClientWriteBuffer, int iPort, int delay, int nMaxClients, int timer, FILE *fp);
	void ParserMain(shared_ptr<MyClient> pClient, int nLength);
	void TimerFunc();
};

CMyServer::CMyServer(int iMaxClientReadBuffer, int iMaxClientWriteBuffer, int iPort, int delay, int nMaxClients, int timer, FILE *fp)
: CVanilaServer(iMaxClientReadBuffer, iMaxClientWriteBuffer, iPort, delay, nMaxClients, timer, fp)
{
}

void CMyServer::TimerFunc()
{
	printf("Timer function invoke. -> m_nCurrClients = %d\n", m_nCurrClients);
}

void CMyServer::ParserMain(shared_ptr<MyClient> pClient, int nLength)
{
	printf("GGGGGGG ABCDE\n");

	PACKET_HEADER *pph = (PACKET_HEADER *)(pClient->m_ReadBuffer.get());

	printf("ABCDE\n", (char*)pph);
	printf("%s\n", (char*)pph);

	pClient->Send((void*)("T12345\r\n"), 8);

	/*
	switch(ntohl(pph->ulType))	
	{
	case MSG_CHAT:
		PACKET_CHAT *pc;
		pc = (PACKET_CHAT *)pph;
		printf("Client %ld said : %s\n", pClient->m_ulKey, pc->chat);
		pClient->Send(pc, ntohl(pph->ulSize));
		break;
	default:
		break;
	}
	*/
}

int main(int argc, char* argv[])
{
FILE* stream = fopen( "data2", "a+");
	//shared_ptr<CMyServer>pServer = shared_ptr<CMyServer>(new CMyServer(102400, 102400, 5096, 100, 5000, 3000, stream));
shared_ptr<CMyServer>pServer = shared_ptr<CMyServer>(new CMyServer(102400, 102400, 10020, 100, 5000, 3000, stream));

	pServer->InitNetwork();
	pServer->Start();
//	pServer->Connect("222.231.60.234", 5555, 5000);

#ifdef	WIN32
	Sleep(30000);
#endif
#ifdef	__LINUX__
	sleep(1000);	
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
