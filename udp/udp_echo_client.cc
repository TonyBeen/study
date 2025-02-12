// Description: UDP echo client with path MTU discovery.
// g++ udp_echo_client.cc -o udp_echo_client
#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#define SERVER_PORT 12345
#define BUFFER_SIZE 4096

int discover_path_mtu(const struct sockaddr_in &servaddr) {
    int sockfd;
    char probe_packet[BUFFER_SIZE];
    int mtu = 1500;

    // 创建UDP套接字
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return -1;
    }

    // 设置不分片选项
    int dontfrag = IP_PMTUDISC_DO;
    if (setsockopt(sockfd, IPPROTO_IP, IP_MTU_DISCOVER, &dontfrag, sizeof(dontfrag)) < 0) {
        std::cerr << "Failed to set IP_MTU_DISCOVER option" << std::endl;
        close(sockfd);
        return -1;
    }

    while (mtu > 0) {
        memset(probe_packet, 'a', mtu - 28); // 减去IPv4头（20字节）和UDP头（8字节）
        probe_packet[mtu - 28] = '\0';

        // 发送探测数据包
        int sent_bytes = sendto(sockfd, probe_packet, mtu - 28, 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
        if (sent_bytes < 0) {
            std::cerr << "MTU " << mtu << " Byte size too large, continue to detect smaller MTU" << std::endl;
            mtu -= 10;
        } else {
            struct timeval tv;
            tv.tv_sec = 1;
            tv.tv_usec = 0;
            setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

            char buffer[BUFFER_SIZE];
            socklen_t len = sizeof(servaddr);
            int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&servaddr, &len);
            if (n > 0 && strncmp(probe_packet, buffer, n) == 0) {
                std::cout << "The MTU of the path is: " << mtu << " B" << std::endl;
                close(sockfd);
                return mtu;
            } else {
                std::cerr << "MTU " << mtu << " No response, continue to detect smaller MTU" << std::endl;
                mtu -= 10;
            }
        }
    }

    close(sockfd);
    return -1;
}

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <server address>" << std::endl;
        return 1;
    }

    const char* server_address = argv[1];
    // 解析服务器地址
    struct hostent* host = gethostbyname(server_address);
    if (host == nullptr) {
        std::cerr << "Failed to resolve hostname: " << server_address << std::endl;
        return 1;
    }

    int sockfd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in servaddr;

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
        return -1;
    }

    // 清空服务器地址结构
    memset(&servaddr, 0, sizeof(servaddr));

    // 填充服务器信息
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVER_PORT);
    memcpy(&servaddr.sin_addr, host->h_addr, host->h_length);

    // 探测路径MTU
    int mtu = discover_path_mtu(servaddr);
    if (mtu <= 0) {
        std::cerr << "Path MTU detection failed" << std::endl;
        close(sockfd);
        return 1;
    }

    while (true) {
        std::cout << "Please enter the data to be sent:";
        std::string message;
        std::getline(std::cin, message);
        if (message.empty()) {
            break;
        }

        // 发送数据
        sendto(sockfd, message.c_str(), message.size(), 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));

        // 接收回显数据
        socklen_t len = sizeof(servaddr);
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&servaddr, &len);
        buffer[n] = '\0';
        std::cout << "RX:" << buffer << std::endl;
    }

    close(sockfd);
    return 0;
}