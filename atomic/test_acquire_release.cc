/*************************************************************************
    > File Name: test_acquire_release.cc
    > Author: hsz
    > Brief: 测试会不会重排, 导致线程死循环
    > Created Time: 2024年09月20日 星期五 10时36分54秒
 ************************************************************************/

#include <iostream>
#include <atomic>
#include <thread>

std::atomic<int> A{0};
std::atomic<int> B{0};
std::atomic<bool> flag{false}; 

void thread1() {
    while (!flag.load()) {
    }

    // 如果A被重排到B load之后, 则会导致线程2卡死
    A.store(1, std::memory_order_release);

    while (B.load(std::memory_order_acquire) == 0) {
    }
}

void thread2() {
    while (!flag.load()) {
    }

    while (A.load(std::memory_order_acquire) == 0) {
    }

    B.store(1, std::memory_order_release);
}

int main(int argc, char **argv)
{
    std::thread th1(thread1);
    std::thread th2(thread2);

    flag.store(true);

    th1.join();
    th2.join();
    return 0;
}
