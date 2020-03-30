#ifndef __CSERVER__
#define	__CSERVER__

#include <stdarg.h>

#ifdef	WIN32
#include <process.h>
#include <limits.h>
#include "winsock2_fd_size.h"
#pragma comment (lib,"winmm.lib")
#endif

#ifdef __LINUX__
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <syslog.h>
#include <stdarg.h>
#include <arpa/inet.h>
#include <signal.h>

#define	closesocket(s)	close(s)
#define	WSAGetLastError()	errno
#define	SOCKET_ERROR	-1	
extern int errno;

#endif

#include <boost/shared_ptr.hpp>

using namespace boost;

#ifndef	NULL
#define	NULL	0
#endif

#ifndef	SOCKET_
#define	SOCKET	int
#endif

#ifndef	USHORT
#define	USHORT	unsigned short int
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET	-1
#endif

#include "packet_header.h"

#ifdef WIN32
#pragma warning(disable:4786)
#endif

#include <map>
using namespace std;

#include "CRemoveList.h"

template <typename C> class CServer
{
public:
	int m_iMaxClientReadBuffer;	// Size of each client's read buffer
	int m_iMaxClientWriteBuffer;	// Size of each client's write buffer
	int m_nCurrClients;		// How many clients are connected
	unsigned long m_ulCurrKey;	// Current key to be assigned to the next connecting user
	int m_iPort;			// Port number
	int m_delay;			// delay for select()
	int m_nMaxClients;		// Max number of possible clients
	int m_timer;			// milliseconds of timer interval
	FILE *m_fp;

#ifdef	WIN32
	HANDLE	m_hThread;
	HANDLE  m_HAcceptThread;
	HANDLE	m_hStopEvent;
#endif

#ifdef	__LINUX__
	pthread_t m_Thread;
	pthread_t m_AcceptThread;
	bool	m_bStopThread;
#endif

private:
	SOCKET m_socket;
#ifdef WIN32
	unsigned long m_CurrTime;
	unsigned long m_LastTime;
#endif 
#ifdef __LINUX__
	struct timeval m_CurrTime;
	struct timeval m_LastTime;
#endif
	unsigned long m_TimeDiff;

	shared_ptr<CRemoveList> m_pRemoveList;

public:
	map<unsigned int, shared_ptr< C > > m_ClientList;
	CServer(int iMaxClientReadBuffer, int iMaxClientWriteBuffer, int iPort, int delay, int nMaxClients, int timer, FILE *fp);
	virtual ~CServer();

	bool InitNetwork();
	void CleanUpNetwork();

	bool Start();
	bool Stop();
	void CleanUp();

	virtual bool AddClient(SOCKET socket);
	virtual bool RemoveClient(unsigned long ulKey); 

	virtual bool Parser( shared_ptr< C > pClient );
	virtual void ParserMain(shared_ptr< C > pClient) {};
	virtual void TimerFunc()	{};
	shared_ptr<C> GetClient(unsigned long ulKey);
	bool Connect(char *szAddr, int iPort, int mdelay);

#ifdef	WIN32
	static unsigned int __stdcall ThreadFunc(void *Param);
	static unsigned int __stdcall AcceptThreadFunc(void *Param);
#endif
#ifdef	__LINUX__
	static void *ThreadFunc(void *Param);
	static void *AcceptThreadFunc(void *Param);
#endif

	void log(const char *fmt, ... );

private:

#ifdef __LINUX__
	int m_pipe[2];		// pipe for canceling select()
	void CancelSelect();

#endif
	void AcceptProcess();
	void Process();
	
};

/*
 * FUNCTION: CServer
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   CServer 생성자 
 *-----------------------------------------------------------------------------
 * PARAMETERS
 *   iMaxClientReadBuffer  : [IN] 수신버퍼
 *   iMaxClientWriteBuffer : [IN] 송신버퍼
 *   iPort                 : [IN] 서버 Port
 *   delay                 : [IN] 이벤트 대기 시간
 *   nMaxClients           : [IN] 서버에 연결되는 최대 Client수
 *   timer                 : [IN] Timer Function의 동작 시간
 *   fp                    : [IN] LOG 파일을 저장하기 위한 파일 핸들러
 *-----------------------------------------------------------------------------
 * SEE
 *   none
 *-----------------------------------------------------------------------------
 * RETURN VALUE
 *   none
 *-----------------------------------------------------------------------------
 * SIDE EFFECTS
 *   CRemoveList 를 생성한다.
 *-----------------------------------------------------------------------------
 * Author
 *   Bard
 *-----------------------------------------------------------------------------
 * History
 *   2008. 06. ?? : Created
 */
template <typename C>
CServer<C>::CServer(int iMaxClientReadBuffer, int iMaxClientWriteBuffer, int iPort, int delay, int nMaxClients, int timer, FILE *fp)
{
	m_nCurrClients = 0;
	m_ulCurrKey = 0;
	m_iMaxClientReadBuffer = iMaxClientReadBuffer;
	m_iMaxClientWriteBuffer = iMaxClientWriteBuffer;
	m_socket = 0;
	m_iPort = iPort;
	m_delay = delay;
	m_nMaxClients = nMaxClients;
	m_timer = timer;
	m_fp = fp;

#ifdef	WIN32
	m_hThread = m_hStopEvent = NULL;
#endif
#ifdef	__LINUX__
	m_Thread =(pthread_t)NULL;
	m_bStopThread = false;
#endif

#ifdef WIN32
	m_CurrTime = 0;
	m_LastTime = 0;
#endif
	m_TimeDiff = 0;

	m_pRemoveList = shared_ptr<CRemoveList>(new CRemoveList());

#ifdef	__LINUX__
	pipe(m_pipe);

	signal(SIGPIPE, SIG_IGN);
#endif
}

template <typename C>
CServer<C>::~CServer()
{
	CleanUp();
}

/*
 * FUNCTION: InitNetwork
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   윈속을 연다, 윈도우즈에서 만 쓰임
 *-----------------------------------------------------------------------------
 * PARAMETERS
 *   none
 *-----------------------------------------------------------------------------
 * SEE
 *   none
 *-----------------------------------------------------------------------------
 * RETURN VALUE
 *   항상 true
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
template <typename C>
bool CServer<C>::InitNetwork()
{
#ifdef	WIN32
	WSADATA     wd;
	if (WSAStartup(0x0202, &wd) != 0)
	{
		return false;
	}
#endif

	return true;
}
/*
 * FUNCTION: CleanUpNetwork
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   윈속을 닫는다, 윈도우즈에서 만 쓰임
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
template <typename C>
void CServer<C>::CleanUpNetwork()
{
#ifdef	WIN32
	WSACleanup();
#endif
}

/*
 * FUNCTION: Start
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   CServer 구동시킴
 *-----------------------------------------------------------------------------
 * PARAMETERS
 *   none
 *-----------------------------------------------------------------------------
 * SEE
 *   none
 *-----------------------------------------------------------------------------
 * RETURN VALUE
 *   항상 true
 *-----------------------------------------------------------------------------
 * SIDE EFFECTS
 *   실 Server 의 처리를 하는 Thread 실행
 *-----------------------------------------------------------------------------
 * Author
 *   Bard
 *-----------------------------------------------------------------------------
 * History
 *   2008. 06. ?? : Created
 */
template <typename C>
bool CServer<C>::Start()
{
#ifdef	WIN32
	m_hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if( !m_hStopEvent ) return false;

	m_ulCurrKey = 0;
	
	// accept thread
	m_hAcceptThread = (HANDLE)_beginthreadex(NULL, 
		0, 
		AcceptThreadFunc, 
		this, 0, 
		NULL);
	
	// io process thread
	m_hThread = (HANDLE)_beginthreadex(NULL, 
		0, 
		ThreadFunc, 
		this, 0, 
		NULL);
	if( !m_hThread || !m_hAcceptThread ) return false;
#endif
#ifdef	__LINUX__
	m_bStopThread = false;
	m_ulCurrKey = 0;
	pthread_create(&m_AcceptThread, NULL, AcceptThreadFunc, this);	
	pthread_create(&m_Thread, NULL, ThreadFunc, this);	
#endif
	return true;
}

/*
 * FUNCTION: Stop
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   CServer을 중단시킴
 *-----------------------------------------------------------------------------
 * PARAMETERS
 *   none
 *-----------------------------------------------------------------------------
 * SEE
 *   none
 *-----------------------------------------------------------------------------
 * RETURN VALUE
 *   항상 true
 *-----------------------------------------------------------------------------
 * SIDE EFFECTS
 *   실 Server 의 처리를 하는 Thread 를 중단
 *-----------------------------------------------------------------------------
 * Author
 *   Bard
 *-----------------------------------------------------------------------------
 * History
 *   2008. 06. ?? : Created
 */
template <typename C>
bool CServer<C>::Stop()
{
#ifdef	WIN32
	if( NULL == m_hThread || NULL == m_hStopEvent ) return false;
	SetEvent(m_hStopEvent);
	WaitForSingleObject(m_hAcceptThread, INFINITE);
	WaitForSingleObject(m_hThread, INFINITE);
#endif
#ifdef	__LINUX__
	if( (pthread_t)NULL == m_Thread ) return false;
	m_bStopThread = true;
	pthread_join(m_AcceptThread,NULL);
	pthread_join(m_Thread, NULL);
#endif
	return true;
}

template <typename C>
void CServer<C>::CleanUp()
{
#ifdef	__LINUX__
	close(m_pipe[1]);
	close(m_pipe[0]);
#endif
	m_ClientList.clear();
	CleanUpNetwork();
}

/*
 * FUNCTION: AddClient
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   서버에 클라이언트를 추가한다.
 *-----------------------------------------------------------------------------
 * PARAMETERS
 *   socket : [IN] 서버에 추가할 Client의 소켓
 *-----------------------------------------------------------------------------
 * SEE
 *   none
 *-----------------------------------------------------------------------------
 * RETURN VALUE
 *   항상 true
 *-----------------------------------------------------------------------------
 * SIDE EFFECTS
 *   m_ClientList에 클라이언트가 추가됨
 *-----------------------------------------------------------------------------
 * Author
 *   Bard
 *-----------------------------------------------------------------------------
 * History
 *   2008. 06. ?? : Created
 */
template <typename C>
bool CServer<C>::AddClient(SOCKET socket)
{
	typename map <unsigned int, shared_ptr< C > >::iterator R;

	R = m_ClientList.find(m_ulCurrKey);	
	while( m_ClientList.end() != R )	{
		m_ulCurrKey++;
		R = m_ClientList.find(m_ulCurrKey);	
	}

#ifdef	__LINUX__
	shared_ptr< C > pClient = shared_ptr< C >( new C(m_ulCurrKey, m_iMaxClientReadBuffer, m_iMaxClientWriteBuffer, m_pipe[1], m_pRemoveList ) );
#endif
#ifdef	WIN32
	shared_ptr< C > pClient = shared_ptr< C >( new C(m_ulCurrKey, m_iMaxClientReadBuffer, m_iMaxClientWriteBuffer, m_pRemoveList ) );
#endif
	pClient->m_socket = socket;

	m_ClientList.insert( typename map <unsigned int, shared_ptr< C > >::value_type(m_ulCurrKey, pClient));

	log("AddClient : key = %ld\n", pClient->m_ulKey);
	m_ulCurrKey++;		// If overflowed, it automatically returns to 0
	m_nCurrClients++;
	return true;
}

template <typename C>
bool CServer<C>::RemoveClient(unsigned long ulKey)
{
	typename map <unsigned int, shared_ptr< C > >::iterator R;

	R = m_ClientList.find(ulKey);	
	if( m_ClientList.end() == R ) return false;

	//shared_ptr<C> pClient = (*R).second;
	m_ClientList.erase(ulKey);
	m_nCurrClients--;
	log("Remove Client: key = %ld\n", ulKey);

	return true;
}

template <typename C>
bool CServer<C>::Parser(shared_ptr< C > pClient)
{
	//printf("%s\n", pClient->m_ReadBuffer.get()); // for telnet test
	if( pClient->m_iReadBuffer <= 0 ) return false;

	PACKET_HEADER *pph;
	unsigned long ulSize, ulType;

	while( (unsigned long)pClient->m_iReadBuffer >= sizeof(PACKET_HEADER) )	{
		if( pClient->m_bRemove ) break; // 클라이언트 처리 상태 변경 시 중지

		pph = (PACKET_HEADER *)pClient->m_ReadBuffer.get();
        ulSize = ntohl(pph->ulSize);
        ulType = ntohl(pph->ulType);
		if( (unsigned long)pClient->m_iReadBuffer < ulSize ) break;
		ParserMain(pClient);
		pClient->m_iReadBuffer -= ulSize;
		if( pClient->m_iReadBuffer > 0 )	{
			//memcpy(pClient->m_ReadBuffer, &pClient->m_ReadBuffer[ulSize], pClient->m_iReadBuffer);
			memcpy(pClient->m_ReadBuffer.get(), pClient->m_ReadBuffer.get()+ulSize, pClient->m_iReadBuffer);
		}
		if( pClient->m_iReadBuffer < 0 ) pClient->m_iReadBuffer = 0;
	}

	return true;
}

template <typename C>
void CServer<C>::log(const char *fmt, ... )
{
	va_list args;
	va_start(args, fmt);
	if( m_fp != NULL )
	{
		vfprintf(m_fp, fmt, args);
		fflush(m_fp);
	}
	else
	{
#ifdef __LINUX__
		vsyslog(LOG_DEBUG, fmt, args );
#endif

	}
	va_end(args);
}


#ifdef	WIN32
template <typename C>
unsigned int __stdcall CServer<C>::AcceptThreadFunc(void *Param)
#endif
#ifdef	__LINUX__
template <typename C>
void *CServer<C>::AcceptThreadFunc(void *Param)
#endif
{
	CServer<C> *pServer = (CServer <C> *)Param;

	// bind 
	char Option = 1;
#ifdef	WIN32
	const int iOption = 1;
#endif
	pServer->m_socket = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(pServer->m_socket, SOL_SOCKET, SO_REUSEADDR, &Option, sizeof(Option));

#ifdef	WIN32
	ioctlsocket(pServer->m_socket, FIONBIO, (u_long FAR *)&iOption);
#endif
#ifdef	__LINUX__
	fcntl(pServer->m_socket, F_SETFL, O_NONBLOCK);

    struct linger Linger;
    Linger.l_onoff  = 1;
    Linger.l_linger = 0;      /* 0 for abortive disconnect */
    setsockopt(pServer->m_socket, SOL_SOCKET, SO_LINGER, &Linger, sizeof(Linger));
#endif

	struct sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	sa.sin_port = htons((USHORT)pServer->m_iPort);

	pServer->log( "Port -> : %d\n", pServer->m_iPort);
	if ( 0 != ::bind(pServer->m_socket, (struct sockaddr*) &sa, sizeof(sa)))	{
#ifdef	WIN32
		pServer->log( "Error on bind() : %d\n", GetLastError());
		printf("Error on bind() : %d\n");
		return -1;
#endif
#ifdef	__LINUX__
		pServer->log( "Error on bind() : %d\n", errno);
		return NULL;
#endif
	}
	if ( 0 != listen(pServer->m_socket, SOMAXCONN))	{
#ifdef	WIN32
		pServer->log( "Error on listen() : %d\n", GetLastError());
		return -1;
#endif
#ifdef	__LINUX__
		pServer->log( "Error on listen() : %d\n", errno);
		return NULL;
#endif
	}

#ifdef	WIN32
	DWORD dwRet;
	while(true)	
	{
		dwRet = WaitForSingleObject(pServer->m_hStopEvent, 0);
		if( WAIT_OBJECT_0 == dwRet ) break;
#endif
#ifdef	__LINUX__
	while(!pServer->m_bStopThread)	{
#endif
		pServer->AcceptProcess();
	}

#ifdef	WIN32
	closesocket(pServer->m_socket);
#endif
#ifdef	__LINUX__
	close(pServer->m_socket);
#endif
	return 0;
}


#ifdef	WIN32
template <typename C>
unsigned int __stdcall CServer<C>::ThreadFunc(void *Param)
#endif
#ifdef	__LINUX__
template <typename C>
void *CServer<C>::ThreadFunc(void *Param)
#endif
{
	CServer<C> *pServer = (CServer <C> *)Param;
#ifdef	WIN32
	DWORD dwRet;
	while(true)	
	{
		dwRet = WaitForSingleObject(pServer->m_hStopEvent, 0);
		if( WAIT_OBJECT_0 == dwRet ) break;
#endif
#ifdef	__LINUX__
	while(!pServer->m_bStopThread)	{
#endif
		pServer->Process();
	}
	return 0;
}

#ifdef	__LINUX__
#define	PIPE_BUFFER_SIZE	1024
#endif

template <typename C>
void CServer<C>::AcceptProcess()
{
	int iRet;		// for select()
#ifdef WIN32
	struct sockaddr_in ca;
	int    nLength;
	int iOption = 1;
#endif
	fd_set readfds, exceptfds;
	struct timeval delay;
	SOCKET socket;
	
	FD_ZERO(&readfds);
	FD_ZERO(&exceptfds);

	FD_SET(m_socket, &readfds);
	FD_SET(m_socket, &exceptfds);

	// accept thread pipe 제거

	int    nDelay = m_delay;

	delay.tv_sec = nDelay / 1000; 
	delay.tv_usec = nDelay * 1000;

	// pipe를 제거하고 accept는 accept socket+1을 select한다.
	iRet = select(m_socket+1, &readfds, 0, &exceptfds, &delay);
	if( 0 == iRet ) return;

	// check exception
	if( FD_ISSET(m_socket, &exceptfds) )	
	{
		log( "Except on accepting socket\n");
#ifdef	WIN32
		log( "Error : %d\n", WSAGetLastError());
#endif
#ifdef	__LINUX__
		log( "Error : %d\n", errno);
#endif
		return; // break;
	}

	// accpet
	if( FD_ISSET(m_socket, &readfds) )	
	{
#ifdef	WIN32
    	nLength = sizeof(ca);
    	socket = accept(m_socket,(sockaddr *)&ca,&nLength);
    	if(INVALID_SOCKET == socket)	{
    		if( WSAEWOULDBLOCK != WSAGetLastError() )	
    			closesocket(socket);
#endif
#ifdef	__LINUX__
    	socket = accept(m_socket, NULL, NULL);
    	if( -1 == socket )	{
    		if( EAGAIN != errno )
            {
				log( "accept : INVALID_SOCKET\n");
            }
#endif
    	} else {
    		if( m_nCurrClients < m_nMaxClients ) {
#ifdef	WIN32
    			ioctlsocket(socket, FIONBIO, (u_long FAR *)&iOption);
#endif
#ifdef	__LINUX__
    			fcntl(socket, F_SETFL, O_NONBLOCK);
#endif
				// m_ClientList -> Lock
				m_pRemoveList->Lock();
    			AddClient(socket);
				m_pRemoveList->Unlock();
  			} else {
#ifdef	WIN32
    			closesocket(socket);
#endif
#ifdef	__LINUX__
    			close(socket);
#endif
    		}
        }
	}
}

template <typename C>
void CServer<C>::Process()
{
	int iRet;		// for select()
	fd_set readfds, writefds, exceptfds;
	struct timeval delay;
	typename map <unsigned int, shared_ptr< C > >::iterator R;
	shared_ptr< C > pClient;
#ifdef WIN32
    int iError;
#endif
	int nRecv, nSend;
	int maxfd = 0;

#ifdef	__LINUX__
	char pipeBuffer[PIPE_BUFFER_SIZE];
#endif

#ifdef WIN32
	m_CurrTime = m_LastTime = timeGetTime();
#endif
#ifdef	__LINUX__
	gettimeofday(&m_CurrTime, NULL);
	gettimeofday(&m_LastTime, NULL);
#endif
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);

#ifdef	__LINUX__
	maxfd = m_socket;
	FD_SET(m_pipe[0], &readfds);
	if( maxfd < m_pipe[0] ) maxfd = m_pipe[0];	
#endif
    int nDelay = m_delay;

	for( R = m_ClientList.begin(); R != m_ClientList.end(); R++ )
	{
		pClient = ( shared_ptr<C> )(*R).second;
		if( !pClient->m_bRemove )	
		{
			FD_SET(pClient->m_socket, &readfds);
			FD_SET(pClient->m_socket, &exceptfds);
			if( pClient->m_iWriteBuffer > 0 ) FD_SET(pClient->m_socket, &writefds);
#ifdef	__LINUX__
			if( maxfd < pClient->m_socket ) maxfd = pClient->m_socket;
#endif
		}
	}

	delay.tv_sec = nDelay / 1000; 
	delay.tv_usec = nDelay * 1000;

	iRet = select(maxfd + 1, &readfds, &writefds, &exceptfds, &delay);
	if( 0 == iRet ) goto check_timer;

#ifdef 	__LINUX__
	// check for pipe
	if( FD_ISSET(m_pipe[0], &readfds) )
		nRecv = read(m_pipe[0], pipeBuffer, PIPE_BUFFER_SIZE);
#endif
	
	// Check for exceptfds
	for( R = m_ClientList.begin(); R != m_ClientList.end(); R++ )	{
		pClient = ( shared_ptr<C> )(*R).second;
		if( !pClient->m_bRemove )	{
			if( FD_ISSET(pClient->m_socket, &exceptfds) )	{
				pClient->RemoveClient();
			}
		}
	}

	// Check for readfds
	for( R = m_ClientList.begin(); R != m_ClientList.end(); R++ )	
	{
		pClient = ( shared_ptr<C> )(*R).second;
		if( !pClient->m_bRemove )	
		{
			if( FD_ISSET(pClient->m_socket, &readfds) )	
			{
				if((pClient->m_iMaxReadBuffer - pClient->m_iReadBuffer) <= 0) 
				{	
					pClient->RemoveClient();
					continue;
				}

				nRecv = recv(pClient->m_socket, 
					&pClient->m_ReadBuffer[pClient->m_iReadBuffer],
					pClient->m_iMaxReadBuffer - pClient->m_iReadBuffer,
					0);
				if( nRecv < 0 )	
				{
#ifdef	WIN32
					iError = WSAGetLastError();
					if( WSAEWOULDBLOCK != iError )	{
#endif
#ifdef	__LINUX__
					if( EAGAIN != errno )	{
#endif
						pClient->RemoveClient();
					}
				} else if( 0 == nRecv )	 {
					pClient->RemoveClient();
				} else if( nRecv > 0 ) {
					pClient->m_iReadBuffer += nRecv;
					if( !Parser(pClient) ) 
					{
						pClient->RemoveClient();
					}
				}
			}
		}
	}

	// Check for writefds
	for( R = m_ClientList.begin(); R != m_ClientList.end(); R++ )
	{
		pClient = ( shared_ptr<C> )(*R).second;
		if( !pClient->m_bRemove )
		{
			if( FD_ISSET(pClient->m_socket, &writefds) )
			{

				nSend = send(pClient->m_socket, pClient->m_WriteBuffer.get(), pClient->m_iWriteBuffer, 0);
				if( nSend < 0 )
				{
#ifdef	WIN32
					iError = WSAGetLastError();
					if( WSAEWOULDBLOCK != iError )
					{
#endif
#ifdef	__LINUX__
					if( EAGAIN != errno )
					{
#endif
						pClient->RemoveClient();
					}
				}
				else if ( nSend > 0 )
				{
					pClient->Lock();
					pClient->m_iWriteBuffer -= nSend;
					if( pClient->m_iWriteBuffer > 0 )
					{
						memcpy(pClient->m_WriteBuffer.get(),
							&pClient->m_WriteBuffer[nSend],
							pClient->m_iWriteBuffer);
					}
					pClient->Unlock();
				}

			}
		}
	}

check_timer:
#ifdef	WIN32
	m_CurrTime = timeGetTime();
	if( m_CurrTime < m_LastTime ) m_CurrTime = m_LastTime = 0;
	m_TimeDiff += m_CurrTime - m_LastTime;
#endif
#ifdef	__LINUX__
	gettimeofday(&m_CurrTime, NULL);
	m_TimeDiff += ( m_CurrTime.tv_sec -	m_LastTime.tv_sec ) * 1000 + ( m_CurrTime.tv_usec - m_LastTime.tv_usec ) / 1000;
#endif
	if(m_TimeDiff > (unsigned long)m_timer)
	{
		TimerFunc();
		m_TimeDiff = 0;
	}
	m_LastTime = m_CurrTime;

    m_pRemoveList->Lock();
    for(unsigned int i=0; i<m_pRemoveList->m_RemoveList.size(); i++)
    {
		RemoveClient(m_pRemoveList->m_RemoveList[i]);
    }
    m_pRemoveList->m_RemoveList.clear();
    m_pRemoveList->Unlock();
}

/*
 * FUNCTION: GetClient
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   하당 고유키에 대한 고유키값을 구함
 *-----------------------------------------------------------------------------
 * PARAMETERS
 *   ulKey : [IN] Client의 고유키
 *-----------------------------------------------------------------------------
 * SEE
 *   TimerFunc()
 *-----------------------------------------------------------------------------
 * RETURN VALUE
 *   shared_ptr<C> : 고유키에 해당하는 Client
 *   shared_ptr<C>.get() == NULL : 고유키에 해당하는 Client가 없는 경우
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
template <typename C>
shared_ptr<C> CServer<C>::GetClient(unsigned long ulKey)
{
	// 정대원 수정
	// C* -> shared_ptr<C>
	//typename map <unsigned int, C*>::iterator I;
	typename map <unsigned int, shared_ptr<C> >::iterator I;
	shared_ptr<C> pClient;

	I = m_ClientList.find(ulKey);

	if( m_ClientList.end() == I )
		return pClient;
	else
	{
		pClient = (shared_ptr<C>)(*I).second;
		return pClient;
	}
}

/*
 * FUNCTION: Connect
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   특정 서버로 연결을 시킴
 *-----------------------------------------------------------------------------
 * PARAMETERS
 *   szAddr : [IN] 연결할 서버의 주소
 *   iPort  : [IN] 연결할 서버의 Port
 *   mdelay : [IN] 서버로의 연결을 기다릴 대기시간
 *-----------------------------------------------------------------------------
 * SEE
 *   AddClient()
 *-----------------------------------------------------------------------------
 * RETURN VALUE
 *   true  : 연결이 성공했을 경우
 *   false : 연결이 실패했을 경우
 *-----------------------------------------------------------------------------
 * SIDE EFFECTS
 *   ClientList에 연결된 Client 추가
 *-----------------------------------------------------------------------------
 * Author
 *   Bard
 *-----------------------------------------------------------------------------
 * History
 *   2008. 06. ?? : Created
 */
template <typename C>
bool CServer<C>::Connect(char *szAddr, int iPort, int mdelay)
{
	struct  sockaddr_in addr;    
	SOCKET s;

	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if( INVALID_SOCKET == s ) return false;
#ifdef	WIN32
	int iOption = 1;
	ioctlsocket(s, FIONBIO, (u_long FAR *)&iOption);
#endif
#ifdef	__LINUX__
	fcntl(s, F_SETFL, O_NONBLOCK);
#endif
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = inet_addr(szAddr);
	addr.sin_port        = htons(iPort);

	if( 0 != connect(s,(const struct sockaddr *)&addr, sizeof(addr)) )	
	{
#ifdef WIN32
		if( WSAEWOULDBLOCK != WSAGetLastError() )	
		{
			printf("An error occurred while connect() : %d\n", WSAGetLastError());
			closesocket(s);
			return false;
		}
#endif
#ifdef __LINUX__
		if( EINPROGRESS != errno )	
		{
			printf("An error occurred while connect() : %s(%d)\n", strerror(errno), errno);
			close(s);
			return false;
		}
#endif
	} 
	else	
	{
		return true;
	}

	fd_set rset, wset, eset;
	struct timeval delay;
	int idelay = 0, n;

	FD_ZERO(&rset);
	FD_ZERO(&wset);
	FD_ZERO(&eset);

	FD_SET(s, &rset);
	FD_SET(s, &wset);
	FD_SET(s, &eset);

	if( mdelay > 1000 )	
	{
		idelay = mdelay / 1000;
		mdelay -= idelay * 1000;
	}

	delay.tv_sec = idelay;
	delay.tv_usec = mdelay * 1000;

	int max_fd = 0;

#ifdef __LINUX__
	max_fd = s + 1;
#endif

	n = select(max_fd, &rset, &wset, &eset, &delay);

	if( SOCKET_ERROR == n )	
	{
		printf("An error occured while select()\n");
		closesocket(s);
		return false;
	} else if ( 0 == n )	{	// Time Out
		printf("Time Out\n");
		closesocket(s);
		return false;
	}

	if( FD_ISSET(s, &eset) )
	{
		closesocket(s);
		return false;
	}	

	if( FD_ISSET(s, &rset) || FD_ISSET(s, &wset) )	{
#ifdef	__LINUX__
		if( 0 != connect(s,(const struct sockaddr *)&addr, sizeof(addr)))
			return false;
#endif
		if( AddClient(s) )
			return true;
		else 
			return false;
	}
	return false;
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
template <typename C>
void CServer<C>::CancelSelect()
{
	write(m_pipe[1], "C", 1);
}

#endif
#endif
