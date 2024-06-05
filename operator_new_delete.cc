/*************************************************************************
    > File Name: operator_new_delete.cc
    > Author: hsz
    > Brief:
    > Created Time: Sat 16 Mar 2024 07:41:17 PM CST
 ************************************************************************/

#include <string.h>
#include <time.h>
#include <iostream>
#include <sstream>
#include <memory>
#include <functional>
#include <map>

void* operator new(std::size_t size) {
    std::cout << __PRETTY_FUNCTION__ << "\t" << size << std::endl;
    return std::malloc(size);
}

void operator delete(void* ptr) noexcept {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    std::free(ptr);
}

void* operator new(std::size_t size, const std::nothrow_t& tag) noexcept {
    std::cout << __PRETTY_FUNCTION__ << "\t" << size << std::endl;
    return std::malloc(size);
}

void operator delete(void* ptr, const std::nothrow_t& tag) noexcept {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    std::free(ptr);
}

void* operator new[](size_t size) {
    std::cout << __PRETTY_FUNCTION__ << "\t" << size << std::endl;
    return std::malloc(size);
}

void operator delete[](void *ptr) {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    std::free(ptr);
}

void* operator new[](size_t size, const std::nothrow_t& tag) noexcept {
    std::cout << __PRETTY_FUNCTION__ << "\t" << size << std::endl;
    return std::malloc(size);
}

void operator delete[](void *ptr, const std::nothrow_t& tag) noexcept {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    std::free(ptr);
}

class Foo
{
public:
    Foo()
    {
        printf("%s()\n", __func__);
    }
    ~Foo()
    {
        printf("%s()\n", __func__);
    }
};

int main()
{
    printf("sizeof(Foo) = %zu\n", sizeof(Foo));
    auto ptr = new Foo;
    delete ptr;

    ptr = new Foo[2];
    delete [] ptr;

    ptr = new (std::nothrow) Foo();
    ::operator delete(ptr, std::nothrow);

    ptr = new (std::nothrow) Foo[2];
    ::operator delete[](ptr, std::nothrow);

    return 0;
}



