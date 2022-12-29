/*************************************************************************
    > File Name: udp_server.cc
    > Author: hsz
    > Brief:
    > Created Time: Mon 30 May 2022 03:26:42 PM CST
 ************************************************************************/


#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFF_LEN 1024
#define SERVER_IP "10.0.24.17"
#define SERVER_PORT 9000

void handle_udp_msg(int fd)
{
    char buf[BUFF_LEN];  //接收缓冲区，1024字节
    socklen_t len;
    int count;
    struct sockaddr_in client_addr;  //clent_addr用于记录发送方的地址信息
    while(1) {
        memset(buf, 0, BUFF_LEN);
        len = sizeof(client_addr);
        count = recvfrom(fd, buf, BUFF_LEN, 0, (struct sockaddr*)&client_addr, &len);  //recvfrom是拥塞函数，没有数据就一直拥塞
        if(count == -1) {
            printf("recieve data fail!\n");
            return;
        }

        printf("[%s:%d]:%s,%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), buf, count);  //打印client发过来的信息

        if (count > 0) {
            int n = sprintf(buf, "I have recieved %d bytes data!\n", count);  // 回复client
            sendto(fd, buf, n, 0, (struct sockaddr*)&client_addr, len);  //发送信息给client，注意使用了clent_addr结构体指针
        }
    }
}

int main(int argc, char* argv[])
{
    int server_fd, ret;
    struct sockaddr_in addr;
    socklen_t len = sizeof(sockaddr_in);

    server_fd = socket(AF_INET, SOCK_DGRAM, 0); // AF_INET:IPV4;SOCK_DGRAM:UDP
    if(server_fd < 0) {
        perror("create socket fail!");
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(SERVER_IP); // IP地址，需要进行网络序转换，INADDR_ANY：本地地址
    addr.sin_port = htons(SERVER_PORT);  // 端口号，需要网络序转换

    ret = bind(server_fd, (struct sockaddr*)&addr, len);
    if(ret < 0) {
        perror("socket bind fail!");
        return -1;
    }

    getsockname(server_fd, (sockaddr *)&addr, &len);
    printf("waiting for client... [%s:%d]\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
    handle_udp_msg(server_fd);   // 处理接收到的数据

    close(server_fd);
    return 0;
}
