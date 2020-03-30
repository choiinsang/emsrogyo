#ifndef __CTHREAD__
#define	__CTHREAD__

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <queue>
#include <boost/shared_ptr.hpp>
#include <sys/time.h>
using namespace std;
using namespace boost;

template<typename R>
class CThread
{
protected:
	pthread_t m_hThread;
	shared_ptr<pthread_mutex_t> m_pMutex;	// shared mutex	
	unsigned long m_identifier;
	
	shared_ptr<pthread_cond_t> m_pCond;	// shared conditional variable
	shared_ptr < queue < shared_ptr < R > > > m_pQueue;

	bool m_bStopThread;

public:

	CThread(unsigned long identifier, shared_ptr<pthread_mutex_t> pMutex, shared_ptr<pthread_cond_t> pCond, shared_ptr< queue < shared_ptr < R > > > pQueue);
	virtual ~CThread();

	shared_ptr< queue < shared_ptr < R > > > getQueue() { return m_pQueue; };

	void startThread();
	void stopThread();
	static void *ThreadFunc(void *Param);


	shared_ptr<pthread_mutex_t> getMutex() { return m_pMutex; };
	shared_ptr<pthread_cond_t> getCond() { return m_pCond; };

	bool getStopThread() { return m_bStopThread; };
	unsigned long getIdentifier() { return m_identifier; };
	
	void Lock();
	void Unlock();
	void setSignal() { pthread_cond_signal(m_pCond.get()); };

	void addQueue(shared_ptr<R> pRequest);
	virtual void processRequest(shared_ptr<R> pRequest) { };
	
	virtual bool preprocess() { return true; }; //Thread Preprocess add -by ischoi 150612

private:
	bool Process();
};

/*
 * FUNCTION: CThread
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   CThread의 생성자
 *-----------------------------------------------------------------------------
 * PARAMETERS
 *   identifier : [IN] 삭제할 Client의 고유키값
 *   pMutex     : [IN] 각 쓰레드 끼리 제어할 mutex
 *   pCond      : [IN] Commend가 들어왔음을 알리는 Event
 *   pQueue     : [IN] Request 작업을 담아두는 Queue
 *-----------------------------------------------------------------------------
 */
template<typename R>
CThread<R>::CThread(unsigned long identifier, shared_ptr<pthread_mutex_t> pMutex, shared_ptr<pthread_cond_t> pCond, shared_ptr < queue < shared_ptr < R > > > pQueue)
: m_pMutex(pMutex), m_identifier(identifier), m_pCond(pCond), m_pQueue(pQueue)
{
	m_bStopThread = false;
	m_hThread = (pthread_t )NULL;
}

template<typename R>
CThread<R>::~CThread()
{
	stopThread();
}

/*
 * FUNCTION: startThread
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   쓰레드를 실행한다.
 *-----------------------------------------------------------------------------
 */
template<typename R>
void CThread<R>::startThread()
{
	m_bStopThread = false;
	pthread_create(&m_hThread, NULL, ThreadFunc, this);
}

/*
 * FUNCTION: stopThread
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   쓰레드를 중단한다.
 *-----------------------------------------------------------------------------
 */
template<typename R>
void CThread<R>::stopThread()
{
	if( (pthread_t )NULL == m_hThread ) return;

	m_bStopThread = true;
	setSignal();
	pthread_join(m_hThread, NULL);
	m_hThread = (pthread_t)NULL;
}

/*
 * FUNCTION: Lock
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   다른 쓰레드를 묶어 동기화를 시킨다.
 *-----------------------------------------------------------------------------
 * SIDE EFFECTS
 *   타 쓰레드들의 해당평션 진입을 막는다.
 *-----------------------------------------------------------------------------
 */
template<typename R>
void CThread<R>::Lock()
{
	pthread_mutex_lock(m_pMutex.get());
}

/*
 * FUNCTION: Unlock
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   다른 쓰레드를 풀어 동기화를 해제 시킨다.
 *-----------------------------------------------------------------------------
 * SIDE EFFECTS
 *   타 쓰레드들의 해당평션 진입을 허용한다.
 *-----------------------------------------------------------------------------
 */
template<typename R>
void CThread<R>::Unlock()
{
	pthread_mutex_unlock(m_pMutex.get());
}

/*
 * FUNCTION: addQueue
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   쓰레드에 Request(JOB)을 추가한다.
 *-----------------------------------------------------------------------------
 */
template<typename R>
void CThread<R>::addQueue(shared_ptr<R> pRequest)
{
	Lock();
	m_pQueue->push(pRequest);
	Unlock();
	setSignal();
}

/*
 * FUNCTION: ThreadFunc
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   쓰레드를 돌림
 *   addQueue 에 의해 Request가 등록된후 event가 호출되면
 *   processRequest를 실행하여 해당 JOB이 처리된다.
 *-----------------------------------------------------------------------------
 * PARAMETERS
 *   Param : [IN] 쓰레드 시작시 넘어오는 parameter
 *-----------------------------------------------------------------------------
 */
template<typename R>
void *CThread<R>::ThreadFunc(void *Param)
{
	CThread *pThread = (CThread *)Param;

	while( true )
	{
		if(!pThread->Process()) 
			break;
  }
	return NULL;
}

template<typename R>
bool CThread<R>::Process()
{
	unsigned int rc = 0;
	shared_ptr<R> pRequest;
	struct timeval  now;
	struct timespec ts;
	bool bFound(false);

	if( getStopThread() ) 
		return false;

	if( !preprocess() )
		return true;
	
	pthread_mutex_lock(getMutex().get());
		
	rc = 0;
	gettimeofday(&now, NULL);
	//ts.tv_sec  = now.tv_sec + 10;
	ts.tv_sec  = now.tv_sec + 2;
	ts.tv_nsec = now.tv_usec * 1000;
	
	if( 0 == m_pQueue->size() ) 
		rc = pthread_cond_timedwait(getCond().get(), getMutex().get(), &ts);

	if( m_pQueue.get()->size() > 0 && 0 == rc ) 
	{
		bFound = false;

		while(m_pQueue.get()->size() > 0)
		{
			pRequest = m_pQueue->front();
			m_pQueue->pop();

			//if (!pRequest->getClient()->m_bRemove) //일부 예제에서 주석처리 해야함 //gogosi
			//{
			//	bFound = true;
			//	break;
			//}
			if (pRequest->getClient() != NULL) 
			{
				bFound = true;
				break;
			}
		}

		pthread_mutex_unlock(getMutex().get());

		if (bFound)
		{
			processRequest(pRequest);
		}
	}
	else
	{
		pthread_mutex_unlock(getMutex().get());
	}

	return true;
}

#endif
