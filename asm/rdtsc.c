/*************************************************************************
    > File Name: rdtsc.c
    > Author: hsz
    > Brief:
    > Created Time: Wed 29 Dec 2021 02:28:35 PM CST
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <endian.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>

/*
    https://cloud.tencent.com/developer/article/1033254?from=15425

    RDTSC指令，意思是读取时间标记计数器(Read Time-Stamp Counter)，
    Time-stamp counter 是处理器内部的一个64位的MSR (model specific register)，
    处理器每时钟周期递增时间标签计数器 MSR 一次，在处理器复位时将它重设为 0。
    RDTSC 指令把 TSC的值低32位装入EAX中，高32位装入EDX中。
    如果CPU的主频是200MHz，那么在一秒钟内，TSC的值增加 200,000,000 次。
    所以在计算的时候，把两次的TSC差值除以两次的时间差值就是CPU的主频。

    1、多核时代不宜再用 x86 的 RDTSC 指令测试指令周期和时间
    http://blog.csdn.net/solstice/article/details/5196544

    2、RDTSC命令详解
    http://blog.csdn.net/tbwood/article/details/5536597
*/

/**
 * 但是在多核时代，RDTSC 指令的准确度大大削弱了，原因有如下几点： 
 * 1. 不能保证同一块主板上每个核的 CPU 时钟周期数（Time Stamp Counter）是同步的； 
 * 2. CPU 的时钟频率可能变化，例如笔记本电脑的节能功能； 
 * 3. 乱序执行导致 RDTSC 测得的周期数不准。 虽然 RDTSC 废掉了，高精度计时还是有办法的，
 * 	  在 Windows 上用 QueryPerformanceCounter 和 QueryPerformanceFrequency，
 * 	  Linux 上用 POSIX 的 clock_gettime 函数，以 CLOCK_MONOTONIC 参数调用。
 */

static inline unsigned long long rte_rdtsc(void)
{
	union {
		unsigned long long tsc_64;
        #if BYTE_ORDER == LITTLE_ENDIAN
        struct {
			unsigned lo_32;
			unsigned hi_32;
		};
        #elif BYTE_ORDER == BIG_ENDIAN
        struct {
			unsigned hi_32;
			unsigned lo_32;
		};
        #endif
	} tsc;

	asm volatile("rdtsc" :
			"=a" (tsc.lo_32),
			"=d" (tsc.hi_32));
	return tsc.tsc_64;
}

static unsigned long long __one_msec;
static unsigned long long __one_sec;
static unsigned long long __metric_diff = 0;

void set_time_metric(int ms)
{
	unsigned long long now, startup, end;
	unsigned long long begin = rte_rdtsc();
	usleep(ms * 1000);
	end        = rte_rdtsc();
	__one_msec = (end - begin) / ms;
	__one_sec  = __one_msec * 1000;     // 获取CPU频率

	startup    = rte_rdtsc();           // 获取当前cpu时间戳
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
	now        = tp.tv_sec * __one_sec; // 获取系统绝对时间
    // now = time(NULL) * __one_sec;    // 获取系统实时时间
	if (now > startup) {
		__metric_diff = now - startup;
	} else {
		__metric_diff = 0;
	}
}

static pthread_once_t __once_control2 = PTHREAD_ONCE_INIT;

static void time_metric_once(void)
{
	set_time_metric(1000);
}

int acl_fiber_gettimeofday(struct timeval *tv)
{
	unsigned long long now;

	if (__metric_diff == 0) {
		if (pthread_once(&__once_control2, time_metric_once) != 0) {
			abort();
		}
	}
    printf("__once_control2 = %d\n", __once_control2);

	now = rte_rdtsc() + __metric_diff;
    printf("now = %lld, rdtsc = %lld\n", now, now - __metric_diff);
	tv->tv_sec  = now / __one_sec;
	tv->tv_usec = (1000 * (now % __one_sec) / __one_msec);
	return 0;
}

int main(int argc, char **argv)
{
    struct timeval tv;
    acl_fiber_gettimeofday(&tv);
    printf("__one_msec = %lld, __one_sec = %lld, __metric_diff = %lld\n",
        __one_msec, __one_sec, __metric_diff);

    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    printf("rte_rdtsc retrun time = %ld.%ld, clock_gettime CLOCK_MONOTONIC return time = %ld.%ld\n",
        tv.tv_sec, tv.tv_usec, tp.tv_sec, tp.tv_nsec);
    assert(tv.tv_sec == tp.tv_sec);
    return 0;
}
