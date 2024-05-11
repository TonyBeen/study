/*************************************************************************
    > File Name: class.cc
    > Author: hsz
    > Brief:
    > Created Time: 2024年05月11日 星期六 10时08分02秒
 ************************************************************************/

#include "class.h"
#include <stdio.h>
#include <string.h>

CClass::CClass()
{
    size_t size = sizeof(_buffer);
    memset(_buffer, '1', size);
    _buffer[size - 1] = '\0';
}

CClass::~CClass()
{
}

void CClass::show()
{
    printf("%s\n", _buffer);
}

void CClass::Show(CClass *ptr)
{
    ptr->show();
}

void CClass::NoParam()
{
}

void CClass::Param(int)
{
}

extern "C" {
    CClass* createClass() {
        return new CClass();
    }

    void destroyClass(CClass* obj) {
        delete obj;
    }
}