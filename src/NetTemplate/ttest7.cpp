#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include "CThreadPool.h"
#include "CServer.h"
#include "CMysql.h"
#include "mysql/errmsg.h"
#include <string>
#include "packet_header.h"
#include "packets.h"
#include "CClient.h"
#include "CMultiServer.h"
#include "CRequest.h"
#include "CRemoveList.h"

using namespace std;

// client

class MyClient : public CClient
{
public:
	//MyClient(unsigned long ulKey, int iMaxReadBuffer, int iMaxWriteBuffer, int pipe, shared_ptr<CRemoveList> pRemoveList);
	MyClient(unsigned long ulKey, int iMaxReadBuffer, int iMaxWriteBuffer, int pipe, shared_ptr<CRemoveList <unsigned long > > pRemoveList);
	~MyClient() {};
};

//MyClient::MyClient(unsigned long ulKey, int iMaxReadBuffer, int iMaxWriteBuffer, int pipe, shared_ptr<CRemoveList> pRemoveList)
MyClient::MyClient(unsigned long ulKey, int iMaxReadBuffer, int iMaxWriteBuffer, int pipe, shared_ptr<CRemoveList <unsigned long > > pRemoveList)
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
private:
	shared_ptr<CMysql> m_pMysql;
	string m_host, m_user, m_passwd, m_dbname;
	int m_port;

public:
	CMyThread(unsigned long identifier, shared_ptr<pthread_mutex_t> pMutex, shared_ptr<pthread_cond_t>pCond, shared_ptr< queue < shared_ptr < CMyRequest > > > pQueue);
	void processRequest(shared_ptr<CMyRequest> pRequest);
	bool connect(const char *host, const char *user, const char *passwd, const char *dbname, unsigned int port);
};

CMyThread::CMyThread(unsigned long identifier, shared_ptr<pthread_mutex_t> pMutex, shared_ptr<pthread_cond_t>pCond, shared_ptr< queue < shared_ptr < CMyRequest > > > pQueue)
: CThread<CMyRequest>(identifier, pMutex, pCond, pQueue)
{
	m_pMysql = shared_ptr<CMysql>(new CMysql);
}

bool CMyThread::connect(const char *host, const char *user, const char *passwd, const char *dbname, unsigned int port)
{
	m_host = host;
	m_user = user;
	m_passwd = passwd;
	m_dbname = dbname;
	m_port = port;

	return m_pMysql->connect(m_host.c_str(), m_user.c_str(), m_passwd.c_str(), m_dbname.c_str(), m_port);
}

void CMyThread::processRequest(shared_ptr<CMyRequest> pRequest)
{
	PACKET_HEADER *pph = (PACKET_HEADER *)pRequest->getPacket().get();

	switch(pph->ulType)	
	{
	case MSG_CHAT:
		PACKET_CHAT *pc;
		pc = (PACKET_CHAT *)pph;
		printf("Client %ld said : %s\n", pRequest->getClient()->m_ulKey, pc->chat);
		break;
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
};

CMyServer::CMyServer(int iMaxClientReadBuffer, int iMaxClientWriteBuffer, int iPort, int delay, int nMaxClients, int timer, int nWorkers, FILE *fp)
: CVanilaServer(iMaxClientReadBuffer, iMaxClientWriteBuffer, iPort, delay, nMaxClients, timer, nWorkers, fp)
{
}

void CMyServer::TimerFunc()
{
	printf("Timer function invoke. -> m_ulCurrKey = %ld\n", m_ulCurrKey);
}

#define	MAX_WORKERS	300

int main(int argc, char* argv[])
{
	printf("Cur=>main start\n");

	FILE* stream = fopen( "data2", "a+");

	printf("Cur=>open data2\n");

	//shared_ptr<CMyServer>pServer = shared_ptr<CMyServer>(new CMyServer(102400, 102400, 5096, 100000, 5000, 3000, MAX_WORKERS, stream));
	shared_ptr<CMyServer>pServer = shared_ptr<CMyServer>(new CMyServer(102400, 102400, 10020, 100000, 5000, 3000, MAX_WORKERS, stream));

	printf("Cur=>new CMyServer\n");

	pServer->InitNetwork();
	pServer->Start();
	//	pServer->Connect("222.231.60.234", 5555, 5000);

	printf("Cur=>pServer->Start\n");

#ifdef	WIN32
	Sleep(30000);
#endif
#ifdef	__LINUX__
	printf("Cur=>sleep\n");	
	sleep(30000);
	printf("Cur=>sleepEnd\n");
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
