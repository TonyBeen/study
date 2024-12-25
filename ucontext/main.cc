/*************************************************************************
    > File Name: main.cc
    > Author: hsz
    > Brief: g++ main.cc fiber.cpp -o main.out -llog -pthread -g
    > Created Time: 2024年12月25日 星期三 17时46分27秒
 ************************************************************************/

#include <inttypes.h>
#include <stdio.h>
#include <unistd.h>

#include "fiber.h"

void func()
{
    // 创建主协程
    eular::Fiber::GetThis();

    eular::Fiber::sp fiber = std::make_shared<eular::Fiber>([] () {
        while (true) {
            printf("%s begin\n", __func__);
            printf("%s end\n", __func__);
            eular::Fiber::Yeild2Hold();
        }
    }, 128 * 1024);

    eular::Fiber::sp temp = std::make_shared<eular::Fiber>([fiber] () {
        while (true) {
            printf("-------------%s begin\n", __func__);
            printf("-------------%s end\n", __func__);
            eular::Fiber::Yeild2Hold();
        }
    }, 128 * 1024);

    for (size_t i = 0; i < 5; i++) {
        fiber->Resume();
        temp->Resume();
        sleep(1);
    }
}

void func_2()
{
    printf("fiber(%" PRIu64 ") will hold\n", eular::Fiber::GetThis()->FiberId());
    eular::Fiber::Yeild2Hold();
    printf("fiber(%" PRIu64 ") exit\n", eular::Fiber::GetThis()->FiberId());
}

int main(int argc, char **argv)
{
    func();

    eular::Fiber::sp fiber = std::make_shared<eular::Fiber>(func_2, 128 * 1024);
    printf("resume fiber(%" PRIu64 ")\n", fiber->FiberId());
    fiber->Resume();
    fiber->Resume();

    return 0;
}
