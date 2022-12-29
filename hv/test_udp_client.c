/*************************************************************************
    > File Name: test_udp_client.c
    > Author: hsz
    > Brief:
    > Created Time: Mon 27 Jun 2022 10:52:36 AM CST
 ************************************************************************/

#include "hv/hloop.h"
#include "hv/htime.h"

void on_timer(htimer_t* timer) {
    char str[DATETIME_FMT_BUFLEN] = {0};
    datetime_t dt = datetime_now();
    datetime_fmt(&dt, str);

    printf("> %s\n", str);
    // 获取userdata
    hio_t* io = (hio_t*)hevent_userdata(timer);
    // 发送时间字符串
    hio_write(io, str, strlen(str));
}

void on_recvfrom(hio_t* io, void* buf, int readbytes) {
    printf("< %.*s\n", readbytes, (char*)buf);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: cmd port\n");
        return -10;
    }
    int port = atoi(argv[1]);

	// 创建事件循环
    hloop_t* loop = hloop_new(0);
    // 创建UDP客户端
    hio_t* io = hloop_create_udp_client(loop, "127.0.0.1", port);
    if (io == NULL) {
        return -20;
    }
    // 设置read回调
    hio_setcb_read(io, on_recvfrom);
    // 开始读
    hio_read(io);
    // 添加一个定时器
    htimer_t* timer = htimer_add(hevent_loop(io), on_timer, 1000, INFINITE);
    // 设置userdata
    hevent_set_userdata(timer, io);
	// 运行事件循环
    hloop_run(loop);
    // 释放事件循环
    hloop_free(&loop);
    return 0;
}
