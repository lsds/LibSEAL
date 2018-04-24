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
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/tcp.h> // for TCP_NODELAY
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "tcp_net.h"
#include "crypto.h"

static char** hosts;
static int* fds;
static int port;
static int quorum_size;

int connect_to_server(char* host) {
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1) {
		perror("Error while creating the socket! ");
		exit(errno);
	}

	int flag = 1;
	int result = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
			(char*) &flag, sizeof(int));
	if (result == -1) {
		perror("Error while setting TCP NO DELAY! ");
	}

	struct sockaddr_in addr;
	addr.sin_addr.s_addr = inet_addr(host);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	while (1) {
		if (connect(fd, (struct sockaddr *) &(addr), sizeof(addr)) < 0) {
			perror("Cannot connect! ");
			sleep(1);
		} else {
			printf("[%d] Connection successful to %s:%i!\n", fd, host, port);
			break;
		}
	}

	return fd;
}

void run(void) {
	int i;
	char clear[MSG_SIZE];
	char cipher[ENC_MSG_SIZE];

	fds = (int*)malloc(sizeof(*fds)*quorum_size);
	for (i=0; i<quorum_size; i++) {
		fds[i] = connect_to_server(hosts[i]);
	}

	while (1) {
		printf(">===== new iteration =====<\n");

		memcpy(clear, (unsigned char *)"0123456789012345", 16); // large number
		clear[12] = '\0';
		encrypt(clear, MSG_SIZE, cipher, ENC_MSG_SIZE);
		for (i=0; i<quorum_size; i++) {
			sendMsg(fds[i], cipher, ENC_MSG_SIZE);
			printf("send to %d: [%s]\n", i, clear);
		}

		for (i=0; i<quorum_size; i++) {
			recvMsg(fds[i], cipher, ENC_MSG_SIZE);
			decrypt(cipher, ENC_MSG_SIZE, clear, MSG_SIZE);
			printf("receive from %d: [%s]\n", i, clear);
		}

		sleep(1);
	}

	for (i=0; i<quorum_size; i++) {
		close(fds[i]);
	}
	free(fds);
}

int main(int argc, char **argv) {
	if (argc < 4) {
		printf("Usage: %s <port> <quorum_size> <host1> [... <hostn>]\n", argv[0]);
		return -1;
	}

	port = atoi(argv[1]);
	quorum_size = atoi(argv[2]);
	hosts = &argv[3];

	run();

	return 0;
}
