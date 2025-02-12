#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_PORT 8888
#define BUFFER_SIZE 65536

int main(int argc, char **argv) {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    // 创建UDP套接字
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // 绑定地址
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    printf("listen port %d ...\n", SERVER_PORT);

    // 接收数据
    while (1) {
        int recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &addr_len);
        if (recv_len < 0) {
            perror("recvfrom");
            continue;
        }
        buffer[recv_len] = '\0';
        printf("RX: [%s:%d] ===> %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), recv_len);

        // 发送响应
        if (sendto(sockfd, buffer, recv_len, 0, (struct sockaddr *)&client_addr, addr_len) < 0) {
            perror("sendto");
        }
    }

    close(sockfd);
    return 0;
}