/*************************************************************************
    > File Name: tcp_simultaneous_open.cc
    > Author: hsz
    > Brief:
    > Created Time: 2024年10月25日 星期五 15时35分09秒
 ************************************************************************/

#include <iostream>
#include <string>
#include <cstring>
#include <thread>
#include <map>
#include <vector>
#include <thread>
#include <functional>

#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

#include "common.h"

#include <log/log.h>

#define LOG_TAG "TCP simultaneous open"

int32_t Bind(const char *local_host, uint16_t local_port)
{
    // 创建套接字
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("socket error, errno: %d, errstr: %s\n", errno, strerror(errno));
        return -1;
    }

    int32_t flag = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (char*)&flag, sizeof(flag))) {
        perror("setsockopt(SO_REUSEPORT) error");
        close(sockfd);
        return -1;
    }

    sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = inet_addr(local_host);
    local_addr.sin_port = htons(local_port);

    if (bind(sockfd, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
        perror("bind error");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

void thread_func(int32_t sock, const char *peer_host, uint16_t peer_port)
{
    sockaddr_in remote_addr;
    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_addr.s_addr = inet_addr(peer_host);
    remote_addr.sin_port = htons(peer_port);

    if (peer_port == 12999) {
        usleep(5);
    }

    LOGI("begin");
    if (TimeoutConnect(sock, remote_addr, 1000)) {
        send(sock, "hello", 6, 0);
        char buf[16] = { '\0' };
        recv(sock, buf, sizeof(buf), 0);
        LOGI("recv: %s", buf);
    }
    LOGI("end");
}

int main(int argc, char **argv)
{
    const char *bind_host = "192.168.3.10";
    int32_t firstSock = Bind(bind_host, 12999);
    int32_t secondSock = Bind(bind_host, 13999);
    if (firstSock < 0 || secondSock < 0) {
        return 0;
    }

    std::thread th1(std::bind(&thread_func, firstSock, bind_host, 13999));
    std::thread th2(std::bind(&thread_func, secondSock, bind_host, 12999));

    th1.join();
    th2.join();
    return 0;
}
