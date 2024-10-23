/*************************************************************************
    > File Name: tcp_punch_hole.cc
    > Author: hsz
    > Brief:
    > Created Time: 2024年10月23日 星期三 09时46分57秒
 ************************************************************************/

#include <iostream>
#include <string>
#include <cstring>
#include <thread>
#include <map>
#include <vector>

#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

#include "common.h"

#define SERVER_PORT 13999

int main(int argc, char **argv)
{
    uint16_t local_port = 12999;
    const char *server_addr = nullptr;
    char c = '\0';
    while ((c = ::getopt(argc, argv, "s:p:")) > 0) {
        switch (c) {
        case 's':
            server_addr = optarg;
            break;
        case 'p':
            local_port = atoi(optarg);
            break;
        default:
            break;
        }
    }

    if (server_addr == nullptr) {
        printf("server address not set\n");
        return 0;
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("socket error, errno: %d, errstr: %s\n", errno, strerror(errno));
        return 0;
    }

    int32_t flag = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (char*)&flag, sizeof(flag))) {
        perror("setsockopt(SO_REUSEPORT) error");
        return 0;
    }

    sockaddr_in s_addr;
    memset(&s_addr, 0, sizeof(s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = inet_addr(server_addr);
    s_addr.sin_port = htons(SERVER_PORT);

    if (!TimeoutConnect(sockfd, s_addr, 1000)) {
        return 0;
    }

    sockaddr_in loacl_addr;
    socklen_t addr_len = sizeof(loacl_addr);
    getpeername(sockfd, (sockaddr *)&loacl_addr, &addr_len);
    printf("local info: %s:%u\n", inet_ntoa(loacl_addr.sin_addr), ntohs(loacl_addr.sin_port));

    // 连接成功
    PeerMessage msg;
    int32_t n = recv(sockfd, &msg, sizeof(PeerMessage), 0);
    if (n < 0) {
        perror("recv error");
        return 0;
    }

    printf("Peer Info: %s:%u\n", msg.host, msg.port);

    close(sockfd);

    // 创建套接字
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("socket error, errno: %d, errstr: %s\n", errno, strerror(errno));
        return 0;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (char*)&flag, sizeof(flag))) {
        perror("setsockopt(SO_REUSEPORT) error");
        return 0;
    }
    memset(&s_addr, 0, sizeof(s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = inet_addr(msg.host);
    s_addr.sin_port = htons(msg.port);

    int32_t i = 0;
    for (i = 0; i < 10; ++i) {
        if (TimeoutConnect(sockfd, s_addr, 100)) {
            printf("connect success\n");
            break;
        }
        usleep(500 * 1000);
    }

    if (i == 10) {
        printf("connect failed\n");
    }
    return 0;
}