/*************************************************************************
    > File Name: test_lambda.cc
    > Author: hsz
    > Brief:
    > Created Time: Mon 20 Feb 2023 09:19:27 AM CST
 ************************************************************************/

#include <assert.h>
#include <iostream>
#include <functional>

using namespace std;

/**
 * lambda也是c++11中引入的，在C++11中，lambda表达式只能用捕获this，this是当前对象的一个只读的引用。
 * 在C++17中，可以捕获*this, *this是当前对象的一个拷贝，捕获当前对象的拷贝.
 * 能够确保当前对象释放后， lambda表达式能安全的调用this中的变量和方法。
 */

class Any {
public:
    Any() :
        this_e(this)
    { 
        printf("%s() this = %p\n", __func__, this);
    }

    Any(const Any &other)
    {
        printf("%s(const Any &other)\n", __func__);    
    }

    ~Any()
    {
        printf("%s()\n", __func__);
    }

    void print() const { printf("%s() this = %p\n", __func__, this); }
    void print_nonconst() { printf("%s()\n", __func__); }

    auto exec() -> std::function<void()>
    {
        auto lambda = [*this]()
        {
            // 此时*this是const Any类型, 会调用一次拷贝构造, 当不存在拷贝构造时无法捕获*this
            printf("%s()\n", __func__);
            assert(this != this_e); // 由于拷贝构造不会修改this_e的值，故this_e还保留着之前对象的地址
            this->print(); // 同样可以使用this(但只能调用const属性的函数), 此时的this是*this新创建的Any的地址

            // 通过const_cast来消除const，从而调用非const属性的函数
            const_cast<Any *>(this)->print_nonconst();
        };

        return lambda;
    }

    void *this_e;
};

/**
 * c++11无法捕获*this，导致在get()函数返回的lambda在main函数调用时会崩溃
 * 
 * c++17后可以捕获*this，来解决此问题
 */
auto get() -> std::function<void()>
{
    Any any;
    return any.exec();
}

int main(int argc, char **argv)
{
    get()();
    return 0;
}
