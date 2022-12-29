/*************************************************************************
    > File Name: JsonReader.cpp
    > Author: hsz
    > Brief:
    > Created Time: Tue 28 Jun 2022 09:55:05 AM CST
 ************************************************************************/

#include "jsonreader.h"

namespace eular {

JsonReader::JsonReader()
{

}

JsonReader::~JsonReader()
{

}

/**
 * @brief 解析json文件
 * 
 * @param json文件路径
 * @return true 成功
 * @return false 失败
 */
bool JsonReader::parse(const String8 &path)
{
    Json::Reader reader;
    if (false == reader.parse(path.c_str(), mJsonRoot)) {
        return false;
    }


}

bool JsonReader::parse(const std::string &data)
{

}

} // namespace eular
