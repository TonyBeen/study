/*************************************************************************
    > File Name: test_jemalloc_perload.cc
    > Author: hsz
    > Brief: 测试预加载替换 jemalloc
    > Created Time: 2024年10月28日 星期一 16时45分17秒
 ************************************************************************/

#include <cstdlib>
#include <stdio.h>
#include <string.h>
#include <iostream>

// 当使用 export LD_PRELOAD= 预加载jemalloc库时
// malloc 将会被 jemalloc的符号替换, 做到无需修改源代码即可使用jemalloc

/**
 * @brief 原理
 * 
 * 加载顺序
 * 1、可执行程序
 * 2、LD_PRELOAD 库
 * 3、其他库，根据其在链接时的顺序
 * 
 * 当多个库中有相同符号时, 动态链接器会选择第一个加载的符号. 故使用 LD_PRELOAD 可以替换标准库的 malloc
 */

int main(int argc, char **argv)
{
    size_t size = 1024;
    void* ptr = malloc(size);
    if (!ptr) {
        std::cerr << "Memory allocation failed" << std::endl;
        return 1;
    }

    memset(ptr, 0, size);

    free(ptr);
    return 0;
}
