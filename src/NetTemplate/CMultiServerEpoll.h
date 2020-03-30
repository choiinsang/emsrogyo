#ifndef __CMULTISERVER_EPOLL__
#define __CMULTISERVER_EPOLL__

#include "CServerEpoll.h"
#include "CThreadPool.h"

#include "Log.h"
#include "sth_syslog.h"

template <typename C, typename R, typename T>	// Client, Request, Thread
class CMultiServer : public CServer<C>
{
private:
    int m_nWorkers;
    shared_ptr< CThreadPool< R, T > > m_pThreadPool;

	CLog* pLog;
public:
    CMultiServer(int iMaxClientReadBuffer, int iMaxClientWriteBuffer, int iPort, int delay, int nMaxClients, int timer, int nWorkers, FILE *fp);
    virtual ~CMultiServer() {};
    virtual void ParserMain( shared_ptr<C> pClient );
	virtual void ParserMain( shared_ptr<C> pClient, unsigned long _ulSize);
    shared_ptr< CThreadPool< R, T > > GetThreadPool();
};

/*
 * FUNCTION: CMultiServer
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   CMultiServer ������ 
 *-----------------------------------------------------------------------------
 * PARAMETERS
 *   iMaxClientReadBuffer  : [IN] ���Ź���
 *   iMaxClientWriteBuffer : [IN] �۽Ź���
 *   iPort                 : [IN] ���� Port
 *   delay                 : [IN] �̺�Ʈ ��� �ð�
 *   nMaxClients           : [IN] ������ ����Ǵ� �ִ� Client��
 *   timer                 : [IN] Timer Function�� ���� �ð�
 *   nWorkers              : [IN] ������ ������ ����
 *   fp                    : [IN] LOG ������ �����ϱ� ���� ���� �ڵ鷯
 *-----------------------------------------------------------------------------
 * SEE
 *   none
 *-----------------------------------------------------------------------------
 * RETURN VALUE
 *   none
 *-----------------------------------------------------------------------------
 * SIDE EFFECTS
 *   CRemoveList �� �����Ѵ�.
 *-----------------------------------------------------------------------------
 * Author
 *   Bard
 *-----------------------------------------------------------------------------
 * History
 *   2008. 06. ?? : Created
 */
template <typename C, typename R, typename T>	
CMultiServer<C, R, T>::CMultiServer(int iMaxClientReadBuffer, int iMaxClientWriteBuffer, int iPort, int delay, int nMaxClients, int timer, int nWorkers, FILE *fp)
: CServer<C>(iMaxClientReadBuffer, iMaxClientWriteBuffer, iPort, delay, nMaxClients, timer, fp)
{
	//Trace(TR_INFO, "QQQQQQQQQQQQQQQQQQQQQ");
	pLog = &CLog::GetInstance(); 

    m_nWorkers = nWorkers;
    m_pThreadPool = shared_ptr< CThreadPool< R, T > >(new CThreadPool < R, T > (m_nWorkers) );
}

/*
 * FUNCTION: ParserMain
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   thread pool�� Request �߰�
 *-----------------------------------------------------------------------------
 * PARAMETERS
 *   pClient  : [IN] �۾��� ���� Client
 *-----------------------------------------------------------------------------
 * SEE
 *   none
 *-----------------------------------------------------------------------------
 * RETURN VALUE
 *   none
 *-----------------------------------------------------------------------------
 * SIDE EFFECTS
 *   none
 *-----------------------------------------------------------------------------
 * Author
 *   Bard
 *-----------------------------------------------------------------------------
 * History
 *   2008. 06. ?? : Created
 */
template <typename C, typename R, typename T>	
void CMultiServer<C, R, T>::ParserMain( shared_ptr<C> pClient )
{
	//SM_N pLog->Write("N F[CMultiServer<C, R, T>::ParserMain]");

	/*
	{//hsj
		shared_ptr<R> pRequest = shared_ptr<R>(new R);

		pRequest->setPacket((char *)pClient->m_ReadBuffer.get(), strlen(pClient->m_ReadBuffer.get()));
		pRequest->setClient(pClient);

		m_pThreadPool->addQueue(pRequest);
		return;
	}
	*/

    PACKET_HEADER *pph = (PACKET_HEADER *)pClient->m_ReadBuffer.get();
    shared_ptr<R> pRequest = shared_ptr<R>(new R);

    pRequest->setPacket((char *)pph, ntohl(pph->ulSize));
    pRequest->setClient(pClient);

    m_pThreadPool->addQueue(pRequest);
}

template <typename C, typename R, typename T>	
void CMultiServer<C, R, T>::ParserMain( shared_ptr<C> pClient, unsigned long _ulSize )
{
	//SM_N pLog->Write("N F[CMultiServer<C, R, T>::ParserMain]");

	shared_ptr<R> pRequest = shared_ptr<R>(new R);

	pRequest->setPacket((char *)pClient->m_ReadBuffer.get(), _ulSize);
	pRequest->setClient(pClient);

	m_pThreadPool->addQueue(pRequest);

	/*
	PACKET_HEADER *pph = (PACKET_HEADER *)pClient->m_ReadBuffer.get();
	shared_ptr<R> pRequest = shared_ptr<R>(new R);

	pRequest->setPacket((char *)pph, ntohl(pph->ulSize));
	pRequest->setClient(pClient);

	m_pThreadPool->addQueue(pRequest);
	*/
}

/*
 * FUNCTION: GetThreadPool
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   thread pool�� Request �߰�
 *-----------------------------------------------------------------------------
 * PARAMETERS
 *   none
 *-----------------------------------------------------------------------------
 * SEE
 *   none
 *-----------------------------------------------------------------------------
 * RETURN VALUE
 *   shared_ptr< CThreadPool< R, T > > : ThreadPool �� ���� �ڵ鷯
 *-----------------------------------------------------------------------------
 * SIDE EFFECTS
 *   none
 *-----------------------------------------------------------------------------
 * Author
 *   Bard
 *-----------------------------------------------------------------------------
 * History
 *   2008. 06. ?? : Created
 */
template <typename C, typename R, typename T>
shared_ptr< CThreadPool< R, T > > CMultiServer<C, R, T>::GetThreadPool()
{
    return m_pThreadPool;
};

#endif

