#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

int main()
{
	int sock;
	int iPort = 5096;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	
	struct sockaddr_in sa, ca;
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = inet_addr("127.0.0.1");
	sa.sin_port = htons(iPort);

	connect(sock, (struct sockaddr *)&sa, sizeof(sa));

	int ret = 0;
	char buf[4096];
	int size = sizeof(ca);
	
	int debug = 0;
	while( true )
	{
		sprintf(buf, "Hello world! %d\n", debug++);
		ret = send(sock, buf, strlen(buf), 0);
		if( -1 == ret )
		{
			printf("error sendto\n");
		}
		else
		{
			printf("recv ...\n");
			ret = recv(sock, buf, sizeof(buf), 0);
			if( -1 == ret )
			{
				printf("error recvfrom\n");
			}
			else
			{
				buf[ret] = 0;
				printf("recvfrom ==> %s\n", buf);
			}
			printf("recv finished.\n");
		}
		sleep(1);
	}

	close(sock);
	return 0;
}
