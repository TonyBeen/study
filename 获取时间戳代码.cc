/*************************************************************************
    > File Name: 获取时间戳代码.cc
    > Author: hsz
    > Brief:
    > Created Time: Thu 14 Dec 2023 03:25:25 PM CST
 ************************************************************************/

#include <iostream>
#include <chrono>

void time_epoch()
{
    // 创建表示指定日期时间的时间点
    std::tm timeinfo = {};
    timeinfo.tm_year = 2023 - 1900;  // 年份从1900开始计算
    timeinfo.tm_mon = 0;  // 月份从0开始计算（0代表一月）
    timeinfo.tm_mday = 1;  // 日期

    // 将时间转换为时间点
    std::time_t time = std::mktime(&timeinfo);
    auto tp = std::chrono::system_clock::from_time_t(time);

    // 计算毫秒时间戳
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count();
    std::cout << "Milliseconds since epoch: " << milliseconds << std::endl;
}

int main(int argc, char **argv)
{
    time_epoch();
    return 0;
}
