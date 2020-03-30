#ifndef __LOCK_OBJECT__
#define __LOCK_OBJECT__

#ifdef WIN32
#include "windows.h"
#endif
#ifdef __LINUX__
#include <pthread.h>
#endif

class LockObject
{
private:
#ifdef WIN32
	CRITICAL_SECTION m_cs;
#endif
#ifdef __LINUX__
	pthread_mutex_t m_cs;
#endif

public:
	LockObject();
	virtual ~LockObject();

	void Lock();
	void Unlock();
};

#endif
