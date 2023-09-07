/*************************************************************************
    > File Name: pretty_function.cc
    > Author: hsz
    > Brief:
    > Created Time: Thu 07 Sep 2023 04:46:56 PM CST
 ************************************************************************/

#include <string>
#include <stdio.h>
using namespace std;

#ifdef __linux__
#define __PROPERTY_FUNCTION__ __PRETTY_FUNCTION__
#elif defined(_WIN32) || defined(_WIN64)
#define __PROPERTY_FUNCTION__ __FUNCSIG__
#endif

namespace eular {

template<typename T>
class Foo
{
public:
    Foo() { printf("%s\n", __PROPERTY_FUNCTION__); }
    ~Foo() { printf("%s\n", __PROPERTY_FUNCTION__); }
};

}

template<typename T>
void f()
{
    printf("%s\n", __PROPERTY_FUNCTION__);
}

int main()
{
    // void f() [with T = long int]
    f<int64_t>();
    // void f() [with T = std::__cxx11::basic_string<char>]
    f<std::string>();
    // void f() [with T = double]
    f<double>();
    f<eular::Foo<int32_t>>();

    eular::Foo<double> foo;
    return 0;
}
