/*************************************************************************
    > File Name: regex.cc
    > Author: hsz
    > Brief:
    > Created Time: 2025年09月26日 星期五 11时26分25秒
 ************************************************************************/

#include <iostream>
#include <regex>
#include <string>

bool regexMatch(const std::string& str, const std::string& pattern) {
    try {
        std::regex re(pattern);
        return std::regex_match(str, re);
    } catch (const std::regex_error& e) {
        std::cerr << "正则表达式语法错误: " << e.what() << std::endl;
        return false;
    }
}

void test_regex()
{
    std::string text = "I want to update the client.";
    std::regex pattern("up.*client");
    std::smatch match;

    // 查找匹配
    if (std::regex_search(text, match, pattern)) {
        std::cout << "Match found!" << std::endl;

        // 输出匹配的起始和结束位置
        std::cout << "Matched substring: " << match.str() << std::endl;
        std::cout << "Start position: " << match.position() << std::endl;
        std::cout << "End position: " << match.position() + match.length() << std::endl;
    } else {
        std::cout << "No match found!" << std::endl;
    }

    std::regex match_all(".*");
    if (std::regex_search(text, match_all)) {
        std::cout << "Match all!" << std::endl;
    } else {
        std::cout << "No match all!" << std::endl;
    }
}

int main()
{
    // 测试正则表达式
    test_regex();

    // . 匹配任意单个字符
    std::cout << regexMatch("cat", "c.t") << std::endl;
    // .* 匹配任意数量(包括零个)的任意字符
    std::cout << regexMatch("file_v1.backup", "file_.*\\.backup") << std::endl;
    // \\. 匹配字面量 . 正常只需要"\."但是代码中需要对‘\'进行转义
    std::cout << regexMatch("123-abc", "\\d{3}.[a-z]{3}") << std::endl;
    return 0;
}