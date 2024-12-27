/*************************************************************************
    > File Name: bengch.cc
    > Author: hsz
    > Brief:
    > Created Time: 2024年12月27日 星期五 11时47分01秒
 ************************************************************************/

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <vector>
#include <memory>

#include <utils/elapsed_time.h>

#include "aco.h"

#define COROUTINE_LOOP_COUNT    30000
#define FIBER_COUNT             100

struct task
{
    task() {
        // printf("%s\n", __PRETTY_FUNCTION__);
    }
    ~task() {
        // printf("%s\n", __PRETTY_FUNCTION__);
    }
};


void coroutine()
{
    // NOTE aco_exit会调用aco_yield, 导致ptr无法引用计数减1
    {
        std::shared_ptr<task> ptr = std::make_shared<task>();
        for (int32_t i = 0; i < COROUTINE_LOOP_COUNT; ++i) {
            aco_yield();
        }
    }

    aco_exit();
}

int main(int argc, char **argv)
{
    signal(SIGINT, [] (int32_t sig) {
        printf("SIGINT catch. %p\n", aco_gtls_co);
    });

    aco_thread_init(NULL);
    aco_t* main_co = aco_create(NULL, NULL, 0, NULL, NULL);
    aco_share_stack_t* sstk = aco_share_stack_new(0);

    std::vector<std::shared_ptr<aco_t>> co_vec(FIBER_COUNT, nullptr);
    // construct
    for (int32_t i = 0; i < FIBER_COUNT; ++i) {
        std::shared_ptr<aco_t> ptr(aco_create(main_co, sstk, 0, coroutine, nullptr), [] (aco_t *co) {
            aco_destroy(co);
        });
        co_vec[i] = ptr;
    }

    // resume
    eular::ElapsedTime elapsed(eular::ElapsedTimeType::MILLISECOND);
    elapsed.start();
    for (;;) {
        for (int32_t i = 0; i < FIBER_COUNT; ++i) {
            aco_resume(co_vec[i].get());
        }

        if (co_vec[0]->is_end) {
            break;
        }
    }
    elapsed.stop();

    auto diff_time = elapsed.elapsedTime();
    printf("%d context switch in %" PRIu64 " ms\n", COROUTINE_LOOP_COUNT * FIBER_COUNT, diff_time);
    printf("one context switch in %.3f ns\n", (diff_time * 1000000) / (COROUTINE_LOOP_COUNT * FIBER_COUNT * 1.0));
    printf("%.3f resume/yield pair per millisecond\n", COROUTINE_LOOP_COUNT * FIBER_COUNT * 1.0 / diff_time);

    aco_share_stack_destroy(sstk);
    return 0;
}
