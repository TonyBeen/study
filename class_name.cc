/*************************************************************************
    > File Name: class_name.cc
    > Author: hsz
    > Brief:
    > Created Time: 2024年05月19日 星期日 14时39分19秒
 ************************************************************************/

#include <stdlib.h>
#include <cxxabi.h>

#include <typeinfo>
#include <string>
#include <iostream>

std::string demangle(const char* name)
{
    int status = 0;
    char* buf = abi::__cxa_demangle(name, NULL, NULL, &status);
    if (status == 0 && buf) {
        std::string s(buf);
        free(buf);
        return s;
    }
    return std::string(name);
}

template <typename T> struct ClassNameHelper { static std::string name; };
template <typename T> std::string ClassNameHelper<T>::name = demangle(typeid(T).name());

class TypeInfo
{
public:

};

struct StInfo
{
};


int main(int argc, char **argv)
{
    // 使用-m分别编译32和64位程序
    std::cout << ClassNameHelper<signed char>::name << std::endl;   // signed char
    std::cout << ClassNameHelper<char>::name << std::endl;          // char
    std::cout << ClassNameHelper<unsigned char>::name << std::endl; // unsigned char

    std::cout << ClassNameHelper<int8_t>::name << std::endl;    // signed char
    std::cout << ClassNameHelper<uint8_t>::name << std::endl;   // unsigned char

    std::cout << ClassNameHelper<short>::name << std::endl;     // short
    std::cout << ClassNameHelper<int16_t>::name << std::endl;   // short
    std::cout << ClassNameHelper<uint16_t>::name << std::endl;  // unsigned short

    std::cout << ClassNameHelper<int>::name << std::endl;       // int
    std::cout << ClassNameHelper<int32_t>::name << std::endl;   // int
    std::cout << ClassNameHelper<uint32_t>::name << std::endl;  // unsigned int

    std::cout << ClassNameHelper<long>::name << std::endl;      // long
    std::cout << ClassNameHelper<int64_t>::name << std::endl;   // 32: long long, 64: long
    std::cout << ClassNameHelper<uint64_t>::name << std::endl;  // 32: unsigned long long, 64: unsigned long

    std::cout << ClassNameHelper<float>::name << std::endl;     // float
    std::cout << ClassNameHelper<double>::name << std::endl;    // double

    std::cout << ClassNameHelper<TypeInfo>::name << std::endl;  // TypeInfo
    std::cout << ClassNameHelper<StInfo>::name << std::endl;    // StInfo

    return 0;
}
