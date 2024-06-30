/*************************************************************************
    > File Name: libhv_kcp.cc
    > Author: hsz
    > Brief:
    > Created Time: 2024年06月15日 星期六 17时01分34秒
 ************************************************************************/

#include "hv/hloop.h"
#include "hv/hbase.h"
#include "hv/hsocket.h"
#include "hv/hthread.h"

#define RECV_BUFSIZE    8192
static char recvbuf[RECV_BUFSIZE];

hio_t* sockio = NULL;
hloop_t* loop = NULL;

static void on_recv(hio_t* io, void* buf, int readbytes)
{
    char localaddrstr[SOCKADDR_STRLEN] = { 0 };
    char peeraddrstr[SOCKADDR_STRLEN] = { 0 };
    printf("[%s] <=> [%s]: ",
        SOCKADDR_STR(hio_localaddr(io), localaddrstr),
        SOCKADDR_STR(hio_peeraddr(io), peeraddrstr));
    
    printf("%.*s\n", readbytes, (char*)buf);
    fflush(stdout);
}

void *thread_routine(void*)
{
    hio_read(sockio);
    hio_setcb_read(sockio, on_recv);
    hio_set_readbuf(sockio, recvbuf, RECV_BUFSIZE);
    hloop_run(loop);
    hloop_free(&loop);

    return 0;
}

int main()
{
    loop = hloop_new(0);
    const char* host = "127.0.0.1";
    uint16_t port = 9000;
    sockio = hloop_create_udp_client(loop, host, port);
    
    int fd = hio_fd(sockio);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(host);
    addr.sin_port = htons(port);

    uint16_t conv = 0x1024;
    printf("conv = 0x%x\n", conv);

    static kcp_setting_t s_kcp_setting;
    memset(&s_kcp_setting, 0, sizeof(kcp_setting_t));
    s_kcp_setting.conv = conv;
    s_kcp_setting.nodelay = 1;
    s_kcp_setting.interval = 10;
    s_kcp_setting.fastresend = 2;
    s_kcp_setting.nocwnd = 1;
    s_kcp_setting.sndwnd = 1024;
    s_kcp_setting.rcvwnd = 1024;
    hio_set_kcp(sockio, &s_kcp_setting);

    hthread_t threadHandle = hthread_create(thread_routine, NULL);
    if (threadHandle == NULL) {
        printf("hthread_create error\n");
        return 0;
    }

    int32_t fileFd = ::open("/home/eular/VSCode/Image_800mb.rar", O_RDONLY);
    assert(fileFd > 0);

    static const uint32_t SIZE = (1400 - 24) * 16;

    char buf[SIZE] = {0};
    uint16_t times = 0;
    while (true) {
        int32_t readSize = ::read(fileFd, buf, sizeof(buf));
        if (readSize < 0) {
            perror("read error");
            break;
        }

        if (readSize == 0) {
            break;
        }

        hio_write(sockio, buf, readSize);
        hv_msleep(10);
    }

    return 0;
}
