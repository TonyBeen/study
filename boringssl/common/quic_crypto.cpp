#include "quic_crypto.h"
#include <cstring>
#include <openssl/hkdf.h>
#include <openssl/aead.h>  // BoringSSL EVP_AEAD 接口

static bool build_tls13_hkdf_label(const char* label,
                                   const uint8_t* context, size_t context_len,
                                   uint16_t out_len,
                                   uint8_t* out, size_t* out_len_io) {
    const char* tls13 = "tls13 ";
    const size_t label_len = std::strlen(label);
    const size_t full_label_len = std::strlen(tls13) + label_len;
    const size_t needed = 2 + 1 + full_label_len + 1 + context_len;
    if (*out_len_io < needed) return false;

    uint8_t* p = out;
    p[0] = (uint8_t)(out_len >> 8);
    p[1] = (uint8_t)(out_len & 0xff);
    p += 2;

    p[0] = (uint8_t)full_label_len; p += 1;
    std::memcpy(p, tls13, std::strlen(tls13)); p += std::strlen(tls13);
    std::memcpy(p, label, label_len); p += label_len;

    p[0] = (uint8_t)context_len; p += 1;
    if (context_len) { std::memcpy(p, context, context_len); p += context_len; }

    *out_len_io = needed;
    return true;
}

bool tls13_hkdf_expand_label(const EVP_MD* md,
                             const uint8_t* secret, size_t secret_len,
                             const char* label,
                             const uint8_t* context, size_t context_len,
                             uint8_t* out, size_t out_len) {
    uint8_t info[256]; size_t info_len = sizeof(info);
    if (!build_tls13_hkdf_label(label, context, context_len, (uint16_t)out_len, info, &info_len)) {
        return false;
    }

    // 直接使用 BoringSSL 的 HKDF_expand
    return HKDF_expand(out, out_len, md, secret, secret_len, info, info_len);
}

bool quic_derive_keys(const EVP_MD* md,
                      size_t aead_key_len, size_t iv_len, size_t hp_key_len,
                      const uint8_t* traffic_secret, size_t secret_len,
                      uint8_t* out_key, uint8_t* out_iv, uint8_t* out_hp) {
    if (!tls13_hkdf_expand_label(md, traffic_secret, secret_len, "quic key", nullptr, 0, out_key, aead_key_len))
        return false;
    if (!tls13_hkdf_expand_label(md, traffic_secret, secret_len, "quic iv", nullptr, 0, out_iv, iv_len))
        return false;
    if (!tls13_hkdf_expand_label(md, traffic_secret, secret_len, "quic hp", nullptr, 0, out_hp, hp_key_len))
        return false;
    return true;
}

bool quic_hp_mask_aes(const uint8_t* hp_key, size_t hp_key_len,
                      const uint8_t sample[16],
                      uint8_t out_mask[5]) {
    const EVP_CIPHER* cipher = (hp_key_len == 16) ? EVP_aes_128_ecb() : EVP_aes_256_ecb();
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return false;

    bool ok = false;
    int outlen = 0;
    uint8_t block[16];

    if (EVP_EncryptInit_ex(ctx, cipher, nullptr, hp_key, nullptr) != 1) goto out;
    EVP_CIPHER_CTX_set_padding(ctx, 0);

    if (EVP_EncryptUpdate(ctx, block, &outlen, sample, 16) != 1) goto out;
    if (outlen != 16) goto out;

    std::memcpy(out_mask, block, 5);
    ok = true;
out:
    EVP_CIPHER_CTX_free(ctx);
    OPENSSL_cleanse(block, sizeof(block));
    return ok;
}

void make_nonce_from_pn(const uint8_t iv[12], uint64_t packet_number, uint8_t nonce[12]) {
    std::memcpy(nonce, iv, 12);
    for (int i = 0; i < 8; ++i) {
        nonce[12 - 8 + i] ^= (uint8_t)((packet_number >> (8*(7 - i))) & 0xff);
    }
}

bool aead_seal(const EVP_AEAD* aead,
               const uint8_t* key, size_t key_len,
               const uint8_t* nonce, size_t nonce_len,
               const uint8_t* aad, size_t aad_len,
               const uint8_t* plaintext, size_t pt_len,
               std::vector<uint8_t>& out_ciphertext) {
    EVP_AEAD_CTX ctx;
    if (!EVP_AEAD_CTX_init(&ctx, aead, key, key_len, EVP_AEAD_max_tag_len(aead), nullptr)) {
        return false;
    }
    out_ciphertext.resize(pt_len + EVP_AEAD_max_overhead(aead));
    size_t out_len = 0;
    const int ok = EVP_AEAD_CTX_seal(&ctx,
                                     out_ciphertext.data(), &out_len, out_ciphertext.size(),
                                     nonce, nonce_len,
                                     plaintext, pt_len,
                                     aad, aad_len);
    EVP_AEAD_CTX_cleanup(&ctx);
    if (!ok) return false;
    out_ciphertext.resize(out_len);
    return true;
}

bool aead_open(const EVP_AEAD* aead,
               const uint8_t* key, size_t key_len,
               const uint8_t* nonce, size_t nonce_len,
               const uint8_t* aad, size_t aad_len,
               const uint8_t* ciphertext, size_t ct_len,
               std::vector<uint8_t>& out_plaintext) {
    EVP_AEAD_CTX ctx;
    if (!EVP_AEAD_CTX_init(&ctx, aead, key, key_len, EVP_AEAD_max_tag_len(aead), nullptr)) {
        return false;
    }
    out_plaintext.resize(ct_len);
    size_t out_len = 0;
    const int ok = EVP_AEAD_CTX_open(&ctx,
                                     out_plaintext.data(), &out_len, out_plaintext.size(),
                                     nonce, nonce_len,
                                     ciphertext, ct_len,
                                     aad, aad_len);
    EVP_AEAD_CTX_cleanup(&ctx);
    if (!ok) return false;
    out_plaintext.resize(out_len);
    return true;
}

bool quic_derive_initial_secrets_sha256(const uint8_t* client_dcid, size_t dcid_len,
                                        uint8_t* client_initial, uint8_t* server_initial) {
    static const uint8_t initial_salt[20] = {
        0x38,0x76,0x2c,0xf7,0xf5,0x59,0x34,0xb3,0x4d,0x17,
        0x9a,0xe6,0xa4,0xc8,0x0c,0xad,0xcc,0xbb,0x7f,0x0a
    }; // RFC 9001 v1
    const EVP_MD* md = EVP_sha256();
    uint8_t initial_secret[EVP_MAX_MD_SIZE];
    size_t mdlen = EVP_MD_size(md);

    // 使用 BoringSSL 的 HKDF_extract 计算 PRK
    if (!HKDF_extract(initial_secret, &mdlen, md,
                      client_dcid, dcid_len,
                      initial_salt, sizeof(initial_salt))) {
        return false;
    }

    bool ok = true;
    if (!tls13_hkdf_expand_label(md, initial_secret, mdlen, "client in", nullptr, 0,
                                 client_initial, mdlen)) ok = false;
    if (!tls13_hkdf_expand_label(md, initial_secret, mdlen, "server in", nullptr, 0,
                                 server_initial, mdlen)) ok = false;

    OPENSSL_cleanse(initial_secret, sizeof(initial_secret));
    return ok;
}