#ifndef __CSERVER_UDP__
#define	__CSERVER_UDP__

#include <stdarg.h>

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

#include <algorithm>
using namespace std;

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
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

#include <string>
using namespace std;

#include <boost/shared_array.hpp>
#include <boost/unordered_map.hpp>
using namespace boost;

#include "CRemoveList.h"
#include "CSendQueue.h"

template <typename C> class CServerUdp
{
public:
	int m_iMaxReadBuffer;		// Size of shared socket read buffer
	int m_iMaxWriteBuffer;		// Size of shared socket  write buffer
	int m_nCurrClients;		// How many clients are connected
	int m_iPort;			// Port number
	int m_delay;			// delay for select()
	int m_nMaxClients;		// Max number of possible clients
	int m_timer;			// milliseconds of timer interval
	int m_cleaner_timer;		// milliseconds of cleaner timer interval
	FILE *m_fp;

	pthread_t m_Thread;
	bool	m_bStopThread;

	shared_array< char > m_ReadBuffer;
	shared_ptr<CSendQueue> m_pQueue;

private:
	SOCKET m_socket;
	
	struct timeval m_CurrTime;
	struct timeval m_LastTime;
	
	unsigned long m_TimeDiff;		// 타이머용 시간누적변수
	unsigned long m_CleanerTimeDiff;	// 클리너용 시간누적변수

	shared_ptr<CRemoveList <string> > m_pRemoveList;

public:
	unordered_map<string, shared_ptr< C > > m_ClientList;
	CServerUdp(int iMaxReadBuffer, int iMaxWriteBuffer, int iPort, int delay, int nMaxClients, int timer, int cleaner_timer, FILE *fp);
	virtual ~CServerUdp();

	bool InitNetwork();
	void CleanUpNetwork();

	bool Start();
	bool Stop();
	void CleanUp();

	virtual bool AddClient(string IP, int Port);
	virtual bool RemoveClient(string strKey); 

	virtual bool Parser( shared_ptr< C > pClient, int nRecv );
	virtual void ParserMain(shared_ptr< C > pClient, int nRecv) {};
	virtual void TimerFunc()	{};
	void Cleaner();
	void CheckRemove(pair<string, shared_ptr<C> >);
 
	shared_ptr<C> GetClient(string strKey);

	static void *ThreadFunc(void *Param);
	void log(const char *fmt, ... );

private:

	int m_pipe[2];		// pipe for canceling select()
	void CancelSelect();

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
CServerUdp<C>::CServerUdp(int iMaxReadBuffer, int iMaxWriteBuffer, int iPort, int delay, int nMaxClients, int timer, int cleaner_timer, FILE *fp)
{
	m_nCurrClients = 0;
	m_iMaxReadBuffer = iMaxReadBuffer;
	m_iMaxWriteBuffer = iMaxWriteBuffer;
	m_socket = 0;
	m_iPort = iPort;
	m_delay = delay;
	m_nMaxClients = nMaxClients;
	m_timer = timer;
	m_cleaner_timer = cleaner_timer;
	m_fp = fp;

	m_Thread =(pthread_t)NULL;
	m_bStopThread = false;

	m_TimeDiff = 0;
	m_CleanerTimeDiff = 0;

	m_ReadBuffer = shared_array<char>(new char[m_iMaxReadBuffer]);
	m_pRemoveList = shared_ptr<CRemoveList <string> >(new CRemoveList<string>());

	m_pQueue = shared_ptr<CSendQueue>(new CSendQueue());

	pipe(m_pipe);
	signal(SIGPIPE, SIG_IGN);
}

template <typename C>
CServerUdp<C>::~CServerUdp()

{
	CleanUp();
}

template <typename C>
bool CServerUdp<C>::InitNetwork()
{
	return true;
}

template <typename C>
void CServerUdp<C>::CleanUpNetwork()
{
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
bool CServerUdp<C>::Start()
{
	m_bStopThread = false;
	pthread_create(&m_Thread, NULL, ThreadFunc, this);	
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
bool CServerUdp<C>::Stop()
{
	if( (pthread_t)NULL == m_Thread ) return false;
	m_bStopThread = true;
	pthread_join(m_Thread, NULL);
	return true;
}

template <typename C>
void CServerUdp<C>::CleanUp()
{
	close(m_pipe[1]);
	close(m_pipe[0]);
	m_ClientList.clear();
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
bool CServerUdp<C>::AddClient(string IP, int Port)
{
	string strKey = IP + ":" + lexical_cast<string>(Port);
	m_ClientList[strKey] = shared_ptr< C >( new C(IP, Port, m_pipe[1], m_pRemoveList, m_pQueue ) );

	log("AddClient : key = %s\n", strKey.c_str());
	m_nCurrClients++;
	return true;
}

/*
 * FUNCTION: RemoveClient
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   서버에 클라이언트를 제거한다.
 *-----------------------------------------------------------------------------
 * PARAMETERS
 *   ulKey : [IN] 삭제할 Client의 고유키
 *-----------------------------------------------------------------------------
 * SEE
 *   none
 *-----------------------------------------------------------------------------
 * RETURN VALUE
 *   항상 true
 *-----------------------------------------------------------------------------
 * SIDE EFFECTS
 *   m_ClientList에 클라이언트가 제거됨
 *-----------------------------------------------------------------------------
 * Author
 *   Bard
 *-----------------------------------------------------------------------------
 * History
 *   2008. 06. ?? : Created
 */
template <typename C>
bool CServerUdp<C>::RemoveClient(string strKey)
{
	if( m_ClientList.end() == m_ClientList.find(strKey) ) return false;

	//shared_ptr<C> pClient = (*R).second;
	m_ClientList.erase(strKey);
	m_nCurrClients--;
	log("Remove Client: key = %s\n", strKey.c_str());

	return true;
}

/*
 * FUNCTION: Parser
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   Client들로부터 넘어온 데이터를 처리하여 ParserMain Function을 호출
 *-----------------------------------------------------------------------------
 * PARAMETERS
 *   pClient : [IN] 처리할 데이터가 있는 Client의 핸들러
 *-----------------------------------------------------------------------------
 * SEE
 *   Process(), ParserMain(pClient)
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
bool CServerUdp<C>::Parser(shared_ptr< C > pClient, int nRecv)
{
	pClient->UpdateAccess();
	ParserMain(pClient, nRecv);
	return true;
}

/*
 * FUNCTION: log
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   log를 남김
 *-----------------------------------------------------------------------------
 * PARAMETERS
 *   fmt : [IN] 저장할 log 값
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
void CServerUdp<C>::log(const char *fmt, ... )
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
		vsyslog(LOG_DEBUG, fmt, args );
	}
	va_end(args);
}

/*
 * FUNCTION: ThreadFunc
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   쓰레드를 돌림
 *-----------------------------------------------------------------------------
 * PARAMETERS
 *   Param : [IN] 쓰레드 시작시 넘어오는 parameter
 *-----------------------------------------------------------------------------
 * SEE
 *   ThreadMain()
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
void *CServerUdp<C>::ThreadFunc(void *Param)
{
	CServerUdp<C> *pServer = (CServerUdp <C> *)Param;

	// bind 
	char Option = 1;

	pServer->m_socket = socket(AF_INET, SOCK_DGRAM, 0);
	setsockopt(pServer->m_socket, SOL_SOCKET, SO_REUSEADDR, &Option, sizeof(Option));

	fcntl(pServer->m_socket, F_SETFL, O_NONBLOCK);

	struct linger Linger;
	Linger.l_onoff  = 1;
	Linger.l_linger = 0;      /* 0 for abortive disconnect */
	setsockopt(pServer->m_socket, SOL_SOCKET, SO_LINGER, &Linger, sizeof(Linger));
	setsockopt(pServer->m_socket, SOL_SOCKET, SO_RCVBUF, 
			&pServer->m_iMaxReadBuffer, sizeof(pServer->m_iMaxReadBuffer));
	setsockopt(pServer->m_socket, SOL_SOCKET, SO_SNDBUF, 
			&pServer->m_iMaxWriteBuffer, sizeof(pServer->m_iMaxWriteBuffer));

	struct sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	sa.sin_port = htons((USHORT)pServer->m_iPort);

	pServer->log( "Port -> : %d\n", pServer->m_iPort);
	if ( 0 != ::bind(pServer->m_socket, (struct sockaddr*) &sa, sizeof(sa)))	{
		pServer->log( "Error on bind() : %d\n", errno);
		return NULL;
	}

	// select loop
	while(!pServer->m_bStopThread)	{
		pServer->Process();
	}

	pServer->log( "close()\n");
	close(pServer->m_socket);
	return 0;
}

#define	PIPE_BUFFER_SIZE	1024

/*
 * FUNCTION: Process
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   Server의 주된 작업이 실행
 *-----------------------------------------------------------------------------
 * PARAMETERS
 *   none
 *-----------------------------------------------------------------------------
 * SEE
 *   TimerFunc()
 *-----------------------------------------------------------------------------
 * RETURN VALUE
 *   none
 *-----------------------------------------------------------------------------
 * SIDE EFFECTS
 *   일정 시간별로 TimerFunc() 함수 호출
 *-----------------------------------------------------------------------------
 * Author
 *   Bard
 *-----------------------------------------------------------------------------
 * History
 *   2008. 06. ?? : Created
 */
template <typename C>
void CServerUdp<C>::Process()
{
	int iRet;		// for select()
	fd_set readfds, writefds, exceptfds;
	struct timeval delay;
	shared_ptr< C > pClient;
	struct sockaddr_in ca;
	int nRecv, nSend;
	int addr_size = sizeof(ca);
	int maxfd = 0;
	unsigned long delta;

	char pipeBuffer[PIPE_BUFFER_SIZE];

	gettimeofday(&m_CurrTime, NULL);
	gettimeofday(&m_LastTime, NULL);
	
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);

	FD_SET(m_socket, &readfds);
	if( m_pQueue->GetSize() > 0 ) FD_SET(m_socket, &writefds);
	FD_SET(m_socket, &exceptfds);
	
	maxfd = m_socket;
	
	FD_SET(m_pipe[0], &readfds);
	if( maxfd < m_pipe[0] ) maxfd = m_pipe[0];	
	int nDelay = m_delay;

	delay.tv_sec = nDelay / 1000; 
	delay.tv_usec = nDelay * 1000;

	iRet = select(maxfd + 1, &readfds, &writefds, &exceptfds, &delay);
	if( 0 == iRet ) goto check_timer;

	// Error on listening socket

	if( FD_ISSET(m_socket, &exceptfds) )	{
		log( "Except on accepting socket\n");
		log( "Error : %d\n", errno);
		return; // break;
	}

	// recvfrom

	if( FD_ISSET(m_socket, &readfds) )	
	{
		while ( true )
		{
			nRecv = recvfrom(m_socket, m_ReadBuffer.get(), m_iMaxReadBuffer, 0,
				(struct sockaddr *)&ca, (socklen_t *)&addr_size);
			if( nRecv > 0 )
			{
				string IP = (string)(inet_ntoa(ca.sin_addr));
				int Port = ntohs(ca.sin_port);

				string strKey = IP + ":" + lexical_cast<string>(Port);
				if( m_ClientList.find(strKey) == m_ClientList.end() )
					AddClient(IP, Port);
				Parser(m_ClientList[strKey], nRecv);
		
			}
			if( 0 == nRecv )	// orderly shutdown
			{
				string IP = (string)(inet_ntoa(ca.sin_addr));
				int Port = ntohs(ca.sin_port);

				string strKey = IP + ":" + lexical_cast<string>(Port);
				RemoveClient(strKey);
			}
			else
			{
				if( EAGAIN != errno && EWOULDBLOCK != errno )
				{
					log( "Except on recvfrom\n");
					log( "Error : %d\n", errno);
				}
				break;
			}
		}
	}
	
	if( FD_ISSET(m_socket, &writefds) )	
	{
		while ( m_pQueue->GetSize() > 0 )
		{
			shared_ptr<CSendPacket> pPacket = m_pQueue->Pop();
			
			memset(&ca, 0, addr_size);
			ca.sin_family = AF_INET;
			ca.sin_addr.s_addr = inet_addr(pPacket->m_IP.c_str());
			ca.sin_port = htons(pPacket->m_Port);

			nSend = sendto(m_socket, pPacket->m_data.get(), pPacket->m_nSize, 0,
				(struct sockaddr *)&ca, addr_size);

			if( -1 == nSend )
			{
				if( EAGAIN != errno && EWOULDBLOCK != errno )
				{
					log( "Except on recvfrom\n");
					log( "Error : %d\n", errno);
				}
				m_pQueue->AddQueue(pPacket);	
				break;
			}
		}
	}

	// check for pipe
	if( FD_ISSET(m_pipe[0], &readfds) )
		nRecv = read(m_pipe[0], pipeBuffer, PIPE_BUFFER_SIZE);

check_timer:
	gettimeofday(&m_CurrTime, NULL);
	delta = ( m_CurrTime.tv_sec -	m_LastTime.tv_sec ) * 1000 + ( m_CurrTime.tv_usec - m_LastTime.tv_usec ) / 1000;

	m_TimeDiff += delta;
	if(m_TimeDiff > (unsigned long)m_timer)
	{
		TimerFunc();
		m_TimeDiff = 0;
	}

	m_CleanerTimeDiff += delta;
	if(m_CleanerTimeDiff > (unsigned long)m_cleaner_timer)
	{
		Cleaner();
		m_CleanerTimeDiff = 0;
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
shared_ptr<C> CServerUdp<C>::GetClient(string strKey)
{
	if( m_ClientList.end() == m_ClientList.find(strKey) )
		return shared_ptr<C>(NULL);
	else
	{
		return m_ClientList[strKey];
	}
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
template <typename C>
void CServerUdp<C>::CancelSelect()
{
	write(m_pipe[1], "C", 1);
}

template <typename C>
void CServerUdp<C>::Cleaner()
{
	for_each( m_ClientList.begin(), m_ClientList.end(), bind(&CServerUdp<C>::CheckRemove, this, _1) );
}

template <typename C>
void CServerUdp<C>::CheckRemove(pair<string, shared_ptr<C> > Pair) 
{
	struct timeval curr_time;

	gettimeofday(&curr_time, NULL);
	shared_ptr<C> pClient = Pair.second;
	unsigned long delta = (curr_time.tv_sec - pClient->m_lastAccessed.tv_sec) * 1000 
		+ (curr_time.tv_usec - pClient->m_lastAccessed.tv_usec) / 1000;

	if( delta >= (unsigned long)m_cleaner_timer )
	{
		pClient->m_bRemove = true;
		m_pRemoveList->AddRemoveList(pClient->GetKey());
	}
}

#endif
