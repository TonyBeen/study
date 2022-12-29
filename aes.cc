/*************************************************************************
    > File Name: aes.cc
    > Author: hsz
    > Brief:
    > Created Time: Sun 26 Dec 2021 03:06:38 PM CST
 ************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <iostream>
#include <openssl/aes.h>
#include <openssl/evp.h>

using namespace std;

void printfHex(const uint8_t *src, uint32_t len, const char *perfix)
{
    if (perfix) {
        printf("%s\n", perfix);
    }
    for (int i = 0; i < len; ++i) {
        if (i != 0 && i % 16 == 0) {
            printf("\n");
        }
        printf("%02x ", src[i]);
    }
    printf("\n");
}

void AesEncrypt(unsigned char* pchIn, int nInLen, unsigned char *ciphertext, int &ciphertext_len, unsigned char * pchKey)
{
    EVP_CIPHER_CTX *en;
    en = EVP_CIPHER_CTX_new();
    assert(en);

    EVP_CIPHER_CTX_init(en);
    const EVP_CIPHER *cipher_type;
    unsigned char *passkey, *passiv, *plaintxt;
    unsigned char *plaintext = NULL;
 
    unsigned char iv[] = {0};
 
    cipher_type = EVP_aes_128_ecb();
    EVP_EncryptInit_ex(en, cipher_type, NULL, pchKey, iv);
 
    //当长度正好为16字节的倍数时，同样需要padding
    static const int MAX_PADDING_LEN = 16;
 
    EVP_CIPHER_CTX_set_padding(en, 1);
    if(!EVP_EncryptInit_ex(en, NULL, NULL, NULL, NULL))
    {
        printf("ERROR in EVP_EncryptInit_ex \n");
        return;
    }
 
    int bytes_written = 0;
    ciphertext_len = 0;
    if(!EVP_EncryptUpdate(en, ciphertext, &bytes_written, (unsigned char *) pchIn, nInLen))
    {
        printf("ERROR in EVP_EncryptUpdate \n");
        return;
    }
    ciphertext_len += bytes_written;
 
    if(!EVP_EncryptFinal_ex(en, ciphertext + bytes_written, &bytes_written))
    {
        printf("ERROR in EVP_EncryptFinal_ex \n");
        return;
    }
    ciphertext_len += bytes_written;
    ciphertext[ciphertext_len] = 0;
 
    EVP_CIPHER_CTX_cleanup(en);
 
    EVP_CIPHER_CTX_free(en);
}

void AesDecrypt(unsigned char* pchInPut, int nInl, unsigned char *pchOutPut, unsigned char *pchKey)
{
    unsigned char achIv[8];
    EVP_CIPHER_CTX *decryptCtx;
    decryptCtx = EVP_CIPHER_CTX_new();
    assert(decryptCtx);

    EVP_CIPHER_CTX_init(decryptCtx);
 
    EVP_DecryptInit_ex(decryptCtx, EVP_aes_128_ecb(), NULL, pchKey, achIv);
    int nLen = 0;
    int nOutl = 0;
 
    EVP_DecryptUpdate(decryptCtx, pchOutPut+nLen, &nOutl, pchInPut+nLen, nInl);
    nLen += nOutl;
 
    EVP_DecryptFinal_ex(decryptCtx, pchOutPut+nLen, &nOutl); 
    nLen+=nOutl;
    pchOutPut[nLen]=0;
    EVP_CIPHER_CTX_cleanup(decryptCtx);
 
    EVP_CIPHER_CTX_free(decryptCtx);
}

int main(int argc, char **argv)
{
    static const uint16_t bufsize = 1024;
    uint8_t in[bufsize] = {0};
    uint8_t out[bufsize] = {0};

    printf("AES encode!\n");
    printf("please input something you want to encrypt[Press enter to end and at most 1024 bytes]:\n");
    scanf("%[^\n]", in);
    printf("Check your input: %s\n", in);

    int encodeSize = 0;
    int decodeSize = 0;
    uint8_t key[16] = {'1', '2', '3', '4', '5', '6'};

    AesEncrypt(in, strlen((char *)in), out, encodeSize, key);
    printf("encode size = %d\n", encodeSize);
    printfHex(out, encodeSize, "after aes encrypt");
    
    memset(in, 0, bufsize);
    memcpy(in, out, encodeSize);
    memset(out, 0, bufsize);
    AesDecrypt(in, encodeSize, out, key);
    printf("after aes decrypt:\n%s\n", out);

    return 0;
}
