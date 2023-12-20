/*************************************************************************
    > File Name: test_callstack.cc
    > Author: hsz
    > Brief: g++ test_cxxabi_stacktrace.cc callstack.cpp -o test_cxxabi_stacktrace -rdynamic -std=c++11 -O0
    > Created Time: Mon 11 Dec 2023 06:36:38 PM CST
 ************************************************************************/

#include <iostream>
#include "callstack.h"

class ClassA {
public:
    void functionA() {
        utils::CallStack stack;
        stack.update();
        const auto &frames = stack.frames();

        for (const auto &it : frames)
        {
            std::cout << it << std::endl;
        }
    }

    void functionB() {
        functionA();
    }
};

class ClassB {
public:
    void functionC() {
        ClassA objA;
        objA.functionB();
    }

    void functionD() {
        functionC();
    }
};

class ClassC {
public:
    void functionE() {
        ClassB objB;
        objB.functionD();
    }
};

int main(int argc, char **argv)
{
    ClassC objC;
    objC.functionE();
    return 0;
}

