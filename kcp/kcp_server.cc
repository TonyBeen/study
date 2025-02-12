/*************************************************************************
    > File Name: kcp_main.cc
    > Author: hsz
    > Brief:
    > Created Time: Wed 05 Jun 2024 09:50:57 AM CST
 ************************************************************************/

#include <stdio.h>
#include <iostream>
#include <chrono>
#include <atomic>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/poll.h>

#include <utils/timer.h>
#include <log/log.h>
#include <log/callstack.h>

#include "ikcp.h"

#define LOG_TAG "kcp-server"

#define BUFF_LEN        4 * 1400
#define SERVER_PORT     21501

struct KcpUserParam
{
    ikcpcb*     kcp_handle;
    int32_t     sock_fd;
    sockaddr_in sock_addr;
};

uint64_t GetCurrentTimeMS()
{
    std::chrono::steady_clock::time_point tm = std::chrono::steady_clock::now();
    std::chrono::milliseconds mills = 
        std::chrono::duration_cast<std::chrono::milliseconds>(tm.time_since_epoch());
    return mills.count();
}

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
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(SERVER_PORT);

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

    int32_t flag = fcntl(server_fd, F_GETFL);
    fcntl(server_fd, F_SETFL, flag | O_NONBLOCK);

    return server_fd;
}

int KcpOutput(const char *buf, int len, ikcpcb *kcp, void *user)
{
    KcpUserParam *__kcp = static_cast<KcpUserParam *>(user);
    if (buf && len > 0) {
        LOGD("kcp callback. sendto [%s:%d] len %d", inet_ntoa(__kcp->sock_addr.sin_addr), ntohs(__kcp->sock_addr.sin_port), len);
        return ::sendto(__kcp->sock_fd, buf, len, 0, (sockaddr *)&__kcp->sock_addr, sizeof(__kcp->sock_addr));
    }

    return 0;
}

std::atomic<uint64_t> g_recvSize;
void onTimeOut()
{
    double recvSize = static_cast<double>(g_recvSize.load());
    g_recvSize = 0;
    if (recvSize / 1000 / 1000 > 0) {
        LOGW("onTimeOut() %.2f MB/s", recvSize / 1000 / 1000);
    } else if (recvSize / 1000 > 0) {
        LOGW("onTimeOut() %.2f KB/s", recvSize / 1000);
    } else {
        LOGW("onTimeOut() %.2f B/s", recvSize);
    }
}

void ThreadEntry()
{
    int32_t sockFd = CreateUDPSocket();
    LOG_ASSERT2(sockFd > 0);

    KcpUserParam * pKcpUserParam = (KcpUserParam *)malloc(sizeof(KcpUserParam));
    ikcpcb *kcpHandle = ikcp_create(0x1024, pKcpUserParam);
    pKcpUserParam->kcp_handle = kcpHandle;
    pKcpUserParam->sock_fd = sockFd;

    static const uint32_t INTERVAL = 20;
    int32_t interval = INTERVAL;

    ikcp_setoutput(kcpHandle, &KcpOutput);
    ikcp_wndsize(kcpHandle, 512, 512);
    ikcp_nodelay(kcpHandle, 1, interval, 2, 1);

    struct pollfd fds[1];
    fds[0].fd = sockFd;
    fds[0].events = POLLIN;

    uint64_t timeOld = GetCurrentTimeMS();
    uint64_t timeNow = 0;

    eular::TimerManager timerMagager;
    timerMagager.addTimer(1000, &onTimeOut, 1000);
    timerMagager.startTimer(false);

    while (true) {
        int nEvent = poll(fds, sizeof(fds) / sizeof(pollfd), interval);
        if (nEvent == -1) {
            perror("Poll error: ");
            break;
        }

        if (nEvent == 0) {
            timeOld += interval;
            ikcp_update(kcpHandle, timeOld);
            continue;
        }

        // 计算下次等待时间
        timeNow = GetCurrentTimeMS();
        if ((timeNow - timeOld) > interval) {
            uint64_t timeFuture = ikcp_check(kcpHandle, timeNow);
            interval = timeFuture - timeNow;
        }

        // 如果<=0则直接调用, 一般不会小于0
        if (interval <= 0)
        {
            ikcp_update(kcpHandle, timeOld);
            interval = INTERVAL;
            timeOld = timeNow;
        }

        if (fds[0].revents & POLLIN) {
            // 接收数据
            static char buffer[BUFF_LEN];
            struct sockaddr_in clientAddr;
            socklen_t clientAddrLen = sizeof(clientAddr);
            do
            {
                ssize_t bytesRead = recvfrom(sockFd, buffer, BUFF_LEN, 0, (sockaddr *)(&clientAddr), &clientAddrLen);
                if (bytesRead == -1) {
                    if (errno != EAGAIN) {
                        LOGE("Failed to receive data: %d:%s", errno, strerror(errno));
                    }
                    break;
                } else {
                    LOGD("receive from [%s:%d] %zd\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), bytesRead);
                    pKcpUserParam->sock_addr = clientAddr;
                    int32_t status = ikcp_input(kcpHandle, buffer, bytesRead);
                    if (status < 0) {
                        LOGE("ikcp_input error. %d\n", status);
                    }   
                }
            } while (true);

            int32_t canReadSize = ikcp_peeksize(kcpHandle);
            if (canReadSize > 0) {
                char *buffer = (char *)malloc(canReadSize);
                int32_t nRecv = ikcp_recv(kcpHandle, buffer, canReadSize);
                LOGD("ikcp_recv size %d", canReadSize);
                g_recvSize += nRecv;
                free(buffer);
            }
        }
    }
}

void CatchSignal(int32_t sig)
{
    eular::CallStack stack;
    stack.update();
    stack.log("Kcp-Server", eular::LogLevel::LEVEL_FATAL);
}

int main(int argc, char **argv)
{
    signal(SIGSEGV, &CatchSignal);
    signal(SIGABRT, &CatchSignal);

    eular::log::InitLog(eular::LogLevel::LEVEL_INFO);

    ThreadEntry();
    return 0;
}
