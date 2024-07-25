#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <iostream>

#include <localcharset.h>
#include "iconv.h"

#define INVALID_ICONV_HANDLE ((iconv_t)-1)
#define INVALID_ICONV_RETURN ((size_t)-1)

#define CACHE_SIZE  1024

// export PATH=$PATH:/home/eular/VSCode/libiconv-1.15/install/lib
// export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/eular/VSCode/libiconv-1.15/install/lib

bool UTF8_2_UTF16LE(const std::string &u8String, std::u16string &u16String)
{
    if (u8String.empty())
    {
        return true;
    }

    char *pU8Begin = (char *)u8String.c_str();
    size_t inputSize = u8String.size();

    std::u16string outU16String;
    outU16String.reserve(inputSize);

    iconv_t iconvHandle = iconv_open("UTF-16LE", "UTF-8");
    if (INVALID_ICONV_HANDLE == iconvHandle) {
        perror("iconv_open");
        return false;
    }

    printf("inputSize: %zu\n", inputSize);

    bool result = true;
    while (inputSize > 0)
    {
        char outputBuf[CACHE_SIZE] = {0};

        char *pOutputBuf = outputBuf;
        size_t outputLen = CACHE_SIZE;
        size_t leftOutputLen = CACHE_SIZE;

        size_t nRet = iconv(iconvHandle, &pU8Begin, &inputSize, &pOutputBuf, &leftOutputLen);
        if (INVALID_ICONV_RETURN == nRet) {
            switch (errno) {
            case EINVAL:
                printf("An incomplete multibyte sequence has been encountered in the input.\n");
                result = false;
                break;
            case EILSEQ:
                printf("An invalid multibyte sequence has been encountered in the input.\n");
                result = false;
                break;
            case E2BIG:
                printf("buffer is small\n");
                break;
            default:
                break;
            }
        }

        if (!result)
        {
            outU16String.clear();
            break;
        }

        printf("nRet: %zu left: %zu leftOutputLen: %zu\n", nRet, inputSize, leftOutputLen);
        outU16String.append((char16_t *)outputBuf, (outputLen - leftOutputLen) / sizeof(char16_t));
    }

    u16String.append(outU16String);
    iconv_close(iconvHandle);
    return result;
}

bool UTF16LE_2_UTF8(const std::u16string &u16String, std::string &u8String)
{
    if (u16String.empty())
    {
        return true;
    }

    char *pU8Begin = (char *)u16String.c_str();
    size_t inputSize = u16String.size() * sizeof(char16_t);

    std::string outU8String;
    outU8String.reserve(inputSize);

    iconv_t iconvHandle = iconv_open("UTF-8", "UTF-16LE");
    if (INVALID_ICONV_HANDLE == iconvHandle) {
        perror("iconv_open");
        return false;
    }

    bool result = true;
    while (inputSize > 0)
    {
        char outputBuf[CACHE_SIZE] = {0};

        char *pOutputBuf = outputBuf;
        size_t outputLen = CACHE_SIZE;
        size_t leftOutputLen = CACHE_SIZE;

        size_t nRet = iconv(iconvHandle, &pU8Begin, &inputSize, &pOutputBuf, &leftOutputLen);
        if (INVALID_ICONV_RETURN == nRet) {
            switch (errno) {
            case EINVAL:
                printf("An incomplete multibyte sequence has been encountered in the input.\n");
                result = false;
                break;
            case EILSEQ:
                printf("An invalid multibyte sequence has been encountered in the input.\n");
                result = false;
                break;
            case E2BIG:
                printf("buffer is small\n");
                break;
            default:
                break;
            }
        }

        if (!result)
        {
            outU8String.clear();
            break;
        }

        printf("nRet: %zu left: %zu leftOutputLen: %zu\n", nRet, inputSize, leftOutputLen);
        outU8String.append(outputBuf, (outputLen - leftOutputLen) / sizeof(char));
    }

    u8String.append(outU8String);
    iconv_close(iconvHandle);
    return result;
}

int main()
{
    const char* charset = locale_charset();
    printf("local charset: %s\n", charset);

    char utf8string[] =
        "你好世界你好世界你好世界你好世界你好世界你好世界你好世界你好世界你好世界"
        "你好世界你好世界你好世界你好世界你好世界你好世界你好世界你好世界你好世界"
        "你好世界你好世界你好世界你好世界你好世界你好世界你好世界你好世界你好世界"
        "你好世界你好世界你好世界你好世界你好世界你好世界你好世界你好世界你好世界"
        "你好世界你好世界你好世界你好世界你好世界你好世界你好世界你好世界你好世界"
        "你好世界你好世界你好世界你好世界你好世界你好世界你好世界你好世界你好世界";

    std::u16string u16String;
    UTF8_2_UTF16LE(utf8string, u16String);

    std::string u8String;
    UTF16LE_2_UTF8(u16String, u8String);

    printf("%s\n", u8String.c_str());
    return 0;
}