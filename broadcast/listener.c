/*************************************************************************
    > File Name: listener.c
    > Author: hsz
    > Brief:
    > Created Time: 2024年04月04日 星期四 15时28分41秒
 ************************************************************************/

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <ifaddrs.h>

#include <unistd.h>

#define PORT atoi(argv[2])
#define GROUP argv[1]
#define BUFF_SIZE 2000

int main(int argc, char *argv[])
{
    if (argc != 3) {
       printf("Command line args should be multicast group and port\n");
       printf("(e.g. for SSDP, `listener 239.255.255.250 1900`)\n");
       return 1;
    }

    struct sockaddr_in addr, caddr;
    int sd, read_size;
    char buff[BUFF_SIZE];

    if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket");
        exit(1);
    }

    /* set up destination address */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    // Set group for all addresses
    struct ip_mreqn group;
    struct ifaddrs *addrs;
    getifaddrs(&addrs);
    struct ifaddrs *tmp = addrs;
    while (tmp)
    {
        if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET)
        {
            struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
            group.imr_multiaddr.s_addr = inet_addr(GROUP);
            printf("found address: %s\n", inet_ntoa(pAddr->sin_addr));
            group.imr_address.s_addr = inet_addr(inet_ntoa(pAddr->sin_addr));
            group.imr_ifindex = 0;
            setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group));
        }
        tmp = tmp->ifa_next;
    }
    freeifaddrs(addrs);

    if (bind(sd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        return 1;
    }

    int c = sizeof(caddr);
    while (1)
    {
        read_size = recvfrom(sd, buff, BUFF_SIZE - 1, MSG_WAITALL, (struct sockaddr *)&caddr, &c);
        sendto(sd, "Hello im server", 15, 0, (struct sockaddr *)&caddr, c);
        buff[read_size] = '\0';
        printf("%s\n", inet_ntoa(caddr.sin_addr));
        printf("MESG: %s\n", buff);
    }
}