/*************************************************************************
    > File Name: test_getifaddrs.cc
    > Author: hsz
    > Brief:
    > Created Time: 2024年10月25日 星期五 14时40分06秒
 ************************************************************************/

#include <iostream>
#include <string>
#include <vector>
#include <map>

#include <sys/types.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <ifaddrs.h>

inline uint32_t NET_MASK(uint32_t ip, uint32_t mask)
{
    const uint32_t block_bits = 32 - mask;
    return ip >> block_bits << block_bits;
}

bool IsPrivateIPv6(const std::string& ip)
{
    struct in6_addr in6_addr {};
    int result = ::inet_pton(AF_INET6, ip.c_str(), &in6_addr);
    if (result != 1) {
        return false;
    }

    // Convert network byteorder to host byteorder.
    uint32_t iph0 = ntohl(((uint32_t*)&in6_addr)[0]);

    // https://en.wikipedia.org/wiki/Private_network

    // fc00::/7
    return (NET_MASK(iph0, 7) == 0xfc000000);
}

bool IsReservedIPv6(const std::string& ip)
{
    struct in6_addr in6_addr {};
    int result = ::inet_pton(AF_INET6, ip.c_str(), &in6_addr);
    if (result != 1) {
        return false;
    }

    // Convert network byteorder to host byteorder.
    uint32_t iph0 = ntohl(((uint32_t*)&in6_addr)[0]);
    uint32_t iph1 = ntohl(((uint32_t*)&in6_addr)[1]);
    uint32_t iph2 = ntohl(((uint32_t*)&in6_addr)[2]);
    uint32_t iph3 = ntohl(((uint32_t*)&in6_addr)[3]);

    // https://en.wikipedia.org/wiki/Reserved_IP_addresses

    return (iph0 == 0 && iph1 == 0 && iph2 == 0 && iph3 == 0)             // ::/128
           || (iph0 == 0 && iph1 == 0 && iph2 == 0 && iph3 == 0x00000001) // ::1/128
           || (iph0 == 0 && iph1 == 0 && iph2 == 0x0000ffff)              // ::ffff:0:0/96
           || (iph0 == 0 && iph1 == 0 && iph2 == 0xffff0000)              // ::ffff:0:0:0/96
           || (iph0 == 0x0064ff9b && iph1 == 0 && iph2 == 0)              // 64:ff9b::/96
           || (iph0 == 0x01000000 && iph1 == 0)                           // 100::/64
           || (iph0 == 0x20010000)                                        // 2001::/32
           || (NET_MASK(iph0, 28) == 0x20010020)                          // 2001:20::/28
           || (iph0 == 0x20010db8)                                        // 2001:db8::/32
           || (NET_MASK(iph0, 16) == 0x20020000)                          // 2002::/16
           || (NET_MASK(iph0, 7) == 0xfc000000)                           // fc00::/7
           || (NET_MASK(iph0, 10) == 0xfe800000)                          // fe80::/10
           || (NET_MASK(iph0, 8) == 0xff000000);                          // ff00::/8
}

void ListNetworkInterfacesFromPOSIX(std::vector<std::string> &interface_v4_address,
                                    std::vector<std::string> &interface_v6_address,
                                    std::vector<std::string> &interface_name)
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
            printf("%s -----> %s, %u, %u\n", ifa->ifa_name, address, IsPrivateIPv6(address), IsReservedIPv6(address));
            break;
        }

        case AF_INET6: {
            char address[INET6_ADDRSTRLEN] = {0};
            struct sockaddr_in6 *ipv6 = reinterpret_cast<struct sockaddr_in6 *>(ifa->ifa_addr);
            inet_ntop(AF_INET6, &(ipv6->sin6_addr), address, sizeof(address));
            it->second.v6 = address;
            printf("%s -----> %s, %u, %u\n", ifa->ifa_name, address, IsPrivateIPv6(address), IsReservedIPv6(address));
            break;
        }
        default:
            break;
        }
    }

    freeifaddrs(interfaces);

    for (auto &it : hostConfigMap) {
        interface_name.push_back(it.first);

        std::string &v4 = it.second.v4;
        if (v4.empty()) {
            v4 = "127.0.0.1";
        }
        interface_v4_address.push_back(v4);


        std::string &v6 = it.second.v6;
        if (v6.empty()) {
            v6 = "::1";
        }
        interface_v6_address.push_back(v6);

        printf("if: %s IPv4: %s, IPv6: %s\n", it.first.c_str(), v4.c_str(), v6.c_str());
    }
}

int main(int argc, char **argv)
{
    std::vector<std::string> interface_v4_address;
    std::vector<std::string> interface_v6_address;
    std::vector<std::string> interface_name;
    ListNetworkInterfacesFromPOSIX(interface_v4_address, interface_v6_address, interface_name);

    return 0;
}
