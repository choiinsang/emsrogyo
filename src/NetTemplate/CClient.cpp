#include "CClient.h"

#ifdef __LINUX__
#include <unistd.h>
#include <string.h>
#endif

#ifndef NULL
#define	NULL	0
#endif

/*
 * FUNCTION: CClient
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   CClientEpoll ������ 
 *-----------------------------------------------------------------------------
 * PARAMETERS
 *   ulKey           : [IN] Client�� ����Ű��
 *   iMaxReadBuffer  : [IN] ���Ź���
 *   iMaxWriteBuffer : [IN] �۽Ź���
 *   pipe            : [IN] Send �� ���� �ﰢ���� ������ ���� �߰�
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
#ifdef __LINUX__
CClient::CClient(unsigned long ulKey, int iMaxReadBuffer, int iMaxWriteBuffer, int pipe, shared_ptr<CRemoveList <unsigned long> > pRemoveList)
{
	m_pipe = pipe;
#else
CClient::CClient(unsigned long ulKey, int iMaxReadBuffer, int iMaxWriteBuffer, shared_ptr<CRemoveList <unsigned long> > pRemoveList)
{
#endif
	m_ulKey = ulKey;
	m_iMaxReadBuffer = iMaxReadBuffer;
	m_iMaxWriteBuffer = iMaxWriteBuffer; 

	m_ReadBuffer = shared_array< char >( new char[m_iMaxReadBuffer] );
	m_WriteBuffer = shared_array< char >( new char[m_iMaxWriteBuffer] );

	memset(m_ReadBuffer.get(), 0, sizeof(m_iMaxReadBuffer));
	memset(m_WriteBuffer.get(), 0, sizeof(m_iMaxWriteBuffer));

	m_iReadBuffer = m_iWriteBuffer = 0;

	m_pRemoveList = pRemoveList;

	m_socket = 0;
	m_bRemove = false;
}

#include <stdio.h>

/*
 * FUNCTION: ~CClient
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   CClient �Ҹ���
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
CClient::~CClient()
{
	if( m_socket )	{
#ifdef WIN32
		closesocket(m_socket);
#endif
#ifdef __LINUX__
		close(m_socket);
#endif
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
bool CClient::Send(void *buf,int len)
{
	//printf("1\n");

	if( 0 == m_socket ) 
		return false;

	//printf("2\n");

	if( m_iWriteBuffer + len > m_iMaxWriteBuffer )	
	{
		//printf("3\n");
		return false;
	}

	Lock();
	memcpy(&m_WriteBuffer[m_iWriteBuffer], buf, len);
	m_iWriteBuffer += len;
	Unlock();

	//printf("4\n");

	return true;
}

/*
 * FUNCTION: CancelSelect(Linux)
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   CServer�� Select�� �ﰢ���� ������ �ϵ��� �Ѵ�.
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
#ifdef	__LINUX__
void CClient::CancelSelect()
{
        write(m_pipe, "C", 1);
}
#endif

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
void CClient::RemoveClient()
{
        Lock();
        m_bRemove = true;
        Unlock();
        m_pRemoveList->AddRemoveList(m_ulKey);
}

