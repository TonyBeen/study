/*************************************************************************
    > File Name: std_is_fundamental.cc
    > Author: hsz
    > Brief:
    > Created Time: 2024年05月21日 星期二 14时50分32秒
 ************************************************************************/

#include <stdio.h>
#include <stdint.h>

#include <iostream>
#include <type_traits>

#include "clazz.h"

int main(int argc, char **argv)
{
    // 普通数据类型
    static_assert(std::is_fundamental<char>::value);
    static_assert(std::is_fundamental<unsigned char>::value);

    static_assert(std::is_fundamental<short>::value);
    static_assert(std::is_fundamental<wchar_t>::value);
    static_assert(std::is_fundamental<unsigned short>::value);

    static_assert(std::is_fundamental<int>::value);
    static_assert(std::is_fundamental<unsigned int>::value);

    static_assert(std::is_fundamental<long>::value);
    static_assert(std::is_fundamental<long long>::value);
    static_assert(std::is_fundamental<unsigned long>::value);
    static_assert(std::is_fundamental<unsigned long long>::value);

    static_assert(std::is_fundamental<float>::value);
    static_assert(std::is_fundamental<double>::value);
    static_assert(std::is_fundamental<long double>::value); // 64位下占 16字节, 32位下占 12字节

    // 枚举类型
    static_assert(std::is_fundamental<Type>::value == false);
    static_assert(std::is_fundamental<ClassType>::value == false);
    static_assert(std::is_fundamental<TypeInt>::value == false);
    static_assert(std::is_fundamental<ClassTypeInt>::value == false);

    // 结构体 类 联合体
    static_assert(std::is_fundamental<Class>::value == false);
    static_assert(std::is_fundamental<Struct>::value == false);
    static_assert(std::is_fundamental<Union>::value == false);

    return 0;
}
