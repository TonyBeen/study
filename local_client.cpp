/*************************************************************************
    > File Name: local_client.cpp
    > Author: hsz
    > Mail:
    > Created Time: Fri 03 Sep 2021 10:24:51 AM CST
 ************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <signal.h>

#define LOCAL_CLIENT_SOCK_PATH_FMT "/tmp/log_client_sock"
#define LOCAL_SERVER_SOCK_PATH "/tmp/log_server_sock"

void signal_handle(int sig)
{
    printf(" signal capture sig: %d\n", sig);
    unlink(LOCAL_CLIENT_SOCK_PATH_FMT);
    exit(0);
}

int main(int argc, const char* argv[])
{
    signal(SIGINT, signal_handle);
    signal(SIGQUIT, signal_handle);
    int fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if(fd == -1)
    {
        perror("socket error");
        exit(1);
    }

    unlink(LOCAL_CLIENT_SOCK_PATH_FMT);

    // 给客户端绑定一个套接字文件
    struct sockaddr_un client;
    client.sun_family = AF_LOCAL;
    strcpy(client.sun_path, LOCAL_CLIENT_SOCK_PATH_FMT);
    int ret = bind(fd, (struct sockaddr*)&client, sizeof(client));
    if(ret == -1)
    {
        perror("bind error");
        exit(1);
    }

    // 初始化server信息
    struct sockaddr_un serv;
    serv.sun_family = AF_LOCAL;
    strcpy(serv.sun_path, LOCAL_SERVER_SOCK_PATH);

    // 连接服务器
    if (connect(fd, (struct sockaddr*)&serv, sizeof(serv)) == -1) {
        perror("connect error:");
        return 0;
    }

    // 通信
    while(1)
    {
        char buf[1024] = {0};
        fgets(buf, sizeof(buf), stdin);
        send(fd, buf, strlen(buf)+1, 0);

        // 接收数据
        recv(fd, buf, sizeof(buf), 0);
        printf("recv buf: %s\n", buf);
    }

    close(fd);
    unlink(LOCAL_CLIENT_SOCK_PATH_FMT);

    return 0;
}

