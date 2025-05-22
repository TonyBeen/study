/*************************************************************************
    > File Name: getsockname.cc
    > Author: hsz
    > Brief:
    > Created Time: 2025年05月22日 星期四 17时09分31秒
 ************************************************************************/

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <unistd.h>

#include <map>
#include <string>

void print_iface_by_ip(const char* ip)
{
    struct ifaddrs *interfaces = nullptr;
    struct ifaddrs *ifa = nullptr;

    if (getifaddrs(&interfaces) == -1) {
        perror("getifaddrs error");
        return;
    }

    struct InternalHostConfig {
        std::string v4;
        std::string v6;
    };

    std::map<std::string, InternalHostConfig> hostConfigMap;
    for (ifa = interfaces; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) {
            continue;
        }

        if (!(ifa->ifa_flags & IFF_UP) || !(ifa->ifa_flags & IFF_RUNNING)) {
            continue;
        }

        // Skip local loopback address
        if (ifa->ifa_flags & IFF_LOOPBACK) {
            continue;
        }

        auto it = hostConfigMap.find(ifa->ifa_name);
        if (it == hostConfigMap.end()) {
            it = hostConfigMap.insert(std::make_pair(ifa->ifa_name, InternalHostConfig())).first;
        }

        switch (ifa->ifa_addr->sa_family) {
        case AF_INET: {
            char address[INET_ADDRSTRLEN] = {0};
            struct sockaddr_in *ipv4 = reinterpret_cast<struct sockaddr_in *>(ifa->ifa_addr);
            inet_ntop(AF_INET, &(ipv4->sin_addr), address, sizeof(address));
            it->second.v4 = address;
            break;
        }

        case AF_INET6: {
            char address[INET6_ADDRSTRLEN] = {0};
            struct sockaddr_in6 *ipv6 = reinterpret_cast<struct sockaddr_in6 *>(ifa->ifa_addr);
            inet_ntop(AF_INET6, &(ipv6->sin6_addr), address, sizeof(address));
            it->second.v6 = address;
            break;
        }
        default:
            break;
        }
    }

    freeifaddrs(interfaces);

    for (auto &it : hostConfigMap) {
        std::string &v4 = it.second.v4;
        if (v4.empty()) {
            v4 = "127.0.0.1";
        }
        printf("Interface: %s, IPv4: %s\n", it.first.c_str(), v4.c_str());
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <nic>\n", argv[0]);
        return 1;
    }
    const char *nic = argv[1];

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(0); // Bind to any available port
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
        perror("bind");
        close(sockfd);
        return 1;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, nic, strlen(nic)) < 0) {
        perror("setsockopt");
        close(sockfd);
        return 1;
    }

    sockaddr_in remote_addr;
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(54321); // Example port
    remote_addr.sin_addr.s_addr = inet_addr("114.114.114.114"); // Example IP

    const char *message = "Hello, World!";
    sendto(sockfd, message, strlen(message), 0, (struct sockaddr*)&remote_addr, sizeof(remote_addr));
    printf("Sent message to %s:%d\n", inet_ntoa(remote_addr.sin_addr), ntohs(remote_addr.sin_port));

    memset(&local_addr, 0, sizeof(local_addr));
    socklen_t addr_len = sizeof(local_addr);
    getsockname(sockfd, (struct sockaddr*)&local_addr, &addr_len);
    char* src_ip = inet_ntoa(local_addr.sin_addr);
    printf("Source IP: %s\n", src_ip);
    print_iface_by_ip(src_ip);
    close(sockfd);
    return 0;
}