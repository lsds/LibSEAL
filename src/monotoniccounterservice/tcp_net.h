/*
 * From https://github.com/schaars/kzimp/blob/master/checkpointing/src/comm_mech/tcp_net.h
 */

#ifndef TCP_NET_H_
#define TCP_NET_H_

int recvMsg(int s, void *buf, size_t len);

void sendMsg(int s, void *msg, size_t size);

#endif /* TCP_NET_H_ */
