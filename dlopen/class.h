/*************************************************************************
    > File Name: class.h
    > Author: hsz
    > Brief:
    > Created Time: 2024年05月11日 星期六 10时07分58秒
 ************************************************************************/

#pragma once

class CClass
{
public:
    CClass();
    ~CClass();

private:
    void show();
    static void Show(CClass *ptr);
    static void NoParam();
    void Param(int );
    // virtual void show();

private:
    char _buffer[64];
};