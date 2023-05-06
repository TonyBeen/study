#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>
#include <json/json.h>

int main()
{
    Json::FastWriter writer;
    Json::StyledWriter style;
    Json::Value val1;
    val1["node"] = 100;

    Json::Value array;

    Json::Value array1;
    array1.append("array1");
    array1.append(true);
    array1.append(200);

    Json::Value array2;
    array2.append("array2");
    array2.append(false);
    array2.append(300);

    array.append(array1);
    array.append(array2);
    val1["array"] = array;

    Json::String str = writer.write(val1);
    std::cout << str << std::endl;

    str = style.write(val1);
    std::cout << str << std::endl;

    std::cout << "***************************\n";
    Json::Value root;
    Json::Reader reader;
    reader.parse(str, root);

    std::cout << root["node"].asInt() << std::endl;

    array = root["array"];
    for (int i = 0; i < array.size(); ++i) {
        std::cout << root["array"][i].type() << ", " << root["array"][i] << std::endl;
    }

    return 0;
}