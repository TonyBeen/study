/*************************************************************************
    > File Name: local_server.cpp
    > Author: hsz
    > Mail:
    > Created Time: Fri 03 Sep 2021 10:24:41 AM CST
 ************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <error.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <signal.h>
#include <utils/utils.h>
#include <vector>

#define LOCAL_SOCK_PATH "/tmp/log_sock_server"

void signal_interrupt(int sig)
{
    printf(" SIGINT capture sig: %d\n", sig);
    unlink(LOCAL_SOCK_PATH);
    exit(0);
}
void signal_quit(int sig)
{
    printf(" SIGQUIT capture sig: %d\n", sig);
    unlink(LOCAL_SOCK_PATH);
    exit(0);
}

int main(int argc, char *argv[])
{
    const char *procName = argv[0];
    // if (argv[0][0] == '.' && argv[0][1] == '/') {
    //     procName = argv[0] + 2;
    // } else {
    //     procName = argv[0];
    // }
    std::vector<pid_t> pidVec = getPidByName(procName);
    if (pidVec.size() > 0) {
        for(int i = 0; i < pidVec.size(); ++i) {
            printf("%d ", pidVec[i]);
        }
        printf("\n");
    }

    signal(SIGINT, signal_interrupt);
    signal(SIGQUIT, signal_quit);
    int lfd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if(lfd == -1)
    {
        perror("socket error");
        exit(1);
    }

    // 如果套接字文件存在, 删除套接字文件
    unlink(LOCAL_SOCK_PATH);

    // 绑定
    struct sockaddr_un serv;
    serv.sun_family = AF_LOCAL;
    strcpy(serv.sun_path, LOCAL_SOCK_PATH);
    int ret = bind(lfd, (struct sockaddr*)&serv, sizeof(serv));
    if(ret == -1)
    {
        perror("bind error");
        exit(1);
    }
     
    // 监听
    ret = listen(lfd, 36);
    if(ret == -1)
    {
        perror("listen error");
        exit(1);
    }

    // 等待接收连接请求
    struct sockaddr_un client;
    socklen_t len = sizeof(client);
    int cfd = accept(lfd, (struct sockaddr*)&client, &len);
    if(cfd == -1)
    {
        perror("accept error");
        exit(1);
    }
    printf("======client bind file: %s\n", client.sun_path);
     
    // 通信
    while(1)
    {
        char buf[1024] = {0};
        int recvlen = recv(cfd, buf, sizeof(buf), 0);
        if(recvlen == -1) {
            perror("recv error");
            exit(1);
        } else if(recvlen == 0) {
            printf("client disconnect ....\n");
            break;
        } else {
            printf("%s", buf);
        }
    }
    close(cfd);
    close(lfd);
    if (unlink(LOCAL_SOCK_PATH)) {
        perror("unlink error:");
    }
    return 0;
}

