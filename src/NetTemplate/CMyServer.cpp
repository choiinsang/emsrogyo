#include "CMyServer.h"

CMyServer::CMyServer(int iMaxClientReadBuffer, int iMaxClientWriteBuffer, int iPort, int delay, int nMaxClients, int timer, int nWorkers, FILE *fp)
: CVanilaServer(iMaxClientReadBuffer, iMaxClientWriteBuffer, iPort, delay, nMaxClients, timer, nWorkers, fp)
{
}

void CMyServer::TimerFunc()
{
        printf("Timer function invoke. m_nCurrClients = %d\n", m_nCurrClients);
}

