#ifndef _STH_NETWORK_H_
#define _STH_NETWORK_H_

#ifdef __cplusplus
extern "C"
{
#endif				/* __cplusplus */

#define STR_GET_MSG "GET %s?%s\r\n"

// Socket 을 스트림 형식으로 읽어 들이거나 쓸 때 사용한다.
typedef struct
{
int socket;
FILE *rx;
FILE *tx;
}
StreamSocket;


int Accept (int fd);
char *GetPeerIP (int sockfd);
char *GetHTTP (char *target, char *query, const char *szHost,
	 const char *szPort, int qSize);
int TcpListen (const char *host, const char *serv, socklen_t * addrlenp);
int TcpConnect (const char *host, const char *serv);
int TcpConnectTimeout (const char *host, const char *serv, int nsec);
ssize_t Readline (int fd, void *ptr, size_t maxlen);
StreamSocket *OpenStreamSocket (int socket);
void CloseStreamSocket (StreamSocket * pSocket);


// INETD 방식으로 소켓을 읽고 쓸 때 사용
#define	INETD_SOCK_IDENTY	0

#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif				// _STH_NETWORK_H_
