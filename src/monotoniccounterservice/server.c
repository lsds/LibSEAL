/*
 * Copyright 2018 Imperial College London
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at   
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/tcp.h> // for TCP_NODELAY
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>

#include "tcp_net.h"
#include "crypto.h"

void read_process_message(int fd) {
	char clear[MSG_SIZE];
	char cipher[ENC_MSG_SIZE];

	recvMsg(fd, cipher, ENC_MSG_SIZE);
	decrypt(cipher, ENC_MSG_SIZE, clear, MSG_SIZE);

	encrypt(clear, MSG_SIZE, cipher, ENC_MSG_SIZE);
	sendMsg(fd, cipher, ENC_MSG_SIZE);
}

int main(int argc, char **argv) {
	if (argc != 2) {
		printf("Usage: %s <port>\n", argv[0]);
		return -1;
	}

	int port = atoi(argv[1]);

	//create socket
	int ssocket = socket(AF_INET, SOCK_STREAM, 0);
	if (ssocket == -1) {
		perror("Error while creating the server socket! ");
		exit(errno);
	}

	int flag = 1;
	int result = setsockopt(ssocket, IPPROTO_TCP, TCP_NODELAY,
			(char*) &flag, sizeof(int));
	if (result == -1) {
		perror("Error while setting TCP NO DELAY on server socket! ");
	}

	struct sockaddr_in  bootstrap_sin;
	bootstrap_sin.sin_addr.s_addr = htonl(INADDR_ANY);
	bootstrap_sin.sin_family = AF_INET;
	bootstrap_sin.sin_port = htons(port);

	if (bind(ssocket, (struct sockaddr*) &bootstrap_sin, sizeof(bootstrap_sin)) == -1)  {
		perror("Error while binding to the socket! ");
		exit(errno);
	}

	// make the socket listening for incoming connections
	int backlog = 10;
	if (listen(ssocket, backlog) == -1) {
		perror("Error while calling listen! ");
		exit(errno);
	}

	int nfds = 0;
	int fds[10]; // 10 simultaneous connections should be more than sufficient

	while (1) {
		int i;

		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(ssocket, &readfds);

		int maxsock = ssocket;
		for (i=0; i<nfds; i++) {
			FD_SET(fds[i], &readfds);
			if (fds[i] > maxsock) {
				maxsock = fds[i];
			}
		}

		struct timeval timeout;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		int s = select(maxsock+1, &readfds, NULL, NULL, &timeout);
		if (s == -1) {
			perror("Select error");
			break;
		}

		//check other fds
		for (i=0; i<nfds; i++) {
			if (FD_ISSET(fds[i], &readfds)) {
				read_process_message(fds[i]);
			}
		}

		// accept new connection
		if (FD_ISSET(ssocket, &readfds)) {
			struct sockaddr_in csin;
			int sinsize = sizeof(csin);
			int fd = accept(ssocket, (struct sockaddr*) &csin, (socklen_t*) &sinsize);
			if (fd == -1) {
				perror("An invalid socket has been accepted! ");
				continue;
			}

			int flag = 1;
			int result = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
			if (result == -1) {
				perror("Error while setting TCP NO DELAY for accepted socket! ");
			}

			if (nfds == 10) {
				printf("Cannot accept new connection from %s:%i: too many open connections\n", inet_ntoa(csin.sin_addr), ntohs(csin.sin_port));
				close(fd);
			} else {
				printf("A connection has been accepted from %s:%i\n", inet_ntoa(csin.sin_addr), ntohs(csin.sin_port));
				fds[nfds++] = fd;
			}
		}
	}

	return 0;
}
