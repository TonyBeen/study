/*************************************************************************
    > File Name: test_eventfd.cc
    > Author: hsz
    > Brief:
    > Created Time: Mon 04 Jul 2022 09:46:37 AM CST
 ************************************************************************/

#include <sys/eventfd.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <assert.h>
#include <iostream>
#include <thread>
#include <string.h>

using namespace std;

int evfd;

void thread_1()
{
    fd_set set;
    FD_ZERO(&set);
    fd_set readSets;
    fd_set errorSets;

    struct timeval timeout;
    timeout.tv_sec  = 1;
    timeout.tv_usec = 0;
    FD_SET(evfd, &set);

    while (1) {
        FD_ZERO(&readSets);
        FD_SET(evfd, &readSets);
        int ret = select(evfd + 1, &readSets, NULL, &errorSets, NULL);
        if (ret < 0) {
            perror("select error");
            break;
        }
        if (ret == 0) {
            printf("timeout\n");
            continue;
        }

        usleep(100 * 1000);
        uint64_t value;
        int readSize = read(evfd, &value, sizeof(value));
        if (readSize < 0) {
            printf("%d,%s\n", errno, strerror(errno));
        }
        assert(readSize == 8);
        printf("read: %lu\n", value);
    }
}

int main(int argc, char **argv)
{
    evfd = eventfd(0, EFD_NONBLOCK);
    assert(evfd > 0);
    std::thread th(thread_1);
    th.detach();
    uint64_t i = 0;
    while (1) {
        uint64_t value = 1;
        // NOTE 每次写入的数据会将未读的数据加上, 使用read读会重置为0. 即写快读满的情况下, 一次读到的数据大于1
        write(evfd, &value, sizeof(value));
        usleep(500 * 1000);
    }

    return 0;
}
