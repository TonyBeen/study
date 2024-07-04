/*************************************************************************
    > File Name: 结构化绑定.cc
    > Author: hsz
    > Brief:
    > Created Time: Mon 20 Feb 2023 11:01:10 AM CST
 ************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <string>
#include <map>

struct Node
{
    int size;
    char buffer[128];
};

Node &get()
{
    static Node node;
    node.size = 10;
    strncpy(node.buffer, "Hello World", sizeof(node.buffer));
    printf("node.buffer = %p\n", node.buffer);
    return node;
}

void func()
{
    std::map<std::string, int> map;
    map["Hello"] = 100;
    map["World"] = 200;
    for (const auto &[key, val] : map) {
        std::cout << "key = " << key << ", value = " << val << std::endl;
    }
}

int main(int argc, char **argv)
{
    // 此处声明时需与对象的元素个数一致，且按照元素顺序声明
    // 可以是auto(拷贝), auto &(引用), const auto &(常量引用)
    const auto &[size, buffer] = get();
    std::cout << "size = " << size << ", buffer = " << buffer << std::endl;
    printf("main() buffer = %p\n", buffer);

    func();
    return 0;
}
