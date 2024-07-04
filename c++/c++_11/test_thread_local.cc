/*************************************************************************
    > File Name: test_thread_local.cc
    > Author: hsz
    > Brief: 测试C++11 thread_local特性
    > Created Time: Tue 09 Nov 2021 10:17:59 AM CST
 ************************************************************************/

#include <iostream>
#include <thread>
#include <mutex>

using namespace std;

// __thread
static thread_local int gCount = 0;

class Test01 {
public:
    Test01() {}

    static thread_local int num;
};

// 静态成员变量带有thread_local关键字的需要在类外声明或初始化时也带上thread_local
// 不然会报错 undefined reference to `Test01::num'
thread_local int Test01::num;


void test_01(Test01 *ptr)
{
    ptr->num++;
    cout << "Thread id " << this_thread::get_id() << ", num = " << ptr->num << endl;
}

void test_02(Test01 *ptr)
{
    cout << "Thread id " << this_thread::get_id() << ", num = " << ptr->num << endl;
}

int main(int argc, char **argv)
{
    Test01 t;

    thread th1(test_01, &t);
    th1.join();

    thread th2(test_02, &t);
    th2.join();
    return 0;
}

/*
结果
Thread id 140014962792192, num = 11
Thread id 140014962792192, num = 10
*/
