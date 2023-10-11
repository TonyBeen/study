/*************************************************************************
    > File Name: mmap.cc
    > Author: hsz
    > Brief:
    > Created Time: Thu 28 Sep 2023 03:11:10 PM CST
 ************************************************************************/

#include <stdlib.h>
#include <iostream>
#include <string>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

std::string generate_random_string(int length)
{
    std::string visible_chars = 
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()_+-=[]{}|;':,.<>?";

    std::string random_string;
    srand(time(NULL));

    for (int i = 0; i < length; i++) {
        int random_index = rand() % visible_chars.length();
        random_string += visible_chars[random_index];
    }

    return random_string;
}


int main()
{
    int fd;
    char* mappedMem;
    const size_t fileSize = 512 * 1024; // 512KB

    // 打开/创建文件
    fd = open("./mappedfile.dat", O_RDWR | O_CREAT | O_TRUNC, 0664);
    if (fd == -1) {
        std::cout << "无法打开/创建文件" << std::endl;
        return 1;
    }

    // 调整文件大小
    if (ftruncate(fd, fileSize) == -1) {
        std::cout << "无法调整文件大小" << std::endl;
        close(fd);
        return 1;
    }

    // 将文件映射到内存中
    mappedMem = static_cast<char*>(mmap(NULL, fileSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    if (mappedMem == MAP_FAILED) {
        std::cout << "无法创建内存映射" << std::endl;
        close(fd);
        return 1;
    }

    // 使用内存映射文件
    std::string randString = generate_random_string(1024);
    memcpy(mappedMem, randString.c_str(), randString.length());

    printf("%s\n", mappedMem);

    // 刷新到磁盘
    if (msync(mappedMem, fileSize, MS_SYNC)) {
        perror("msync");
        exit(EXIT_FAILURE);
    }

    // 解除内存映射
    if (munmap(mappedMem, fileSize) == -1) {
        std::cout << "无法解除内存映射" << std::endl;
    }

    // 修改文件为实际大小
    // if (ftruncate(fd, randString.length() / 2) != 0) {
    //     std::cout << "无法调整文件大小" << std::endl;
    //     close(fd);
    //     return 1;
    // }

    // 关闭文件
    close(fd);

    return 0;
}

