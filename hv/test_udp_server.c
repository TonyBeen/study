/*************************************************************************
    > File Name: test_udp_server.c
    > Author: hsz
    > Brief:
    > Created Time: Mon 27 Jun 2022 10:53:03 AM CST
 ************************************************************************/

#include "hv/hloop.h"
#include "hv/hsocket.h"

static void on_recvfrom(hio_t* io, void* buf, int readbytes) {
    printf("on_recvfrom fd=%d readbytes=%d\n", hio_fd(io), readbytes);
    char localaddrstr[SOCKADDR_STRLEN] = {0};
    char peeraddrstr[SOCKADDR_STRLEN] = {0};
    printf("[%s] <=> [%s]\n",
            SOCKADDR_STR(hio_localaddr(io), localaddrstr),
            SOCKADDR_STR(hio_peeraddr(io), peeraddrstr));
    printf("< %.*s", readbytes, (char*)buf);
    // 回显数据
    printf("> %.*s", readbytes, (char*)buf);
    hio_write(io, buf, readbytes);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s port\n", argv[0]);
        return -10;
    }
    int port = atoi(argv[1]);

	// 创建事件循环
    hloop_t* loop = hloop_new(0);
    // 创建UDP服务
    hio_t* io = hloop_create_udp_server(loop, "0.0.0.0", port);
    if (io == NULL) {
        return -20;
    }
    // 设置read回调
    hio_setcb_read(io, on_recvfrom);
    // 开始读
    hio_read(io);
    // 运行事件循环
    hloop_run(loop);
    // 释放事件循环
    hloop_free(&loop);
    return 0;
}