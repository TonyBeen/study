/*
 * @Description: 
 * @Autor: alias
 * @Date: 2021-09-02 17:40:08
 * @LastEditors: Seven
 * @LastEditTime: 2021-09-02 17:51:57
 */
#include <iostream>
#include <string.h>
#include <type_traits>

using namespace std;

void test(const char *param1, const char *param2) __nonnull((1, 2));
void test(const char *param1, const char *param2)
{

}

int main(int argc, char **argv)
{
    /**
     * 标准lambda表达式
     * [caputure](params) opt->return { 函数体 };
     *      caputure: 捕获方式
     *      params: 传参，可以多个
     *      opt: mutable(值传递时可以修改外部变量), exception(是否抛出异常以及何种异常), attribute(属性)
     * Lambda传参方式有两种，一种捕获，一种类似函数传参
     * 捕获：要求Lambda函数内部使用变量必须在定义Lambda之前存在
     *      捕获有不捕获[]，值捕获[=]，引用捕获[&]
     * 捕获扩展：
     *      [=, &mem]:表示除了mem变量采用引用捕获，其他都是值捕获
     *      [mem]: 只捕获mem变量
     *      [=]: 值捕获外部作用域中的变量(要求是在Lambda之前的变量)
     * 传参：要求则没有那么高，参数只需在调用之前存在即可，传参类型可以值也可以是引用指针等
     *
     */
    {
        // 无参值捕获
        int x = 0, y = 0;
        // printf("&x = %p, &y = %p\n", &x, &y);
        auto add = [=]() mutable {
            x = 10;
            y = 10;
            return x + y;
        };
        std::cout << add() << ", " << x << ", " << y << std::endl;
    }

    {
        // 两者不矛盾，中括号里的是捕获外界变量并以值传递方式进入函数内部，而传参是以引用方式，故可以修改值
        // 中括号的作用是在没有参数的时候可以在lambda函数内部使用外部变量
        // 有参值捕获
        auto add = [=](int &x, int &y) mutable {
            x = 10;
            y = 10;
            return x + y;
        };
        int x = 0, y = 0;
        std::cout << add(x, y) << std::endl;
        std::cout << x << ", " << y << std::endl;
    }

    {
        int a = 0;
        // 引用传参
        auto add = [&](int &x, int &y) mutable->int {
            a = 11;
            x = 11;
            y = 11;
            return a + x + y; 
        };
        int x = 2, y = 3;
        std::cout << add(x, y) << std::endl;
        std::cout << "a = " << a << ", x = " << x << ", y = " << y << std::endl;
    }

    return 0;
}