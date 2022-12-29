/*************************************************************************
    > File Name: md5.cc
    > Author: hsz
    > Brief:
    > Created Time: Tue 21 Dec 2021 04:56:59 PM CST
 ************************************************************************/

#include <stdio.h>
#include <string.h>
#include <openssl/md2.h>
#include <openssl/md4.h>
#include <openssl/md5.h>
#include <openssl/sha.h>

int main()
{
    unsigned char in[] = "HSZ@1772747354";
    unsigned char out[128];

    size_t n;
    int i;

    n = strlen((const char *)in);

    MD5(in, n, out);    // 16byte
    printf("\n\nMD5 digest result :\n");
    for (i = 0; i < 16; i++)
        printf("0x%02x ", out[i]);

    SHA1(in, n, out);   // 20byte
    printf("\n\nSHA1 digest result :\n");
    for (i = 0; i < 20; i++)
        printf("0x%02x ", out[i]);

    SHA256(in, n, out); // 32byte
    printf("\n\nSHA256 digest result :\n");
    for (i = 0; i < 32; i++)
        printf("0x%02x ", out[i]);

    SHA512(in, n, out);     // 64byte
    printf("\n\nSHA512 digest result :\n");
    for (i = 0; i < 64; i++)
        printf("0x%02x ", out[i]);

    printf("\n");
    return 0;
}
