/*************************************************************************
    > File Name: udp_server.cc
    > Author: hsz
    > Brief:
    > Created Time: Mon 30 May 2022 03:26:42 PM CST
 ************************************************************************/


#include <stdio.h>
#include <assert.h>

#include <string>
#include <map>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFF_LEN        1400
#define SERVER_PORT     9000

std::map<std::string, int32_t> g_clientHostMap;

int32_t CreateSocket(const std::string &local_host, std::string &bind_host)
{
    int32_t ufd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    assert(ufd > 0);

    struct sockaddr_in addr;
    socklen_t len = sizeof(sockaddr_in);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(local_host.c_str());
    addr.sin_port = htons(32000);

    int32_t reuse = 1;
    setsockopt(ufd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    setsockopt(ufd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));

    int32_t status = ::bind(ufd, (sockaddr *)&addr, len);
    if (status < 0) {
        perror("bind error");
    }
    assert(status == 0);
    status = getsockname(ufd, (sockaddr *)&addr, &len);
    if (0 == status) {
        char ipv4Host[INET_ADDRSTRLEN] = {0};
        inet_ntop(AF_INET, &addr.sin_addr, ipv4Host, INET_ADDRSTRLEN);
        bind_host = std::string(ipv4Host) + ":" + std::to_string(ntohs(addr.sin_port));
    }

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100 * 1000;
    setsockopt(ufd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    return ufd;
}

void sig_catch(int32_t sig)
{
    if (sig == SIGINT) {
        system("clear");
        printf("--------------------------------------------------\n");
        for (auto it = g_clientHostMap.begin(); it != g_clientHostMap.end(); ++it) {
            printf("[%s:%d]\n", it->first.c_str(), it->second);
        }

        printf("--------------------------------------------------\n");
        printf("size: %zu\n", g_clientHostMap.size());
    }

    exit(0);
}

void handle_udp_msg(int fd)
{
    char buf[BUFF_LEN];
    socklen_t len;
    int count;
    struct sockaddr_in client_addr;
    while(1) {
        memset(buf, 0, BUFF_LEN);
        len = sizeof(client_addr);
        count = recvfrom(fd, buf, BUFF_LEN, 0, (struct sockaddr*)&client_addr, &len);
        if (count == -1) {
            perror("recvfrom error");
            break;
        }

        std::string host = inet_ntoa(client_addr.sin_addr);
        uint16_t port = ntohs(client_addr.sin_port);
        printf("[%s:%d] ==> %s\n", host.c_str(), port, buf);

        g_clientHostMap[host] = port;

        if (count > 0) {
            int n = sprintf(buf, "%s:%d\n", host.c_str(), port);
            sendto(fd, buf, n, 0, (struct sockaddr*)&client_addr, len);
        }
    }
}

int main(int argc, char* argv[])
{
    signal(SIGINT, sig_catch);

    int server_fd, ret;
    struct sockaddr_in addr;
    socklen_t len = sizeof(sockaddr_in);

    std::string bind_host;
    for (int32_t i = 0; i < 3; ++i) {
        CreateSocket("0.0.0.0", bind_host);
    }

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

    getsockname(server_fd, (sockaddr *)&addr, &len);
    printf("waiting for client... [%s:%d]\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
    handle_udp_msg(server_fd);   // 处理接收到的数据

    close(server_fd);
    return 0;
}
