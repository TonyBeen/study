/*************************************************************************
    > File Name: sched_yeild.cc
    > Author: hsz
    > Brief:
    > Created Time: Wed 20 Dec 2023 09:48:08 AM CST
 ************************************************************************/

#include <iostream>
#include <unistd.h>
#include <sched.h>

void test_sched_yeild()
{
    struct timespec begin, end;
    clock_gettime(CLOCK_MONOTONIC, &begin);
    int recyle = 50;
    for (int i = 0; i < recyle; ++i) {
        sched_yield();
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    printf("sched_yield consume %ldns\n", (end.tv_nsec - begin.tv_nsec) / recyle);
}

int main(int argc, char **argv)
{
    test_sched_yeild();

    return 0;
}
