/*************************************************************************
    > File Name: test_pthread_kill.cc
    > Author: hsz
    > Brief: g++ test_pthread_kill.cc -std=c++11 -g -lpthread
    > Created Time: 2024年04月03日 星期三 15时44分54秒
 ************************************************************************/

#include <stdio.h>
#include <string>

#include <pthread.h>
#include <unistd.h>
#include <signal.h>

bool exited = false;

void signal_handler(int sig)
{
    const char *signalName = nullptr;
    switch (sig)
    {
    case SIGUSR1:
        signalName = "SIGUSR1";
        break;
    case SIGUSR2:
        signalName = "SIGUSR2";
        break;
    
    default:
        signalName = "unknow";
        break;
    }

    printf("(%zu)Signal %s received\n", pthread_self(), signalName);
}

void *thread_routine(void *arg)
{
    struct sigaction sa1;
    struct sigaction sa2;

    // 设置信号处理程序
    sa1.sa_handler = signal_handler;
    sigemptyset(&sa1.sa_mask);
    sa1.sa_flags = 0;

    // 安装信号处理程序
    if (sigaction(SIGUSR1, &sa1, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    sa2.sa_handler = signal_handler;
    sigemptyset(&sa2.sa_mask);
    sa2.sa_flags = 0;
    if (sigaction(SIGUSR2, &sa2, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    while (!exited)
    {
        sleep(1);
        printf("============> %zu sleeping\n", pthread_self());
    }
}

int main(int argc, char **argv)
{
    static const int32_t threadCount = 8;
    pthread_t threadVec[threadCount] = {0};

    for (int32_t i = 0; i < threadCount; ++i)
    {
        auto ret = pthread_create(&threadVec[i], nullptr, thread_routine, nullptr);
        if (ret != 0)
        {
            perror("pthread_create error");
            exit(0);
        }
    }

    sleep(2);
    for (int32_t i = 0; i < 2; ++i)
    {
        for (int32_t j = 0; j < threadCount; ++j)
        {
            if (j & 0x01)
            {
                pthread_kill(threadVec[j], SIGUSR1);
            }
            else
            {
                pthread_kill(threadVec[j], SIGUSR2);
            }
        }
        sleep(1);
        printf("\n");
    }

    sleep(2);
    exited = true;
    for (int32_t j = 0; j < threadCount; ++j)
    {
        pthread_join(threadVec[j], nullptr);
    }

    printf("press any key to stop\n");
    getchar();

    return 0;
}
