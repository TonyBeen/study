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

// ������json����
void test_create_json()
{
    format_begin(__PRETTY_FUNCTION__);

    Json jsonObj;

    jsonObj["name"] = "��";
    jsonObj["age"] = 30;
    jsonObj["answer"]["everything"] = 42;
    jsonObj["object"] = { {"currency", "USD"}, {"value", 42.99} };
    jsonObj["pets"] = { "cat", "dog" };

    /**
     * dump�������utf8�����ʽ, ���ļ����뷽ʽ�޸�ΪGBK����dumpʱ��������
     * �޸ĵ��ĸ��������Է�ֹ, ����name��Ӧ��ֵΪ��
     * 
     * ��һ��������ʾ����, �ڶ���������ʾ������ʽ
     * ������������ʾ�Ƿ�֤����ASCII��, ���Է�ASCII�����ת��
     * ���ĸ�������ʾ������ʽ, Ĭ��strict, ����������UTF-8����ʱ�׳��쳣
     * replace ��U+FFFD�滻��Ч��UTF-8����
     * ignore ������Ч��UTF-8����
     */

    std::string jsonContent = jsonObj.dump(4, ' ', true, nlohmann::detail::error_handler_t::ignore); // ���ó�4���ո��������ʽ
    printf("%s\n", jsonContent.c_str());

    jsonContent = jsonObj.dump(4, ' ', true, nlohmann::detail::error_handler_t::replace); // ���ó�4���ո��������ʽ
    printf("%s\n", jsonContent.c_str());

    try
    {
        jsonContent = jsonObj.dump(4, ' ', true, nlohmann::detail::error_handler_t::strict); // ���ó�4���ո��������ʽ
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
        // ���Ͳ�ƥ����׳��쳣
        jsonObj.at("age").get_to(name);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }

    try
    {
        // ���Ͳ�ƥ����׳��쳣
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
