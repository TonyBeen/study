/*************************************************************************
    > File Name: common.h
    > Author: hsz
    > Brief:
    > Created Time: 2024年10月23日 星期三 09时47分51秒
 ************************************************************************/

#pragma once

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <memory>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

struct PeerMessage
{
    uint16_t    is_server;
    char        host[32];
    uint16_t    port;
};

auto size = sizeof(PeerMessage);

bool ReusePortAddr(int32_t sock, int32_t reuse = 1)
{
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, (char*)&reuse, sizeof(reuse))) {
        perror("setsockopt(SO_REUSEPORT) error");
        return false;
    }

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse))) {
        perror("setsockopt(SO_REUSEPORT) error");
        return false;
    }

    return true;
}

bool TimeoutConnect(int32_t sockfd, sockaddr_in addr, uint32_t timeoutMS)
{
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("fcntl error");
    }

    std::shared_ptr<void>(nullptr, [&](void *) {
        // 恢复套接字为阻塞模式
        fcntl(sockfd, F_SETFL, flags);
    });

    // 连接到目标地址
    int ret = connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    if (ret < 0 && errno != EINPROGRESS) { // 连接失败且不是EINPROGRESS
        // EINPROGRESS 表示仍在进行连接
        printf("Failed to connect: [%d:%s]\n", errno, strerror(errno));
        return false;
    }

    // 在非阻塞模式下，需要使用 select() 或 epoll() 等函数来等待连接完成
    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(sockfd, &fdset);
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    ret = select(sockfd + 1, nullptr, &fdset, nullptr, &timeout);
    if (ret < 0) {
        printf("Failed to select: [%d:%s]\n", errno, strerror(errno));
        return false;
    } else if (ret == 0) { // 超时了
        printf("Connection timed out\n");
        return false;
    } else {  // 连接成功或失败
        int valopt = -1;
        socklen_t optlen = sizeof(valopt);
        getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (void*)&valopt, &optlen);  // 获取连接结果
        if (valopt != 0) {  // 连接失败
            printf("Failed to connect: [%d:%s]\n", valopt, strerror(valopt));
            return false;
        }
    }

    return true;
}
