/*************************************************************************
    > File Name: thread_bind_cpu.cc
    > Author: hsz
    > Brief:
    > Created Time: Tue 09 Jan 2024 09:02:26 AM CST
 ************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

#define MAX_CPU 16

int32_t gCpuSize = 0;

void log(const char *func)
{
    printf("%s() %d: %s\n", func, errno, strerror(errno));
}

void* thread_func(void* arg)
{
    int cpu = *(int*)arg;
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    int32_t code = pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if (code < 0) {
        log(__func__);
        return NULL;
    }

    // CPU_COUNT返回值表示当前线程与几个CPU具有亲和性, 当设置了线程绑定CPU后,
    // CPU_COUNT会返回1, 表示绑定了一个特定的CPU
    int num_cpus = CPU_COUNT(&cpuset);
    for (int32_t i = 0; i < gCpuSize; ++i) {
        if (CPU_ISSET(i, &cpuset)) {
            printf("Thread running on CPU: %d\n", i);
            break;
        }
    }

    return NULL;
}

int main()
{
    // 获取CPU数量
    int32_t num_cpus = sysconf(_SC_NPROCESSORS_CONF);
    gCpuSize = num_cpus;

    pthread_t threads[MAX_CPU] = {0};
    int32_t cpuIndexVec[MAX_CPU];

    for (int i = 0; i < num_cpus; i++) {
        cpuIndexVec[i] = i;
        pthread_create(&threads[i], NULL, thread_func, &cpuIndexVec[i]);

        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        pthread_setaffinity_np(threads[i], sizeof(cpu_set_t), &cpuset);
    }

    for (int i = 0; i < num_cpus; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
