/*************************************************************************
    > File Name: convert.cxx
    > Author: hsz
    > Brief:
    > Created Time: Thu 28 Jul 2022 02:35:51 PM CST
 ************************************************************************/

#ifndef __CODE_CONVERT_H__
#define __CODE_CONVERT_H__

#include <string>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#elif defined(__linux__)
#include <locale.h>
#endif

using namespace std;

// gbk字符串srcStr 转换为utf8字符串utfStr ，字符串utfStr的缓存最大大小 maxUtfStrlen
//失败返回-1,成功返回大于0 ，maxUtfStrlen的大小至少是源字符串有效长度大小2倍加1
inline int gbk2utf8(char *utfStr, size_t maxUtfStrlen, const char *srcStr)
{
    if (!srcStr || !utfStr)
    {
        printf("Bad Parameter\n");
        return -1;
    }
#if defined(_WIN32) || defined(_WIN64)
    int len = MultiByteToWideChar(CP_ACP, 0, (LPCCH)srcStr, -1, NULL, 0);
    unsigned short *strUnicode = new unsigned short[len + 1];
    memset(strUnicode, 0, len * 2 + 2);
    MultiByteToWideChar(CP_ACP, 0, (LPCCH)srcStr, -1, (LPWSTR)strUnicode, len);
    len = WideCharToMultiByte(CP_UTF8, 0, (LPWSTR)strUnicode, -1, NULL, 0, NULL, NULL);
    if (len > (int)maxUtfStrlen)
    {
        printf("Dst Str memory not enough\n");
        delete[] strUnicode;
        return -1;
    }
    WideCharToMultiByte(CP_UTF8, 0, (LPWSTR)strUnicode, -1, utfStr, len, NULL, NULL);
    delete[] strUnicode;
    return len;
#else
    // 首先先将gbk编码转换为unicode编码
    if (NULL == setlocale(LC_ALL, "zh_CN.gbk")) //设置转换为unicode前的码,当前为gbk编码
    {
        printf("参数有错误\n");
        return -1;
    }

    int unicodeLen = mbstowcs(NULL, srcStr, 0); //计算转换后的长度
    if (unicodeLen <= 0)
    {
        printf("不能转换!!!unicodeLen:(%d)\n", unicodeLen);
        return -1;
    }
    wchar_t *unicodeStr = (wchar_t *)calloc(sizeof(wchar_t), unicodeLen + 1);
    mbstowcs(unicodeStr, srcStr, strlen(srcStr)); //将gbk转换为unicode
    //将unicode编码转换为utf8编码
    if (NULL == setlocale(LC_ALL, "zh_CN.utf8")) //设置unicode转换后的码,当前为utf8
    {
        printf("参数有错误\n");
        free(unicodeStr);
        return -1;
    }
    int utfLen = wcstombs(NULL, unicodeStr, 0); //计算转换后的长度
    if (utfLen <= 0)
    {
        printf("不能转换!!!utfLen:(%d)\n", utfLen);
        free(unicodeStr);
        return -1;
    }
    else if (utfLen >= (int)maxUtfStrlen) //判断空间是否足够
    {
        printf("Dst Str memory not enough\n");
        free(unicodeStr);
        return -1;
    }
    wcstombs(utfStr, unicodeStr, utfLen);
    utfStr[utfLen] = 0; //添加结束符
    free(unicodeStr);
    return utfLen;
#endif
}

// gbk字符串srcStr 转换为utf8字符串target
inline int gbk2utf8(std::string &target, const char *srcStr)
{
    if (!srcStr)
    {
        assert(false && "string is empty");
    }
    int tarLen = (int)strlen(srcStr) * 2 + 1;
    char *tarStr = new char[tarLen];
    gbk2utf8(tarStr, tarLen - 1, srcStr);
    target = tarStr;
    delete[] tarStr;
    return tarLen;
}

/**
 * @brief 将gbk编码的字符串转化为utf-8
 * 
 * @param out 输出utf-8字符串
 * @param in 输入gbk字符串
 * @return int 失败返回负值，成功返回长度
 */
int GBK2UTF8(std::string &out, const std::string &in)
{
    static const char *fromCode = "GBK";
    static const char *toCode = "UTF-8";

    

}

#endif
