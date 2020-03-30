#ifndef __CMYTHREAD__
#define __CMYTHREAD__

#include "CMyRequest.h"

class CMyThread : public CThread<CMyRequest>
{
public:
        CMyThread(unsigned long identifier, shared_ptr<pthread_mutex_t> pMutex, shared_ptr<pthread_cond_t>pCond, shared_ptr< queue < shared_ptr < CMyRequest > > > pQueue);
        void processRequest(shared_ptr<CMyRequest> pRequest);
};

#endif

