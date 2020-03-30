#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include "CServer.h"
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

//MyClient::MyClient(unsigned long ulKey, int iMaxReadBuffer, int iMaxWriteBuffer, int pipe)
//: CClient(ulKey, iMaxReadBuffer, iMaxWriteBuffer, pipe)
MyClient::MyClient(unsigned long ulKey, int iMaxReadBuffer, int iMaxWriteBuffer, int pipe, shared_ptr<CRemoveList <unsigned long> > pRemoveList)
: CClient(ulKey, iMaxReadBuffer, iMaxWriteBuffer, pipe, pRemoveList)
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
	printf("Timer function invoke. -> m_ulCurrKey = %ld\n", m_ulCurrKey);
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
		break;
	default:
		break;
	}
}

int main(int argc, char* argv[])
{
	FILE* stream = fopen( "data2", "a+");
	shared_ptr<CMyServer>pServer = shared_ptr<CMyServer>(new CMyServer(102400, 102400, 10020, 100000, 5000, 3000, stream));

	pServer->InitNetwork();
	pServer->Start();
	//if( !pServer->Connect("222.231.60.65", 5096, 5000) )
	if( !pServer->Connect("127.0.0.1", 10020, 5000) )
		printf("Connection failed\n");
	else
		printf("Connection succeeded\n");

#ifdef	WIN32
	Sleep(3);
#endif
#ifdef	__LINUX__
	sleep(3);	
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
