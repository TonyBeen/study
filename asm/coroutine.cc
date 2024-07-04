/*************************************************************************
    > File Name: coroutine.cc
    > Author: hsz
    > Brief:
    > Created Time: 2024年06月21日 星期五 11时28分59秒
 ************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define STACK_SIZE (64 * 1024) // <-- caution !!!

uint64_t world(uint64_t num) {
  printf("hello from world, %ld\t", num);
  return 42;
}

uint64_t hello(uint64_t num) { return world(num); }

int main() {
  uint64_t num = 5, res = 0;

  char *stack = (char *)malloc(STACK_SIZE); // 分配调用栈 !!!
  char *sp = stack + STACK_SIZE;

  asm volatile("movq    %%rcx, 0(%1)\n\t"
               "movq    %1, %%rsp\n\t"
               "movq    %3, %%rdi\n\t"
               "call    *%2\n\t"
               : "=r"(res)                /* output */
               : "b"((uintptr_t)sp - 16), /* input  */
                 "d"((uintptr_t)hello),
                 "a"((uintptr_t)num));

  asm volatile("movq    0(%%rsp), %%rcx" : :);

  printf("num = %ld, res = %ld\n", num, res);
  return 0;
}
// hello from world, 5        num = 5, res = 42
