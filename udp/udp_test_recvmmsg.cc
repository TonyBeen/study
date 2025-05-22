/*************************************************************************
    > File Name: udp_test_recvmmsg.cc
    > Author: hsz
    > Brief:
    > Created Time: 2025年03月17日 星期一 14时26分52秒
 ************************************************************************/

// #define _GNU_SOURCE
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>

#define VLEN 10
#define BUFSIZE 200
#define TIMEOUT 1

int main(void)
{
    int sockfd, retval;
    char bufs[VLEN][BUFSIZE + 1];
    struct iovec iovecs[VLEN];
    struct mmsghdr msgs[VLEN];
    struct timespec timeout;
    struct sockaddr_in addr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1)
    {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(1234);
    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    memset(msgs, 0, sizeof(msgs));
    for (size_t i = 0; i < VLEN; i++)
    {
        iovecs[i].iov_base = bufs[i];
        iovecs[i].iov_len = BUFSIZE;
        msgs[i].msg_hdr.msg_iov = &iovecs[i];
        msgs[i].msg_hdr.msg_iovlen = 1;
    }

    timeout.tv_sec = TIMEOUT;
    timeout.tv_nsec = 0;

    struct mmsghdr mmsg[32];
    struct iovec dummy[32];
    for (int32_t i = 0; i < 32; ++i) {
        struct msghdr *msg = &mmsg[i].msg_hdr;
        memset(msg, 0, sizeof(struct msghdr));
        msg->msg_name = &kcp_conn->remote_host;
        msg->msg_namelen = sizeof(sockaddr_t);
        msg->msg_iov = data;
        msg->msg_iovlen = size;
    }

    retval = recvmmsg(sockfd, msgs, VLEN, 0, &timeout);
    if (retval == -1)
    {
        perror("recvmmsg()");
        exit(EXIT_FAILURE);
    }

    printf("%d messages received\n", retval);
    for (size_t i = 0; i < retval; i++)
    {
        bufs[i][msgs[i].msg_len] = 0;
        printf("%zu %s", i + 1, bufs[i]);
    }
    exit(EXIT_SUCCESS);
}