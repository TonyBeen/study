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

#include "common.h"

#define PORT 14000

void start_server(int server_sock) {
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    std::string peer_host;
    uint16_t peer_port = 0;
    std::vector<std::pair<std::string, uint16_t>> peerVec;
    std::vector<int32_t> sockVec;
    while (true) {
        int client_sock = accept(server_sock, (struct sockaddr*)&address, &addrlen);
        if (client_sock < 0) {
            std::cerr << "Failed to accept connection\n";
            continue;
        }

        peer_host = inet_ntoa(address.sin_addr);
        peer_port = ntohs(address.sin_port);
        std::cout << "New connection from " << peer_host << ":" << peer_port << std::endl;

        sockVec.push_back(client_sock);
        peerVec.emplace_back(std::move(peer_host), peer_port);
        if (peerVec.size() == 2) {
            // 将B的信息发送给A
            PeerMessage msg;
            msg.is_server = 1;
            strcpy(msg.host, peerVec[1].first.c_str());
            msg.port = peerVec[1].second;
            send(sockVec[0], &msg, sizeof(PeerMessage), 0);

            memset(&msg, 0, sizeof(PeerMessage));
            msg.is_server = 1;
            strcpy(msg.host, peerVec[0].first.c_str());
            msg.port = peerVec[0].second;
            send(sockVec[1], &msg, sizeof(PeerMessage), 0);
            break;
        }
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

    // 绑定的网卡接口名称
    // const char *interface = "eth0";
    // if (setsockopt(server_sock, SOL_SOCKET, SO_BINDTODEVICE, interface, strlen(interface)) < 0) {
    //     perror("SO_BINDTODEVICE failed");
    //     close(server_sock);
    //     return -1;
    // }

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
    while ((c = ::getopt(argc, argv, "a:")) > 0) {
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
