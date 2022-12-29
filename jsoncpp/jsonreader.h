/*************************************************************************
    > File Name: JsonReader.h
    > Author: hsz
    > Brief:
    > Created Time: Tue 28 Jun 2022 09:54:59 AM CST
 ************************************************************************/

#ifndef __JSON_READER_H__
#define __JSON_READER_H__

#include <json/json.h>
#include <utils/string8.h>
#include <string>
#include <map>

namespace eular {

class JsonReader
{
public:
    JsonReader();
    ~JsonReader();

    bool parse(const String8 &path);
    bool parse(const std::string &data);

private:
    Json::Value     mJsonRoot;
    friend class JsonWriter;
};

} // namespace eular

#endif // __JSON_READER_H__