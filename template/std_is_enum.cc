/*************************************************************************
    > File Name: std_is_enum.cc
    > Author: hsz
    > Brief:
    > Created Time: 2024年05月21日 星期二 14时58分55秒
 ************************************************************************/

#include <stdio.h>
#include <stdint.h>

#include <iostream>
#include <type_traits>

#include "clazz.h"

int main(int argc, char **argv)
{
    static_assert(std::is_enum<Type>::value);
    static_assert(std::is_enum<ClassType>::value);
    static_assert(std::is_enum<TypeInt>::value);
    static_assert(std::is_enum<ClassTypeInt>::value);

    return 0;
}
