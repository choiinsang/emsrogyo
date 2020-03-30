#ifndef __CCLIENT_EPOLL__
#define __CCLIENT_EPOLL__

#include <string>
using namespace std;

#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
using namespace boost;

#include "Log.h"
#include "LockObject.h"
#include "CRemoveList.h"

#include "NetTemplateDefine.h"

#ifndef	SOCKET
#define	SOCKET	int
#endif



class CClientEpoll : public LockObject
{
public:
	int		m_iMaxReadBuffer;
	int		m_iMaxWriteBuffer;
	int		m_iReadBuffer;
	int		m_iWriteBuffer;
	bool	m_bRemove;
	SOCKET	m_socket; 
	unsigned long m_ulKey;

	bool	mIs_ItoSconnect;			//false:접속을 받은 상태, true:내가 접속한 상태
	bool	mIs_Connected;				//접속이 이뤄진 상태 
	struct timeval mtime_ConnectStart;	//접속 시작한 시간(ItoS인경우)

	int	m_epoll_fd;
	shared_ptr<CRemoveList <unsigned long> > m_pRemoveList;

	CLog* pLog;

public:
	shared_array< char > m_ReadBuffer; 
	shared_array< char > m_WriteBuffer;

	string m_IP;
	int m_nPort;

	bool m_IsHelloEnd;

public:
	CClientEpoll(unsigned long ulKey, int iMaxReadBuffer, int iMaxWriteBuffer, int epoll_fd, shared_ptr<CRemoveList <unsigned long> > pRemoveList);
	virtual ~CClientEpoll();
	bool Send(void *buf,int len);

	char* GetReadBuffer()	{	return m_ReadBuffer.get();	}
	unsigned long GetKey()	{	return m_ulKey;				}
	void RemoveClient();

	virtual bool CheckRemoveClient()	{	return false; 	}

	virtual void Connected() {}; //hsj
};

#endif
