/*************************************************************************
    > File Name: test_libaco.cc
    > Author: hsz
    > Brief:
    > Created Time: 2024年06月17日 星期一 09时52分39秒
 ************************************************************************/

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

#include <assert.h>

#include "aco.h"


// 经测试 libaco不支持在协程内部创建协程并执行,
// 故libaco属于非对称协程模型
aco_share_stack_t* sstk = nullptr;

void foo()
{
    printf("co: %p: entry foo()\n", aco_get_co());
    aco_exit();
}

void co_fp0()
{
    aco_t* main_co = static_cast<aco_t *>(aco_get_arg());
    printf("co: %p: entry: %p\n", aco_get_co(), main_co);
    aco_t *co = aco_create(main_co, sstk, 0, foo, nullptr);
    aco_resume(co);
    printf("co: %p:  exit to main_co: %p\n", aco_get_co(), main_co);
    aco_exit();
}

int main(int argc, char **argv)
{
    aco_thread_init(NULL);
    aco_t* main_co = aco_create(NULL, NULL, 0, NULL, NULL);
    sstk = aco_share_stack_new(0);
    aco_t* co = aco_create(main_co, sstk, 0, co_fp0, main_co);
    int ct = 0;

    aco_resume(co);

    printf("main_co: yield to co: %p: %d\n", co, ct);
    aco_resume(co);

    printf("main_co: destroy and exit\n");
    aco_destroy(co);
    co = NULL;
    aco_share_stack_destroy(sstk);
    sstk = NULL;
    aco_destroy(main_co);
    main_co = NULL;

    return 0;
}
