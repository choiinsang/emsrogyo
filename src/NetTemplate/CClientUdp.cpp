#include "CClientUdp.h"

#include <unistd.h>
#include <string.h>
#include <boost/lexical_cast.hpp>
using namespace boost;

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
CClientUdp::CClientUdp(string IP, int Port, int pipe, shared_ptr<CRemoveList <string> > pRemoveList, shared_ptr<CSendQueue> pQueue)
{
	m_IP = IP;
	m_Port = Port;

	m_strKey = m_IP + ":" + lexical_cast<string>(m_Port);

	m_pipe = pipe;
	m_pRemoveList = pRemoveList;
	m_pQueue = pQueue;
	m_bRemove = false;

	UpdateAccess();
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

CClientUdp::~CClientUdp()
{
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
bool CClientUdp::Send(void *buf,int len)
{
	m_pQueue->AddQueue(m_IP, m_Port, (char *)buf, len);
	UpdateAccess();
	CancelSelect();
	return true;
}

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
void CClientUdp::RemoveClient()
{
        Lock();
        m_bRemove = true;
        Unlock();

        string strKey = m_IP + ":" + lexical_cast<string>(m_Port);
	m_pRemoveList->AddRemoveList(strKey);
}

void CClientUdp::CancelSelect()
{
	write(m_pipe, "C", 1);
}
