/*************************************************************************
    > File Name: JsonWriter.h
    > Author: hsz
    > Brief:
    > Created Time: Tue 28 Jun 2022 09:55:17 AM CST
 ************************************************************************/

#ifndef __JSON_WRITER_H__
#define __JSON_WRITER_H__

#include <json/json.h>

namespace eular {

class JsonWriter
{
public:
    JsonWriter();
    ~JsonWriter();

private:
    

    friend class JsonReader;
};

} // namespace eular

#endif  // __JSON_WRITER_H__
