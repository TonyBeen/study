/*************************************************************************
    > File Name: 自动推导.cpp
    > Author: hsz
    > Brief:
    > Created Time: Tue 26 Jul 2022 04:54:54 PM CST
 ************************************************************************/

#include <iostream>
#include <tuple>
using namespace std;

#define HAS_MEMBER(XXX) \
template<typename T, typename... Args>\
struct has_member_##XXX \
{ \
private:  \
    template<typename U> \
    static auto Check(int) -> decltype(std::declval<U>().XXX(std::declval<Args>()...), std::true_type());  \
    template<typename U> \
    static std::false_type Check(...); \
public: \
    static constexpr auto value = decltype(Check<T>(0))::value; \
}

class foo
{
public:
    foo() {}
    ~foo() {}
    void test() {}
    void test(int32_t) {}
    int32_t test(int32_t, double) { return 0; }
};

class foo2
{
public:
    foo2() {}
    ~foo2() {}

    int32_t test(int, const char *) { return 0; }
};

// 参考https://zh.cppreference.com/w/cpp/language/decltype
template<typename T, typename R, typename... Args>
struct has_member_test
{
private:
    template<typename U>
    // 指针后面是返回值的自动类型推导。decltype加逗号表达式，返回最后一个参数的类型即std::true_type
    static auto Check(int) -> decltype(std::declval<U>().test(std::declval<Args>()...),
        std::is_same<R, decltype(std::declval<U>().test(std::declval<Args>()...))>::value,
        std::true_type()) {}

    template<typename U>
    static std::false_type Check(...) {}


public:
    // 这个value是std::false_type或std::true_type的value
    static constexpr auto value = decltype(Check<T>(0))::value;
};

int main(int argc, char **argv)
{
    std::cout << "has test in foo " << has_member_test<foo, void, std::iostream>::value << std::endl;
    std::cout << "has test in foo2 " << has_member_test<foo2, int, int, const char *>::value << std::endl;
    std::cout << "has test in foo " << has_member_test<foo, void, double>::value << std::endl;

    static_assert(std::is_same<std::tuple<double>, std::tuple<int>>::value == false);
    return 0;
}
