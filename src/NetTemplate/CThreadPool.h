#ifndef __CTHREAD_POOL__
#define __CTHREAD_POOL__

#include "CThread.h"
#include <boost/shared_array.hpp>
using namespace boost;

template<typename R, typename T>
class CThreadPool
{
private:
	shared_array< shared_ptr < T > > m_ppThreads;
	shared_ptr<pthread_mutex_t> m_pMutex;
	shared_ptr<pthread_cond_t> m_pCond;
	shared_ptr < queue < shared_ptr < R > > > m_pQueue;

	int m_nThreads;

public:
	CThreadPool(int nThreads);
	virtual ~CThreadPool();

	void Lock();
	void Unlock();
	
	void setSignal() 
	{
		pthread_cond_signal(m_pCond.get()); 
	};

	shared_ptr< queue < shared_ptr < R > > > getQueue() 
	{
		return m_pQueue; 
	};

	shared_ptr<T> getThread(int i);

	void addQueue(shared_ptr<R> pRequest);
};

/*
 * PARAMETERS
 *   nThreads : [IN] ������ ������ ����
 *-----------------------------------------------------------------------------
 * SIDE EFFECTS
 *   nThreads ����ŭ �����尡 �����ȴ�.
 */
template<typename R, typename T>
CThreadPool<R,T>::CThreadPool(int nThreads): m_nThreads(nThreads)
{
	m_pMutex = shared_ptr<pthread_mutex_t>(new pthread_mutex_t);
	pthread_mutex_init(m_pMutex.get(), NULL);
	m_pCond = shared_ptr<pthread_cond_t>(new pthread_cond_t);
	pthread_cond_init(m_pCond.get(), NULL);

	m_pQueue = shared_ptr < queue < shared_ptr < R > > > ( new queue < shared_ptr < R > > );
	
	m_ppThreads = shared_array< shared_ptr < T > >( new shared_ptr<T>[m_nThreads] );

	for(int i=0; i<m_nThreads; i++)
	{
		m_ppThreads[i] = shared_ptr<T>( new T(i, m_pMutex, m_pCond, m_pQueue) );
		m_ppThreads[i]->startThread();
	}	
}

/*
 *-----------------------------------------------------------------------------
 * SIDE EFFECTS
 *   ��� �����带 �����Ų��.
 *-----------------------------------------------------------------------------
 */
template<typename R, typename T>
CThreadPool<R,T>::~CThreadPool()
{
    for(int i=0; i<m_nThreads; i++)
    {
        m_ppThreads[i]->stopThread();
    }

	pthread_cond_destroy(m_pCond.get());
	pthread_mutex_destroy(m_pMutex.get());
}

/*
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   �ٸ� �����带 ���� ����ȭ�� ��Ų��.
 *-----------------------------------------------------------------------------
 * SIDE EFFECTS
 *   Ÿ ��������� �ش���� ������ ���´�.
 *-----------------------------------------------------------------------------
 */
template<typename R, typename T>
void CThreadPool<R,T>::Lock()
{
	pthread_mutex_lock(m_pMutex.get());
}

/*
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   �ٸ� �����带 Ǯ�� ����ȭ�� ���� ��Ų��.
 *-----------------------------------------------------------------------------
 * SIDE EFFECTS
 *   Ÿ ��������� �ش���� ������ ����Ѵ�.
 *-----------------------------------------------------------------------------
 */
template<typename R, typename T>
void CThreadPool<R,T>::Unlock()
{
	pthread_mutex_unlock(m_pMutex.get());
}

/*
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   �����带 ���ϴ�.
 *-----------------------------------------------------------------------------
 */
template<typename R, typename T>
shared_ptr<T> CThreadPool<R,T>::getThread(int i)
{
	return m_ppThreads[i];
}

/*
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   Request �� ������Ǯ�� �ִ´�.
 *-----------------------------------------------------------------------------
 */
template<typename R, typename T>
void CThreadPool<R,T>::addQueue(shared_ptr<R> pRequest)
{
	Lock();
	m_pQueue->push(pRequest);	
	setSignal();
	Unlock();
}

#endif

