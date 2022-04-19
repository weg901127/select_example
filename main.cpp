#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include "sys/socket.h"
#include "sys/time.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "netinet/in.h"
#include "arpa/inet.h"

#define FD_MAX_SIZE 1024

int main(int argc, char **argv)
{
	int server_sockfd, client_sockfd, sockfd;
	int state, client_len;
	int pid;
	int i, max_client, maxfd;
	int client[FD_MAX_SIZE];

	FILE *fp;
	struct sockaddr_in clientaddr, serveraddr;
	struct timeval tv;
	fd_set readfds, otherfds, allfds;

	int current_cli_num;
	char buf[100000];
	char line[255];

	memset(line, 0x00, 255);
	state = 0;
	current_cli_num = 3;

	if ((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket error : ");
		exit(0);
	}

	memset(&serveraddr, 0x00, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(80);

	state = bind(server_sockfd, (struct sockaddr *)&serveraddr,
	             sizeof(serveraddr));
	if (state == -1) {
		perror("bind error : ");
		exit(0);
	}

	state = listen(server_sockfd, 5);
	if (state == -1) {
		perror("listen error : ");
		exit(0);
	}

	client_sockfd = server_sockfd;

	max_client = -1;
	maxfd = server_sockfd;

	for (i = 0; i < FD_MAX_SIZE; i++) {
		client[i] = -1;
	}

	FD_ZERO(&readfds);
	FD_SET(server_sockfd, &readfds);

	printf("\nTCP Server Waiting ...");
	fflush(stdout);

	while(1)
	{
		allfds = readfds;
		state = select(maxfd + 1 , &allfds, NULL, NULL, NULL);

		if (FD_ISSET(server_sockfd, &allfds)) {
			client_len = sizeof(clientaddr);
			client_sockfd = accept(server_sockfd,
			                       (struct sockaddr *)&clientaddr, reinterpret_cast<socklen_t *>(&client_len));
			printf("\nconnection from (%s , %d)",
			       inet_ntoa(clientaddr.sin_addr),ntohs(clientaddr.sin_port));

			for (i = 0; i < FD_MAX_SIZE; i++)
			{
				if (client[i] < 0) {
					client[i] = client_sockfd;
					printf("\nclientNUM=%d\nclientFD=%d\n", i+1, client_sockfd);
					break;
				}
			}

			printf("accept [%d]\n", client_sockfd);
			printf("===================================\n");
			if (i == FD_MAX_SIZE) {
				perror("too many clients\n");
			}
			FD_SET(client_sockfd,&readfds);

			if (client_sockfd > maxfd)
				maxfd = client_sockfd;

			if (i > max_client)
				max_client = i;

			if (--state <= 0)
				continue;

		}
		for (i = 0; i <= max_client; i++)
		{
			if ((sockfd = client[i]) < 0) {
				continue;
			}

			if (FD_ISSET(sockfd, &allfds))
			{
				memset(buf, 0x00, 100000);

				if (read(sockfd, buf, 255) <= 0) {
					close(sockfd);
					perror("Close sockfd ");
					FD_CLR(sockfd, &readfds);
					client[i] = -1;
				}
				printf("[%d]RECV %s\n", sockfd, buf);
				write(sockfd, "HTTP/1.1 200 OK\n"
				              "Host: 127.0.0.1\n"
				              "Content-Type: text/html; charset=UTF-8\n"
				              "Connection: close\n"
				              "Content-Length: 2", 17 + 17 + 40 + 19 + 17);
				if (--state <= 0) {
					std::cout << state << std::endl;
					break;
				}
			}
		}
	}
}