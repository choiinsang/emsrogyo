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
 *   CMultiServer 생성자 
 *-----------------------------------------------------------------------------
 * PARAMETERS
 *   iMaxClientReadBuffer  : [IN] 수신버퍼
 *   iMaxClientWriteBuffer : [IN] 송신버퍼
 *   iPort                 : [IN] 서버 Port
 *   delay                 : [IN] 이벤트 대기 시간
 *   nMaxClients           : [IN] 서버에 연결되는 최대 Client수
 *   timer                 : [IN] Timer Function의 동작 시간
 *   nWorkers              : [IN] 생성할 쓰레드 개수
 *   fp                    : [IN] LOG 파일을 저장하기 위한 파일 핸들러
 *-----------------------------------------------------------------------------
 * SEE
 *   none
 *-----------------------------------------------------------------------------
 * RETURN VALUE
 *   none
 *-----------------------------------------------------------------------------
 * SIDE EFFECTS
 *   CRemoveList 를 생성한다.
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
 *   thread pool에 Request 추가
 *-----------------------------------------------------------------------------
 * PARAMETERS
 *   pClient  : [IN] 작업이 들어온 Client
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
 *   thread pool에 Request 추가
 *-----------------------------------------------------------------------------
 * PARAMETERS
 *   none
 *-----------------------------------------------------------------------------
 * SEE
 *   none
 *-----------------------------------------------------------------------------
 * RETURN VALUE
 *   shared_ptr< CThreadPool< R, T > > : ThreadPool 에 대한 핸들러
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

