/*************************************************************************
    > File Name: test_memcpy.cc
    > Author: hsz
    > Brief:
    > Created Time: 2025年09月17日 星期三 14时31分57秒
 ************************************************************************/

#include <iostream>
#include <ctype.h>
#include <inttypes.h>
#include <utils/elapsed_time.h>

using namespace std;

int main(int argc, char **argv)
{
    const size_t size = 1024 * 1024;
    char *src = (char *)malloc(size);
    char *dst = (char *)malloc(size);

    {
        eular::ElapsedTime et(eular::ElapsedTimeType::MICROSECOND);
        et.start();
        for (size_t i = 0; i < 1; i++) {
            memcpy(dst, src, size);
        }
        et.stop();
        printf("Copying 1MB of memory at once costs %" PRIu64 " us\n", et.elapsedTime());
    }

    {
        eular::ElapsedTime et(eular::ElapsedTimeType::MICROSECOND);
        et.start();
        size_t cycle = 32;
        size_t size_per_cycle = size / cycle;
        for (size_t i = 0; i < cycle; i++) {
            memcpy(dst + i * size_per_cycle, src + i * size_per_cycle, size_per_cycle);
        }
        et.stop();
        printf("Copying 1MB of memory %zu times in a loop costs %" PRIu64 " us\n", cycle, et.elapsedTime());
    }

    free(src);
    free(dst);
    return 0;
}
