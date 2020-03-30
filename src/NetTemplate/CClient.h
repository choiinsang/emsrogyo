#ifndef __CCLIENT__
#define __CCLIENT__

#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
using namespace boost;

#ifdef	WIN32
#include "winsock2_fd_size.h"
#endif

#include "LockObject.h"
#include "CRemoveList.h"

#ifndef	SOCKET
#define	SOCKET	int
#endif

class CClient : public LockObject
{
public:
	int		m_iMaxReadBuffer;
	int		m_iMaxWriteBuffer;
	int		m_iReadBuffer;
	int		m_iWriteBuffer;
	bool	m_bRemove;
	SOCKET	m_socket; 
	unsigned long m_ulKey;

	shared_ptr<CRemoveList <unsigned long> > m_pRemoveList;

#ifdef	__LINUX__
	int		m_pipe;		// pipe for canceling select()
#endif

public:
	shared_array< char > m_ReadBuffer; 
	shared_array< char > m_WriteBuffer;

public:
#ifdef	__LINUX__
	CClient(unsigned long ulKey, int iMaxReadBuffer, int iMaxWriteBuffer, int pipe, shared_ptr<CRemoveList <unsigned long> > pRemoveList);
	void CancelSelect();
#endif
#ifdef	WIN32
	CClient(unsigned long ulKey, int iMaxReadBuffer, int iMaxWriteBuffer, shared_ptr<CRemoveList <unsigned long> > pRemoveList);
#endif
	virtual ~CClient();
	bool Send(void *buf,int len);

	char* GetReadBuffer()	{	return m_ReadBuffer.get();	}
	unsigned long GetKey()	{	return m_ulKey;				}
	void RemoveClient();

	virtual bool CheckRemoveClient()	{	return false; 	}
};

#endif
