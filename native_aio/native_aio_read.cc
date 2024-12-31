/*************************************************************************
    > File Name: native_aio_read.cc
    > Author: hsz
    > Brief: g++ native_aio_read.cc async_io.cpp -o native_aio_read.out
    > Created Time: 2024年12月31日 星期二 15时51分15秒
 ************************************************************************/

#include "async_io.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#define FILEPATH "./aio.out"

void test_read_one(int32_t size = 4096)
{
    aio_context_t context = 0;
    struct iocb io[1], *p[1] = {&io[0]};
    struct io_event e[1];
    uint32_t nr_events = 10;
    struct timespec timeout;
    void *wbuf;
    int ret = 0;
    int block_size = 512; // 默认 512
    int rbuflen = block_size;

    long page_size = sysconf(_SC_PAGE_SIZE);
    if (page_size == -1) {
        perror("sysconf error");
        return;
    }
    printf("Page size: %ldB\n", page_size);

    timeout.tv_sec = 1;
    timeout.tv_nsec = 0;

    // 打开要进行异步IO的文件
    // O_DIRECT 要求数据必须是磁盘块大小的倍数, 一般是512/4k/8k/16k/32k ... mkfs.ext4 -b 4096 /dev/sda
    int fd = open(FILEPATH, O_RDONLY | O_DIRECT);
    if (fd < 0) {
        perror("open error");
        return;
    }

    // int32_t blk_fd = open("/dev/nvme0n1", O_RDONLY);
    // if (blk_fd < 0) {
    //     perror("open /dev/nvme0n1 error");
    //     return;
    // }
    // if (ioctl(blk_fd, BLKSSZGET, &block_size) == 0) {
    //     printf("Block size of %s: %d bytes\n", FILEPATH, block_size);
    // } else {
    //     perror("Failed to get block size");
    // }

    rbuflen = size / block_size * block_size;
    if (size % block_size) {
        rbuflen += block_size;
    }
    // 分配对齐内存
    wbuf = aligned_alloc(page_size, rbuflen);
    if (!wbuf) {
        perror("aligned_alloc error");
        return;
    }
    printf("Buffer allocated at: %p\n", wbuf);

    // 创建异步IO上下文
    ret = io_setup(nr_events, &context);
    if (ret != 0) {
        perror("io_setup error");
        close(fd);
        return;
    }

    // 准备异步读操作
    io_prep_pread(&io[0], fd, wbuf, rbuflen, 0);

    // 提交异步IO任务
    ret = io_submit(context, 1, p);
    if (ret != 1) {
        printf("io_submit error: %d\n", ret);
        io_destroy(context);
        close(fd);
        return;
    }

    // 获取异步IO的结果(阻塞) NOTE 文件携带 O_DIRECT 标志时 min_nr 需要设置为1
    ret = io_getevents(context, 1, 1, e, &timeout);
    if (ret < 0) {
        printf("io_getevents error: %s\n", strerror(-ret));
    } else if (ret > 0) {
        printf("result, res2: %lld, res: %lld\n", e[0].res2, e[0].res);
    } else {
        printf("ret = 0\n");
    }

    io_destroy(context);
    close(fd);
}

int main(int argc, char *argv[])
{
    int32_t size = 4096;
    if (argc > 1) {
        size = atoi(argv[1]);
    }

    size = std::abs(size);
    test_read_one(size);
    return 0;
}
