/*************************************************************************
    > File Name: ssl.cc
    > Author: hsz
    > Brief:
    > Created Time: Mon 20 Dec 2021 11:35:20 AM CST
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <iostream>
#include <openssl/evp.h>

using namespace std;

static EVP_ENCODE_CTX *gEncodeCtx = EVP_ENCODE_CTX_new();
static EVP_ENCODE_CTX *gDecodeCtx = EVP_ENCODE_CTX_new();

uint32_t GetEncodeBufSize(uint32_t len)
{
    uint32_t baseSize = 0;
    if (len > 0) {
        baseSize = len * 8 / 6 + len / 48;  // 加上len/48是因为每48字节加一个\n
        printf("baseSize = %u, len % 3 = %d\n", baseSize, (len % 3));
        switch (len % 3)
        {
        case 0:
            baseSize += 1;  // 结束符\n
            break;
        case 1:
            baseSize += 4;  // 补充的2个00(1byte) + ==(2byte) + 结束符\n
            break;
        case 2:
            baseSize += 3;  // 补充的1个00(1byte) + =(1byte) + 结束符\n
            break;
        default:
            assert(0);
            break;
        }
    }

    return baseSize + 1;    // 防止溢出
}

// 可以编码大小写字母，数字，-=+/*%@()/.,:#!
int encode(uint8_t *out, const uint8_t *str, const uint32_t &len)
{
    if (gEncodeCtx == nullptr) {
        return -1;
    }
    assert(str && len != 0);

    int guessLen = GetEncodeBufSize(len);
    printf("guess len = %d\n", guessLen);

    int32_t outLen, total;
    // openssl base64编码以48字节为一单位，以48*n个字节进行编码，剩余的字节数(小于48)则放入EVP_ENCODE_CTX::enc_data[48],
    // 为EVP_ENCODE_CTX::num赋值为剩余字节数
    EVP_EncodeUpdate(gEncodeCtx, out, &outLen, str, len);
    total = outLen;
    // 此函数作用是处理EVP_ENCODE_CTX::enc_data中的数据
    EVP_EncodeFinal(gEncodeCtx, out + total, &outLen);
    total += outLen;
    return total; // 返回值包括结尾符号\n
}

int decode(uint8_t *out, const uint8_t *in, const uint32_t &inLen)
{
    if (gEncodeCtx == nullptr) {
        return -1;
    }
    assert(in && inLen != 0);

    int32_t outLen, total;
    EVP_DecodeUpdate(gDecodeCtx, out, &outLen, in, inLen);
    total = outLen;
    EVP_DecodeFinal(gDecodeCtx, out + total, &outLen);
    total += outLen;
    return total;
}

int main(int argc, char **argv)
{
    if (gEncodeCtx == nullptr || gDecodeCtx == nullptr) {
        printf("gEncodeCtx == null || gDecodeCtx == null\n");
        return 0;
    }

    if (argc < 2) {
        printf("usage: %s \"string will encode\"\n", argv[0]);
        return 0;
    }
    const uint8_t *stringToEncode = (const uint8_t *)"china is so nb.-=+/*%@$#()/.,:# !";// (const uint8_t *)(argv[1]);
    const uint32_t len = strlen((char *)stringToEncode);
    EVP_EncodeInit(gEncodeCtx);
    EVP_DecodeInit(gDecodeCtx);
    EVP_EncodeSetFlag(gEncodeCtx, 1);

    printf("before encode: %s\n", stringToEncode);
    char out[1024] = {0};
    int outLen = encode((uint8_t *)out, stringToEncode, len);
    printf("outLen %d, after encode: %s\n", outLen, out);

    char decodeBuf[1024] = {0};
    outLen = decode((uint8_t *)decodeBuf, (uint8_t *)out, outLen);
    printf("outLen %d, after decode: %s\n", outLen, decodeBuf);

    EVP_ENCODE_CTX_free(gEncodeCtx);
    EVP_ENCODE_CTX_free(gDecodeCtx);
    return 0;
}
