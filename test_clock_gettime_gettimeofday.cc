#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

void test_gettimeofday()
{
    struct  timeval    tv;
    struct  timezone   tz;
    gettimeofday(&tv,&tz);

    printf("tv_sec: %ld\n", tv.tv_sec);
    printf("tv_usec: %ld\n", tv.tv_usec);
    printf("tz_minuteswest: %d\n", tz.tz_minuteswest);
    printf("tz_dsttime: %d\n", tz.tz_dsttime);
}

void test_clock_gettime(int32_t type = CLOCK_MONOTONIC)
{
    struct timespec begin;
    struct timespec end;

    const char *strType = nullptr;
    switch (type) {
    case CLOCK_REALTIME:
        strType = "CLOCK_REALTIME";
        break;
    case CLOCK_MONOTONIC:
        strType = "CLOCK_MONOTONIC";
        break;
    case CLOCK_PROCESS_CPUTIME_ID:
        strType = "CLOCK_PROCESS_CPUTIME_ID";
        break;
    case CLOCK_THREAD_CPUTIME_ID:
        strType = "CLOCK_THREAD_CPUTIME_ID";
        break;
    case CLOCK_MONOTONIC_RAW:
        strType = "CLOCK_MONOTONIC_RAW";
        break;
    case CLOCK_REALTIME_COARSE:
        strType = "CLOCK_REALTIME_COARSE";
        break;
    case CLOCK_MONOTONIC_COARSE:
        strType = "CLOCK_MONOTONIC_COARSE";
        break;
    case CLOCK_BOOTTIME:
        strType = "CLOCK_BOOTTIME";
        break;
    case CLOCK_REALTIME_ALARM:
        strType = "CLOCK_REALTIME_ALARM";
        break;
    case CLOCK_BOOTTIME_ALARM:
        strType = "CLOCK_BOOTTIME_ALARM";
        break;
    case CLOCK_TAI:
        strType = "CLOCK_TAI";
        break;
    default:
        return;
    }

    int32_t cycle = 500;

    clock_gettime(CLOCK_MONOTONIC_RAW, &begin);
    for (int i = 0; i < cycle; ++i) {
        struct timespec ts;
        clock_gettime(type, &ts);
    }
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);

    uint64_t beginNS = begin.tv_sec * 1000 * 1000 * 1000 + begin.tv_nsec;
    uint64_t endNS = end.tv_sec * 1000 * 1000 * 1000 + end.tv_nsec;

    // 经多次测试, 获取一次CLOCK_MONOTONIC大概耗时60 - 65纳秒, 普遍是62,63
    printf("clock_gettime(%s) consume %ldns\n", strType, (endNS - beginNS) / cycle);
}

int main()
{
    test_clock_gettime();
    return 0;
}