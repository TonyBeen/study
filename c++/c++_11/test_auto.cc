/*************************************************************************
    > File Name: test_auto.cc
    > Author: hsz
    > Brief:
    > Created Time: Mon 20 Feb 2023 10:07:53 AM CST
 ************************************************************************/

#include <iostream>
#include <functional>

using namespace std;

auto func1() -> int
{
    return 10;
}

auto func2() -> std::function<void()>
{
    auto lambda = []() { };
    return lambda;
}

int main(int argc, char **argv)
{
    {
        #define TYPE_INFO(v) std::cout << typeid((v)).name() << std::endl
        auto a = {1}; // std::initializer_list
        TYPE_INFO(a);
        auto b{2};   // int
        TYPE_INFO(b);
        // auto c{1, 2}; // error. direct-list-initialization of ‘auto’ requires exactly one element [-fpermissive]
        // TYPE_INFO(c);
    }
    return 0;
}
