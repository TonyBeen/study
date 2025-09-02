/*************************************************************************
    > File Name: test_mbedtls_rsa.cc
    > Author: hsz
    > Brief:
    > Created Time: 2025年07月15日 星期二 15时22分36秒
 ************************************************************************/


#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

#include <mbedtls/rsa.h>
#include <mbedtls/pk.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/error.h>

#define EXPONENT    65537
#define MIN(x, y) ((x) < (y) ? (x) : (y))

int32_t GenerateRSAKey(std::string &publicKey, std::string &privateKey, int32_t keyBits)
{
    size_t privateKeyLen = 0;
    size_t publicKeyLen = 0;
    int32_t status = 0;

    mbedtls_pk_context          pk;
    mbedtls_entropy_context     entropy;
    mbedtls_ctr_drbg_context    ctr_drbg;
    const char *pers = "rsa_gen_key";

    mbedtls_pk_init(&pk);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    do {
        status = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                (const uint8_t *)pers, strlen(pers));
        if (status != 0) {
            break; // Failed to seed the random number generator
        }
        status = mbedtls_pk_setup(&pk, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA));
        if (status != 0) {
            break; // Failed to setup the PK context
        }
        status = mbedtls_rsa_gen_key(mbedtls_pk_rsa(pk), mbedtls_ctr_drbg_random, &ctr_drbg, keyBits, EXPONENT);
        if (status != 0) {
            break; // Failed to generate RSA key
        }

        privateKey.resize(keyBits);
        status = mbedtls_pk_write_key_pem(&pk, (uint8_t *)privateKey.data(), privateKey.size());
        if (status != 0) {
            privateKey.clear(); // Clear private key on failure
            publicKey.clear(); // Clear public key on failure
            break; // Failed to write private key to PEM format
        }
        privateKeyLen = strlen(privateKey.c_str());
        privateKey.resize(privateKeyLen); // Resize to actual length

        publicKey.resize(keyBits);
        status = mbedtls_pk_write_pubkey_pem(&pk, (uint8_t *)publicKey.data(), publicKey.size());
        if (status != 0) {
            privateKey.clear(); // Clear private key on failure
            publicKey.clear(); // Clear public key on failure
            break; // Failed to write public key to PEM format
        }
        publicKeyLen = strlen(publicKey.c_str());
        publicKey.resize(publicKeyLen); // Resize to actual length
    } while (false);

    mbedtls_pk_free(&pk);
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    return status; // Success
}


int main(int argc, char **argv)
{
    std::string publicKey;
    std::string privateKey;
    int32_t keyBits = 2048; // Default key size
    GenerateRSAKey(publicKey, privateKey, keyBits);
    if (publicKey.empty() || privateKey.empty()) {
        printf("Failed to generate RSA keys.\n");
        return -1;
    }
    mbedtls_pk_context          _publicPKContext;
    mbedtls_pk_context          _privatePKContext;
    mbedtls_ctr_drbg_context    _ctrDrbg;
    mbedtls_rsa_context*        _publicRsaKey{};
    mbedtls_rsa_context*        _privateRsaKey{};

    mbedtls_ctr_drbg_init(&_ctrDrbg);
    int32_t status = 0;
    do {
        mbedtls_pk_init(&_publicPKContext);
        status = mbedtls_pk_parse_public_key(&_publicPKContext, (const uint8_t *)publicKey.data(), publicKey.size() + 1);
        if (status != 0) {
            break; // Failed to parse public key
        }
        _publicRsaKey = mbedtls_pk_rsa(_publicPKContext);

        mbedtls_pk_init(&_privatePKContext);
        status = mbedtls_pk_parse_key(&_privatePKContext, (const uint8_t *)privateKey.data(), privateKey.size() + 1,
                                       NULL, 0, mbedtls_ctr_drbg_random, NULL);
        if (status != 0) {
            break; // Failed to parse private key
        }
        _privateRsaKey = mbedtls_pk_rsa(_privatePKContext);

        mbedtls_rsa_set_padding(_publicRsaKey, MBEDTLS_RSA_PKCS_V15, MBEDTLS_MD_NONE);
        mbedtls_rsa_set_padding(_privateRsaKey, MBEDTLS_RSA_PKCS_V15, MBEDTLS_MD_NONE);
    } while (false);

    if (status) {
        char error_buf[100];
        mbedtls_strerror(status, error_buf, sizeof(error_buf));
        printf("Error: %s\n", error_buf);
        mbedtls_pk_free(&_publicPKContext);
        mbedtls_pk_free(&_privatePKContext);
        mbedtls_ctr_drbg_free(&_ctrDrbg);

        return status; // Return the error code if any operation failed
    }

    int32_t keySize = mbedtls_rsa_get_len(_privateRsaKey);
    int32_t blockSize = keySize - 11;
    printf("Public Key Size: %d bits, Block Size: %d bytes\n", keySize * 8, blockSize);
    std::vector<uint8_t> blockVec(keySize);
    uint8_t message[4999] = {0};
    size_t messageLen = sizeof(message);
    std::vector<uint8_t> encryptedData;
    std::string decryptedData;

    // sha-256
    mbedtls_md_context_t sha256_ctx;
    uint8_t sha256_hash[32] = {0};
    status = mbedtls_md_setup(&sha256_ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
    if (status != 0) {
        printf("Failed to setup SHA256\n");
        return -1;
    }
    status = mbedtls_md_starts(&sha256_ctx);
    if (status != 0) {
        printf("SHA256 start failed\n");
        return -1;
    }
    status = mbedtls_md_update(&sha256_ctx, message, messageLen);
    if (status != 0) {
        printf("SHA256 update failed\n");
        return -1;
    }
    status = mbedtls_md_finish(&sha256_ctx, sha256_hash);
    if (status != 0) {
        printf("SHA256 finish failed\n");
        return -1;
    }

    // private key encrypt
    int32_t status = mbedtls_rsa_pkcs1_sign(_privateRsaKey, mbedtls_ctr_drbg_random, &_ctrDrbg,
                                            MBEDTLS_MD_SHA256, 32, sha256_hash, blockVec.data());
    if (status != 0) {
        return status;
    }
    encryptedData.insert(encryptedData.end(), blockVec.begin(), blockVec.begin() + keySize);
    printf("Encrypted Data Size: %zu bytes\n", encryptedData.size());

    // public key decrypt
    keySize = mbedtls_rsa_get_len(_publicRsaKey);
    status = mbedtls_rsa_pkcs1_verify(_publicRsaKey, MBEDTLS_MD_SHA256, sizeof(sha256_hash), sha256_hash, encryptedData.data());
    printf("status = %d\n", status);
    if (status != 0) {
        char errBuf[512] = {0};
        mbedtls_strerror(status, errBuf, sizeof(errBuf));
        printf("mbedtls_rsa_pkcs1_verify error: [%d,%s]\n", status, errBuf);
        return status;
    }

    // for (size_t i = 0; i < messageLen; i += blockSize) {
    //     size_t blockLen = MIN((size_t)blockSize, messageLen - i);
    //     printf("blockLen: %zu, keySize: %d\n", blockLen, keySize);
    //     int32_t status = mbedtls_rsa_pkcs1_encrypt(_privateRsaKey, mbedtls_ctr_drbg_random, &_ctrDrbg,
    //                                                static_cast<int32_t>(blockLen), (const uint8_t *)&message[i], blockVec.data());
    //     if (status != 0) {
    //         return status;
    //     }
    //     encryptedData.insert(encryptedData.end(), blockVec.begin(), blockVec.begin() + keySize);
    // }
    // printf("Encrypted Data Size: %zu bytes\n", encryptedData.size());

    // // public key decrypt
    // keySize = mbedtls_rsa_get_len(_publicRsaKey);
    // for (size_t i = 0; i < encryptedData.size(); i += keySize) {
    //     size_t blockLen = MIN((size_t)keySize, messageLen - i);
    //     int32_t status = mbedtls_rsa_pkcs1_decrypt(_publicRsaKey, mbedtls_ctr_drbg_random, &_ctrDrbg,
    //                                                &blockLen, encryptedData.data(), blockVec.data(), blockVec.capacity());
    //     if (status != 0) {
    //         char errBuf[512] = {0};
    //         mbedtls_strerror(status, errBuf, sizeof(errBuf));
    //         printf("mbedtls_rsa_pkcs1_decrypt error: [%d,%s]\n", status, errBuf);
    //         return status;
    //     }
    //     decryptedData.append((const char *)blockVec.data(), blockLen);
    // }

    return 0;
}
