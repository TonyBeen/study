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

#include <string>

#define BUFF_LEN 1024

int main(int argc, char **argv)
{
    if (argc != 5) {
        printf("usage: %s -h host -p port\n", argv[0]);
        return 0;
    }

    uint16_t port = 9000;
    const char *host = "127.0.0.1";

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
    
    int ufd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    assert(ufd > 0);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(host);
    addr.sin_port = htons(port);

    char buf[1400] = {0};
    uint32_t i = 0;
    std::string str;
    str.reserve(1400);
    while (true) {
        str.clear();
        sprintf(buf, "[%d:%d] [1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ]", getpid(), ++i);
        str.append(buf);

        int nsend = sendto(ufd, buf, strlen(buf), 0, (sockaddr *)&addr, len);
        printf("buf %s, nsend %d\n", buf, nsend);

        usleep(300 * 1000);
    }

    close(ufd);
    return 0;
}
