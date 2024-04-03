/*************************************************************************
    > File Name: test_json.cpp
    > Author: hsz
    > Brief:
    > Created Time: Sat 25 Nov 2023 01:42:12 PM CST
 ************************************************************************/

#include <stdio.h>
#include <iostream>

#include "nlohmann/json.hpp"

using Json = nlohmann::json;

void format_begin(const char *func)
{
    printf("-------------------%s--------------------\n", func);
}

void format_end()
{
    printf("\n");
}

// 创建简单json对象
void test_create_json()
{
    format_begin(__PRETTY_FUNCTION__);

    Json jsonObj;

    jsonObj["name"] = "轲";
    jsonObj["age"] = 30;
    jsonObj["answer"]["everything"] = 42;
    jsonObj["object"] = { {"currency", "USD"}, {"value", 42.99} };
    jsonObj["pets"] = { "cat", "dog" };

    /**
     * dump输出的是utf8编码格式, 将文件编码方式修改为GBK会在dump时触发崩溃
     * 修改第四个参数可以防止, 但是name对应的值为空
     * 
     * 第一个参数表示缩进, 第二个参数表示缩进方式
     * 第三个参数表示是否保证都是ASCII码, 即对非ASCII码进行转义
     * 第四个参数表示错误处理方式, 默认strict, 当遇到不是UTF-8编码时抛出异常
     * replace 用U+FFFD替换无效的UTF-8序列
     * ignore 忽略无效的UTF-8序列
     */

    std::string jsonContent = jsonObj.dump(4, ' ', true, nlohmann::detail::error_handler_t::ignore); // 设置成4个空格的缩进方式
    printf("%s\n", jsonContent.c_str());

    jsonContent = jsonObj.dump(4, ' ', true, nlohmann::detail::error_handler_t::replace); // 设置成4个空格的缩进方式
    printf("%s\n", jsonContent.c_str());

    try
    {
        jsonContent = jsonObj.dump(4, ' ', true, nlohmann::detail::error_handler_t::strict); // 设置成4个空格的缩进方式
        printf("%s\n", jsonContent.c_str());
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

    format_end();
}

void test_read_json()
{
    format_begin(__PRETTY_FUNCTION__);
    Json jsonObj;
    jsonObj["name"] = "Tom";
    jsonObj["age"] = 30;
    jsonObj["pets"] = { "cat", "dog" };

    std::string name;
    printf("name: %s\n", jsonObj.at("name").get_to(name).c_str());

    try
    {
        // 类型不匹配会抛出异常
        jsonObj.at("age").get_to(name);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }

    try
    {
        // 类型不匹配会抛出异常
        std::string temp = jsonObj["age"];
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }

    std::cout << "age :" << jsonObj.at("age") << std::endl;

    const auto size = jsonObj.at("pets").size();
    std::cout << "pets :" << std::endl;
    for( int i = 0;i < size;++i)
    {
        std::cout << "\t" << jsonObj.at("pets").at(i) << std::endl;
    }

    format_end();
}

int main(int argc, char **argv)
{
    test_create_json();
    test_read_json();
    return 0;
}
