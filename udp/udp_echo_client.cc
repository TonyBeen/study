// Description: UDP echo client with path MTU discovery.
// g++ udp_echo_client.cc -o udp_echo_client
#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#define SERVER_PORT 12345
#define BUFFER_SIZE 65536

#define IPV4_HEADER_SIZE    20
#define UDP_HEADER_SIZE     8

int discover_path_mtu(const struct sockaddr_in &servaddr)
{
    int sockfd;
    char probe_packet[BUFFER_SIZE];

    int32_t mtu_lbound, mtu_current, mtu_ubound, mtu_best;
    mtu_best = 68;
    mtu_lbound = 68;
    mtu_ubound = 65535;

    struct sockaddr_in remote_host;

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

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 500000; // 500ms
    // set timeout to input operations
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) != 0) {
        perror("Error in setsockopt(timeout/udp)");
        return -1;
    }

    while (mtu_lbound <= mtu_ubound) { // binary search
        mtu_current = (mtu_lbound + mtu_ubound) / 2;
        memset(probe_packet, 'a', mtu_current - IPV4_HEADER_SIZE - UDP_HEADER_SIZE); // 减去IPv4头（20字节）和UDP头（8字节）
        probe_packet[mtu_current - IPV4_HEADER_SIZE - UDP_HEADER_SIZE] = '\0';

        // 发送探测数据包
        int bytes = sendto(sockfd, probe_packet, mtu_current - IPV4_HEADER_SIZE - UDP_HEADER_SIZE, 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
        if (bytes < 0) {
            // packet too big for the local interface
            if (errno == EMSGSIZE) {
                printf("packet(%d) too big for local interface\n", mtu_current);
                mtu_ubound = mtu_current - 1; // update range
                continue;
            }

            perror("Error in sendto()");
            return -1;
        }

        char buffer[BUFFER_SIZE];
        socklen_t len = sizeof(remote_host);
        bytes = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&remote_host, &len);
        if (bytes < 0) {
            // timeout
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("timeout\n");
                mtu_ubound = mtu_current - 1; // update range
                continue;
            }

            perror("Error in recvfrom()");
            return -1;
        } else if (bytes > 0) {
            if (strncmp(probe_packet, buffer, bytes) == 0) {
                mtu_lbound = mtu_current + 1; // update range
            } else {
                mtu_ubound = mtu_current - 1; // update range
            }

            if (mtu_current > mtu_best)
				mtu_best = mtu_current;
        } else {
            printf("recvfrom() returned 0\n");
            return -1;
        }
    }

    close(sockfd);
    std::cout << "The MTU of the path is: " << mtu_best << " B" << std::endl;
    return mtu_best;
}

int main(int argc, char* argv[])
{
    int opt;
    const char *server_address = "127.0.0.1";
    int32_t port = SERVER_PORT;
    while ((opt = getopt(argc, argv, "s:p:")) != -1) {
        switch (opt) {
        case 's':
            server_address = optarg;
            break;
        case 'p':
            port = atoi(optarg);
            break;
        default: /* '?' */
            fprintf(stderr, "Usage: %s -s server_address -p port\n", argv[0]);
            return 0;
        }
    }

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
    servaddr.sin_port = htons(port);
    memcpy(&servaddr.sin_addr, host->h_addr, host->h_length);

    // 探测路径MTU
    int mtu = discover_path_mtu(servaddr);
    if (mtu <= 0) {
        std::cerr << "Path MTU detection failed" << std::endl;
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