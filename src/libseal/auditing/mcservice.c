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

#include "mcservice.h"

#ifdef USE_MONOTONIC_COUNTER_SERVICE

#define MC_SERVICE_PORT 8000
#define MSG_SIZE 16
#define ENC_MSG_SIZE MSG_SIZE + 32 // size of the encrypted message: IV + TAG + ciphertext

//NOTE: this code (but the initialize functions) should be thread-safe

static int quorum_size = 0;
static unsigned char original_ciphertext[ENC_MSG_SIZE] = {
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
	0xce, 0x1e, 0xca, 0x36, 0xe2, 0x31, 0x7, 0xb9, 0x63, 0x61, 0xf6, 0x96, 0x53, 0x2b, 0xac, 0xa9,
	0xc, 0xe7, 0xf9, 0xac, 0x68, 0x60, 0x4d, 0xf, 0x8c, 0x5a, 0x6d, 0x78, 0x59, 0xc1, 0xa2, 0xac
};

#endif

#ifdef COMPILE_WITH_INTEL_SGX

void ecall_mcservice_initialize(int q) {
#ifdef USE_MONOTONIC_COUNTER_SERVICE
	quorum_size = q;
#endif
}

#else

void ocall_mcservice_network_round(void) {
#ifdef USE_MONOTONIC_COUNTER_SERVICE
	mcservice_network_round();
#endif
}

#endif


#ifdef USE_MONOTONIC_COUNTER_SERVICE

#ifdef COMPILE_WITH_INTEL_SGX

#include <openssl/evp.h>
#include "sgx_error.h"

extern int my_printf(const char *format, ...);
extern int my_fprintf(FILE *stream, const char *format, ...);
extern sgx_status_t ocall_mcservice_network_round(void);

static const unsigned char key[16] = "0123456789012345";
static const unsigned char original_plaintext[MSG_SIZE] = "0123456789012345";
static void handleErrors(int loc) {
	my_printf("Problem with mcservice at %d\n", loc);
	exit(-1);
}

static int ssl_encrypt(const unsigned char *plaintext, int plaintext_len, const unsigned char *key, unsigned char *iv, unsigned char *ciphertext, unsigned char *tag) {
	EVP_CIPHER_CTX *ctx;
	int len;
	int ciphertext_len;

	/* Create and initialise the context */
	if(!(ctx = EVP_CIPHER_CTX_new())) handleErrors(1);

	/* Initialise the encryption operation. */
	if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL))
		handleErrors(2);

	/* Set IV length if default 12 bytes (96 bits) is not appropriate */
	if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 16, NULL))
		handleErrors(3);

	/* Initialise key and IV */
	if(1 != EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv)) handleErrors(4);

	/* Provide the message to be encrypted, and obtain the encrypted output.
	 * EVP_EncryptUpdate can be called multiple times if necessary
	 *                       */
	if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
		handleErrors(5);
	ciphertext_len = len;

	/* Finalise the encryption. Normally ciphertext bytes may be written at
	 * this stage, but this does not occur in GCM mode
	 *                       */
	if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) handleErrors(6);
	ciphertext_len += len;

	/* Get the tag */
	if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag))
		handleErrors(7);

	/* Clean up */
	EVP_CIPHER_CTX_free(ctx);

	return ciphertext_len;
}

static int ssl_decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *tag, const unsigned char *key, unsigned char *iv, unsigned char *plaintext) {
	EVP_CIPHER_CTX *ctx;
	int len;
	int plaintext_len;
	int ret;

	/* Create and initialise the context */
	if(!(ctx = EVP_CIPHER_CTX_new())) handleErrors(8);

	/* Initialise the decryption operation. */
	if(!EVP_DecryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL))
		handleErrors(9);

	/* Set IV length. Not necessary if this is 12 bytes (96 bits) */
	if(!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 16, NULL))
		handleErrors(10);

	/* Initialise key and IV */
	if(!EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv)) handleErrors(11);

	/* Provide the message to be decrypted, and obtain the plaintext output.
	 * EVP_DecryptUpdate can be called multiple times if necessary
	 */
	if(!EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
		handleErrors(12);
	plaintext_len = len;

	/* Set expected tag value. Works in OpenSSL 1.0.1d and later */
	if(!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag))
		handleErrors(13);

	/* Finalise the decryption. A positive return value indicates success,
	 * anything else is a failure - the plaintext is not trustworthy.
	 */
	ret = EVP_DecryptFinal_ex(ctx, plaintext + len, &len);

	/* Clean up */
	EVP_CIPHER_CTX_free(ctx);

	if(ret > 0) {
		plaintext_len += len;
		return plaintext_len;
	} else {
		return -1;
	}
}

void mcservice_encrypt_round(void) {
	if (quorum_size == 0) {
		return;
	}

	unsigned char ciphertext[ENC_MSG_SIZE] = {0};
	int r = ssl_encrypt(original_plaintext, MSG_SIZE, key, ciphertext, ciphertext + 32, ciphertext + 16);
	if (r == -1) {
		my_printf("encryption error in %s!\n", __func__);
	}
}

void mcservice_decrypt_round(void) {
	if (quorum_size == 0) {
		return;
	}

	unsigned char plaintext[MSG_SIZE] = {0};
	int i;
	for (i=0; i<quorum_size; i++) { 
		int r = ssl_decrypt(original_ciphertext+32, ENC_MSG_SIZE-32, original_ciphertext+16, key, original_ciphertext, plaintext);
		if (r == -1) {
			my_printf("decryption error in %s!\n", __func__);
		}
	}
}

void mcservice_network_round(void) {
	if (quorum_size > 0) {
		sgx_status_t ret = ocall_mcservice_network_round();
		if (ret != SGX_SUCCESS) {
			my_fprintf(0, "Error %d with ocall_mcservice_network_round() in %s!\n", ret, __func__);
		}
	}
}

#else

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

struct mc_server {
	char ip[17];
	int fd;
};

static struct mc_server* servers = NULL;

static int connect_to_server(char* host) {
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
	addr.sin_port = htons(MC_SERVICE_PORT);

	while (1) {
		if (connect(fd, (struct sockaddr *) &(addr), sizeof(addr)) < 0) {
			perror("Cannot connect! ");
			sleep(1);
		} else {
			printf("[%d] Connection successful to %s:%i!\n", fd, host, MC_SERVICE_PORT);
			break;
		}
	}

	return fd;
}

int mcservice_initialize(void) {
	int i, n;

	const char* filename = "monotonic_counter_service.txt";
	FILE* f = fopen(filename, "r");
	if (!f) {
		printf("%s:%i cannot open file %s!\n", __func__, __LINE__, filename);
		return -1;
	}

	// get file size then read it all
	fseek(f, 0, SEEK_END);
	int len = ftell(f);
	rewind(f);
	char* allchars = (char*)malloc(sizeof(*allchars)*(len+1));
	n = fread(allchars, sizeof(char), len, f);
	if (n != len) {
		printf("%s:%i cannot read all characters: %d != %d!\n", __func__, __LINE__, n, len);
		return -1;
	}
	fclose(f);
	allchars[len] = '\0';

	// split the buffer to get the ip addresses
	int start = 0;
	for (i=0; i<len; i++) {
		if (allchars[i] == '\n') {
			if (i-start > 16) {
				printf("reading malformed line @[%d, %d] in [%s]\n", start, i, allchars);
			} else {
				servers = (struct mc_server*)realloc(servers, sizeof(*servers)*(quorum_size+1));
				memcpy(servers[quorum_size].ip, allchars+start, i-start);
				servers[quorum_size].ip[i-start] = '\0';
				printf("reading server [%s]\n", servers[quorum_size].ip);
				servers[quorum_size].fd = connect_to_server(servers[quorum_size].ip);
				quorum_size++;
			}
			start = i+1;
		}
	}
	printf("mc_service, quorum_size=%d\n", quorum_size);

	return quorum_size;
}

void mcservice_network_round(void) {
	int i;
	if (quorum_size == 0) {
		return;
	}

	for (i=0; i<quorum_size; i++) {
		sendMsg(servers[i].fd, original_ciphertext, ENC_MSG_SIZE);
	}

	for (i=0; i<quorum_size; i++) {
		recvMsg(servers[i].fd, original_ciphertext, ENC_MSG_SIZE);
	}
}

#endif // COMPILE_WITH_INTEL_SGX

#endif // USE_MONOTONIC_COUNTER_SERVICE
