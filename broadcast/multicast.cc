/*************************************************************************
    > File Name: multicast.cc
    > Author: hsz
    > Brief:
    > Created Time: 2024年04月05日 星期五 15时14分09秒
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include <iostream>
#include <string>
#include <vector>

#if defined(WIN32) || defined(_WIN32) || defined(_WIN64) || defined(__NT__)
#define OS_WINDOWS
#define  _WINSOCK_DEPRECATED_NO_WARNINGS

#include <WinSock2.h>
#include <Ws2tcpip.h> // for struct ip_mreq
#include <iphlpapi.h>

#define socket_t SOCKET

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "IPHLPAPI.lib")

#elif defined(__linux__) || defined(__linux)
#define OS_LINUX
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <fcntl.h>
#include <unistd.h>

#define socket_t int
#define INVALID_SOCKET  -1
#define SOCKET_ERROR    -1
#endif

#define MULTICAST_PORT  13000
#define MULTICAST_GROUP "239.255.255.250"
#define BUFFER_SIZE     1024

void ErrorExit(const char* msg)
{
#ifdef OS_WINDOWS
    std::cout << msg << " " << WSAGetLastError() << std::endl;
#else
    perror(msg);
#endif // OS_WINDOWS

    exit(0);
}

std::vector<std::string> GetLocalAddress()
{
    std::vector<std::string> localHostVec;
#if defined(OS_WINDOWS)
    PIP_ADAPTER_INFO pAdapterInfo;
    DWORD dwRetVal = 0;

    ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
    pAdapterInfo = (IP_ADAPTER_INFO *)malloc(ulOutBufLen);
    if (pAdapterInfo == NULL) {
        printf("Error allocating memory needed to call GetAdaptersinfo\n");
        return localHostVec;
    }

    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
        free(pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);
    }

    if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
        PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
        while (pAdapter) {
            printf("\tComboIndex: \t%d\n", pAdapter->ComboIndex);
            printf("\tAdapter Name: \t%s\n", pAdapter->AdapterName);
            printf("\tAdapter Desc: \t%s\n", pAdapter->Description);
            printf("\tAdapter Addr: \t");
            for (UINT i = 0; i < pAdapter->AddressLength; i++) {
                if (i == (pAdapter->AddressLength - 1))
                    printf("%.2X\n", (int)pAdapter->Address[i]);
                else
                    printf("%.2X-", (int)pAdapter->Address[i]);
}
            printf("\tIndex: \t%d\n", pAdapter->Index);
            printf("\tType: \t");
            switch (pAdapter->Type) {
            case MIB_IF_TYPE_OTHER:
                printf("Other\n");
                break;
            case MIB_IF_TYPE_ETHERNET:
                printf("Ethernet\n");
                break;
            case MIB_IF_TYPE_TOKENRING:
                printf("Token Ring\n");
                break;
            case MIB_IF_TYPE_FDDI:
                printf("FDDI\n");
                break;
            case MIB_IF_TYPE_PPP:
                printf("PPP\n");
                break;
            case MIB_IF_TYPE_LOOPBACK:
                printf("Loopback\n");
                break;
            case MIB_IF_TYPE_SLIP:
                printf("Slip\n");
                break;
            case IF_TYPE_IEEE80211:
                printf("IEEE 802.11\n");
                break;
            default:
                printf("Unknown type %ld\n", pAdapter->Type);
                break;
            }

            if (MIB_IF_TYPE_ETHERNET != pAdapter->Type && IF_TYPE_IEEE80211 != pAdapter->Type)
            {
                pAdapter = pAdapter->Next;
                continue;
            }

            // 过滤掉无效IP
            if (std::string(pAdapter->IpAddressList.IpAddress.String) == "0.0.0.0")
            {
                pAdapter = pAdapter->Next;
                continue;
            }

            localHostVec.push_back(pAdapter->IpAddressList.IpAddress.String);
            printf("\tIP Address: \t%s\n", pAdapter->IpAddressList.IpAddress.String);
            printf("\tIP Mask: \t%s\n", pAdapter->IpAddressList.IpMask.String);
            printf("\tGateway: \t%s\n", pAdapter->GatewayList.IpAddress.String);

            pAdapter = pAdapter->Next;
            printf("\n");
        }
    }
    else {
        printf("GetAdaptersInfo failed with error: %d\n", dwRetVal);
    }

    if (pAdapterInfo)
    {
        free(pAdapterInfo);
    }
#else
    struct ifaddrs* addrs;
    getifaddrs(&addrs);
    struct ifaddrs* tmp = addrs;
    while (tmp)
    {
        if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET)
        {
            struct sockaddr_in* pAddr = (struct sockaddr_in*)tmp->ifa_addr;
            localHostVec.push_back(inet_ntoa(pAddr->sin_addr));
        }
        tmp = tmp->ifa_next;
    }
    freeifaddrs(addrs);
#endif

    return localHostVec;
}

int main(int argc, char** argv)
{
#ifdef OS_WINDOWS
    WSADATA WSAData;
    WORD sockVersion = MAKEWORD(2, 2);
    assert(WSAStartup(sockVersion, &WSAData) == 0);
#endif

    socket_t groupSock = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (INVALID_SOCKET == groupSock)
    {
        ErrorExit("socket error!");
    }

#if defined(OS_LINUX)
    fcntl(groupSock, F_SETFL, O_NONBLOCK);
#else
    u_long iMode = 1;
    ioctlsocket(groupSock, FIONBIO, &iMode);
#endif

    struct sockaddr_in localAddr;
    localAddr.sin_family = AF_INET;
    localAddr.sin_port = htons(MULTICAST_PORT);
    localAddr.sin_addr.s_addr = INADDR_ANY;

    if (SOCKET_ERROR == bind(groupSock, (sockaddr*)&localAddr, sizeof(localAddr)))
    {
        ErrorExit("bind error");
    }

    // 加入组播
    auto localHostVec = GetLocalAddress();
    for (const auto& host : localHostVec)
    {
        struct ip_mreq mreq;
        mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_GROUP);
        printf("local host: %s\n", host.c_str());
        mreq.imr_interface.s_addr = inet_addr(host.c_str());
        if (SOCKET_ERROR == setsockopt(groupSock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq)))
        {
            ErrorExit("setsockopt(IP_ADD_MEMBERSHIP) error");
        }
    }

    const uint32_t waitMS = 500;

    struct sockaddr_in remoteHost;
    socklen_t c = sizeof(remoteHost);
    fd_set rfds;

    struct sockaddr_in multicastGroupAddr;
    memset(&multicastGroupAddr, 0, sizeof(multicastGroupAddr));
    multicastGroupAddr.sin_family = AF_INET;
    multicastGroupAddr.sin_addr.s_addr = inet_addr(MULTICAST_GROUP);
    multicastGroupAddr.sin_port = htons(MULTICAST_PORT);

    while (true)
    {
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = waitMS * 1000;

        FD_ZERO(&rfds);
        FD_SET(groupSock, &rfds);
        int32_t ret = select(groupSock + 1, &rfds, nullptr, nullptr, &tv);
        if (ret < 0)
        {
            ErrorExit("select error");
        }
        else if (ret == 0)
        {
            static const char* message = "Hello, World!";
            int nbytes = sendto(groupSock, message, strlen(message), 0, (struct sockaddr*)&multicastGroupAddr,
                sizeof(multicastGroupAddr));
            if (nbytes < 0)
            {
                ErrorExit("sendto error");
            }
            printf("send message success\n");
        }
        else
        {
            char buffer[BUFFER_SIZE] = { 0 };
            int32_t n = recvfrom(groupSock, buffer, BUFFER_SIZE, 0, (sockaddr*)&remoteHost, &c);
            if (n < 0)
            {
                ErrorExit("recvfrom error");
            }
            else
            {
                bool isLocal = false;
                std::string remoteIp = inet_ntoa(remoteHost.sin_addr);
                for (const auto& ip : localHostVec)
                {
                    if (remoteIp == ip)
                    {
                        isLocal = true;
                        break;
                    }
                }

                if (!isLocal)
                {
                    printf("recvfrom(%s:%u): %s\n", remoteIp.c_str(), ntohs(remoteHost.sin_port), buffer);
                }
            }
        }
    }

    return 0;
}
