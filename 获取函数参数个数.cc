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
    static const std::size_t size = sizeof...(Args);
    static_assert(size >= 0, "");
};

template<typename R, typename... Args>
struct FunctionTraits<R(Args...)> : public FunctionTraits<Args...>
{};

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
    void get() const {}
    void set(int32_t) {}
    void set2(int32_t, double) {}

    static void Get() {}
    static void Set(int32_t) {}
};

int main(int argc, char **argv)
{
    std::cout << FunctionTraits<decltype(&main)>::size << std::endl;
    std::cout << FunctionTraits<decltype(&FunctionTest::get)>::size << std::endl;
    std::cout << FunctionTraits<decltype(&FunctionTest::set)>::size << std::endl;
    std::cout << FunctionTraits<decltype(&FunctionTest::set2)>::size << std::endl;
    std::cout << FunctionTraits<decltype(&FunctionTest::Get)>::size << std::endl;
    std::cout << FunctionTraits<decltype(&FunctionTest::Set)>::size << std::endl;
    return 0;
}
