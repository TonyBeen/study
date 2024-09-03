/*************************************************************************
    > File Name: udp_server.cc
    > Author: hsz
    > Brief:
    > Created Time: Mon 30 May 2022 03:26:42 PM CST
 ************************************************************************/


#include <stdio.h>

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

void sig_catch(int32_t sig)
{
    if (sig == SIGINT) {
        system("clear");
        for (auto it = g_clientHostMap.begin(); it != g_clientHostMap.end(); ++it) {
            printf("[%s:%d]", it->first.c_str(), it->second);
        }
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
            int n = sprintf(buf, "I have recieved %d bytes data!\n", count);
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
