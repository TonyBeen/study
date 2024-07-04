/*************************************************************************
    > File Name: ucontext.cpp
    > Author: hsz
    > Mail:
    > Created Time: Sun 19 Sep 2021 11:01:29 AM CST
 ************************************************************************/

#include <iostream>
#include <ucontext.h>
using namespace std;

void  test()
{
    cout << __func__ << endl;
}

void test02(void *ucp)
{
    cout << __func__ << endl;
}

typedef void (*user_context_t)();


static ucontext_t tmp;
void ContextEntry()
{
    cout << __func__ << endl;
}

static ucontext_t mainContext;
static char mainStack[1024] = {0};

int main(int argc, char **argv)
{
    getcontext(&mainContext);
    cout << "main context" << endl;
    mainContext.uc_stack.ss_sp = mainStack;
    mainContext.uc_stack.ss_size = 1024;
    mainContext.uc_stack.ss_flags = 0;
    mainContext.uc_link = nullptr; //&tmp;
    makecontext(&mainContext, ContextEntry, 0);
    test();
    ucontext_t ucp;
    getcontext(&ucp);
    char stack[1024] = {0};
    ucp.uc_stack.ss_sp = stack;
    ucp.uc_stack.ss_size = 1024;
    ucp.uc_stack.ss_flags = 0;
    ucp.uc_link = &mainContext;
    makecontext(&ucp, (user_context_t)test02, 1, nullptr);
    test();
    // ucp未设置uc_link时, 从setcontext处退出线程，不执行test()
    // 当为ucp设置了uc_link后，如果函数执行的是死循环，则此处线程将会陷进死循环，不会跳出，故也不会执行test()
    // 当对mainContext不使用makecontext时，则会陷入test test test02循环，由此可得出getcontext获取的是
    // getcontext其下的栈空间环境和所需的寄存器
    // setcontext(&ucp);

    // int swapcontext(ucontext_t *oucp, const ucontext_t *ucp);
    /**
     * 1、保存当前栈至oucp
     * 2、如果ucp经makecontext, 执行ucp的func
     * 3、执行ucp.uc_link上下文，为空则退出线程
     * 4、不为空，执行func，uc_link，直到uc_link为空退出线程
     */

    // 将mainContext.uc_link赋值tmp时，会执行70行test
    swapcontext(&tmp, &ucp);
    test();
    return 0;
}
