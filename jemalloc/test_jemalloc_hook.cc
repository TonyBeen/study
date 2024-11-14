/*************************************************************************
    > File Name: test_jemalloc_hook.cc
    > Author: hsz
    > Brief: 测试通过 hook 替换标准库的 malloc
    > Created Time: 2024年10月28日 星期一 17时04分58秒
 ************************************************************************/

// g++ test_jemalloc_hook.cc -std=c++11 -ljemalloc -pthread -ldl

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <string>

#include <signal.h>

#include <unistd.h>
#include <dlfcn.h>

#include <jemalloc/jemalloc.h>

// 这种情况需要将 malloc/calloc 等全都 hook, 假设没有 hook realloc, 第三方库使用此函数申请的内存使用free会出现段错误
// c++ 重载 operator new/operator delete 一般是成对的, 默认的 new/delete 底层使用 malloc 无需考虑

struct ReplaceMallocFree
{
    using SelfMalloc = void *(*)(size_t);
    using SelfFree = void (*)(void *);

    ReplaceMallocFree()
    {
        m_malloc = (SelfMalloc)dlsym(RTLD_NEXT, "malloc");
        m_free = (SelfFree)dlsym(RTLD_NEXT, "free");

        assert(m_malloc && m_free);

        printf("malloc = %p, free = %p\n", m_malloc, m_free);
    }

    SelfMalloc  m_malloc;
    SelfFree    m_free;
};

ReplaceMallocFree g_replaceMallocFree;

#ifdef __cplusplus
extern "C" {
#endif

void* malloc(size_t size)
{
    // printf 有调用malloc行为, 会导致无限递归出现段错误
    write(STDOUT_FILENO, "---> malloc\n", 12);
    return je_malloc(size);
}

void free(void *ptr)
{
    write(STDOUT_FILENO, "---> free\n", 12);
    je_free(ptr);
}

#ifdef __cplusplus
}
#endif


void catch_sig(int32_t sig)
{
    if (sig == SIGABRT) {
        printf("\nSUCCESS\n");
    }

    exit(0);
}

int main()
{
    // 标准库的 free 检测到异常会调用 abort
    signal(SIGABRT, catch_sig);

    size_t size = 1024;
    void* ptr = malloc(size);
    if (!ptr) {
        printf("Memory allocation failed\n");
        return 1;
    }

    memset(ptr, 0, size);

    uint32_t *pp = new uint32_t;
    free(pp);

    printf("-------------\n");

    g_replaceMallocFree.m_free(ptr);
    return 0;
}
