#include "fiber.h"
#include <log/log.h>
#include <utils/errors.h>
#include <utils/thread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/time.h>
#include <sys/epoll.h>

#define LOG_TAG "epoll"
#define EPOLL_EVENT_SIZE 512
#define RD_BUF_SIZE 512
#define WR_BUF_SIZE 512

/**
EPOLLERR = 8               ----发生错误
EPOLLET = 2147483648       ----默认为水平触发，设置该事件后则边缘触发
EPOLLHUP = 16              ----挂起状态
EPOLLIN = 1                ----可读
EPOLLMSG = 1024            ----忽略
EPOLLONESHOT = 1073741824  ----一次性行为。在退出一个事件后，FD内部禁用
EPOLLOUT = 4               ----可写
EPOLLPRI = 2               ----紧急可读
EPOLLRDBAND = 128          ----读取优先
EPOLLRDNORM = 64           ----相当于epollin
EPOLLWRBAND = 512          ----写入优先
EPOLLWRNORM = 256          ----相当于epollout
*/

int InitSocket(uint16_t port = 8000)
{
    int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        LOGE("socket error. error code = %d, error message: %s", errno, strerror(errno));
        return eular::UNKNOWN_ERROR;
    }
    sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); //inet_addr("172.25.12.215");

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (::bind(sock, (sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        LOGE("bind error. error code = %d, error message: %s", errno, strerror(errno));
        goto error_return;
    }
    if (::listen(sock, 128) < 0) {
        LOGE("listen error. error code = %d, error message: %s", errno, strerror(errno));
        goto error_return;
    }
    return sock;

error_return:
    ::close(sock);
    return eular::UNKNOWN_ERROR;
}

static int gServerSocket = 0;
static int gEpollFd = 0;

void AcceptClient()
{
    LOGI("%s() begin", __func__);
    if (!gServerSocket || !gEpollFd) {
        return;
    }
    static epoll_event event;
    sockaddr_in client_addr;
    bzero(&client_addr, sizeof(client_addr));
    socklen_t addrLen = 0;
    int clientSock = 0;
    clientSock = ::accept(gServerSocket, (sockaddr *)&client_addr, &addrLen);
    if (clientSock < 0) {
        LOGE("accept error. error code = %d, error message: %s", errno, strerror(errno));
        return;
    }
    LOGI("[IP: %s, port: %u] connected.", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    // 设置非阻塞
    int flags = fcntl(clientSock, F_GETFL, 0);
    fcntl(clientSock, F_SETFL, flags | O_NONBLOCK);
    int keepAlive = 1;      // 开启keepalive属性
    int keepIdle = 3;      // 如该连接在60秒内没有任何数据往来,则进行探测 
    int keepInterval = 2;   // 探测时发包的时间间隔为5 秒
    int keepCount = 2;      // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.
    setsockopt(clientSock, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepAlive, sizeof(keepAlive));
    setsockopt(clientSock, SOL_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(keepIdle));
    setsockopt(clientSock, SOL_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval));
    setsockopt(clientSock, SOL_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount));

    event.data.fd = clientSock;
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(gEpollFd, EPOLL_CTL_ADD, clientSock, &event) != 0) {
        LOGE("epoll_ctl error. error code = %d, error message: %s", errno, strerror(errno));
    }
    LOGI("%s() end", __func__);
}

static epoll_event gEvent;
static int gClientFd = 0;
void ReadFormClient()
{
    LOGI("%s() begin", __func__);
    if (gClientFd <= 0) {
        return;
    }
    char readBuf[RD_BUF_SIZE] = {0};
    bzero(readBuf, RD_BUF_SIZE);
    int readSize = ::recv(gClientFd, readBuf, RD_BUF_SIZE, 0);
    if (readSize < 0) {
        LOGE("read error. error code = %d, error message: %s", errno, strerror(errno));
        if (errno == ECONNRESET) {
            epoll_ctl(gEpollFd, EPOLL_CTL_DEL, gClientFd, &gEvent);
            close(gClientFd);
        }
    } else if (readSize == 0) {
        // client quit
        struct tcp_info info;
        socklen_t len;
        getsockopt(gClientFd, IPPROTO_TCP, TCP_INFO, &info, &len);
        if (info.tcpi_state == TCP_CLOSING || info.tcpi_state == TCP_CLOSE_WAIT) {
            LOGI("close wait");
        }
        LOGI("client quit fd = %d, errno %d, %s. info: state %d", gClientFd, errno, strerror(errno), info.tcpi_state);
        epoll_ctl(gEpollFd, EPOLL_CTL_DEL, gClientFd, &gEvent);
        close(gClientFd);
    } else {
        LOGI("recv buf: %s\n", readBuf);
    }
    LOGI("%s() end", __func__);
    gClientFd = 0;
}

int main(int argc, char *argv[])
{
    int serverSock = InitSocket();
    if (serverSock < 0) {
        return 0;
    }

    int keepAlive = 1;      // 开启keepalive属性
    int keepIdle = 3;       // 如该连接在60秒内没有任何数据往来,则进行探测 
    int keepInterval = 2;   // 探测时发包的时间间隔为5 秒
    int keepCount = 2;      // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.
    setsockopt(serverSock, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepAlive, sizeof(keepAlive));
    setsockopt(serverSock, SOL_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(keepIdle));
    setsockopt(serverSock, SOL_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval));
    setsockopt(serverSock, SOL_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount));

    char readBuf[RD_BUF_SIZE] = {0};
    int allSock[EPOLL_EVENT_SIZE] = {0};
    int clientSock = -1;
    sockaddr_in client_addr;
    bzero(&client_addr, sizeof(client_addr));
    socklen_t addrLen = 0;

    struct epoll_event event;
    struct epoll_event allEvent[EPOLL_EVENT_SIZE];

    int epollfd = epoll_create(EPOLL_EVENT_SIZE);
    event.events = EPOLLIN | EPOLLET;   // 边沿触发，数据到达才会触发
    event.data.fd = serverSock;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, serverSock, &event);
    gServerSocket = serverSock;
    gEpollFd = epollfd;
    eular::Fiber::GetThis();
    eular::Fiber::sp acceptFiebr(new eular::Fiber(AcceptClient));
    eular::Fiber::sp readFiber(new eular::Fiber(ReadFormClient));

    LOGI("server fd[%d] waiting for client...", serverSock);
    while (1) {
        int ret = epoll_wait(epollfd, allEvent, EPOLL_EVENT_SIZE, -1);
        if (ret < 0) {
            LOGE("epoll_wait error. error code = %d, error message: %s", errno, strerror(errno));
            if (errno == EINTR) {
                continue;
            }
            break;
        } else if (ret > 0) {
            LOGI("event size = %d", ret);
            for (int i = 0; i < ret; ++i) {
                epoll_event &ev = allEvent[i];
                int fdtmp = allEvent[i].data.fd;
                gClientFd = fdtmp;
                LOGI("fd = %d, event = 0x%x\n", fdtmp, ev.events);
                if (ev.events & (EPOLLHUP | EPOLLRDHUP | EPOLLERR)) { // 退出事件或对端关闭写
                    LOGI("client %d exit.", ev.data.fd);
                    close(ev.data.fd);
                    epoll_ctl(epollfd, EPOLL_CTL_DEL, ev.data.fd, nullptr);
                    continue;
                }
                if (fdtmp == serverSock) {    // accept event
                    LOGI("client is comed. event = 0x%x", ev.events);
                    acceptFiebr->Resume();
                    acceptFiebr->Reset(AcceptClient);
                } else if (allEvent[i].events & EPOLLIN) {    // read event
                    readFiber->Resume();
                    readFiber->Reset(ReadFormClient);
                }
            }
        }
    }
    exit(0);
}