/*************************************************************************
    > File Name: test.cc
    > Author: hsz
    > Brief:
    > Created Time: 2025年08月18日 星期一 17时03分18秒
 ************************************************************************/

#include <iostream>
#include <string>
#include <vector>
#include <cxxabi.h>
#include <type_traits>

std::string demangle(const char *sym)
{
    int32_t status = 0;
    char *demangled = abi::__cxa_demangle(sym, nullptr, nullptr, &status);
    if (status == 0 && demangled != nullptr) {
        std::string ret = std::string(demangled);
        free(demangled);
        return ret;
    }

    return sym;
}

// 主模板
template <typename T>
struct MultidimPointer {
    using type = T;
};

// 针对数组 T[N] 部分特化，递归
template <typename T, size_t N>
struct MultidimPointer<T[N]> {
    using type = typename MultidimPointer<T>::type *;
};

// 辅助类型别名（简化使用）
template <typename T>
using MultidimPointer_t = typename MultidimPointer<T>::type;

enum {
    OK,
};

enum class ErrorCode {
    Success,
};

using array_t = int32_t[2][3];
array_t& get()
{
    static array_t array = {0};
    return array;
}

int main(int argc, char **argv)
{
    // int32_t array[10][6][3] = {0};
    // std::cout << typeid(array).name() << std::endl;
    // std::cout << demangle(typeid(array).name()) << std::endl;

    // decltype(array) array_2;
    // array_2[0][0][0] = 1;
    // std::cout << array_2 << ", " << array << std::endl;

    // std::cout << demangle(typeid(MultidimPointer_t<decltype(array)>).name()) << std::endl;
    // std::cout << demangle(typeid(MultidimPointer_t<int32_t>).name()) << std::endl;

    // std::string stringArray[10][6][3] = {};
    // std::cout << typeid(stringArray).name() << std::endl;
    // std::cout << demangle(typeid(stringArray).name()) << std::endl;

    // std::vector<int32_t> vectorArray[10][6][3] = {};
    // std::cout << typeid(vectorArray).name() << std::endl;
    // std::cout << demangle(typeid(vectorArray).name()) << std::endl;

    // std::vector<std::vector<std::string>> vec_2d;
    // std::cout << typeid(vec_2d).name() << std::endl;
    // std::cout << demangle(typeid(vec_2d).name()) << std::endl;

    // std::vector<std::vector<std::vector<std::string>>> vec_3d;
    // std::cout << typeid(vec_3d).name() << std::endl;
    // std::cout << demangle(typeid(vec_3d).name()) << std::endl;

    /////////////////////////////////////////////////////
    // std::cout << std::is_enum<decltype(OK)>::value << std::endl;
    // std::cout << std::is_enum<decltype(ErrorCode::Success)>::value << std::endl;
    // int32_t ret = OK;
    // ErrorCode err = ErrorCode::Success;
    // std::cout << std::is_enum<decltype(ret)>::value << std::endl;
    // std::cout << std::is_enum<decltype(err)>::value << std::endl;

    ////////////////////////////////////////////////////
    // using array_dest_type = decltype(new int32_t[2][2]);
    // array_dest_type pointer = new int32_t[2][2];
    // pointer[0][0] = 1;
    // pointer[0][1] = 2;
    // pointer[1][0] = 3;
    // pointer[1][1] = 4;
    // std::cout << pointer[0][0] << ", " << pointer[0][1] << std::endl;
    // std::cout << pointer[1][0] << ", " << pointer[1][1] << std::endl;
    // std::cout << typeid(array_dest_type).name() << std::endl;
    // std::cout << demangle(typeid(array_dest_type).name()) << std::endl;
    return 0;
}
