/*************************************************************************
    > File Name: tcp_server.cc
    > Author: hsz
    > Brief:
    > Created Time: Tue 22 Oct 2024 07:14:52 PM CST
 ************************************************************************/

#include <iostream>
#include <string>
#include <cstring>
#include <thread>
#include <map>
#include <vector>

#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

#define PORT 13999

bool TimeoutConnect(int32_t sockfd, sockaddr_in addr, uint32_t timeoutMS)
{
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    // 连接到目标地址
    int ret = connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    if (ret < 0 && errno != EINPROGRESS) { // 连接失败且不是EINPROGRESS
        // EINPROGRESS 表示仍在进行连接
        printf("Failed to connect: [%d:%s]\n", errno, strerror(errno));
        return false;
    }

    // 在非阻塞模式下，需要使用 select() 或 epoll() 等函数来等待连接完成
    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(sockfd, &fdset);
    struct timeval timeout;
    timeout.tv_sec = timeoutMS / 1000;
    timeout.tv_usec = timeoutMS % 1000 * 1000;
    ret = select(sockfd + 1, nullptr, &fdset, nullptr, &timeout);
    if (ret < 0) {
        printf("Failed to select: [%d:%s]\n", errno, strerror(errno));
        return false;
    } else if (ret == 0) { // 超时了
        printf("Connection timed out\n");
        return false;
    } else {  // 连接成功或失败
        int valopt;
        socklen_t optlen = sizeof(valopt);
        getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (void*)&valopt, &optlen);  // 获取连接结果
        if (valopt != 0) {  // 连接失败
            printf("Failed to connect: [%d:%s]\n", valopt, strerror(valopt));
            return false;
        }
    }

    // 恢复套接字为阻塞模式
    fcntl(sockfd, F_SETFL, flags);

    return true;
}

void start_server(int server_sock) {
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    
    std::string peer_host;
    uint16_t peer_port = 0;
    std::vector<std::pair<std::string, uint16_t>> peerVec;
    while (true) {
        int client_sock = accept(server_sock, (struct sockaddr*)&address, &addrlen);
        if (client_sock < 0) {
            std::cerr << "Failed to accept connection\n";
            continue;
        }

        peer_host = inet_ntoa(address.sin_addr);
        peer_port = ntohs(address.sin_port);
        std::cout << "New connection from " << peer_host << ":" << peer_port << std::endl;
    }
}

int32_t Server(const char *addr = nullptr)
{
    int server_sock = 0;
    struct sockaddr_in address;
    // Create socket file descriptor
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        return -1;
    }

    // Enable SO_REUSEPORT
    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) != 0) {
        perror("setsockopt(SO_REUSEPORT) error");
        close(server_sock);
        return -1;
    }

    // Set server address and port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    if (addr != nullptr) {
        address.sin_addr.s_addr = inet_addr(addr);
    }
    address.sin_port = htons(PORT);

    // Bind the socket to the network address and port
    if (bind(server_sock, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind error");
        close(server_sock);
        return -1;
    }

    // Start listening for incoming connections
    if (listen(server_sock, 128) < 0) {
        perror("listen error");
        close(server_sock);
        return -1;
    }

    std::cout << "Server listening on port " << PORT << std::endl;

    // Start handling connections
    start_server(server_sock);

    // Close the server socket
    close(server_sock);
}

int main(int argc, char *argv[])
{
    bool is_client = false;
    const char *addr = nullptr;
    char c = '\0';
    while ((c = ::getopt(argc, argv, "a")) > 0) {
        switch (c) {
        case 'a':
            addr = optarg;
            break;
        default:
            break;
        }
    }

    Server(addr);
    return 0;
}
