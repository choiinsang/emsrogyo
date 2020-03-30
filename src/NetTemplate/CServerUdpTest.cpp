#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include "CServerUdp.h"
#include <string>
#include "packet_header.h"
#include "packets.h"
#include "CClientUdp.h"

using namespace std;

// client

class MyClient : public CClientUdp
{
public:
        MyClient(string IP, int Port, int pipe, shared_ptr<CRemoveList <string> > pRemoveList, shared_ptr<CSendQueue> pQueue);
        ~MyClient() {};
};

MyClient::MyClient(string IP, int Port, int pipe, shared_ptr<CRemoveList <string> > pRemoveList, shared_ptr<CSendQueue> pQueue)
: CClientUdp(IP, Port, pipe, pRemoveList, pQueue)
{
	printf("%s:%d\n", m_IP.c_str(), Port);
}

// server

typedef class CServerUdp<MyClient> CVanilaServer;

class CMyServer : public CVanilaServer
{
public:
	CMyServer(int MaxReadBuffer, int MaxWriteBuffer, int iPort, int delay, int nMaxClients, int timer, int cleaner_timer, FILE *fp);
	void ParserMain(shared_ptr<MyClient> pClient, int nRecv);
	void TimerFunc();
};

CMyServer::CMyServer(int MaxReadBuffer, int MaxWriteBuffer, int iPort, int delay, int nMaxClients, int timer, int cleaner_timer, FILE *fp)
: CVanilaServer(MaxReadBuffer, MaxWriteBuffer, iPort, delay, nMaxClients, timer, cleaner_timer, fp)
{
}

void CMyServer::TimerFunc()
{
	printf("Timer function invoke. -> m_nCurrClients = %d, m_pQueue size = %d\n", m_nCurrClients, m_pQueue->GetSize());
}

void CMyServer::ParserMain(shared_ptr<MyClient> pClient, int nRecv)
{
	char buffer[1024];
	memset(buffer, 0, 1024);
	memcpy(buffer, m_ReadBuffer.get(), nRecv);	

	printf("%s:%d Client said %s\n", pClient->GetIP().c_str(), pClient->GetPort(), buffer);
	
	pClient->Send((void *)buffer, strlen(buffer));
}

int main(int argc, char* argv[])
{
FILE* stream = fopen( "data2", "a+");
	shared_ptr<CMyServer>pServer = shared_ptr<CMyServer>(new CMyServer(1024000, 1024000, 5096, 100, 5000, 3000, 5000, stream));

	pServer->InitNetwork();
	pServer->Start();
//	pServer->Connect("222.231.60.234", 5555, 5000);

	sleep(1000);	

	printf("Stopping ...\n");
	pServer->Stop();
	printf("Stopped\n");
	pServer->CleanUpNetwork();
        fclose(stream);

	char c;

	fgets(&c, 1, stdin);
	return 0;
}
