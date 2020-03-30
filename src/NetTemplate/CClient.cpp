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
 *   CClientEpoll 생성자 
 *-----------------------------------------------------------------------------
 * PARAMETERS
 *   ulKey           : [IN] Client의 고유키값
 *   iMaxReadBuffer  : [IN] 수신버퍼
 *   iMaxWriteBuffer : [IN] 송신버퍼
 *   pipe            : [IN] Send 에 대한 즉각적인 반응을 위해 추가
 *   pRemoveList     : [IN] Client를 제가 하기 위한 List
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
 *   CClient 소멸자
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
 *   Client의 소켓을 닫는다.
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
 *   Client 로 데이터를 전송한다
 *-----------------------------------------------------------------------------
 * PARAMETERS
 *   buf : [IN] 해당 Client로 보낼 데이터
 *   len : [IN] 보낼 데이터의 크기
 *-----------------------------------------------------------------------------
 * SEE
 *   none
 *-----------------------------------------------------------------------------
 * RETURN VALUE
 *   true  : Client의 버퍼에 데이터를 성공적으로 담았을때
 *   false : 버퍼가 꽉 찾거나 사이즈를 벋어나서 데이터를 담지 못했을 경우
 *-----------------------------------------------------------------------------
 * SIDE EFFECTS
 *   Client에 보낼 데이터를 담는다.
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
 *   CServer의 Select가 즉각적인 응답을 하도록 한다.
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
 *   현 Client를 제거한다.
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

