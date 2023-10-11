/*************************************************************************
    > File Name: test_mmap.cc
    > Author: hsz
    > Brief: 测试映射文件
    > Created Time: Sat 11 Feb 2023 03:13:58 PM CST
 ************************************************************************/

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <error.h>
#include <errno.h>
#include <assert.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#define LOG_TAG "mmap_test"

#define FILE_NAME       "file.txt"
#define FILE_MAX_SIZE   (1024 * 10)

void test_mmap_file(int argc, char **argv)
{
    char begin = 'A';
    int distance = 'Z' - 'A';
    srand(time(nullptr));

    int fd = open(FILE_NAME, O_RDWR|O_CREAT|O_TRUNC, 0664);
    if (fd < 0) {
        perror("open file " FILE_NAME " error");
        return;
    }

    // 文件不能是一个0大小的文件，所以我们需要修改文件的大小
    // leek，write，或者ftruncate都可以

    // lseek(fd, 1023, SEEK_END);
    // write(fd, "\0", 1);

    // 将参数fd指定的文件大小改为参数length指定的大小, 必须是以写入模式打开的文件
    assert(ftruncate(fd, 1024) == 0);

    char *mmapPtr = (char *)mmap(nullptr, FILE_MAX_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mmapPtr == MAP_FAILED) {
        perror("mmap failed");
        goto end;
    }

    {
        // 直接修改内存可以读写文件内容，mmap创建的是虚拟内存，linux下交换内存(swap空间)是虚拟内存与物理内存的切换空间
        int size = 1024;
        for (int i = 0; i < size; ++i) {
            if (i && i % 16 == 0) {
                mmapPtr[i] = '\n';
            } else {
                mmapPtr[i] =  rand() % distance + begin;
            }
        }
        // msync(mmapPtr, size, MS_SYNC);
    }

end:
    if (mmapPtr) {
        munmap(mmapPtr, FILE_MAX_SIZE);
    }

    if (fd > 0) {
        char buf[1024] = { 0 };
        while (true) {
            int readSize = read(fd, buf, sizeof(buf));
            if (readSize == 0) {
                break;
            }
            if (readSize < 0) {
                if (errno != EAGAIN) {
                    perror("read error");
                }
                break;
            }
            printf("%s", buf);
        }
        printf("\n");
        close(fd);
    }
}

/**
 * @brief 
 * 
 * @param argc 参数个数
 * @param argv 参数数组
 * @param envp 环境变量
 * @return int 
 */
int main(int argc, char **argv, char **envp)
{
    test_mmap_file(argc, argv);
    return 0;
}
