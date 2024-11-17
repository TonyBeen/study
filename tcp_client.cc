#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>

#define MAXLINE 10
#define SERV_PORT 9000
#define SERV_HOST "127.0.0.1"

int main(int argc, char *argv[])
{
    struct sockaddr_in servaddr;
    char buf[MAXLINE];
    int sockfd, i;
    char ch = 'a';

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(SERV_HOST);
    servaddr.sin_port = htons(SERV_PORT);

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)))
    {
        printf("errno = %d, errstr = %s\n", errno, strerror(errno));
        return 0;
    }

    printf("connect success\n");
    int idx = 0;
    while (idx++ < 2)
    {
        for (i = 0; i < MAXLINE / 2; i++)
            buf[i] = ch;
        ch++;
        for (; i < MAXLINE; i++)
            buf[i] = ch;
        ch++;
        if (ch > 'z')
        {
            ch = 'a';
        }

        int wrSize = write(sockfd, buf, sizeof(buf));
        printf("write size = %d\n", wrSize);
        sleep(1);
    }
    sleep(10);
    close(sockfd);
    return 0;
}
