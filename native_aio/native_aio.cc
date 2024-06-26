/*************************************************************************
    > File Name: native_aio.cc
    > Author: hsz
    > Brief: g++ native_aio.cc async_io.cpp -o native_aio.out
    > Created Time: Sat 28 Oct 2023 05:39:17 PM CST
 ************************************************************************/

#include "async_io.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#define FILEPATH "./aio.out"

void test_write_one()
{
    aio_context_t context;
    struct iocb io[1], *p[1] = {&io[0]};
    struct io_event e[1];
    uint32_t nr_events = 10;
    struct timespec timeout;
    void *wbuf;
    int wbuflen = 1024;
    int ret, num = 0, i;

    wbuf = aligned_alloc(64, wbuflen);
    posix_memalign((void **)&wbuf, 512, wbuflen);

    timeout.tv_sec = 0;
    timeout.tv_nsec = 10000000;

    // 1. 打开要进行异步IO的文件
    int fd = open(FILEPATH, O_RDWR | O_CREAT | O_TRUNC | O_DIRECT, 0664);
    if (fd < 0) {
        printf("open error: %d\n", errno);
        return;
    }

    // 2. 创建一个异步IO上下文
    if (0 != io_setup(nr_events, &context)) {
        printf("io_setup error: %d\n", errno);
        return;
    }

    // 3. 创建一个异步IO任务
    io_prep_pwrite(&io[0], fd, wbuf, wbuflen, 0);

    // 4. 提交异步IO任务
    if ((ret = io_submit(context, 1, p)) != 1) {
        printf("%d: io_submit error: %d\n", i, ret);
        io_destroy(context);
        return;
    }

    // 5. 获取异步IO的结果(阻塞)
    ret = io_getevents(context, 0, 1, e, &timeout);
    if (ret < 0) {
        printf("io_getevents error: %d\n", ret);
    } else if (ret > 0) {
        printf("result, res2: %lld, res: %lld\n", e[0].res2, e[0].res);
    }

    io_destroy(context);
}

int main()
{
    test_write_one();
    return 0;
}