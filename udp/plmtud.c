/*************************************************************************
    > File Name: plmtud.c
    > Author: hsz
    > Brief:
    > Created Time: 2025年02月12日 星期三 10时49分52秒
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>

#define PACKET_SIZE 1024
#define TIMEOUT 1

// 计算ICMP校验和
unsigned short checksum(void *b, int len) {
    unsigned short *buf = b;
    unsigned int sum = 0;
    unsigned short result;

    for (sum = 0; len > 1; len -= 2)
        sum += *buf++;
    if (len == 1)
        sum += *(unsigned char *)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

// 创建ICMP报文
void create_icmp_packet(struct icmphdr *icmp_hdr, int sequence) {
    icmp_hdr->type = ICMP_ECHO;
    icmp_hdr->code = 0;
    icmp_hdr->un.echo.id = getpid();
    icmp_hdr->un.echo.sequence = sequence;
    icmp_hdr->checksum = 0;
    icmp_hdr->checksum = checksum(icmp_hdr, sizeof(struct icmphdr));
}

// 发送ICMP报文
void send_icmp_packet(int sockfd, struct sockaddr_in *dest_addr, int sequence) {
    char packet[PACKET_SIZE];
    struct icmphdr *icmp_hdr = (struct icmphdr *)packet;

    create_icmp_packet(icmp_hdr, sequence);

    if (sendto(sockfd, packet, sizeof(struct icmphdr), 0, (struct sockaddr *)dest_addr, sizeof(*dest_addr)) <= 0) {
        perror("sendto");
        exit(EXIT_FAILURE);
    }
}

// 接收ICMP响应
int receive_icmp_response(int sockfd, struct sockaddr_in *src_addr) {
    char buffer[PACKET_SIZE];
    socklen_t addr_len = sizeof(*src_addr);
    struct iphdr *ip_hdr;
    struct icmphdr *icmp_hdr;

    if (recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)src_addr, &addr_len) <= 0) {
        perror("recvfrom");
        exit(EXIT_FAILURE);
    }

    ip_hdr = (struct iphdr *)buffer;
    icmp_hdr = (struct icmphdr *)(buffer + (ip_hdr->ihl * 4));

    if (icmp_hdr->type == ICMP_ECHOREPLY && icmp_hdr->un.echo.id == getpid()) {
        return 1;
    }

    return 0;
}

// Path MTU Discovery
void path_mtu_discovery(const char *hostname) {
    int sockfd;
    struct sockaddr_in dest_addr, src_addr;
    struct hostent *host;
    int sequence = 0;
    int mtu = 1600;

    if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    if ((host = gethostbyname(hostname)) == NULL) {
        perror("gethostbyname");
        exit(EXIT_FAILURE);
    }

    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr = *(struct in_addr *)host->h_addr_list[0];

    while (mtu > 68) {
        printf("Testing MTU: %d\n", mtu);

        // 设置DF（Don't Fragment）标志
        int optval = IP_PMTUDISC_DO;
        if (setsockopt(sockfd, IPPROTO_IP, IP_MTU_DISCOVER, &optval, sizeof(optval)) < 0) {
            perror("setsockopt IP_MTU_DISCOVER");
            exit(EXIT_FAILURE);
        }

        // 发送ICMP报文
        send_icmp_packet(sockfd, &dest_addr, sequence++);

        // 设置超时
        struct timeval timeout = {TIMEOUT, 0};
        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
            perror("setsockopt SO_RCVTIMEO");
            exit(EXIT_FAILURE);
        }

        // 接收ICMP响应
        if (receive_icmp_response(sockfd, &src_addr)) {
            printf("MTU %d is supported\n", mtu);
            break;
        } else {
            printf("MTU %d is not supported, reducing\n", mtu);
            mtu -= 10;
        }
    }

    close(sockfd);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <hostname>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    path_mtu_discovery(argv[1]);

    return 0;
}