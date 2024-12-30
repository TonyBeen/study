/*************************************************************************
    > File Name: test_co.cc
    > Author: hsz
    > Brief:
    > Created Time: 2024年12月30日 星期一 14时16分31秒
 ************************************************************************/

#include <inttypes.h>
#include <stdio.h>
#include <unistd.h>

#include "coroutine.h"

int main(int argc, char **argv)
{
    // 创建主协程
    printf("main Coroutine id = %" PRIu64 "\n", eular::Coroutine::GetThis()->fiberId());

    eular::CoSharedStack stack(128 * 1024);
    uint32_t count = 3;
    eular::Coroutine::SP fiber_1 = std::make_shared<eular::Coroutine>([count] () {
        for (uint32_t i = 0; i < count; ++i) {
            printf("Coroutine(%" PRIu64 ")\n", eular::Coroutine::GetThis()->fiberId());
            eular::Coroutine::Yeild2Hold();
        }
    }, stack);

    eular::Coroutine::SP fiber_2 = std::make_shared<eular::Coroutine>([count] () {
        for (uint32_t i = 0; i < count; ++i) {
            printf("Coroutine(%" PRIu64 ")\n", eular::Coroutine::GetThis()->fiberId());
            eular::Coroutine::Yeild2Hold();
        }
    }, stack);

    for (size_t i = 0; i < count; i++) {
        fiber_1->resume();
        fiber_2->resume();
        sleep(1);
    }

    // 结束回调函数体, 否则增加fiber的引用
    fiber_1->resume();
    fiber_2->resume();

    printf("fiber_1 use_count = %" PRId64 "\n", fiber_1.use_count());
    printf("fiber_2 use_count = %" PRId64 "\n", fiber_2.use_count());

    return 0;
}
