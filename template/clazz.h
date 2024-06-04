/*************************************************************************
    > File Name: clazz.h
    > Author: hsz
    > Brief:
    > Created Time: 2024年05月21日 星期二 14时58分04秒
 ************************************************************************/

#pragma once

const char *ToBool(bool value)
{
    if (value)
    {
        return "true";
    }

    return "false";
}

enum Type {
};

enum class ClassType {
};

enum TypeInt : int {
};

enum class ClassTypeInt : int{
};

class Class {
};

struct Struct {
};

union Union {
};
