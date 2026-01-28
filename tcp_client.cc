#include <stdio.h>
#include <string.h>
#include <string>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>

#include <utils/CLI11.hpp>

#define MAXLINE     10
#define SERV_PORT   8000
#define SERV_HOST   "82.157.8.46"

int main(int argc, char *argv[])
{
    std::string address = SERV_HOST;
    int32_t port = SERV_PORT;
    int32_t block_size = 1460;
    int32_t cycle = 10;

    CLI::App app{"TCP Client Example"};
    app.add_option("-a,--address", address, "Server IP address");
    app.add_option("-p,--port", port, "Server port");
    app.add_option("-b,--block_size", block_size, "Block size to send");
    app.add_option("-c,--cycle", cycle, "Number of cycles to send data");
    CLI11_PARSE(app, argc, argv);

    struct sockaddr_in servaddr;
    char *buf = new char[block_size];
    int sockfd;
    char ch = 'a';

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(address.c_str());
    servaddr.sin_port = htons(port);
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) {
        printf("errno = %d, errstr = %s\n", errno, strerror(errno));
        return 0;
    }

    printf("connect success\n");
    int idx = 0;
    while (idx++ < cycle) {
        int32_t i = 0;
        for (i = 0; i < block_size / 2; i++)
            buf[i] = ch;
        ch++;
        for (; i < block_size; i++)
            buf[i] = ch;
        ch++;
        if (ch > 'z') {
            ch = 'a';
        }

        int wrSize = write(sockfd, buf, block_size);
        printf("write size = %d\n", wrSize);
        usleep(1000 * 1000);
    }

    sleep(10);
    close(sockfd);
    return 0;
}
