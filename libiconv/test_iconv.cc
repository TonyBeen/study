/*************************************************************************
    > File Name: test_iconv.cc
    > Author: hsz
    > Brief:
    > Created Time: 2025年07月18日 星期五 17时43分01秒
 ************************************************************************/

#include <stdio.h>
#include <errno.h>
#include <string>
#include <exception>
#include <iconv.h>

#define CODE_UNSUPPORT "UNSUPPORT"

#define INVALID_ICONV_HANDLE ((iconv_t)-1)
#define INVALID_ICONV_RETURN ((size_t)-1)

#define CACHE_SIZE  64

enum CodeFlag {
    GBK = 0,
    GB2312,
    UTF8,
    UTF16LE,
    UTF16BE,
};

uint32_t _ComputeOutSizeToGBK_UTF16(CodeFlag from, CodeFlag to, uint32_t inputSize)
{
    switch (from) {
    case CodeFlag::UTF8:
        return 2 * inputSize; // 按照最大size计算
    default:
        return inputSize;
    }

    return 0;
}

uint32_t _ComputeOutSizeToUTF8(CodeFlag from, CodeFlag to, uint32_t inputSize)
{
    return 2 * inputSize; // 按照最大size计算, 4 * inputSize / 2
}

uint32_t _ComputeOutSize(CodeFlag from, CodeFlag to, uint32_t inputSize)
{
    if (from == to) {
        return inputSize;
    }

    switch (to) {
    // 两个字节编码
    case CodeFlag::GBK:
    case CodeFlag::GB2312:
    case CodeFlag::UTF16LE:
    case CodeFlag::UTF16BE:
        return _ComputeOutSizeToGBK_UTF16(from, to, inputSize);
    case CodeFlag::UTF8:
        return _ComputeOutSizeToUTF8(from, to, inputSize);
    default:
        break;
    }

    return 0;
}

int32_t UTF8ToUTF16LE(const std::string &u8String, std::wstring &u16String)
{
    if (u8String.empty()) {
        return -1;
    }

    char *pU8Begin = (char *)u8String.c_str();
    size_t inputSize = u8String.size();

    std::wstring outU16String;
    outU16String.reserve(_ComputeOutSize(CodeFlag::UTF8, CodeFlag::UTF16LE, u8String.length()));

    iconv_t iconvHandle = iconv_open("UTF-16LE", "UTF-8");
    if (INVALID_ICONV_HANDLE == iconvHandle) {
        perror("iconv_open");
        return -errno;
    }

    int32_t result = 0;
    while (inputSize > 0) {
        char outputBuf[CACHE_SIZE] = {0};

        char *pOutputBuf = outputBuf;
        size_t outputLen = CACHE_SIZE;
        size_t leftOutputLen = CACHE_SIZE;

        size_t nRet = iconv(iconvHandle, &pU8Begin, &inputSize, &pOutputBuf, &leftOutputLen);
        if (INVALID_ICONV_RETURN == nRet) {
            switch (errno) {
            case EINVAL:
                // printf("An incomplete multibyte sequence has been encountered in the input.\n");
                result = -EINVAL;
                break;
            case EILSEQ:
                // printf("An invalid multibyte sequence has been encountered in the input.\n");
                result = -EILSEQ;
                break;
            case E2BIG:
                break;
            default:
                break;
            }
        }

        if (result < 0) {
            outU16String.clear();
            break;
        }

        outU16String.append((wchar_t *)outputBuf, (outputLen - leftOutputLen) / sizeof(wchar_t));
    }

    u16String.append(outU16String);
    iconv_close(iconvHandle);
    return result;
}

int32_t UTF16LEToUTF8(const std::wstring &u16String, std::string &u8String)
{
    if (u16String.empty()) {
        return -1;
    }

    char *pU8Begin = (char *)u16String.c_str();
    size_t inputSize = u16String.size() * sizeof(wchar_t);

    std::string outU8String;
    outU8String.reserve(_ComputeOutSize(CodeFlag::UTF16LE, CodeFlag::UTF8, u16String.size()));

    iconv_t iconvHandle = iconv_open("UTF-8", "UTF-16LE");
    if (INVALID_ICONV_HANDLE == iconvHandle) {
        perror("iconv_open");
        return -errno;
    }

    int32_t result = 0;
    while (inputSize > 0) {
        char outputBuf[CACHE_SIZE] = {0};

        char *pOutputBuf = outputBuf;
        size_t outputLen = CACHE_SIZE;
        size_t leftOutputLen = CACHE_SIZE;

        size_t nRet = iconv(iconvHandle, &pU8Begin, &inputSize, &pOutputBuf, &leftOutputLen);
        if (INVALID_ICONV_RETURN == nRet) {
            switch (errno) {
            case EINVAL:
                // printf("An incomplete multibyte sequence has been encountered in the input.\n");
                result = -EINVAL;
                break;
            case EILSEQ:
                // printf("An invalid multibyte sequence has been encountered in the input.\n");
                result = -EILSEQ;
                break;
            case E2BIG:
                break;
            default:
                break;
            }
        }

        if (result < 0) {
            outU8String.clear();
            break;
        }

        outU8String.append(outputBuf, (outputLen - leftOutputLen));
    }

    u8String.append(outU8String);
    iconv_close(iconvHandle);
    return result;
}


int main(int argc, char **argv)
{
    std::string u8String = "你好, 世界！你好, 世界！你好, 世界！你好, 世界！你好, 世界！你好, 世界！你好, 世界！你好, 世界！你好, 世界！你好, 世界！你好, 世界！你好, 世界！你好, 世界！你好, 世界！你好, 世界！你好, 世界！你好, 世界！你好, 世界！你好, 世界！你好, 世界！你好, 世界！你好, 世界！";
    std::wstring u16String;
    int32_t ret = UTF8ToUTF16LE(u8String, u16String);
    if (ret < 0) {
        printf("UTF8ToUTF16LE failed, ret: %d\n", ret);
        return -1;
    }

    std::string u8String2;
    ret = UTF16LEToUTF8(u16String, u8String2);
    if (ret < 0) {
        printf("UTF16LEToUTF8 failed, ret: %d\n", ret);
        return -1;
    }

    return 0;
}
