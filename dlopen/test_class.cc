/*************************************************************************
    > File Name: test_class.cc
    > Author: hsz
    > Brief:
    > Created Time: 2024年05月11日 星期六 10时19分15秒
 ************************************************************************/

#include <iostream>
#include <dlfcn.h>

typedef void* (*CreateFunction)();
typedef void (*DestroyFunction)(void *);

typedef void (*DoShow)(void *);

int main(int argc, char **argv)
{
    void* handle = dlopen("./libmyclass.so", RTLD_LAZY);
    if (!handle) {
        std::cerr << "Error opening shared library: " << dlerror() << std::endl;
        return 1;
    }

    // 获取创建和销毁函数的指针
    CreateFunction createFunc = (CreateFunction)dlsym(handle, "createClass");
    if (!createFunc) {
        std::cerr << "Error loading create function: " << dlerror() << std::endl;
        dlclose(handle);
        return 1;
    }

    DestroyFunction destroyFunc = (DestroyFunction)dlsym(handle, "destroyClass");
    if (!destroyFunc) {
        std::cerr << "Error loading destroy function: " << dlerror() << std::endl;
        dlclose(handle);
        return 1;
    }

    void* obj = createFunc();
    if (!obj) {
        std::cerr << "Error creating MyClass instance" << std::endl;
        dlclose(handle);
        return 1;
    }

    DoShow pFnShow = (DoShow)dlsym(handle, "_ZN6CClass4showEv");
    pFnShow(obj);

    destroyFunc(obj);

    // 关闭共享库
    dlclose(handle);

    return 0;
}
