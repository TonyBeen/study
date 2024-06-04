/*************************************************************************
    > File Name: std_enable_if.cc
    > Author: hsz
    > Brief:
    > Created Time: 2024年05月21日 星期二 15时28分45秒
 ************************************************************************/

#include <stdio.h>
#include <stdint.h>

#include <iostream>
#include <type_traits>

#include "clazz.h"

template <typename T>
typename std::enable_if<std::is_integral<T>::value, T>::type
myFunction(T value) {
    // 函数实现
    return value;
}

int main(int argc, char **argv)
{
    int32_t n = myFunction(100);
    ClassType type;
    // myFunction(type); // 编译失败, 只允许整形类型

    return 0;
}
