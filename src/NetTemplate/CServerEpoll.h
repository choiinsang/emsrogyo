#ifndef __CSERVER_EPOLL__
#define	__CSERVER_EPOLL__

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
#include <sys/epoll.h>

#include "Log.h"
#include "sth_syslog.h"


#define	closesocket(s)	close(s)
#define	WSAGetLastError()	errno
#define	SOCKET_ERROR	-1	
extern int errno;

#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>

#ifdef __USETR1LIB__
#include <tr1/unordered_map>
#define unordered_map std::tr1::unordered_map
#else
#include <boost/unordered_map.hpp>
#define unordered_map boost::unordered_map
#endif

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
#include "CClientList.h"
#include "CRemoveList.h"

#include "NetTemplateDefine.h"

template <typename C> class CServer
{
public:
	int m_iMaxClientReadBuffer;	// Size of each client's read buffer
	int m_iMaxClientWriteBuffer;	// Size of each client's write buffer
	int m_nCurrClients;		// How many clients are connected
	int m_iPort;			// Port number
	int m_delay;			// delay for select()
	int m_nMaxClients;		// Max number of possible clients
	int m_timer;			// milliseconds of timer interval
	FILE *m_fp;

	pthread_t m_Thread;
	bool	m_bStopThread;

private:
	SOCKET m_socket;
#ifndef __NOTUSETIMER__
	struct timeval m_CurrTime;
	struct timeval m_LastTime;
	struct timeval mGabTime; //9add
	unsigned long m_TimeDiff;
#endif

// epoll specific

	shared_array<struct epoll_event> m_epoll_events;
protected:
	shared_ptr<CRemoveList <unsigned long > > m_pRemoveList;
	int m_epoll_fd;

public:
	CClientList<C> m_ClientList;
	CServer(int iMaxClientReadBuffer, int iMaxClientWriteBuffer, int iPort, int delay, int nMaxClients, int timer, FILE *fp);
	virtual ~CServer();

	bool InitNetwork();
	void CleanUpNetwork();

	bool Start();
	bool Stop();
	void CleanUp();

	//virtual bool AddClient(SOCKET socket, bool _Is_ItoSconnect=CONNECT_CtoI);
	shared_ptr<C> AddClient(SOCKET socket, bool _Is_ItoSconnect=CONNECT_CtoI);
	virtual bool RemoveClient(unsigned long ulKey); 
	
	virtual void connectFail ( shared_ptr< C > pClient, int iState ) {};  //ischoi
	virtual void sendBufFail ( shared_ptr< C > pClient, int iState ) {};  //ischoi
	virtual void connectError( shared_ptr< C > pClient, int iState ) {};  //ischoi

	virtual bool Parser( shared_ptr< C > pClient );
	virtual void ParserMain(shared_ptr< C > pClient) {};
	virtual void ParserMain(shared_ptr< C > pClient, unsigned long _ulSize) {};

	virtual void TimerFunc() { };
	virtual void RemoveClientList();
	virtual void DoNext(shared_ptr< C > pClient) { };

	shared_ptr<C> GetClient(unsigned long ulKey);
	bool Connect(char *szAddr, int iPort, int mdelay);
	int Connect(char *szAddr, int iPort, shared_ptr< C >&_spClient);

	bool Check_ClientListConnected();

	static void *ThreadFunc(void *Param);
	void *ThreadMain();
	void log(const char *fmt, ... );

	CLog* pLog;

private:

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
	pLog = &CLog::GetInstance();

	m_nCurrClients = 0;
	m_iMaxClientReadBuffer = iMaxClientReadBuffer;
	m_iMaxClientWriteBuffer = iMaxClientWriteBuffer;
	m_socket = 0;
	m_iPort = iPort;
	m_delay = delay;
	m_nMaxClients = nMaxClients;
	m_timer = timer;
	m_fp = fp;

	m_Thread =(pthread_t)NULL;
	m_bStopThread = false;
#ifndef __NOTUSETIMER__
	m_TimeDiff = 0;

	gettimeofday(&m_CurrTime, NULL); //9add
	gettimeofday(&m_LastTime, NULL); //9add
#endif

	m_pRemoveList = shared_ptr<CRemoveList <unsigned long> >(new CRemoveList<unsigned long>());
	signal(SIGPIPE, SIG_IGN);
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
 *   CServerEpoll에서는 기능없음
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
	return true;
}

/*
 * FUNCTION: CleanUpNetwork
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   CServerEpoll에서는 기능없음
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
}

/*
 * FUNCTION: Start
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   CServerEpoll을 구동시킴
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
	m_bStopThread = false;
	pthread_create(&m_Thread, NULL, ThreadFunc, this);	
	return true;
}

/*
 * FUNCTION: Stop
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   CServerEpoll을 중단시킴
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
	if( (pthread_t)NULL == m_Thread ) return false;
	m_bStopThread = true;
	pthread_join(m_Thread, NULL);
	return true;
}

/*
 * FUNCTION: CleanUp
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   서버를 초기화 시킴
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
 *   Client 들을 모두 제거
 *-----------------------------------------------------------------------------
 * Author
 *   Bard
 *-----------------------------------------------------------------------------
 * History
 *   2008. 06. ?? : Created
 */
template <typename C>
void CServer<C>::CleanUp()
{
	m_ClientList.Clear();
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
//bool CServer<C>::AddClient(SOCKET socket, bool _Is_ItoSconnect)
shared_ptr<C> CServer<C>::AddClient(SOCKET socket, bool _Is_ItoSconnect)
{
	typename unordered_map<unsigned int, shared_ptr< C > >::iterator R;

	shared_ptr< C > pClient = shared_ptr< C >( new C(socket, m_iMaxClientReadBuffer, m_iMaxClientWriteBuffer, m_epoll_fd, m_pRemoveList ) );
	pClient->m_socket = socket;

	if(_Is_ItoSconnect == CONNECT_ItoS)
	{
		pClient->mIs_ItoSconnect = CONNECT_ItoS;
		gettimeofday(&pClient->mtime_ConnectStart, NULL);
	}
	else
	{
		pClient->mIs_ItoSconnect = CONNECT_CtoI;
	}

	//SM_N pLog->Write("N F[CServer<C>::AddClient] S[new C, m_socket = socket] V[scoket:%d]", socket);

	struct epoll_event ee;

	ee.events = EPOLLIN | EPOLLERR | EPOLLOUT; //hsj EPOLLOUT 추가
	ee.data.fd = pClient->m_socket;
	
	if( 0 != epoll_ctl( m_epoll_fd, EPOLL_CTL_ADD, pClient->m_socket, &ee) )
	{
		//SM_N pLog->Write("N F[CServer<C>::AddClient] E[0 != epoll_ctl]");
		//log("Error on epoll_ctl() : %d\n", errno);
		//return false; 		;
		return shared_ptr<C>((C*)NULL);
	}

	if(pClient->mIs_ItoSconnect == CONNECT_CtoI)
	{
		struct sockaddr_in ca;
		int addr_size = sizeof(ca);

		if( 0 != getpeername(pClient->m_socket, (struct sockaddr *)&ca, (socklen_t *)&addr_size) )
		{
			//log("Error on getpeername() : %d\n", errno);
			//SM_N pLog->Write("N F[CServer<C>::AddClient] E[Error on getpeername]");
			//return false; 
			return shared_ptr<C>((C*)NULL);
		}

		pClient->m_IP = (string)(inet_ntoa(ca.sin_addr));
		pClient->m_nPort = ntohs(ca.sin_port);
	}

	m_ClientList.Insert(pClient);

	//log("AddClient : key = %ld\n", pClient->m_ulKey);
	m_nCurrClients++;

	//SM_N pLog->Write("N F[CServer<C>::AddClient] D[Func end] V[key=%ld, %d]", pClient->m_ulKey, m_nCurrClients);
	//return true;
	return pClient;
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
bool CServer<C>::RemoveClient(unsigned long ulKey)
{
	shared_ptr<C> pClient;
	pClient = m_ClientList.GetClient(ulKey);
	if(pClient.get() == NULL ) return false;

	if( 0 != epoll_ctl( m_epoll_fd, EPOLL_CTL_DEL, pClient->m_socket, NULL ) )
		log("Error on epoll_ctl() : %d\n", errno);
	
	m_ClientList.Erase(ulKey);
	m_nCurrClients--;
	log("Remove Client: key = %ld\n", ulKey);

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
bool CServer<C>::Parser(shared_ptr< C > pClient)
{
	if( pClient->m_iReadBuffer <= 0 ) 
		return false;

	/*
	220 ESMTP Spamsniper by JiranSoft 3.3.3.0071210
	*(pp+47) == '\r' *(pp+48) == '\n' *(pp+49) == '\0'
	len 49
	*/

	//hsj
	{
		//SM_N //pLog->Write("N F[CServer<C>::Parser] V[get:%s, len:%d, iReadBuf:%d]", ChangeNewline((char*)pClient->m_ReadBuffer.get()), strlen((char*)pClient->m_ReadBuffer.get()) , pClient->m_iReadBuffer);
		if( pClient->m_bRemove ) // 클라이언트 처리 상태 변경 시 중지
		{
			Trace(TR_INFO, "out : by pClient->m_bRemove ");
			return false;
		}

		char *m_pBuf = (char*)pClient->m_ReadBuffer.get();
		
		unsigned long m_len;

		/* save
		char *m_pEndCheck = (char *)memchr(m_pBuf, '\n', pClient->m_iReadBuffer); //\r\n
		if(m_pEndCheck == NULL)
		{
			Trace(TR_INFO, "m_pEndCheck is NULL");
			return true;
		}
		m_len = m_pEndCheck - m_pBuf + 1;
		*/

		if(pClient->m_iReadBuffer < 2)
			return true;

		if( (m_pBuf[pClient->m_iReadBuffer-1] == '\n' && m_pBuf[pClient->m_iReadBuffer-2] == '\r') ||
			(m_pBuf[pClient->m_iReadBuffer-1] == '\r' && m_pBuf[pClient->m_iReadBuffer-2] == '\n') )
		{
			m_len = pClient->m_iReadBuffer;
		}
		else
		{
			return true;
		}


		ParserMain(pClient, m_len);
		pClient->m_iReadBuffer -= m_len;

		if( pClient->m_iReadBuffer > 0 )	
		{
			memcpy(pClient->m_ReadBuffer.get(), pClient->m_ReadBuffer.get()+m_len, pClient->m_iReadBuffer);
		}

		if( pClient->m_iReadBuffer < 0 ) 
			pClient->m_iReadBuffer = 0;

		return true;
	}

	//strnchr(m_pBuf, '\0');

	PACKET_HEADER *pph;
	unsigned long ulSize, ulType;

	while( (unsigned long)pClient->m_iReadBuffer >= sizeof(PACKET_HEADER) )	
	{
		if( pClient->m_bRemove ) // 클라이언트 처리 상태 변경 시 중지
		{
			Trace(TR_INFO, "out : by pClient->m_bRemove ");
			break;
		}

		pph = (PACKET_HEADER *)pClient->m_ReadBuffer.get();
		ulSize = ntohl(pph->ulSize);
		ulType = ntohl(pph->ulType);

		if( (unsigned long)pClient->m_iReadBuffer < ulSize ) 
		{
			Trace(TR_INFO, "out : pClient->m_iReadBuffer < ulSize %d < %d ", pClient->m_iReadBuffer, ulSize);
			break;
		}

		ParserMain(pClient);
		pClient->m_iReadBuffer -= ulSize;

		if( pClient->m_iReadBuffer > 0 )	
		{
			memcpy(pClient->m_ReadBuffer.get(), pClient->m_ReadBuffer.get()+ulSize, pClient->m_iReadBuffer);
		}

		if( pClient->m_iReadBuffer < 0 ) 
			pClient->m_iReadBuffer = 0;
	}

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
void *CServer<C>::ThreadFunc(void *Param)
{
	CServer<C> *pServer = (CServer <C> *)Param;
	return pServer->ThreadMain();
}

/*
 * FUNCTION: ThreadMain
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   EPoll을 생성하고 실행시킴
 *-----------------------------------------------------------------------------
 * PARAMETERS
 *   none
 *-----------------------------------------------------------------------------
 * SEE
 *   Process()
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
void *CServer<C>::ThreadMain()
{
	// epoll

	m_epoll_fd = epoll_create(m_nMaxClients);
	if( -1 == m_epoll_fd )
	{
		log("Error on epool_create() : %d\n", errno);
		return NULL;
	}

	m_epoll_events = shared_array<struct epoll_event>(new struct epoll_event[m_nMaxClients]);

	if(m_iPort != 0){
		// bind 
		char Option = 1;
	
		m_socket = socket(AF_INET, SOCK_STREAM, 0);
		setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &Option, sizeof(Option));
	
		fcntl(m_socket, F_SETFL, O_NONBLOCK);
	
		struct linger Linger;
		Linger.l_onoff  = 1;
		Linger.l_linger = 0;      /* 0 for abortive disconnect */
		setsockopt(m_socket, SOL_SOCKET, SO_LINGER, &Linger, sizeof(Linger));


		struct sockaddr_in sa;
		memset(&sa, 0, sizeof(sa));
		sa.sin_family = AF_INET;
		sa.sin_addr.s_addr = htonl(INADDR_ANY);
		sa.sin_port = htons((USHORT)m_iPort);
	
		log( "Port -> : %d\n", m_iPort);
		if ( 0 != ::bind(m_socket, (struct sockaddr*) &sa, sizeof(sa)))	{
			log( "Error on bind() : %d\n", errno);
			return NULL;
		}
		if ( 0 != listen(m_socket, SOMAXCONN))	{
			log( "Error on listen() : %d\n", errno);
			return NULL;
		}
	
		struct epoll_event ee;
		ee.events = EPOLLIN;
		ee.data.fd = m_socket;
		
		if( 0 != epoll_ctl( m_epoll_fd, EPOLL_CTL_ADD, m_socket, &ee) )
		{
			log("Error on epoll_ctl() : %d\n", errno);
			return NULL;
		}
	}

	// select loop
	while(!m_bStopThread)	{
		Process();
	}

	log( "close()\n");
	if(m_iPort != 0){
		close(m_socket);
	}
	return 0;
}



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
void CServer<C>::Process()
{
	int iRet;
	SOCKET socket;
	int fd;
	int nRecv, nSend;
	struct epoll_event ee;
	
	shared_ptr<C> pClient;

#ifndef __NOTUSETIMER__
	//gettimeofday(&m_CurrTime, NULL); //9add
	//gettimeofday(&m_LastTime, NULL); //9add
#endif

	iRet = epoll_wait(m_epoll_fd, m_epoll_events.get(), m_nMaxClients, m_delay);

	if( -1 == iRet )
	{
		SM_E pLog->Write("E F[CServer<C>::Process] E[epoll_wait -1 == iRet] D[wait fail]");
		//log("Error on epoll_wait()\n", errno);
		return;
	}

	for(int i=0; i < iRet; i++)
	{
		if(m_iPort != 0){
			// accept
	
			if( m_epoll_events[i].data.fd == m_socket ) 
			{
				if( m_epoll_events[i].events & EPOLLIN )
				{
					socket = accept(m_socket, NULL, NULL);
					if( -1 == socket )	
					{
						if( EAGAIN != errno )
						{
							log( "accept : INVALID_SOCKET\n");
						}
					}
					else 
					{
						if( m_nCurrClients < m_nMaxClients )	
						{
							fcntl(socket, F_SETFL, O_NONBLOCK);
							AddClient(socket);
						}
						else 
						{
							close(socket);
						}
					}
				}
				continue;
			}
		}

		fd = m_epoll_events[i].data.fd;
		pClient = m_ClientList.GetClient(fd);

		if( pClient.get() == NULL ) 
		{
			//SM_N pLog->Write("N F[CServer<C>::Process] S[pClient.get() == NULL]");

			continue;
		}

		if( pClient->m_bRemove ) 
		{
			//SM_N pLog->Write("N F[CServer<C>::Process] S[pClient->m_bRemove]");

			continue;
		}

		if( m_epoll_events[i].events & EPOLLIN )
		{
			//SM_N pLog->Write("N F[CServer<C>::Process] S[events & EPOLLIN]");

			if((pClient->m_iMaxReadBuffer - pClient->m_iReadBuffer) <= 0)
			{
				connectFail(pClient, 1);
				pClient->RemoveClient();
				continue;
			}

			nRecv = recv(pClient->m_socket, 
				&pClient->m_ReadBuffer[pClient->m_iReadBuffer],
				pClient->m_iMaxReadBuffer - pClient->m_iReadBuffer,
				0);			

			if( nRecv < 0 )
			{
				if( EAGAIN != errno )
				{
					connectFail(pClient,2);
					pClient->RemoveClient(); //접속에 실패한경우 타는 루틴
					continue;
				}
			} 
			else if( 0 == nRecv )
			{
				connectFail(pClient,2);
				pClient->RemoveClient();
				continue;
			} 
			else if( nRecv > 0 )
			{
				pClient->m_iReadBuffer += nRecv;
				if( !Parser(pClient) )
				{
					pClient->RemoveClient();
					continue;
				}
			}
		}
		
		if( m_epoll_events[i].events & EPOLLOUT )
		{
			if(pClient->mIs_Connected == false) //hsj
			{
				//SM_N pLog->Write("F[CServer<C>::Process] S[mIs_Connected = true] V[scoket:%d]", pClient->m_socket);

				pClient->mIs_Connected = true;

				if(pClient->mIs_ItoSconnect == CONNECT_ItoS)
				{
					/*수정수정
					struct sockaddr_in ca;
					int addr_size = sizeof(ca);

					if( 0 != getpeername(pClient->m_socket, (struct sockaddr *)&ca, (socklen_t *)&addr_size) )
					{
						//SM_N pLog->Write("N F[CServer<C>::Process] E[Error on getpeername():%d]", errno);
						//log("N F[CServer<C>::Process] Error on getpeername() : %d\n", errno); 
						return; //수정수정
					}

					pClient->m_IP = (string)(inet_ntoa(ca.sin_addr));
					pClient->m_nPort = ntohs(ca.sin_port);
					*/

					pClient->Connected();
				}
			}

			nSend = send(pClient->m_socket, pClient->m_WriteBuffer.get(), pClient->m_iWriteBuffer, 0);
			if( nSend < 0 )
			{
				if( EAGAIN != errno )
				{
					sendBufFail(pClient, 1 ); 
					pClient->RemoveClient();
					continue;
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
				DoNext(pClient);
				if( 0 == pClient->m_iWriteBuffer ) 
				{
					ee.events = EPOLLIN | EPOLLERR;
					ee.data.fd = pClient->m_socket;
	
					if( 0 != epoll_ctl( m_epoll_fd, EPOLL_CTL_MOD, pClient->m_socket, &ee) )
					{
						log("Error on epoll_ctl() : %d\n", errno);
						sendBufFail(pClient, 2 );
						pClient->RemoveClient();
						continue;
					}
				}
			}
		}
		
		if( m_epoll_events[i].events & EPOLLERR )
		{
			connectError(pClient, 1);
			pClient->RemoveClient();
			continue;
		}
	}

#ifndef __NOTUSETIMER__
// check_timer
	/*
	gettimeofday(&m_CurrTime, NULL);
	m_TimeDiff += ( m_CurrTime.tv_sec - m_LastTime.tv_sec ) * 1000 + ( m_CurrTime.tv_usec - m_LastTime.tv_usec ) / 1000;
	if(m_TimeDiff > (unsigned long)m_timer)
	{
		TimerFunc();
		m_TimeDiff = 0;
	}

	m_LastTime = m_CurrTime;
	*/

	gettimeofday(&m_CurrTime, NULL);
	//m_TimeDiff += ( m_CurrTime.tv_sec - m_LastTime.tv_sec ) * 1000 + ( m_CurrTime.tv_usec - m_LastTime.tv_usec ) / 1000;

	timersub(&m_CurrTime, &m_LastTime, &mGabTime);

	if(mGabTime.tv_sec < 0)
	{
		pLog->Write("N F(mGabTime.tv_sec  %ld)", mGabTime.tv_sec);
		m_LastTime = m_CurrTime;
	}
	if(mGabTime.tv_usec < 0)
	{
		pLog->Write("N F(mGabTime.tv_usec  %ld)", mGabTime.tv_usec);
	}

	//if(m_TimeDiff > (unsigned long)m_timer)
	if(mGabTime.tv_sec * 1000 + mGabTime.tv_usec/1000 > /*(unsigned long)*/m_timer)
	{
		TimerFunc();
		//m_TimeDiff = 0;

		m_LastTime = m_CurrTime;
	}

#endif

	// Remove marked clients

#ifndef __NOTPROCESSINCSERVER__
	m_pRemoveList->Lock();	
	for(unsigned int i=0; i<m_pRemoveList->m_RemoveList.size(); i++)
	{
		RemoveClient(m_pRemoveList->m_RemoveList[i]);
	}
	m_pRemoveList->m_RemoveList.clear();
	m_pRemoveList->Unlock();	
#endif
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
	shared_ptr<C> pClient;
	pClient = m_ClientList.GetClient(ulKey);

	return pClient;
}

/*
 * FUNCTION: RemoveClientList
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   제거시키려는 Client를 일괄 제거
 *-----------------------------------------------------------------------------
 * PARAMETERS
 *   node
 *-----------------------------------------------------------------------------
 * SEE
 *   node
 *-----------------------------------------------------------------------------
 * RETURN VALUE
 *   node
 *-----------------------------------------------------------------------------
 * SIDE EFFECTS
 *   none
 *-----------------------------------------------------------------------------
 * Author
 *   bestchoice
 *-----------------------------------------------------------------------------
 * History
 *   2008. 08. 18 : Created                                            - 조진영
 */
template <typename C>
void CServer<C>::RemoveClientList()
{
    m_pRemoveList->Lock();
    for(unsigned int i = 0; i < m_pRemoveList->m_RemoveList.size(); i++)
    {
        RemoveClient(m_pRemoveList->m_RemoveList[i]);
    }
    m_pRemoveList->m_RemoveList.clear();
    m_pRemoveList->Unlock();
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
	fcntl(s, F_SETFL, O_NONBLOCK);
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = inet_addr(szAddr);
	addr.sin_port        = htons(iPort);


	if( 0 != connect(s,(const struct sockaddr *)&addr, sizeof(addr)) )
	{
		if( EINPROGRESS != errno )
		{
			printf("An error occurred while connect() : %s(%d)\n", strerror(errno), errno);
			close(s);
			return false;
		}
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
	max_fd = s + 1;

	n = select(max_fd, &rset, &wset, &eset, &delay);

	if( SOCKET_ERROR == n )	
	{
		printf("An error occured while select()\n");
		closesocket(s);
		return false;
	} 
	else if ( 0 == n ) // Time Out
	{
		printf("Time Out\n");
		closesocket(s);
		return false;
	}

	if( FD_ISSET(s, &eset) )
	{
		closesocket(s);
		return false;
	}	

	if( FD_ISSET(s, &rset) || FD_ISSET(s, &wset) )	
	{
		if( 0 != connect(s,(const struct sockaddr *)&addr, sizeof(addr)))
			return false;

		if( AddClient(s) )
			return true;
		else 
			return false;
	}
	return false;
}

template <typename C>
int CServer<C>::Connect(char *szAddr, int iPort, shared_ptr<C>&_spClient)
{
	struct  sockaddr_in addr;    
	SOCKET s;

	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if( INVALID_SOCKET == s ) 
		return false;

	fcntl(s, F_SETFL, O_NONBLOCK);
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = inet_addr(szAddr);
	addr.sin_port        = htons(iPort);

	bool m_IsConNetDirect = false;

	if( 0 != connect(s,(const struct sockaddr *)&addr, sizeof(addr)) )
	{
		if( EINPROGRESS != errno )
		{
			pLog->Write("N F[CServer<C>::Connect] S[EINPROGRESS != errno] V[%s:%d] Ip:%s", strerror(errno), errno, szAddr);
			//printf("An error occurred while connect() : %s(%d)\n", strerror(errno), errno);
			close(s);
			return CONNECT_ConNet_Failed;
		}
	} 
	else
	{
		m_IsConNetDirect = true;
		//return CONNECT_ConNet_Direct;
	}

	_spClient = AddClient(s, CONNECT_ItoS);

	if(_spClient == shared_ptr<C>((C*)NULL))
	{
		SM_E pLog->Write("E# F[CServer<C>::Connect] E[_spClient == shared_ptr<C>((C*)NULL)] V[sk:%ld]", s);
		close(s);
		return CONNECT_ConNet_AddcltFail;
	}	

	if(m_IsConNetDirect)
	{
		SM_E pLog->Write("E# F[CServer<C>::Connect] S[Connect Direct] V[sk:%ld]", s);
		_spClient->mIs_Connected = true;
		return CONNECT_ConNet_Direct;
	}
	else
	{
		//SM_N pLog->Write("N F[CServer<C>::Connect] S[Connect Wait] V[sk:%ld]", s);
		return CONNECT_ConNet_Wait;
	}
}

template <typename C>
bool CServer<C>::Check_ClientListConnected()
{
	shared_ptr< C > pClient;

	typedef typename unordered_map <unsigned long, boost::shared_ptr<C> > mapClient_l;
	typedef typename mapClient_l::iterator itmapClient_l;
	itmapClient_l R;

	struct timeval m_CurrentTime, m_GabTime;
	gettimeofday(&m_CurrentTime, NULL);

	for( R = m_ClientList.Begin(); R != m_ClientList.End(); R++ )	
	{
		pClient = ( shared_ptr<C> )(*R).second;

		//if( !pClient->m_bRemove/* && pClient->mIs_Connected==true*/)	
		{
			//접속된 상태

		}
		//else
		{
			/*
			if(!pClient->m_bRemove)
			{
				timersub(&m_CurrentTime, &pClient->mtime_ConnectStart, &m_GabTime);
				if(m_GabTime.tv_sec > 5)
				{
					printf("!!!!!!!!!!!@@@@@@@@");
					//pClient->Send((void*)" ", strlen(" "));
					send(pClient->m_socket, " ", strlen(" "), 0);
				}
			}
			*/

			//*
			timersub(&m_CurrentTime, &pClient->mtime_ConnectStart, &m_GabTime);

			if(m_GabTime.tv_sec < 0)
			{
				pClient->mtime_ConnectStart = m_CurrentTime;
			}

			int m_DisTime = 20;

			if(pClient->m_IsHelloEnd == true)
			{
				m_DisTime = 80;
				//pLog->Write("E# F[######################### CServer<C>::Connect] ");
			}

			//if(m_GabTime.tv_sec > 30)
			//if(m_GabTime.tv_sec > 15) //ori
			//if(m_GabTime.tv_sec > 20)
			if(m_GabTime.tv_sec > m_DisTime)
			//if(m_GabTime.tv_sec > 105) //hyundaiit.com 이 도메인 같은 경우 응답이 너무 늦게 오기때문에 시간을 늘려놨음
			{
				// 목록에서 삭제
				//SM_N pLog->Write("N F[CServer<C>::Check_ClientListConnected] S[m_GabTime.tv_sec > 30] D[delete Client] V[socket:%ld]", pClient->m_socket);
				connectFail(pClient, 2);
				pClient->RemoveClient();
			}
			//*/
		}
	}

	return true;
}

#endif




