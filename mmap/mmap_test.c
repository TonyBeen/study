/*************************************************************************
    > File Name: mmap_test.c
    > Author: hsz
    > Brief:
    > Created Time: Thu 05 Oct 2023 05:56:19 PM CST
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main()
{
    int fd;
    char *map;          // 映射到的内存区域指针
    const char *filepath = "test.txt";
    const int filesize = 1024;  // 文件大小

    // 打开文件
    fd = open(filepath, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    ftruncate(fd, 1024);

    // 对文件进行内存映射
    map = (char *) mmap(NULL, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // 写入数据到映射区域
    const char *data = "Hello, Memory Map!";
    memcpy(map, data, strlen(data));

    // 刷新到磁盘
    if (msync(map, filesize, MS_SYNC)) {
        perror("msync");
        exit(EXIT_FAILURE);
    }

    // 取消映射
    if (munmap(map, filesize) == -1) {
        perror("munmap");
        exit(EXIT_FAILURE);
    }

    // 关闭文件
    if (close(fd) == -1) {
        perror("close");
        exit(EXIT_FAILURE);
    }

    return 0;
}
