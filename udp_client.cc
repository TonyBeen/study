/*************************************************************************
    > File Name: upd_client.cc
    > Author: hsz
    > Brief:
    > Created Time: Sat 18 Jun 2022 11:45:03 AM CST
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <assert.h>
#include <getopt.h>

#define BUFF_LEN 1024

int main(int argc, char **argv)
{
    if (argc != 5) {
        printf("usage: %s -h host -p port\n", argv[0]);
        return 0;
    }

    uint16_t port = 0;
    const char *host = nullptr;

    char ch = 0;
    while ((ch = getopt(argc, argv, "h:p:")) != -1) {
        switch (ch) {
        case 'h':
            host = optarg;
            break;
        case 'p':
            port = (uint16_t)atoi(optarg);
            break;
        default:
            assert(false);
            break;
        }
    }

    assert(host && port > 1000);

    struct sockaddr_in addr;
    socklen_t len = sizeof(sockaddr_in);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(host);
    addr.sin_port = htons(port);
    int ufd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    assert(ufd > 0);

    char buf[128];
    while (true) {
        scanf("%s", buf);
        if (strcasecmp("quit", buf) == 0) {
            break;
        }
        int nsend = sendto(ufd, buf, strlen(buf), 0, (sockaddr *)&addr, len);
        printf("buf %s, nsend %d\n", buf, nsend);
    }

    close(ufd);
    return 0;
}
