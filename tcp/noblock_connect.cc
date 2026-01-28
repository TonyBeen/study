/*************************************************************************
    > File Name: noblock_connect.cc
    > Author: hsz
    > Brief:
    > Created Time: 2024年04月08日 星期一 17时39分41秒
 ************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <string>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <unistd.h>

#define TCP_SERVER_IP   "0.0.0.0"
#define TCP_SERVER_PORT 14000

int main(int argc, char **argv)
{
    // 创建套接字
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

    // 设置套接字为非阻塞模式
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    // 准备连接的目标地址和端口
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(TCP_SERVER_IP);
    addr.sin_port = htons(TCP_SERVER_PORT);

    // 连接到目标地址
    int ret = connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    if (ret < 0 && errno != EINPROGRESS) { // 连接失败且不是EINPROGRESS
        // EINPROGRESS 表示仍在进行连接
        printf("Failed to connect: [%d:%s]\n", errno, strerror(errno));
        goto _error;
    }

    // 在非阻塞模式下，需要使用 select() 或 epoll() 等函数来等待连接完成
    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(sockfd, &fdset);
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100 * 1000; // 100ms内尝试连接
    ret = select(sockfd + 1, nullptr, &fdset, nullptr, &timeout);
    if (ret < 0) {
        printf("Failed to select: [%d:%s]\n", errno, strerror(errno));
        goto _error;
    } else if (ret == 0) { // 超时了
        printf("Connection timed out\n");
        goto _error;
    } else {  // 连接成功或失败
        int valopt;
        socklen_t optlen = sizeof(valopt);
        getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (void*)&valopt, &optlen);  // 获取连接结果
        if (valopt != 0) {  // 连接失败
            printf("Failed to connect: [%d:%s]\n", valopt, strerror(valopt));
            goto _error;
        }
    }

    // 恢复套接字为阻塞模式
    fcntl(sockfd, F_SETFL, flags);

    // 连接成功后，可以继续发送和接收数据

_error:
    close(sockfd);
    return 0;
}
