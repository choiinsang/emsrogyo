#ifndef __CMONITOR__
#define __CMONITOR__

#include "../common/CClient.h"
#include "../common/CServer.h"
#include "../common/CConnection.h"
#include "../common/LockObject.h"

template <typename CN> 
class CMonitor : public CServer<CClient>, public LockObject
{
public:
	map <LONGLONG, CN *> m_Monitees;		// Monitee = Object to be monitored
	HANDLE m_hThread_m, m_hStopEvent_m;
	int m_cdelay, m_wdelay;		// delay for connect, delay for wait(ms)

	CMonitor(int iMaxClientReadBuffer, int iMaxClientWriteBuffer, int cdelay, int wdelay, int delay, int nMaxClients, int timer, FILE *fp);
	~CMonitor();
	bool LoadMonitees(char *szFile);
	LONGLONG MakeKey(SOCKADDR_IN addr);

	bool AddClient(SOCKET socket);
	bool RemoveClient(unsigned long ulKey);

	void ParserMain(shared_ptr<CClient> pClient);
	virtual void ParserMain_Safe(shared_ptr<CClient> pClient) { };

	bool StartMonitoring();
	bool StopMonitoring();

	virtual void SetMonitoring(CClient *pClient) { };	// Do all required process for monitoring

	CN *GetMonitee(CClient *pClient);	// Get monitee coresspond to pClient

	static unsigned int __stdcall ThreadFunc_m(void *Param);
};

template <typename CN> 
CMonitor<CN>::CMonitor(int iMaxClientReadBuffer, int iMaxClientWriteBuffer, int cdelay, int wdelay, int delay, int nMaxClients, int timer, FILE *fp)
: CServer<CClient>(iMaxClientReadBuffer, iMaxClientWriteBuffer, 0, delay, nMaxClients, timer, fp)
{
	m_cdelay = cdelay;
	m_wdelay = wdelay;
	m_hThread_m = m_hStopEvent_m = NULL;
	m_Monitees.clear();
}

template <typename CN> 
CMonitor<CN>::~CMonitor()
{
	StopMonitoring();

	map <LONGLONG, CN *>::iterator I;

	I = m_Monitees.begin();

	while( m_Monitees.end() != I )
	{
		delete (*I).second;
		I++;
	}
	m_Monitees.clear();
}

template <typename CN> 
bool CMonitor<CN>::AddClient(SOCKET socket)
{
	bool bRet;

	Lock();
	bRet = CServer<CClient>::AddClient(socket);
	Unlock();

	if( bRet ) log("Add - Number of users : %d\n", m_ClientList.size());
	return bRet;
}

template <typename CN> 
bool CMonitor<CN>::RemoveClient(unsigned long ulKey)
{
	bool bRet;
	Lock();

	map <unsigned int, CClient *>::iterator I;
	I = m_ClientList.find(ulKey);	
	if( m_ClientList.end() != I ) 
	{
		CN *pConn = GetMonitee((*I).second);
		if( NULL != pConn )	pConn->SetConnected(false);
	}

	bRet = CServer<CClient>::RemoveClient(ulKey);
	Unlock();

	return bRet;
}

template <typename CN> 
bool CMonitor<CN>::LoadMonitees(char *szFile)
{
	FILE *fp = fopen(szFile, "rt");

	if( NULL == fp ) return false;

	CN *pc;
	SOCKADDR_IN addr;
	unsigned short uiPort;
	char buffer[32];
	LONGLONG key;
	unsigned char *pk = (unsigned char *)&key;

	while( !feof(fp) )
	{
		pc = new CN();
		fscanf(fp, "%s %hd\n", buffer, &uiPort);
		addr.sin_addr.S_un.S_addr = inet_addr(buffer);
		addr.sin_port = uiPort;	
		pc->SetAddr(&addr);
		key = MakeKey(addr);

		m_Monitees.insert(map<LONGLONG, CN *>::value_type(key, pc));
	}

	fclose(fp);
	return true;
}

template <typename CN> 
bool CMonitor<CN>::StartMonitoring()
{
	m_hStopEvent_m = CreateEvent(NULL, TRUE, FALSE, NULL);
	if( !m_hStopEvent_m ) return false;

	m_hThread_m = (HANDLE)_beginthreadex(NULL, 
		0, 
		ThreadFunc_m, 
		this, 0, 
		NULL);
	if( !m_hThread_m ) return false;

	return true;
}

template <typename CN> 
bool CMonitor<CN>::StopMonitoring()
{
	if( NULL == m_hThread_m || NULL == m_hStopEvent_m ) return false;
	SetEvent(m_hStopEvent_m);
	WaitForSingleObject(m_hThread_m, INFINITE);

	return true;
}

template <typename CN> 
void CMonitor<CN>::ParserMain(shared_ptr<CClient> pClient)
{
	Lock();
	ParserMain_Safe(pClient);
	Unlock();
}

template <typename CN> 
LONGLONG CMonitor<CN>::MakeKey(SOCKADDR_IN addr)
{
	LONGLONG key;
	unsigned char *pk = (unsigned char *)&key;

	memcpy(pk, &addr.sin_addr.S_un.S_addr, sizeof(unsigned long));
	memcpy(pk+4, &addr.sin_port, sizeof(unsigned short));
	memset(pk+6, 0, 2);

	return key;
}

template <typename CN> 
CN *CMonitor<CN>::GetMonitee(CClient *pClient)
{
	LONGLONG key = MakeKey(pClient->m_addr);
	unsigned char *pk = (unsigned char *)&key;

	map <LONGLONG, CN *>::iterator I;

	I = m_Monitees.find(key);
	if( m_Monitees.end() != I ) 
		return (*I).second;
	else
		return NULL;
}

template <typename CN> 
unsigned int __stdcall CMonitor<CN>::ThreadFunc_m(void *Param)
{
	CMonitor *pMonitor = (CMonitor *)Param;
	DWORD dwRet;	

	map <LONGLONG, CN *>::iterator I;
	CN *pConnection;
	CClient *pClient;
	SOCKADDR_IN addr;
	char szAddr[32];
	DWORD dwStart, dwEnd;

	while( true )
	{
		dwStart = GetTickCount();
		dwRet = WaitForSingleObject(pMonitor->m_hStopEvent_m, 0);
		if( WAIT_OBJECT_0 == dwRet ) break;

		I = pMonitor->m_Monitees.begin();
		while( pMonitor->m_Monitees.end() != I )
		{
			pConnection = (*I).second;
			if( !pConnection->GetConnected() )
			{
				pConnection->GetAddr(&addr);
				strcpy(szAddr, inet_ntoa(addr.sin_addr));
				pMonitor->Lock();
				if( pMonitor->Connect(szAddr, addr.sin_port, pMonitor->m_cdelay) )
				{
					pConnection->SetConnected(true);
					pConnection->SetKey(pMonitor->m_ulCurrKey-1);
					pClient = pMonitor->GetClient(pConnection->GetKey());
					pMonitor->SetMonitoring(pClient);
				}
				pMonitor->Unlock();
			}
			I++;
		}
		dwEnd = GetTickCount();

		if( (int)(dwEnd - dwStart) < pMonitor->m_wdelay )
			Sleep( pMonitor->m_wdelay - (dwEnd - dwStart) );
	}
	return 0;
}

#endif

