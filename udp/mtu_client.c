#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/icmp6.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/time.h>

#define SERVER_ADDR     "192.168.3.10"
#define SERVER_PORT     8888
#define BUFFER_SIZE     65536
#define TEST_SIZE       1600  // 超过通常的MTU，触发分片

int sockfd;
char buffer[BUFFER_SIZE];
struct sockaddr_in server_addr;

void handle_icmp(int sig) {
    int error;
    socklen_t optlen = sizeof(error);
    if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &optlen) < 0) {
        perror("getsockopt");
        exit(EXIT_FAILURE);
    }
    if (error == EHOSTUNREACH) {
        printf("ICMP: 路由不可达\n");
    } else if (error == EMSGSIZE) {
        printf("ICMP: 数据包过大，必须分片，但DF位设置\n");
    }

    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    struct sigaction sa;
    struct timeval tv;
    int flags;

    // 创建UDP套接字
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // 设置不分片
    int mtu_disc = IP_PMTUDISC_DO;
    if (setsockopt(sockfd, IPPROTO_IP, IP_MTU_DISCOVER, &mtu_disc, sizeof(mtu_disc)) < 0) {
        perror("setsockopt MTU_DISCOVER");
        exit(EXIT_FAILURE);
    }

    // 设置发送缓冲区
    int snd_buf_size = 2 * TEST_SIZE;
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &snd_buf_size, sizeof(snd_buf_size)) < 0) {
        perror("setsockopt SNDBUF");
        exit(EXIT_FAILURE);
    }

    // 准备服务器地址
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);
    server_addr.sin_port = htons(SERVER_PORT);

    // 设置超时和信号处理
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt SNDTIMEO");
        exit(EXIT_FAILURE);
    }

    sa.sa_flags = SA_RESTART;
    sa.sa_handler = handle_icmp;
    sigaction(SIGALRM, &sa, NULL);

    // 构造数据包
    char data[TEST_SIZE];
    memset(data, 'A', TEST_SIZE);

    // 发送数据
    printf("发送数据到 %s:%d...\n", SERVER_ADDR, SERVER_PORT);
    if (sendto(sockfd, data, TEST_SIZE, 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("sendto");
        exit(EXIT_FAILURE);
    }

    // 等待响应
    alarm(5);  // 设置5秒超时
    int recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, NULL, 0);
    alarm(0);

    if (recv_len < 0) {
        perror("recvfrom");
        exit(EXIT_FAILURE);
    }

    buffer[recv_len] = '\0';
    printf("recv %d\n", recv_len);

    close(sockfd);
    return 0;
}