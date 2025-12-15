#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include <openssl/ssl.h>
#include <openssl/evp.h>

// 工具函数：TLS 1.3 HKDF-Expand-Label，用于从 traffic secret 派生 key/iv/hp
bool tls13_hkdf_expand_label(const EVP_MD* md,
                             const uint8_t* secret, size_t secret_len,
                             const char* label,
                             const uint8_t* context, size_t context_len,
                             uint8_t* out, size_t out_len);

// 从 QUIC traffic secret 派生 AEAD key、IV、Header Protection key
bool quic_derive_keys(const EVP_MD* md,
                      size_t aead_key_len, size_t iv_len, size_t hp_key_len,
                      const uint8_t* traffic_secret, size_t secret_len,
                      uint8_t* out_key, uint8_t* out_iv, uint8_t* out_hp);

// 使用 AES-ECB 生成 Header Protection 掩码（适用于 AES-GCM 套件）
bool quic_hp_mask_aes(const uint8_t* hp_key, size_t hp_key_len,
                      const uint8_t sample[16],
                      uint8_t out_mask[5]);

// 计算 AEAD nonce：nonce = iv XOR packet_number（本示例简化 PN 为 64-bit）
void make_nonce_from_pn(const uint8_t iv[12], uint64_t packet_number, uint8_t nonce[12]);

// AEAD 加解密示例（封装 EVP_AEAD_*），AAD 可选
bool aead_seal(const EVP_AEAD* aead,
               const uint8_t* key, size_t key_len,
               const uint8_t* nonce, size_t nonce_len,
               const uint8_t* aad, size_t aad_len,
               const uint8_t* plaintext, size_t pt_len,
               std::vector<uint8_t>& out_ciphertext);

bool aead_open(const EVP_AEAD* aead,
               const uint8_t* key, size_t key_len,
               const uint8_t* nonce, size_t nonce_len,
               const uint8_t* aad, size_t aad_len,
               const uint8_t* ciphertext, size_t ct_len,
               std::vector<uint8_t>& out_plaintext);

// 依据 QUIC v1：从固定 initial_salt 与 client DCID 派生 client/server initial secret（Hash=SHA-256时长度为32）
bool quic_derive_initial_secrets_sha256(const uint8_t* client_dcid, size_t dcid_len,
                                        uint8_t* client_initial /*32*/, uint8_t* server_initial /*32*/);