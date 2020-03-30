#ifndef __CCLIENT__
#define __CCLIENT__

#include <sys/time.h>

#include <string>
using namespace std;

#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
using namespace boost;

#include "LockObject.h"
#include "CRemoveList.h"
#include "CSendQueue.h"

#ifndef	SOCKET
#define	SOCKET	int
#endif

class CClientUdp : public LockObject
{
public:
	string m_IP;
	int m_Port;
	string m_strKey;
	int m_pipe;

	bool	m_bRemove;
	shared_ptr<CRemoveList <string> > m_pRemoveList;
	shared_ptr<CSendQueue > m_pQueue;
	struct timeval m_lastAccessed;
	
public:
	CClientUdp(string IP, int Port, int pipe, shared_ptr<CRemoveList <string> > pRemoveList, shared_ptr<CSendQueue> pQueue);
	virtual ~CClientUdp();
	bool Send(void *buf,int len);

	string GetIP()		{ return m_IP; };
	int GetPort()		{ return m_Port; };

	void RemoveClient();
	virtual bool CheckRemoveClient()	{	return false; 	}

	void CancelSelect();
	void UpdateAccess() 	{ gettimeofday(&m_lastAccessed, NULL); };

	string GetKey()		{ return m_strKey; };
};

#endif
