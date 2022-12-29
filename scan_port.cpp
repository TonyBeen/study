#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <getopt.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <unistd.h>
#include <strings.h>
#include <errno.h>
#include <thread>
#include <functional>
#include <vector>
#include <log/log.h>

#define LOG_TAG "SCAN"

#define MAXPACKET 4096
#define DEFAULT_RESEND 6
#define SERVER_IP   "10.0.24.17"
#define SERVER_PORT 10000

bool needRetry = false;
volatile bool recvFlag = false;
uint16_t peerPort = 0;

std::vector<uint32_t> timeoutPortVec;
std::vector<uint32_t> openedPortVec;

int create_socket(uint16_t port)
{
    int udpsock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udpsock < 0) {
        perror("socket error");
        exit(0);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    addr.sin_port = htons(port);

    int ret = bind(udpsock, (sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        perror("bind error");
        exit(0);
    }

    int reuse = 1;
    setsockopt(udpsock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    return udpsock;
}

void start_scanning(int udpsock, int rawsock, const char *ipaddress, unsigned short port)
{
    unsigned int maxretry = 1;
    struct sockaddr_in myudp;
    socklen_t len = sizeof(sockaddr_in);
    int retry, retval, iplen;
    fd_set fdset;
    struct timeval mytimeout;
    struct icmp *packet;
    struct ip *iphdr;
    struct servent *service;
    unsigned char recvbuff[MAXPACKET];

    mytimeout.tv_sec = 0;
    mytimeout.tv_usec = 50 * 1000;
    retry = 0;

    myudp.sin_addr.s_addr = inet_addr(ipaddress);
    myudp.sin_family = AF_INET;
    myudp.sin_port = htons(port);

    uint32_t addr = myudp.sin_addr.s_addr;

    FD_ZERO(&fdset);
    FD_SET(rawsock, &fdset);
    if ((sendto(udpsock, "HELLO", 5, 0, (struct sockaddr *)&myudp, sizeof(myudp))) < 0) {
        perror("sendto");
        exit(-1);
    }

    retval = select((rawsock + 1), &fdset, NULL, NULL, &mytimeout);
    if (retval > 0) {
        if ((recvfrom(rawsock, recvbuff, sizeof(recvbuff), 0, (sockaddr *)&myudp, &len)) < 0) {
            perror("recvfrom error");
            exit(-1);
        }
        iphdr = (struct ip *)recvbuff;
        iplen = iphdr->ip_hl * 4;
        packet = (struct icmp *)(recvbuff + iplen);
        printf("scan port %d recv from [%s:%d][%d] a ICMP. type = %d, code = %d\n", port, 
            inet_ntoa(myudp.sin_addr), ntohs(myudp.sin_port), myudp.sin_addr.s_addr,
            packet->icmp_type, packet->icmp_code);
        // 端口不可达
        if ((packet->icmp_type == ICMP_UNREACH) && (packet->icmp_code == ICMP_UNREACH_PORT)) {
            return;
        }

        if ((service = getservbyport(htons(port), "udp")) != NULL) {
            printf("UDP service %s open.\n", service->s_name);
        }

        if (myudp.sin_addr.s_addr == addr) {
            openedPortVec.push_back(port);
        }
    } else if (retval == 0) {
        printf("port [%d] time out! the port may be availed!\n", port);
        timeoutPortVec.push_back(port);
        return;
    } else {
        perror("select error.");
        exit(0);
    }
}

void send_data(int udpsock, const char *host, uint16_t port)
{
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(host);
    addr.sin_port = htons(port);

    int code = sendto(udpsock, "HELLO", 5, 0, (sockaddr *)&addr, sizeof(addr));
    if (code <= 0) {
        perror("sendto error");
    }
}

void threadloop(int udp)
{
    int efd = epoll_create(8);
    epoll_event evens[8];
    epoll_event ee;
    ee.data.fd = udp;
    ee.events = EPOLLIN;

    epoll_ctl(efd, EPOLL_CTL_ADD, udp, &ee);

    char buf[1024] = {0};
    while (1) {
        int nev = epoll_wait(efd, evens, 8, 100);
        if (nev < 0) {
            perror("epoll_wait error");
            return;
        }
        if (nev == 0) {
            continue;
        }
        
        epoll_event &ev = evens[0];
        sockaddr_in addr;
        socklen_t len = sizeof(addr);

        memset(buf, 0, sizeof(buf));
        if (recvfrom(ev.data.fd, buf, sizeof(buf), 0, (sockaddr *)&addr, &len) <= 0) {
            perror("recvfrom error");
            continue;
        }

        if (strcasecmp(buf, "HELLO") == 0) {
            recvFlag = true;
            peerPort = ntohs(addr.sin_port);
        }
        printf("[%s:%d]: %s\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), buf);
    }
}

/**
 * usage: 
 * program -h xxx.xxx.xxx.xxx -b 1000 -e 1200
 * 
 */

int main(int argc, char **argv)
{
    const char *host = nullptr;
    uint16_t begin = 3000;
    uint16_t end = 3100;
    int ch = 0;
    while ((ch = getopt(argc, argv, "b:e:h:")) > 0)
    {
        switch (ch)
        {
        case 'h':
            host = optarg;
            break;
        case 'b':
            begin = (uint16_t)atoi(optarg);
            break;
        case 'e':
            end = (uint16_t)atoi(optarg);
            break;
        default:
            break;
        }
    }

    assert(host != nullptr);
    assert(begin < end);
    timeoutPortVec.clear();
    openedPortVec.clear();
    printf("scanning %s[%d] %d - %d\n", host, inet_addr(host), begin, end);

    int udp = create_socket(SERVER_PORT);
    // int rawsock = 0;
    // if ((rawsock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {
    //     perror("socket()");
    //     exit(-1);
    // }

    std::thread th(std::bind(&threadloop, udp));
    th.detach();

#if 0
    while (begin <= end) {
        start_scanning(udp, rawsock, host, begin++);
    }
    printf("maybe opened port:\n");
    for (auto it = timeoutPortVec.begin(); it != timeoutPortVec.end(); ++it) {
        printf("%d\t", *it);
    }
    printf("\n");

    printf("opened port:\n");
    for (auto it = openedPortVec.begin(); it != openedPortVec.end(); ++it) {
        printf("%d\t", *it);
    }
    printf("\n");
#else
    LOGI("begin scan...");

    while (begin <= end) {
        if (recvFlag) {
            break;
        }
        send_data(udp, host, begin++);
        printf("\t%05d", begin);
        if (begin == 0) {
            break;
        }
    }
    
    LOGI("end scan... begin = %u", begin);
#endif

    while (1);
    return 0;
}