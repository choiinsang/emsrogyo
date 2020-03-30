#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>

#include "CThreadPool.h"

#define MAX_THREADS	10


class CMyThread : public CThread<int>
{
public:
	CMyThread(unsigned long identifier, shared_ptr<pthread_mutex_t> pMutex, shared_ptr<pthread_cond_t>pCond, shared_ptr< queue < shared_ptr < int > > > pQueue);
	void processRequest(shared_ptr<int> pRequest);
};

CMyThread::CMyThread(unsigned long identifier, shared_ptr<pthread_mutex_t> pMutex, shared_ptr<pthread_cond_t>pCond, shared_ptr< queue < shared_ptr < int > > > pQueue)
: CThread<int>(identifier, pMutex, pCond, pQueue)
{
}

void CMyThread::processRequest(shared_ptr<int> pRequest)
{
	int wait = rand() % 10;
	printf("I'm %d - processing (%d), wait for %d sec \n", m_identifier, *pRequest, wait);
	sleep(wait);	
}


int main()
{
	shared_ptr< CThreadPool<int, CMyThread> > tp = (shared_ptr< CThreadPool<int, CMyThread> >)
		(new CThreadPool<int, CMyThread>(MAX_THREADS));

	int wait = 0;
	shared_ptr<int> pi;
	int count = 0;
	srand(time(NULL));

	for(;;)
	{
		tp->Lock();
		pi = shared_ptr<int>(new int(count++));
		tp->getQueue()->push(pi);
		tp->setSignal();
		tp->Unlock();
		wait = rand() % 100;
		printf("main - push %d, wait for %.2f sec\n", *pi, (double)wait / 100);
		usleep(wait * 10000);
	}
}

/*
int main()
{
shared_ptr< CThreadPool<int, CMyThread> > tp = (shared_ptr< CThreadPool<int, CMyThread> >)
(new CThreadPool<int, CMyThread>(MAX_THREADS));

int wait = 0;
shared_ptr<int> pi;
int count = 0;
srand(time(NULL));

for(;;)
{
tp->Lock();
pi = shared_ptr<int>(new int(count++));
tp->getQueue()->push(pi);
tp->setSignal();
tp->Unlock();
wait = rand() % 100;
printf("main - push %d, wait for %.2f sec\n", *pi, (double)wait / 100);
usleep(wait * 10000);
}

}
*/
