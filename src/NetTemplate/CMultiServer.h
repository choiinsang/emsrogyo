#ifndef __CMULTISERVER__
#define __CMULTISERVER__

#include "CServer.h"
#include "CThreadPool.h"

template <typename C, typename R, typename T>	// Client, Request, Thread
class CMultiServer : public CServer<C>
{
private:
	int m_nWorkers;
	shared_ptr< CThreadPool< R, T > > m_pThreadPool;

public:
	CMultiServer(int iMaxClientReadBuffer, int iMaxClientWriteBuffer, int iPort, int delay, int nMaxClients, int timer, int nWorkers, FILE *fp);
	~CMultiServer() {};
	virtual void ParserMain( shared_ptr<C> pClient );
	shared_ptr< CThreadPool< R, T > > GetThreadPool();
};

template <typename C, typename R, typename T>	
CMultiServer<C, R, T>::CMultiServer(int iMaxClientReadBuffer, int iMaxClientWriteBuffer, int iPort, int delay, int nMaxClients, int timer, int nWorkers, FILE *fp)
: CServer<C>(iMaxClientReadBuffer, iMaxClientWriteBuffer, iPort, delay, nMaxClients, timer, fp)
{
	m_nWorkers = nWorkers;
	m_pThreadPool = shared_ptr< CThreadPool< R, T > >(new CThreadPool < R, T > (m_nWorkers) );
}

template <typename C, typename R, typename T>	
void CMultiServer<C, R, T>::ParserMain( shared_ptr<C> pClient )
{
	PACKET_HEADER *pph = (PACKET_HEADER *)pClient->m_ReadBuffer.get();
	shared_ptr<R> pRequest = shared_ptr<R>(new R);

	pRequest->setPacket((char *)pph, ntohl(pph->ulSize));
	pRequest->setClient(pClient);

	m_pThreadPool->addQueue(pRequest);
}

template <typename C, typename R, typename T>
shared_ptr< CThreadPool< R, T > > CMultiServer<C, R, T>::GetThreadPool()
{
	return m_pThreadPool;
};

#endif

