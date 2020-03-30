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

	int ret = 0;
	char buf[4096];
	int size = sizeof(ca);
	
	int debug = 0;
	while( true )
	{
		sprintf(buf, "Hello world! %d\n", debug++);
		ret = sendto(sock, buf, strlen(buf), 0,
			(struct sockaddr *)&sa, sizeof(sa)); 
		if( -1 == ret )
		{
			printf("error sendto\n");
		}
		else
		{
			printf("recvfrom ...\n");
			ret = recvfrom(sock, buf, sizeof(buf), 
				0, (struct sockaddr *)&ca, (socklen_t *)&size);
			if( -1 == ret )
			{
				printf("error recvfrom\n");
			}
			else
			{
				buf[ret] = 0;
				printf("recvfrom ==> %s\n", buf);
			}
			printf("recvfrom finished.\n");
		}
		sleep(1);
	}

	close(sock);
	return 0;
}
