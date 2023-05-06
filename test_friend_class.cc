/*************************************************************************
    > File Name: test_friend_class.cc
    > Author: hsz
    > Brief: 测试友元类的子类是否可以访问
    > Created Time: Tue 25 Oct 2022 02:26:52 PM CST
 ************************************************************************/

#include <iostream>
using namespace std;

class Base {
    friend class Test;

protected:
    virtual void func()
    {
        std::cout << __func__ << "()\n";
    }
};

class Drived : public Base {
protected:
    virtual void func()
    {
        std::cout << "Drived::" << __func__ << "()\n";
    }
};

class Test {
public:
    Test()
    {
        Base *b = new Drived;
        b->func();
        delete b;
    }
};

int main(int argc, char **argv)
{
    Test t;
    return 0;
}
