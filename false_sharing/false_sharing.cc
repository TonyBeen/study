/*************************************************************************
    > File Name: false_sharing.cc
    > Author: hsz
    > Brief: g++ false_sharing.cc -o false_sharing -lutils -lpthread
    > Created Time: 2024年06月17日 星期一 18时21分31秒
 ************************************************************************/

#include <thread>

#include <pthread.h>
#include <sched.h>

#include <utils/elapsed_time.h>
#include <assert.h>

/**
 * @brief 错误共享
 *  
 * 即当两个或多个处理器核心同时访问不同的缓存行，但这些缓存行实际上映射到同一个缓存集合中时，会导致性能下降。
 * 这是因为每个核心在修改自己的缓存行时，会导致其他核心缓存行无效，从而增加了总线通信和缓存一致性开销。
 * 
 * 解决办法
 * 1.在数据结构中添加填充字段，确保不同线程访问的数据不在同一缓存行
 * 
 * https://github.com/facebook/folly/blob/main/folly/lang/Align.h
 * 2.使用编译器指令或语言特性强制数据对齐 (folly/flare做法)
 * 3.将需要同时访问的数据分开存储
 */

#define CYCLE 100000

struct Data {
    int a = 0;
    int b = 0;
};

struct Data2 {
    int a = 0;
    char padding[64] = {0};
    int b = 0;
};

void thread1(void *p, bool isSame) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset);

    eular::ElapsedTime stopwatch(ElapsedTimeType::NANOSECOND);

    if (isSame)
    {
        stopwatch.start();
        Data *pData = (Data *)p;
        for (int i = 0; i < CYCLE; ++i) {
            pData->a++;
        }
        stopwatch.stop();
        assert(pData->a == CYCLE);
        printf("thread1: Modifying the same cache line takes time: %zu ns\n", stopwatch.elapsedTime());
    }
    else
    {
        stopwatch.start();
        Data2 *pData = (Data2 *)p;
        for (int i = 0; i < CYCLE; ++i) {
            pData->a++;
        }
        stopwatch.stop();
        assert(pData->a == CYCLE);
        printf("thread1: Modifying different same cache line takes time: %zu ns\n", stopwatch.elapsedTime());
    }
}

void thread2(void *p, bool isSame) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(1, &cpuset);

    eular::ElapsedTime stopwatch(ElapsedTimeType::NANOSECOND);

    if (isSame)
    {
        stopwatch.start();
        Data *pData = (Data *)p;
        for (int i = 0; i < CYCLE; ++i) {
            pData->b++;
        }
        stopwatch.stop();
        assert(pData->b == CYCLE);
        printf("thread2: Modifying the same cache line takes time: %zu ns\n", stopwatch.elapsedTime());
    }
    else
    {
        stopwatch.start();
        Data2 *pData = (Data2 *)p;
        for (int i = 0; i < CYCLE; ++i) {
            pData->b++;
        }
        stopwatch.stop();
        assert(pData->b == CYCLE);
        printf("thread2: Modifying different same cache line takes time: %zu ns\n", stopwatch.elapsedTime());
    }
}

/**
 * @brief 测试使用两个线程修改相同缓存行
 * 
 */
void test_modify_same_cache_line()
{
    Data sharedData;
    std::thread th1(thread1, &sharedData, true);
    std::thread th2(thread2, &sharedData, true);

    th1.join();
    th2.join();
}


/**
 * @brief 测试使用两个线程修改不同缓存行
 * 
 */
void test_modify_diff_cache_line()
{
    Data2 sharedData2;
    std::thread th1(thread1, &sharedData2, false);
    std::thread th2(thread2, &sharedData2, false);

    th1.join();
    th2.join();
}

int main(int argc, char **argv)
{
    // 测试结果:
    // 相同缓存行的运行时间远高于不同缓存行的运行时间, 大约3.7倍
    // thread1: Modifying the same cache line takes time: 2142166 ns
    // thread2: Modifying the same cache line takes time: 2110315 ns
    // thread1: Modifying different same cache line takes time: 575807 ns
    // thread2: Modifying different same cache line takes time: 670728 ns

    test_modify_same_cache_line();
    test_modify_diff_cache_line();

    return 0;
}
