#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include <string>
#include <openssl/ssl.h>
#include "quic_crypto.h"

// BoringSSL QUIC 集成的“每连接状态”
struct QuicLevelKeys {
    const EVP_AEAD* aead = nullptr;      // AEAD 算法（本层统一）
    const EVP_MD*   md   = nullptr;      // 哈希（用于派生）

    // 写方向密钥材料
    uint8_t write_key[32] = {0};
    uint8_t write_iv[12]  = {0};
    uint8_t write_hp[32]  = {0};
    size_t  write_key_len = 0;
    size_t  write_hp_len  = 0;
    bool    write_ready   = false;

    // 读方向密钥材料
    uint8_t read_key[32] = {0};
    uint8_t read_iv[12]  = {0};
    uint8_t read_hp[32]  = {0};
    size_t  read_key_len = 0;
    size_t  read_hp_len  = 0;
    bool    read_ready   = false;
};

struct QuicConn {
    QuicLevelKeys level[ssl_encryption_application + 1];

    std::vector<uint8_t> handshake_out_initial;
    std::vector<uint8_t> handshake_out_hs;
    std::vector<uint8_t> handshake_out_app;

    uint64_t pn_initial = 0;
    uint64_t pn_hs = 0;
    uint64_t pn_app = 0;

    bool handshake_complete = false;
};

size_t select_cipher_params(const SSL_CIPHER* cipher,
                            const EVP_AEAD** out_aead,
                            const EVP_MD** out_md,
                            size_t* out_hp_len);

void attach_quic_method(SSL* ssl, QuicConn* conn);
bool tls_provide_crypto_and_step(SSL* ssl, enum ssl_encryption_level_t level,
                                 const uint8_t* data, size_t len);
enum ssl_encryption_level_t tls_current_write_level(SSL* ssl);
std::vector<uint8_t> make_demo_ping_payload();

// 枚举角色：影响 Initial 层读/写密钥的选择
enum class QuicRole { Client, Server };

// 安装 Initial 层的读/写密钥（AEAD_AES_128_GCM + SHA-256），基于固定 salt 与 client_dcid
bool install_initial_secrets(QuicConn* qc, QuicRole role,
                             const uint8_t* client_dcid, size_t dcid_len);

// 构造 ALPN wire-format
std::vector<uint8_t> build_alpn_wire_format(const std::vector<std::string>& protos);