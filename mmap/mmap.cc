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
#include <atomic>
#include <mutex>
#include <random>
#include <thread>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/syscall.h>

#define gettid() syscall(__NR_gettid)

#define FILE_SIZE 1024 * 1024

char *g_mapPtr = nullptr;
std::mutex g_mutex;
uint32_t g_index = 0;
std::atomic<bool> g_exit(false);

std::string generate_random_string(int length)
{
    std::string visible_chars = 
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()_+-=[]{}|;':,.<>?";

    std::string random_string;
    random_string.reserve(length);
    
    // 生成指定长度随机字符串
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint8_t> dis(0, visible_chars.length() - 1);

    for (int i = 0; i < length; i++) {
        int random_index = rand() % visible_chars.length();
        random_string += visible_chars[dis(gen)];
    }

    return random_string;
}

void thread_func()
{
    while (g_exit.load(std::memory_order_relaxed) == false) {

        // sched_yield();

        std::string randString = generate_random_string(64);
        randString = std::to_string(gettid()) + ": " + randString + "\n";

        std::lock_guard<std::mutex> lock(g_mutex);
        if (g_index + randString.length() > FILE_SIZE) {
            break;
        }

        memcpy(g_mapPtr + g_index, randString.c_str(), randString.length());

        g_index += randString.length();
    }
}

int main()
{
    printf("%ld\n", gettid());
    int fd;
    char* mappedMem;
    const size_t fileSize = FILE_SIZE; // 512KB

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
    g_mapPtr = mappedMem = static_cast<char*>(mmap(NULL, fileSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    if (mappedMem == MAP_FAILED) {
        std::cout << "无法创建内存映射" << std::endl;
        close(fd);
        return 1;
    }

    std::thread t1(thread_func);
    std::thread t2(thread_func);

    // 使用内存映射文件
    getchar();
    g_exit = true;

    t1.join();
    t2.join();

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

