/*************************************************************************
    > File Name: common.hpp
    > Author: hsz
    > Brief:
    > Created Time: 2024年12月16日 星期一 17时23分54秒
 ************************************************************************/

#pragma once

// byte / ns -> MB / s
double BPerNS2MBPerS(double speed)
{
    return speed * 1000000000 / (1024 * 1024);
}