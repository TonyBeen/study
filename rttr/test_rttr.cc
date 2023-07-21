/*************************************************************************
    > File Name: test_rttr.cc
    > Author: hsz
    > Brief:
    > Created Time: Sun 11 Jun 2023 03:19:16 PM CST
 ************************************************************************/

#include <iostream>
#include <rttr/registration>

struct MyStruct { MyStruct() {}; void func(double) {}; int data; };

// 手动注册属性方法和构造函数
RTTR_REGISTRATION
{
    rttr::registration::class_<MyStruct>("MyStruct")
         .constructor<>()
         .property("data", &MyStruct::data)
         
         .method("func", &MyStruct::func);
}

int main() {

    // 遍历类的成员
    rttr::type t = rttr::type::get<MyStruct>();
    for (auto& prop : t.get_properties())
        std::cout << "name: " << prop.get_name() << std::endl;

    for (auto& meth : t.get_methods())
        std::cout << "name: " << meth.get_name() << std::endl;

    //创建类型的实例
    rttr::type t2 = rttr::type::get_by_name("MyStruct");
    rttr::variant var = t2.create();    // 方式1

    rttr::constructor ctor = t2.get_constructor();  // 方式2
    var = ctor.invoke();
    std::cout << var.get_type().get_name() << std::endl;  // 打印类型名称

    //设置/获取属性
    MyStruct obj;

    rttr::property prop = rttr::type::get(obj).get_property("data");
    prop.set_value(obj, 23);

    rttr::variant var_prop = prop.get_value(obj);
    std::cout << var_prop.to_int() << std::endl; // prints '23'

    //调用方法
    MyStruct obj2;

    rttr::method meth = rttr::type::get(obj2).get_method("func");
    meth.invoke(obj2, 42.0);

    rttr::variant var2 = rttr::type::get(obj2).create();
    meth.invoke(var2, 42.0);

    return 0;
}

