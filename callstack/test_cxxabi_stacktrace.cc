/*************************************************************************
    > File Name: test_cxxabi_stacktrace.cc
    > Author: hsz
    > Brief: g++ test_cxxabi_stacktrace.cc -o test_cxxabi_stacktrace -rdynamic -std=c++11 -O0
    > Created Time: Sun 10 Dec 2023 04:00:36 PM CST
 ************************************************************************/

#include <iostream>
#include "cxxabi_stacktrace.hpp"

void Func5()
{
    auto frames = detail::stacktrace(0, 0);
    for (const auto &it : frames)
    {
        std::cout << it << std::endl;
    }
}

void Func4() { Func5(); }
void Func3() { Func4(); }
void Func2() { Func3(); }
void Func1() { Func2(); }

int main()
{
    Func1();
    return 0;
}
