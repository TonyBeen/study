// Description: UDP echo server.
// g++ udp_echo_server.cc -o udp_echo_server
#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 12345
#define BUFFER_SIZE 4096

int main() {
    int sockfd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len;

    // 创建UDP套接字
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return 1;
    }

    // 设置不分片选项
    int dontfrag = IP_PMTUDISC_DO;
    if (setsockopt(sockfd, IPPROTO_IP, IP_MTU_DISCOVER, &dontfrag, sizeof(dontfrag)) < 0) {
        std::cerr << "Failed to set IP_MTU_DISCOVER option" << std::endl;
        close(sockfd);
        return 1;
    }

    // 清空地址结构
    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // 填充服务器信息
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    // 绑定套接字到指定端口
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        close(sockfd);
        return 1;
    }

    std::cout << "UDP echo server started, listening on port " << PORT << "..." << std::endl;

    while (true) {
        len = sizeof(cliaddr);
        // 接收数据
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&cliaddr, &len);
        if (n > 0) {
            buffer[n] = '\0';
            char host_buffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &cliaddr.sin_addr, host_buffer, INET_ADDRSTRLEN);
            printf("RX from [%s:%u] -> %d B\n", host_buffer, ntohs(cliaddr.sin_port), n);

            // 发送回显数据
            sendto(sockfd, buffer, n, 0, (const struct sockaddr *)&cliaddr, len);
        } else {
            perror("recvfrom");
        }
    }

    close(sockfd);
    return 0;
}