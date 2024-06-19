/*************************************************************************
    > File Name: kcp_main.cc
    > Author: hsz
    > Brief:
    > Created Time: Wed 05 Jun 2024 09:50:57 AM CST
 ************************************************************************/

#include <stdio.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>

#include <log/log.h>

#include "ikcp.h"

#define LOG_TAG "kcp"

#define BUFF_LEN        4 * 1400
#define SERVER_PORT     9000

int32_t CreateUDPSocket()
{
    int server_fd, ret;
    struct sockaddr_in addr;
    socklen_t len = sizeof(sockaddr_in);

    server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(server_fd < 0) {
        perror("create socket fail!");
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    ret = bind(server_fd, (struct sockaddr*)&addr, len);
    if(ret < 0) {
        perror("socket bind fail!");
        return -1;
    }

    // 设置地址重用选项，允许多个套接字绑定到相同的地址和端口
    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
        perror("setsockopt(SO_REUSEADDR) error");
    }

    return server_fd;
}

void ThreadEntry()
{
    int32_t sockFd = CreateUDPSocket();
    LOG_ASSERT2(sockFd > 0);

    struct pollfd fds[1];
    fds[0].fd = sockFd;
    fds[0].events = POLLIN;

    while (true) {
        int ret = poll(fds, sizeof(fds), -1);
        if (ret == -1) {
            perror("Poll error: ");
            break;
        }

        if (fds[0].revents & POLLIN) {
            // 接收数据
            static char buffer[BUFF_LEN];
            struct sockaddr_in clientAddr;
            socklen_t clientAddrLen = sizeof(clientAddr);
            ssize_t bytesRead = recvfrom(sockFd, buffer, BUFF_LEN, 0, (sockaddr *)(&clientAddr), &clientAddrLen);
            if (bytesRead == -1) {
                std::cerr << "Failed to receive data: " << strerror(errno) << std::endl;
            } else {
                printf("receive from [%s:%d]\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
            }
        }
    }
}

int main(int argc, char **argv)
{

    return 0;
}
