/*************************************************************************
    > File Name: captcp.cc
    > Author: hsz
    > Brief:
    > Created Time: Tue 23 Nov 2021 01:38:12 PM CST
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
#include <utils/Errors.h>

#include <pcap/pcap.h>

#define LOG_TAG "cap tcp"

void getPacket(u_char *arg, const struct pcap_pkthdr *pkthdr, const u_char *packet)
{
    int *id = (int *)arg;

    printf("id: %d\n", ++(*id));
    printf("Packet length: %d\n", pkthdr->len);
    printf("Number of bytes: %d\n", pkthdr->caplen);
    printf("Recieved time: %s\n", ctime(&pkthdr->ts.tv_sec));
    //print packet
    int i;
    for (i = 0; i < 64; ++i) {
        printf("%02x ", packet[i]);
        if ((i + 1) % 16 == 0)
            printf("\n");
    }
    printf("\n");
    struct ethhdr *eth = (struct ethhdr *)packet; // 以太网帧结构体
    Jarvis::String8 destMac;
    for (int i = 0; i < ETH_ALEN; ++i) {
        destMac.appendFormat("%02x", eth->h_dest[i]);
        if (i < ETH_ALEN - 1) {
            destMac.appendFormat(":");
        }
    }
    Jarvis::String8 srcMac;
    for (int i = 0; i < ETH_ALEN; ++i) {
        srcMac.appendFormat("%02x", eth->h_source[i]);
        if (i < ETH_ALEN - 1) {
            srcMac.appendFormat(":");
        }
    }

    int protocol = ntohs(eth->h_proto);
    printf("dest mac: %s, src mac: %s, protocol: 0x%04x", destMac.c_str(), srcMac.c_str(), protocol);
    if (protocol == ETH_P_IP) {
        printf(", IP protocol received\n");
        struct ip *ip = (struct ip *)(packet + ETH_HLEN); // IP头部, 偏移一个以太网帧长度
        int ipHdrLen = ip->ip_hl * 4;
        printf("IP Header:\n\tlength: %d;\n"
            "\tversion: %d;\n"
            "\ttotal len: %d\n"
            "\tdst ip: %s\n"
            "\tsrc ip: %s\n", ipHdrLen, ip->ip_v, ip->ip_len,
            inet_ntoa(ip->ip_dst), inet_ntoa(ip->ip_src));
        struct tcphdr *tcp = (struct tcphdr *)(ip + ipHdrLen);  // TCP头部
        printf("TCP Header:\n\tdest port: %d\n"
            "\tsrc port: %d\n"
            "\tseq: %d, ack: %d\n"
            "\tfin: %u, syn %d, rst %d, ack %d\n",
            tcp->dest, tcp->source, tcp->seq, tcp->ack_seq,
            tcp->fin, tcp->syn, tcp->rst, tcp->ack);
    } else {
        printf("\n");
    }
    
    sleep(2);
}

int main(int argc, char **argv)
{
    int nRet = 0;
    char errBuf[PCAP_ERRBUF_SIZE] = {0};
    char *ifdev = pcap_lookupdev(errBuf);
    LOGI("ifdev: %s\n", ifdev);

    bpf_u_int32 netIP;
    uint32_t netMask;
    nRet = pcap_lookupnet(ifdev, &netIP, &netMask, errBuf);
    if (nRet) {
        LOGE("pcap_lookipnet error: %s\n", errBuf);
        return nRet;
    }
    struct in_addr addrTmp;
    addrTmp.s_addr = netIP;
    LOGI("IP: [%d] %s", netIP, inet_ntoa(addrTmp));
    addrTmp.s_addr = netMask;
    LOGI("Mask: [%d] %s\n", netMask, inet_ntoa(addrTmp));

    // LOGI("IP: [%d] %s\n", inet_addr("172.25.12.215"), "172.25.12.215");
    pcap_t *capHandle = pcap_open_live(ifdev, 128, 0, 0, errBuf);
    if (capHandle == nullptr) {
        LOGE("errbuf: %s\n", errBuf);
        return 0;
    }

    pcap_loop(capHandle, -1, getPacket, (u_char *)errBuf);

    return 0;
}
