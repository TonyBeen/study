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

// fiber 1和2循环执行
void func()
{
    // 创建主协程
    printf("main fiber id = %" PRIu64 "\n", eular::Fiber::GetThis()->FiberId());

    uint32_t count = 3;
    eular::Fiber::sp fiber_1 = std::make_shared<eular::Fiber>([count] () {
        for (uint32_t i = 0; i < count; ++i) {
            printf("fiber(%" PRIu64 ")\n", eular::Fiber::GetThis()->FiberId());
            eular::Fiber::Yeild2Hold();
        }
    }, 128 * 1024);

    eular::Fiber::sp fiber_2 = std::make_shared<eular::Fiber>([count] () {
        for (uint32_t i = 0; i < count; ++i) {
            printf("fiber(%" PRIu64 ")\n", eular::Fiber::GetThis()->FiberId());
            eular::Fiber::Yeild2Hold();
        }
    }, 128 * 1024);

    for (size_t i = 0; i < count; i++) {
        fiber_1->Resume();
        fiber_2->Resume();
        sleep(1);
    }

    // 结束回调函数体, 否则增加fiber的引用
    fiber_1->Resume();
    fiber_2->Resume();

    printf("fiber_1 use_count = %" PRId64 "\n", fiber_1.use_count());
    printf("fiber_2 use_count = %" PRId64 "\n", fiber_2.use_count());
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
