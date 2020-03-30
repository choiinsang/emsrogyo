#include "LockObject.h"

#ifdef WIN32
LockObject::LockObject()
{
	InitializeCriticalSection(&m_cs);
}

LockObject::~LockObject()
{
	DeleteCriticalSection(&m_cs);
}

void LockObject::Lock()
{
	EnterCriticalSection(&m_cs);
}

void LockObject::Unlock()
{
	LeaveCriticalSection(&m_cs);
}
#endif

#ifdef __LINUX__
LockObject::LockObject()
{
	pthread_mutex_init(&m_cs, NULL);
}

LockObject::~LockObject()
{
	pthread_mutex_destroy(&m_cs);
}

/*
 * FUNCTION: Lock
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   다른 쓰레드의 진입을 막는다.
 *-----------------------------------------------------------------------------
 * PARAMETERS
 *   none
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
void LockObject::Lock()
{
	pthread_mutex_lock(&m_cs);
}

/*
 * FUNCTION: Unlock
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   다른 쓰레드의 진입을 허용한다.
 *-----------------------------------------------------------------------------
 * PARAMETERS
 *   none
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
void LockObject::Unlock()
{
	pthread_mutex_unlock(&m_cs);
}
#endif
