/*************************************************************************
    > File Name: range_for_loop.cc
    > Author: hsz
    > Brief:
    > Created Time: Thu 18 Jan 2024 10:17:04 AM CST
 ************************************************************************/

#include <iostream>
#include <initializer_list>
#include <list>
#include <string>

/**
 * @brief 测试范围for循环释放依赖于begin和end函数
 * 
 * @tparam T 
 * @tparam SIZE 
 */

template<typename T, std::size_t SIZE>
class MyArray {
public:
    MyArray(const std::initializer_list<T> &arr)
    {
        int32_t size = arr.size() <= SIZE ? arr.size() : SIZE;
        int32_t i = 0;
        for (auto it = arr.begin(); i < size; ++i, ++it) {
            data[i] = *it;
        }
    }

    T& operator[](std::size_t index) {
        return data[index];
    }

    const T& operator[](std::size_t index) const {
        return data[index];
    }

    T* begin() {
        return data;
    }

    T* end() {
        return data + SIZE;
    }

private:
    T data[SIZE];
};

std::list<int32_t> get_list()
{
    static std::list<int32_t> ret;
    static int32_t idx = 0;
    ret.push_back(idx);
}

int main() {
    MyArray<int, 5> arr = {1, 2, 3, 4, 5};

    for (const auto& element : arr) {
        std::cout << element << " ";
    }
    std::cout << std::endl;

    std::list<int32_t> intList = {1, 2, 3, 4, 5};
    for (auto it = intList.begin(); it != intList.end(); ) {
        std::cout << *it << " ";
        if (3 == (*it)) {
            it = intList.erase(it);
        } else {
            ++it;
        }
    }
    std::cout << std::endl;

    std::list<std::string> strList = {"1", "2", "3", "4", "5"};
    for (const auto &it : strList) {
        std::cout << it << ", " << typeid(decltype(it)).name() << "\n";
    }
    std::cout << std::endl;

    // for (const auto &it : get_list())
    // {
    //     std::cout << it << "\n";
    // }

    return 0;
}
