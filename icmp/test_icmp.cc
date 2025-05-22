#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <sys/socket.h>

#define BUFFER_SIZE 1024

void handle_icmp_message(struct msghdr *msg) {
    struct icmphdr *icmp_hdr;
    struct iphdr *ip_hdr;
    struct udphdr *udp_hdr;
    unsigned char *buffer;
    ssize_t len;
    
    buffer = (unsigned char *)msg->msg_iov[0].iov_base;
    len = msg->msg_iov[0].iov_len;

    if (len < sizeof(struct iphdr) + sizeof(struct icmphdr)) {
        fprintf(stderr, "Packet too short for ICMP and IP headers\n");
        return;
    }

    ip_hdr = (struct iphdr *)buffer;
    icmp_hdr = (struct icmphdr *)(buffer + sizeof(struct iphdr));

    if (icmp_hdr->type == ICMP_DEST_UNREACH && icmp_hdr->code == ICMP_FRAG_NEEDED) {
        printf("Received ICMP Fragmentation Needed message\n");

        // Extract the original IP header and UDP header from the ICMP payload
        struct iphdr *orig_ip_hdr = (struct iphdr *)(buffer + sizeof(struct iphdr) + sizeof(struct icmphdr));
        struct udphdr *orig_udp_hdr = (struct udphdr *)((unsigned char *)orig_ip_hdr + (orig_ip_hdr->ihl * 4));

        if ((unsigned char *)orig_udp_hdr + sizeof(struct udphdr) > buffer + len) {
            fprintf(stderr, "Packet too short for original UDP header\n");
            return;
        }

        printf("Original packet info:\n");
        printf("Source IP: %s\n", inet_ntoa(*(struct in_addr *)&orig_ip_hdr->saddr));
        printf("Destination IP: %s\n", inet_ntoa(*(struct in_addr *)&orig_ip_hdr->daddr));
        printf("Source Port: %d\n", ntohs(orig_udp_hdr->source));
        printf("Destination Port: %d\n", ntohs(orig_udp_hdr->dest));
    }
}

int main() {
    int sock;
    struct sockaddr_in addr;
    struct msghdr msg;
    struct iovec iov;
    unsigned char buffer[BUFFER_SIZE];
    ssize_t len;

    // 创建原始套接字以接收ICMP消息
    sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // 设置接收缓冲区
    iov.iov_base = buffer;
    iov.iov_len = sizeof(buffer);

    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    while (1) {
        // 接收ICMP消息
        len = recvmsg(sock, &msg, 0);
        if (len < 0) {
            perror("recvmsg");
            close(sock);
            exit(EXIT_FAILURE);
        }

        // 处理ICMP消息
        handle_icmp_message(&msg);
    }

    close(sock);
    return 0;
}