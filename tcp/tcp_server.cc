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

#include <getopt.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080

void handle_client(int client_sock) {
    char buffer[1024] = {0};
    int valread = read(client_sock, buffer, 1024);
    std::cout << "Received: " << buffer << std::endl;
    
    // Send response back to client
    const char *response = "Hello from server";
    send(client_sock, response, strlen(response), 0);
    std::cout << "Response sent\n";
    
    // Close the connection
    close(client_sock);
}

void start_server(int server_sock) {
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    
    std::string peer_host;
    uint16_t peer_port = 0;
    while (true) {
        // Accept incoming connection
        int client_sock = accept(server_sock, (struct sockaddr*)&address, &addrlen);
        if (client_sock < 0) {
            std::cerr << "Failed to accept connection\n";
            continue;
        }

        peer_host = inet_ntoa(address.sin_addr);
        peer_port = ntohs(address.sin_port);
        std::cout << "New connection from " << peer_host << ":" << peer_port << std::endl;
        break;
    }

    struct sockaddr_in bind_address;
    // 尝试连接
    int client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock < 0) {
        printf("socket error, errno: %d, errstr: %s\n", errno, strerror(errno));
        return;
    }

    int32_t opt = 1;
    if (setsockopt(client_sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) != 0) {
        perror("setsockopt(SO_REUSEPORT) error");
        close(client_sock);
        return;
    }

    bind_address.sin_family = AF_INET;
    bind_address.sin_addr.s_addr = INADDR_ANY;
    bind_address.sin_port = htons(PORT);
    if (bind(client_sock, (struct sockaddr*)&bind_address, sizeof(bind_address)) < 0) {
        perror("bind error");
        close(client_sock);
        return;
    }

    int ret = connect(client_sock, (struct sockaddr*)&address, sizeof(address));
    if (ret < 0) {
        printf("Failed to connect: [%d:%s]\n", errno, strerror(errno));
        return;
    }

    printf("connect to %s:%u success\n", peer_host.c_str(), peer_port);
}

int32_t Server(bool is_client = false, const char *addr = nullptr)
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

    if (is_client) {
        int client_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (client_sock < 0) {
            printf("socket error, errno: %d, errstr: %s\n", errno, strerror(errno));
            return -1;
        }

        if (setsockopt(client_sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) != 0) {
            perror("setsockopt(SO_REUSEPORT) error");
            close(client_sock);
            return -1;
        }

        if (bind(client_sock, (struct sockaddr*)&address, sizeof(address)) < 0) {
            perror("bind error");
            close(client_sock);
            return -1;
        }

        address.sin_addr.s_addr = inet_addr(addr);
        int ret = connect(client_sock, (struct sockaddr*)&address, sizeof(address));
        if (ret < 0) {
            printf("Failed to connect: [%d:%s]\n", errno, strerror(errno));
            return 0;
        }

        printf("connect to %s success\n", addr);
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
    while ((c = ::getopt(argc, argv, "a:c")) > 0) {
        switch (c) {
        case 'a':
            addr = optarg;
            break;
        case 'c':
            is_client = true;
            break;
        default:
            break;
        }
    }

    Server(is_client, addr);
    return 0;
}
