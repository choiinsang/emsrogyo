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
 *   CThread�� ������
 *-----------------------------------------------------------------------------
 * PARAMETERS
 *   identifier : [IN] ������ Client�� ����Ű��
 *   pMutex     : [IN] �� ������ ���� ������ mutex
 *   pCond      : [IN] Commend�� �������� �˸��� Event
 *   pQueue     : [IN] Request �۾��� ��Ƶδ� Queue
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
 *   �����带 �����Ѵ�.
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
 *   �����带 �ߴ��Ѵ�.
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
 *   �ٸ� �����带 ���� ����ȭ�� ��Ų��.
 *-----------------------------------------------------------------------------
 * SIDE EFFECTS
 *   Ÿ ��������� �ش���� ������ ���´�.
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
 *   �ٸ� �����带 Ǯ�� ����ȭ�� ���� ��Ų��.
 *-----------------------------------------------------------------------------
 * SIDE EFFECTS
 *   Ÿ ��������� �ش���� ������ ����Ѵ�.
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
 *   �����忡 Request(JOB)�� �߰��Ѵ�.
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
 *   �����带 ����
 *   addQueue �� ���� Request�� ��ϵ��� event�� ȣ��Ǹ�
 *   processRequest�� �����Ͽ� �ش� JOB�� ó���ȴ�.
 *-----------------------------------------------------------------------------
 * PARAMETERS
 *   Param : [IN] ������ ���۽� �Ѿ���� parameter
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

			//if (!pRequest->getClient()->m_bRemove) //�Ϻ� �������� �ּ�ó�� �ؾ��� //gogosi
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
