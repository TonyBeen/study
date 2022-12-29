/*************************************************************************
    > File Name: mutex.c
    > Author: hsz
    > Mail:
    > Created Time: Thu 09 Sep 2021 11:07:05 AM CST
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils/thread.h>
#include <utils/utils.h>
#include <linux/futex.h>
#include <sys/time.h>

#define atomic_or(P, V)     __sync_or_and_fetch((P), (V))       // p: 地址 V: 值，P指向的内容与V相或
#define atomic_and(P, V)    __sync_and_and_fetch((P), (V))
#define atomic_add(P, V)    __sync_add_and_fetch((P), (V))
#define atomic_load(P)      __sync_add_and_fetch((P), (0))
#define atomic_xadd(P, V)   __sync_fetch_and_add((P), (V))

// // P: 地址 O: 旧值 N: 新值; if (O == *P) *p = N;   返回旧值
// #define cmpxchg(P, O, N) __sync_val_compare_and_swap((P), (O), (N))

#define cpu_relax() asm volatile("rep; nop\n": : :"memory")

using namespace eular;

class mutex_t final
{
public:
    mutex_t()
    {
        init();
    }
    ~mutex_t()
    {

    }

    void lock()
    {
        if (mFutex.state != INITED) {
            return;
        }
        for (int i = 0; i < recyle; ++i) {
            if (cmpxchg(&mFutex.mtx, 0, 1) == 0) { // 加锁成功
                mFutex.owner = gettid();
                return;
            }
            cpu_relax();
        }
        // 用户空间加锁失败
        while(cmpxchg(&mFutex.mtx, 0, 1) != 0) {
            futex(&mFutex.mtx, FUTEX_WAIT_PRIVATE, 1);
            // 执行完futex线程是挂起状态
        }
    }

    void unlock()
    {
        if (mFutex.state != INITED) {
            return;
        }

        // 未调用lock直接调用unlock
        if (atomic_load(&mFutex.mtx) == 0) {
            return;
        }

        // 解锁
        cmpxchg(&mFutex.mtx, 1, 0);
        if (atomic_load(&mFutex.mtx) == 0) {    // 解锁成功或此时锁已被抢去
            // 等待用户态其他线程抢锁。抢到锁返回
            for (int i = 0; i < recyle * 2; ++i) {
                if (atomic_load(&mFutex.mtx) == 1) {
                    return;
                }
            }
            futex(&mFutex.mtx, FUTEX_WAKE_PRIVATE, 1);
        }
    }

private:
    void init()
    {
        mFutex.mtx = 0;
        mFutex.owner = 0;
        mFutex.state = INITED;
    }
    long futex(void *addr1, int futex_op, int val,
        const struct timespec *timeout = NULL,
        int *addr2 = NULL, int val3 = 0)
    {
        return syscall(__NR_futex, addr1, futex_op, val, timeout, addr2, val3);
    }
private:
    typedef struct _mutex_t {
        unsigned int    mtx;        // 锁
        unsigned int    owner;      // 此刻拥有锁的线程ID
        signed   int    state;      // 锁的状态
    } mutex;

    enum mtx_state {
        NOT_INIT = 0,
        INITED   = 1,
        DESTROY  = 2
    };
    mutex mFutex;
    const int recyle = 50;
};


int count = 0;
mutex_t mutex;

int threadFunc1(void *arg)
{
    while (1) {
        mutex.lock();
        if (count > 10000) {
            mutex.unlock();
            break;
        }
        printf("tid %ld --> count = %d\n", gettid(), ++count);
        msleep(5);
        mutex.unlock();
    }

    return count;
}

int threadFunc2(void *arg)
{
    while (1) {
        mutex.lock();
        if (count > 10000) {
            mutex.unlock();
            break;
        }
        printf("tid %ld --> count = %d\n", gettid(), ++count);
        msleep(5);
        mutex.unlock();
    }

    return count;
}

int main(int argc, char **argv)
{
    Thread thread1("test", threadFunc1);
    Thread thread2("test", threadFunc2);
    thread1.run();
    thread2.run();

    // int num = 0;
    // printf("cmpxchg(&num, 0, 1) -> ret = %d\n", cmpxchg(&num, 0, 1));
    // printf("atomic_load(&num) -> ret = %d\n", atomic_load(&num));
    // printf("cmpxchg(&num, 0, 1) -> ret = %d\n", cmpxchg(&num, 0, 1));
    printf("thread 1 status = %d\n", thread1.ThreadStatus());
    printf("thread 2 status = %d\n", thread2.ThreadStatus());
    sleep(5);
    thread1.Interrupt();
    thread2.Interrupt();
    return 0;
}
