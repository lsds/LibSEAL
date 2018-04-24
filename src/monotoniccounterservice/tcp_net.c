/*
 * From https://github.com/schaars/kzimp/blob/master/checkpointing/src/comm_mech/tcp_net.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <sys/socket.h>
#include <errno.h>

#include "tcp_net.h"
#include "time.h"

int recvMsg(int s, void *buf, size_t len)
{
  size_t len_tmp = 0;
  int n;

  do
  {
    n = recv(s, &(((char *) buf)[len_tmp]), len - len_tmp, 0);

    if (n == -1)
    {
      perror("tcp_net:recv():");
      exit(-1);
    }

    len_tmp = len_tmp + n;
  } while (len_tmp < len);

  return len_tmp;
}

void sendMsg(int s, void *msg, size_t size)
{
  size_t total = 0; // how many bytes we've sent
  size_t bytesleft = size; // how many we have left to send
  int n = -1;

  while (total < size)
  {
    n = send(s, (char*) msg + total, bytesleft, 0);

    if (n == -1)
    {
      perror("tcp_net:send():");
      exit(-1);
    }

    total += n;
    bytesleft -= n;
  }
}
