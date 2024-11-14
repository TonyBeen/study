/*************************************************************************
    > File Name: test_jemelloc.cc
    > Author: hsz
    > Brief: 测试直接使用 jemalloc
    > Created Time: 2024年10月28日 星期一 11时57分05秒
 ************************************************************************/

#include <cstdlib>
#include <stdio.h>
#include <string.h>
#include <iostream>

#include <jemalloc/jemalloc.h>

// 主动链接jemalloc时需要在编译时带上 --with-jemalloc-prefix=je_
// 不带时编译出来的符号就是 malloc, 防止冲突, 带上后符号是 je_malloc

int main()
{
    size_t size = 1024;
    void* ptr = je_malloc(size);
    if (!ptr) {
        std::cerr << "Memory allocation failed" << std::endl;
        return 1;
    }

    memset(ptr, 0, size);

    ::atexit([](){
        uint64_t epoch = 1;
        size_t sz = sizeof(epoch);
        je_mallctl("epoch", &epoch, &sz, &epoch, sz);

        size_t allocated, active, mapped;
        sz = sizeof(size_t);
        je_mallctl("stats.allocated", &allocated, &sz, NULL, 0);
        je_mallctl("stats.active", &active, &sz, NULL, 0);
        je_mallctl("stats.mapped", &mapped, &sz, NULL, 0);

        printf("allocated/active/mapped: %zu/%zu/%zu\n", allocated, active, mapped);
    });

    // 释放内存
    je_free(ptr);
    return 0;
}
