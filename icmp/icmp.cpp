/*************************************************************************
    > File Name: icmp.cpp
    > Author: hsz
    > Brief:
    > Created Time: Mon 15 Nov 2021 06:19:56 PM CST
 ************************************************************************/

#include <stdio.h>
#include <errno.h>  
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>  
#include <arpa/inet.h>
#include <cstdlib>
//协议相关结构体
#include <netinet/if_ether.h>
#include <string.h>
#include <sys/ioctl.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <net/if.h>
#include <netdb.h>

#include <log/log.h>
#include <utils/string8.h>
#include <utils/errors.h>

#define LOG_TAG "icmp"

int main()
{
    int port = 6000;
    struct protoent *protocol = nullptr;
    protocol = getprotobyname("tcp");
    if (protocol == nullptr) {
        LOGE("getprotobyname error code = %d, error message: %s", errno, strerror(errno));
        return 0;
    }
    int sock = ::socket(PF_PACKET, SOCK_RAW, htonl(ETH_P_ALL)); // protocol->p_proto = IPPROTO_TCP
    if (sock < 0) {
        LOGE("socket error. error code = %d, error message: %s", errno, strerror(errno));
        return UNKNOWN_ERROR;
    }
    sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = PF_PACKET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr("10.0.24.17"); // htonl(INADDR_ANY);

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    // setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &opt, sizeof(opt));
    // struct linger lin;
    // lin.l_onoff = 1;
    // lin.l_linger = 0;
    // setsockopt(sock, SOL_SOCKET, SO_LINGER, &lin, sizeof(linger));  // 这样调用close会发送rst来直接关闭套接字，跳过了timed_wait状态

    // if (::bind(sock, (sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    //     LOGE("bind error. error code = %d, error message: %s", errno, strerror(errno));
    //     return 0;
    // }
    // if (::listen(sock, 128) < 0) { // Operation not supported
    //     LOGE("listen error. error code = %d, error message: %s", errno, strerror(errno));
    //     return 0;
    // }

    // sockaddr_in client_addr;
    // bzero(&client_addr, sizeof(client_addr));
    // socklen_t addrLen = 0;
    // int client = ::accept(sock, (struct sockaddr *)&client_addr, &addrLen); // Operation not supported
    // if (client < 0) {
    //     LOGE("accept error. error code = %d, error message: %s", errno, strerror(errno));
    //     return 0;
    // }

    struct timeval tv;
    tv.tv_usec = 0;  //设置select函数的超时时间为3s
    tv.tv_sec = 3;

    fd_set readfd;
    FD_ZERO(&readfd);
    FD_SET(sock, &readfd);

    int selectRet = 0;
    uint8_t buf[1024] = {0};
    while (1) {
        bzero(buf, sizeof(buf));
        FD_ZERO(&readfd);
        FD_SET(sock, &readfd);
        sockaddr_in cliAddr;
        socklen_t len = 0;
        int recvSize = ::recvfrom(sock, buf, sizeof(buf), 0, (sockaddr *)&cliAddr, &len);
        if (recvSize > 0) {
            eular::String8 str;
            for (int i = 0; i < recvSize; ++i) {
                str.appendFormat("%02x ", buf[i]);
                if (i != 0 && i % 10 == 0) {
                    str.appendFormat("\n");
                }
            }
            LOGI("\n%s", str.c_str());
        }
        selectRet = ::select(sock + 1, &readfd, nullptr, nullptr, &tv);
        if (selectRet < 0) {
            LOGE("select error. error code = %d, error message: %s", errno, strerror(errno));
            return 0;
        }
        if (selectRet == 0) {
            LOGI("select return 0");
        }
        if (selectRet > 0) {
            int size = recv(sock, buf, sizeof(buf), 0);
            LOGI("recv size = %d", size);
            eular::String8 str;
            for (int i = 0; i < size; ++i) {
                str.appendFormat("%02x ", buf[i]);
                if (i != 0 && i % 10 == 0) {
                    str.appendFormat("\n");
                }
            }
            LOGI("\n%s", str.c_str());

            struct ip *ipHeader = (struct ip *)buf;
            int ipHeaderLen = ipHeader->ip_hl * 4;
            LOGI("IP Header: length: %d;\n"
                "version: %d;\n"
                "total len: %d\n"
                "dst ip: %s\n"
                "src ip: %s\n", ipHeaderLen, ipHeader->ip_v, ipHeader->ip_len,
                inet_ntoa(ipHeader->ip_dst), inet_ntoa(ipHeader->ip_src));

            struct tcphdr *tcpHeader = (struct tcphdr *)(buf + ipHeaderLen);
            LOGI("Tcp Header: src port: %d, dst port: %d\n", tcpHeader->th_sport, tcpHeader->th_dport);
            LOGI("Tcp Header: src port: %d, dst port: %d\n", tcpHeader->source, tcpHeader->dest);
            
            LOGI("seq: %u, ack: %u, data offset: %u", tcpHeader->th_seq, tcpHeader->th_ack, tcpHeader->th_off);
            LOGI("seq: %u, ack: %u, data offset: %u", tcpHeader->seq, tcpHeader->ack_seq, tcpHeader->doff);

            LOGI("window size: %u, checksum: %u", tcpHeader->th_win, tcpHeader->th_sum);
            LOGI("window size: %u, checksum: %u", tcpHeader->window, tcpHeader->check);
        }
        sleep(1);
        
    }

    return 0;
}
