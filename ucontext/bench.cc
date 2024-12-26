/*************************************************************************
    > File Name: bench.cc
    > Author: hsz
    > Brief: g++ bench.cc fiber.cpp -o bench.out -llog -lutils -pthread -O2 -g
    > Created Time: 2024年12月26日 星期四 15时48分44秒
 ************************************************************************/

#include <stdio.h>
#include <inttypes.h>
#include <vector>

#include <signal.h>
#include <unistd.h>
#include <utils/elapsed_time.h>
#include <log/log.h>

#include "fiber.h"

#define COROUTINE_LOOP_COUNT    300
#define FIBER_COUNT             10000

void coroutine()
{
    for (int32_t i = 0; i < COROUTINE_LOOP_COUNT; ++i) {
        eular::Fiber::Yeild2Hold();
    }
}

int main(int argc, char **argv)
{
    // ucontext在触发信号后可恢复执行, libconcurrent会产生状态异常
    signal(SIGINT, [] (int32_t sig) {
        printf("SIGINT catch. %p\n", eular::Fiber::GetThis().get());
    });
    eular::log::SetLevel(eular::LogLevel::LEVEL_WARN);
    eular::Fiber::sp main_fiber = eular::Fiber::GetThis();
    std::vector<eular::Fiber::sp> fiber_vec(FIBER_COUNT, nullptr);

    // construct
    for (int32_t i = 0; i < FIBER_COUNT; ++i) {
        fiber_vec[i] = std::move(std::make_shared<eular::Fiber>(coroutine, 1024));
    }

    // resume
    eular::ElapsedTime elapsed(eular::ElapsedTimeType::MILLISECOND);
    elapsed.start();
    for (;;) {
        for (int32_t i = 0; i < FIBER_COUNT; ++i) {
            fiber_vec[i]->Resume();
        }

        if (fiber_vec[0]->State() == eular::Fiber::FiberState::TERM) {
            break;
        }
    }
    elapsed.stop();

    auto diff_time = elapsed.elapsedTime();
    printf("%d context switch in %" PRIu64 " ms\n", COROUTINE_LOOP_COUNT * FIBER_COUNT, diff_time);
    printf("one context switch in %.3f ns\n", (diff_time * 1000000) / (COROUTINE_LOOP_COUNT * FIBER_COUNT * 1.0));
    printf("%.3f resume/yield pair per second\n", COROUTINE_LOOP_COUNT * FIBER_COUNT * 1.0 / (diff_time / 1000));

    return 0;
}
