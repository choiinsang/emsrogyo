#include "CClientEpoll.h"

#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>

#include "Log.h"
#include "sth_syslog.h"

#ifndef NULL
#define	NULL	0
#endif

/*
 * FUNCTION: CClientEpoll
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   CClientEpoll ������ 
 *-----------------------------------------------------------------------------
 * PARAMETERS
 *   ulKey           : [IN] Client�� ����Ű��
 *   iMaxReadBuffer  : [IN] ���Ź���
 *   iMaxWriteBuffer : [IN] �۽Ź���
 *   epoll_fd        : [IN] Epoll Cotorol�� ���� �ڵ鷯
 *   pRemoveList     : [IN] Client�� ���� �ϱ� ���� List
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
CClientEpoll::CClientEpoll(unsigned long ulKey, int iMaxReadBuffer, int iMaxWriteBuffer, int epoll_fd, shared_ptr<CRemoveList <unsigned long> > pRemoveList)
{
	pLog = &CLog::GetInstance(); 
	//SM_N pLog->Write("N F[CClientEpoll::CClientEpoll] S[CClientEpoll()]");

	m_ulKey = ulKey;
	m_iMaxReadBuffer = iMaxReadBuffer;
	m_iMaxWriteBuffer = iMaxWriteBuffer; 

	m_ReadBuffer = shared_array< char >( new char[m_iMaxReadBuffer] );
	m_WriteBuffer = shared_array< char >( new char[m_iMaxWriteBuffer] );

	memset(m_ReadBuffer.get(), 0, sizeof(m_iMaxReadBuffer));
	memset(m_WriteBuffer.get(), 0, sizeof(m_iMaxWriteBuffer));

	m_iReadBuffer = m_iWriteBuffer = 0;
	m_epoll_fd = epoll_fd;
	
	m_pRemoveList = pRemoveList;

	m_socket = -1;
	m_bRemove = false;

	mIs_ItoSconnect = CONNECT_CtoI;
	mIs_Connected = false;

	m_IsHelloEnd = false;
}

/*
 * FUNCTION: ~CClientEpoll
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   CClientEpoll �Ҹ���
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
 *   Client�� ������ �ݴ´�.
 *-----------------------------------------------------------------------------
 * Author
 *   Bard
 *-----------------------------------------------------------------------------
 * History
 *   2008. 06. ?? : Created
 */
CClientEpoll::~CClientEpoll()
{
	//SM_N pLog->Write("N F[CClientEpoll::~CClientEpoll] S[~CClientEpoll()] V[%ld]", m_ulKey);

	if( m_socket >= 0 )	
	{
		close(m_socket);
		m_socket = 0;
	}
}

/*
 * FUNCTION: Send
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   Client �� �����͸� �����Ѵ�
 *-----------------------------------------------------------------------------
 * PARAMETERS
 *   buf : [IN] �ش� Client�� ���� ������
 *   len : [IN] ���� �������� ũ��
 *-----------------------------------------------------------------------------
 * SEE
 *   none
 *-----------------------------------------------------------------------------
 * RETURN VALUE
 *   true  : Client�� ���ۿ� �����͸� ���������� �������
 *   false : ���۰� �� ã�ų� ����� ����� �����͸� ���� ������ ���
 *-----------------------------------------------------------------------------
 * SIDE EFFECTS
 *   Client�� ���� �����͸� ��´�.
 *-----------------------------------------------------------------------------
 * Author
 *   Bard
 *-----------------------------------------------------------------------------
 * History
 *   2008. 06. ?? : Created
 */
bool CClientEpoll::Send(void *buf,int len)
{
	if( 0 > m_socket ) return false;
	if( m_iWriteBuffer + len > m_iMaxWriteBuffer )	
	{
		return false;
	}

	Lock();
	memcpy(&m_WriteBuffer[m_iWriteBuffer], buf, len);
	m_iWriteBuffer += len;
	Unlock();

	struct epoll_event ee;
	ee.events = EPOLLIN | EPOLLOUT | EPOLLERR;
	ee.data.fd = m_socket;

	if( 0 != epoll_ctl( m_epoll_fd, EPOLL_CTL_MOD, m_socket, &ee) )
		return false;

	return true;
}

/*
 * FUNCTION: RemoveClient
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   �� Client�� �����Ѵ�.
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
void CClientEpoll::RemoveClient()
{
	//SM_N pLog->Write("N F[CClientEpoll::RemoveClient] V[%ld]", m_ulKey);

	Lock();
	//=>hsj
	if(m_bRemove)
	{
		Unlock();
		return;
	}
	//<=
	m_bRemove = true;
	Unlock();
	m_pRemoveList->AddRemoveList(m_ulKey);
}
