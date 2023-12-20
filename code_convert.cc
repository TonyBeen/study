#include <stdio.h>
#include <iconv.h>
#include <string.h>
#include <string>
#include <assert.h>
#include <arpa/inet.h>

int code_convert(const char *from_charset, const char *to_charset, char *inbuf, size_t inlen,
                 char *outbuf, size_t outlen)
{
    iconv_t cd;
    char **pin = &inbuf;
    char **pout = &outbuf;

    cd = iconv_open(to_charset, from_charset);
    if (cd == (iconv_t)-1)
        return -1;

    memset(outbuf, 0, outlen);
    int ret = 0;
    if ( (ret = (int)iconv(cd, pin, &inlen, pout, &outlen)) == -1)
    {
        iconv_close(cd);
        return -1;
    }
    iconv_close(cd);
    return ret;
}

bool IsUTF8(const void *pBuffer, long size)
{
    bool IsUTF8 = true;
    unsigned char *start = (unsigned char *)pBuffer;
    unsigned char *end = (unsigned char *)pBuffer + size;
    while (start < end)
    {
        if (*start < 0x80) // (1000 0000): 值小于0x80的为ASCII字符
        {
            start++;
        }
        else if (*start < (0xC0)) // (1100 0000): 值介于0x80与0xC0之间的为无效UTF-8字符
        {
            IsUTF8 = false;
            break;
        }
        else if (*start < (0xE0)) // (1110 0000): 此范围内为2字节UTF-8字符
        {
            if (start >= end - 1)
                break;
            if ((start[1] & (0xC0)) != 0x80)
            {
                IsUTF8 = false;
                break;
            }
            start += 2;
        }
        else if (*start < (0xF0)) // (1111 0000): 此范围内为3字节UTF-8字符
        {
            if (start >= end - 2)
                break;
            if ((start[1] & (0xC0)) != 0x80 || (start[2] & (0xC0)) != 0x80)
            {
                IsUTF8 = false;
                break;
            }
            start += 3;
        }
        else
        {
            IsUTF8 = false;
            break;
        }
    }
    return IsUTF8;
}

#define IS_ANSII(code) (0x00 <  (uint8_t)(code) && (uint8_t)(code) < 0x80)		// ansii字符集
#define IS_2BYTE(code) (0xc0 <= (uint8_t)(code) && (uint8_t)(code) < 0xe0)		// 此范围内为2字节UTF-8字符
#define IS_3BYTE(code) (0xe0 <= (uint8_t)(code) && (uint8_t)(code) < 0xf0)		// 此范围内为3字节UTF-8字符
#define IS_4BYTE(code) (0xf0 <= (uint8_t)(code) && (uint8_t)(code) < 0xf8)		// 此范围内为4字节UTF-8字符

#define IS_UTF8_EXT(code) (0x80 <= (uint8_t)(code) && (uint8_t)(code) < 0xc0)	// 剩余的字符规则

bool isUTF8Encode(const std::string &str, uint32_t *len = nullptr)
{
    if (str.length() == 0) {
        return false;
    }
    const char *start = str.c_str();
    const char *end = start + str.length();
    bool isUtf8 = false;
    uint32_t nLen = 0;

    while (start < end) {
        if (IS_ANSII(*start)) {
            ++nLen;
            ++start;
        } else if (IS_2BYTE(*start)) {
            if ((start + 1) > end) { // 当是utf8时，检测到2字节编码头时后面会跟着一个字符
                return false;
            }
            if (IS_UTF8_EXT(*(start + 1))) {
                ++nLen;
                isUtf8 = true;
            }
            start += 2;
        } else if (IS_3BYTE(*start)) {
        if (start + 2 > end) { // 检测到3字节编码头时，后面会有两个字节的位置
                return false;
            }
            if (IS_UTF8_EXT(*(start + 1)) && IS_UTF8_EXT(*(start + 2))) {
                ++nLen;
                isUtf8 = true;
            }
            start += 3;
        } else if (IS_4BYTE(*start)) {
            if (start + 3 > end) {
                return false;
            }
            if (IS_UTF8_EXT(*(start + 1)) && IS_UTF8_EXT(*(start + 2)) && IS_UTF8_EXT(*(start + 3))) {
                ++nLen;
            }
            start += 4;
        } else {
            isUtf8 = false;
            break;
        }
    }

    if (len && isUtf8) {
        *len = nLen;
    }
    return isUtf8;
}

#define IS_ASCII(code)			(0x00 <  (uint8_t)(code) && (uint8_t)(code) <  0x80)
#define GBK_FIRST_BYTE(code)	(0x80 <  (uint8_t)(code) && (uint8_t)(code) <  0xFF)
#define GBK_SECOND_BYTE(code)	(0x40 <= (uint8_t)(code) && (uint8_t)(code) <= 0xFE && (uint8_t)(code) != 0x7F)

bool IsGBK(const std::string &str)
{
	const uint8_t *begin = (const uint8_t *)(str.c_str());
	size_t length = str.length();

	if (!length)
	{
		return false;
	}

	bool result = true;
	for (size_t i = 0; i < length; )
	{
		if (IS_ASCII(begin[i])) // 如果全是ASCII字符串，也可以认为其采用GBK编码
		{
			++i;
		}
		else if (GBK_FIRST_BYTE(begin[i]) &&
				 GBK_SECOND_BYTE(begin[i + 1]))
		{
			i += 2;
		}
		else
		{
			result = false;
			break;
		}
	}

	return result;
}

#define swap16(arg) (((arg & 0xff00) >> 8) | ((arg & 0x00ff) << 8))
#define swap32(arg) \
    (((arg & 0xff000000) >> 24) | ((arg & 0xff0000) >> 8) | (arg & 0xff00) << 8 | ((arg & 0xff) << 24))

int main()
{
    char out[128] = {0};
    const char *in = "你好안녕하세요";

    printf("isUTF8Encode(%s) %d\n", in, isUTF8Encode(in));
    int ret = code_convert("utf-8", "gbk", (char *)in, strlen(in), out, sizeof(out));
    for (size_t i = 0; i < strlen(out); ++i) {
        printf("0x%x\t", (uint8_t)out[i]);
    }
    printf("\n");

    uint16_t u16 = 0x1122;
    printf("0x%x == 0x%x\n", swap16(u16), htons(u16));
    assert(swap16(u16) == htons(u16));
    uint32_t u32 = 0x11223344;
    printf("0x%x == 0x%x\n", swap32(u32), htonl(u32));
    assert(swap32(u32) == htonl(u32));

    return 0;
}