#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include "sth_network.h"
#include "sth_syslog.h"
#include "sth_misc.h"
#include "sth_string.h"
#include <fcntl.h>

extern int errno;

StreamSocket *
OpenStreamSocket( int socket )
{
	StreamSocket *ret;

	if( (ret = (StreamSocket *)Calloc(sizeof(StreamSocket))) == NULL ) return NULL;

	if( (ret->rx = fdopen(socket, "r")) == NULL )
	{
		Free(ret);
		return NULL;
	}

	if( (ret->tx = fdopen(socket, "w")) == NULL )
	{
		Fclose(ret->rx);
		Free(ret);
		return NULL;
	}

	ret->socket = socket;

	return ret;
}

void
CloseStreamSocket( StreamSocket *pSocket )
{
	if( pSocket == NULL ) return;

	if( pSocket->rx ) Fclose(pSocket->rx);
	if( pSocket->tx ) Fclose(pSocket->tx);
	close(pSocket->socket);
	Free(pSocket);

	return;
}

int 
_connect_nonb(int sockfd, const struct sockaddr *saptr, socklen_t salen, int nsec)
{
	int				flags, n, error;
	socklen_t		len;
	fd_set			rset, wset;
	struct timeval	tval;

	flags = fcntl(sockfd, F_GETFL, 0);
	fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

	error = 0;
	if ( (n = connect(sockfd, (struct sockaddr *) saptr, salen)) < 0)
		if (errno != EINPROGRESS)
			return(-1);

	/* Do whatever we want while the connect is taking place. */

	if (n == 0)
		goto done;	/* connect completed immediately */

	FD_ZERO(&rset);
	FD_SET(sockfd, &rset);
	wset = rset;
	tval.tv_sec = nsec;
	tval.tv_usec = 0;

	if ( (n = select(sockfd+1, &rset, &wset, NULL,
					 nsec ? &tval : NULL)) == 0) {
		close(sockfd);		/* timeout */
		errno = ETIMEDOUT;
		return(-1);
	}

	if (FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset)) {
		len = sizeof(error);
		if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
			return(-1);			/* Solaris pending error */
	} else
		Trace(TR_ERROR,"select error: sockfd not set");

done:
	fcntl(sockfd, F_SETFL, flags);	/* restore file status flags */

	if (error) {
		close(sockfd);		/* just in case */
		errno = error;
		return(-1);
	}
	return(0);
}

int
Accept(int fd)
{
	struct sockaddr_in peeraddr_in;	/* for peer socket address */
	int 	addrlen;
	int	sock;

	addrlen = sizeof(struct sockaddr_in);
	memset ((char *)&peeraddr_in, 0, sizeof(struct sockaddr_in));

again:
	if ( (sock = accept(fd, (struct sockaddr*)&peeraddr_in, &addrlen)) < 0) {
#ifdef	EPROTO
		if (errno == EPROTO || errno == ECONNABORTED)
#else
		if (errno == ECONNABORTED)
#endif
			goto again;
		else
			Trace(TR_ERROR,"accept error");
	}
	return(sock);
}

char *
GetPeerIP(int sockfd)
{
        struct  sockaddr_in     addr;
        static  char    buf[64];
        int     len;

        memset(buf, 0, sizeof(buf));

        /* Client */
        len = sizeof(addr);
        if (getpeername(sockfd, (struct sockaddr *)&addr,  &len)<0)
                return(0);

        strcpy(buf,(char *)inet_ntoa(addr.sin_addr));

        return(buf);
}

char *
GetHTTP( char *target, char *query, const char *szHost, const char *szPort, int qSize )
{
	int dbfd, n = 0, len = 0;
	char buf[4096+1];
	char *ret = NULL, *temp =NULL;
	int wres;
	char msg[qSize+300];

//	TcpConnectTimeout

	if( (dbfd = TcpConnectTimeout(szHost, szPort, 5) ) < 0 ){
		Trace( TR_WARNING, "Unable connect [%s:%s]",szHost,szPort);
		return NULL;
	}

	sprintf (msg, STR_GET_MSG, target, query);

	wres = write(dbfd, msg, strlen(msg));
	if( wres < 0 )
	{
		close(dbfd);
		return NULL;
	}
	buf[0] = '\0';

	while ((n = read(dbfd, buf, 4096)) > 0 ) {
		buf[n] = '\0';
		len += n;
		
		if ((temp = (char *)malloc(len + 1)) == NULL){
			free( ret );
			close(dbfd);
			return NULL;
		}

		if (ret != NULL) {
			strcpy(temp, ret);
			free(ret);
			strcat(temp, buf) ;
		}
		else
		{
			strcpy(temp,buf);	
		}

		ret = temp;
	}

	close(dbfd);

	return ret;
}

int
TcpListen(const char *host, const char *serv, socklen_t *addrlenp)
{
	int				listenfd, n;
	const int		on = 1;
	struct addrinfo	hints, *res, *ressave;

	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0)
		Trace(TR_ERROR,"TcpListen error for %s, %s: %s",
				 host, serv, gai_strerror(n));
	ressave = res;

	do {
		listenfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (listenfd < 0)
			continue;		/* error, try next one */

		setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
		if (bind(listenfd, res->ai_addr, res->ai_addrlen) == 0)
			break;			/* success */

		close(listenfd);	/* bind error, close and try next one */
	} while ( (res = res->ai_next) != NULL);

	if (res == NULL)	/* errno from final socket() or bind() */
		Trace(TR_ERROR,"TcpListen error for %s, %s", host, serv);

	listen(listenfd, 5);

	if (addrlenp)
		*addrlenp = res->ai_addrlen;	/* return size of protocol address */

	freeaddrinfo(ressave);

	return(listenfd);
} 

int
TcpConnect(const char *host, const char *serv)
{
	int				sockfd, n;
	struct addrinfo	hints, *res, *ressave;

	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0)
		Trace(TR_ERROR,"TcpConnect error for %s, %s: %s",
				 host, serv, gai_strerror(n));
	ressave = res;

	do {
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (sockfd < 0)
			continue;	/* ignore this one */

		if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0)
			break;		/* success */

		close(sockfd);	/* ignore this one */
	} while ( (res = res->ai_next) != NULL);

	if (res == NULL)
	{
		Trace(TR_ERROR,"TcpConnect error for %s, %s", host, serv);
		return -1;
	}
	
	freeaddrinfo(ressave);

	return(sockfd);
}

int
TcpConnectTimeout(const char *host, const char *serv, int nsec )
{
	int				sockfd, n;
	struct addrinfo	hints, *res, *ressave;

	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0)
		Trace(TR_ERROR,"tcp_connect error for %s, %s: %s",
				 host, serv, gai_strerror(n));
	ressave = res;

	do {
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (sockfd < 0)
			continue;	/* ignore this one */

		if (_connect_nonb(sockfd, res->ai_addr, res->ai_addrlen, nsec) == 0)
			break;		/* success */

		close(sockfd);	/* ignore this one */
	} while ( (res = res->ai_next) != NULL);

	if (res == NULL)	/* errno set from final connect() */
		Trace(TR_ERROR, "tcp_connect error for %s, %s", host, serv);

	freeaddrinfo(ressave);

	return(sockfd);
}

#ifndef MAXLINE
#define	MAXLINE	4096
#endif

static ssize_t
my_read(int fd, char *ptr, int mode )
{
	static int	read_cnt = 0;
	static char	*read_ptr;
	static char	read_buf[MAXLINE];
	int 		retval;
	int		count=0;

	if( mode == 0 ){
	   if (read_cnt <= 0) {
again:
		if ( (read_cnt = read(fd, read_buf, sizeof(read_buf))) < 0) {
			if (errno == EINTR) {
				if(count > 20) return (-999);
				count++;
				usleep(500);
				goto again;
			}
			return(-1);
		} else if (read_cnt == 0)
			return(0);
		read_ptr = read_buf;
	   }

	   read_cnt--;
	   *ptr = *read_ptr++;
	   return(1);
	} else {
	   retval = read_cnt;
	   if( read_cnt > 0 ){
	   	strncpy( ptr, read_ptr,read_cnt );
		read_cnt = 0;
	   }
	   return(retval);
	}
}

ssize_t
readline(int fd, void *vptr, size_t maxlen)
{
	ssize_t	n, rc;
	char	c, *ptr;

	ptr = vptr;
	for (n = 0; n < maxlen-1; ) {
		if ( (rc = my_read(fd, &c, 0)) == 1) {
			*ptr++ = c;
			n++;

			if (c == '\n')
				break;	/* newline is stored, like fgets() */
		} else if (rc == 0) {
			if (n == 1)
				return(0);	/* EOF, no data read */
			else
				break;		/* EOF, some data was read */
		} else
			return(rc);		/* error, errno set by read() */
	}

	*ptr = 0;	/* null terminate like fgets() */
	return(n);
}
/* end readline */

ssize_t
Readline(int fd, void *ptr, size_t maxlen)
{
	ssize_t		n;

	if ( (n = readline(fd, ptr, maxlen)) < 0)
		perror("readline");

	return(n);
}


