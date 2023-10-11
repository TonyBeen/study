/*************************************************************************
    > File Name: 获取函数参数个数.cc
    > Author: hsz
    > Brief:
    > Created Time: Tue 19 Sep 2023 09:23:36 AM CST
 ************************************************************************/

#include <iostream>
using namespace std;


template<typename... Args>
struct FunctionTraits
{
    static const std::size_t arity = sizeof...(Args);
};

template<typename R, typename... Args>
struct FunctionTraits<R(Args...)> : public FunctionTraits<Args...>
{
    using ReturnType = R;
};

template<typename R, typename... Args>
struct FunctionTraits<R(*)(Args...)> : public FunctionTraits<R(Args...)>
{};

template<typename R, typename C, typename... Args>
struct FunctionTraits<R(C::*)(Args...)> : public FunctionTraits<R(Args...)>
{};

template<typename R, typename C, typename... Args>
struct FunctionTraits<R(C::*)(Args...) const> : public FunctionTraits<R(Args...)>
{};

class FunctionTest
{
private:
    
public:
    void get() {}
    void set(int32_t) {}
    void set2(int32_t, double) {}

    static void Get() {}
    static void Set(int32_t) {}
};

int main(int argc, char **argv)
{
    std::cout << FunctionTraits<decltype(main)>::arity << std::endl;
    std::cout << FunctionTraits<decltype(&FunctionTest::get)>::arity << std::endl;
    std::cout << FunctionTraits<decltype(&FunctionTest::set)>::arity << std::endl;
    std::cout << FunctionTraits<decltype(&FunctionTest::set2)>::arity << std::endl;
    std::cout << FunctionTraits<decltype(&FunctionTest::Get)>::arity << std::endl;
    std::cout << FunctionTraits<decltype(&FunctionTest::Set)>::arity << std::endl;
    return 0;
}
