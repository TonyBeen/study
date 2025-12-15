#include "boringssl_glue.h"
#include <openssl/err.h>
#include <cstdio>
#include <cstring>

static size_t aead_key_len_from_cipher(const SSL_CIPHER* cipher,
                                       const EVP_AEAD** out_aead,
                                       const EVP_MD** out_md,
                                       size_t* hp_len) {
    const int id = SSL_CIPHER_get_id(cipher);
    switch (id) {
        case TLS1_3_CK_AES_128_GCM_SHA256:
            *out_aead = EVP_aead_aes_128_gcm();
            *out_md   = EVP_sha256();
            *hp_len   = 16;
            return 16;
        case TLS1_3_CK_CHACHA20_POLY1305_SHA256:
            *out_aead = EVP_aead_chacha20_poly1305();
            *out_md   = EVP_sha256();
            *hp_len   = 32;
            return 32;
        case TLS1_3_CK_AES_256_GCM_SHA384:
            *out_aead = EVP_aead_aes_256_gcm();
            *out_md   = EVP_sha384();
            *hp_len   = 32;
            return 32;
        default:
            return 0;
    }
}

size_t select_cipher_params(const SSL_CIPHER* cipher,
                            const EVP_AEAD** out_aead,
                            const EVP_MD** out_md,
                            size_t* out_hp_len) {
    return aead_key_len_from_cipher(cipher, out_aead, out_md, out_hp_len);
}

static QuicConn* get_conn(SSL* ssl) {
    return static_cast<QuicConn*>(SSL_get_app_data(ssl));
}

static void quic_set_secret_dir(SSL* ssl, enum ssl_encryption_level_t level,
                                const SSL_CIPHER* cipher,
                                const uint8_t* secret, size_t secret_len,
                                bool is_write) {
    QuicConn* qc = get_conn(ssl);
    const EVP_AEAD* aead = nullptr;
    const EVP_MD* md = nullptr;
    size_t hp_len = 0;
    const size_t key_len = select_cipher_params(cipher, &aead, &md, &hp_len);
    if (key_len == 0) { std::fprintf(stderr, "Unsupported TLS1.3 cipher\n"); return; }

    uint8_t key[32], iv[12], hp[32];
    if (!quic_derive_keys(md, key_len, sizeof(iv), hp_len, secret, secret_len, key, iv, hp)) {
        std::fprintf(stderr, "derive keys failed\n"); return;
    }

    QuicLevelKeys& L = qc->level[level];
    L.aead = aead;
    L.md   = md;
    if (is_write) {
        L.write_key_len = key_len; L.write_hp_len = hp_len;
        std::memcpy(L.write_key, key, key_len);
        std::memcpy(L.write_iv,  iv,  sizeof(iv));
        std::memcpy(L.write_hp,  hp,  hp_len);
        L.write_ready = true;
    } else {
        L.read_key_len = key_len; L.read_hp_len = hp_len;
        std::memcpy(L.read_key, key, key_len);
        std::memcpy(L.read_iv,  iv,  sizeof(iv));
        std::memcpy(L.read_hp,  hp,  hp_len);
        L.read_ready = true;
    }

    OPENSSL_cleanse(key, sizeof(key));
    OPENSSL_cleanse(iv, sizeof(iv));
    OPENSSL_cleanse(hp, sizeof(hp));
    std::fprintf(stderr, "[QUIC] %s_secret level=%d key_len=%zu hp_len=%zu\n",
                 is_write ? "write" : "read", (int)level, key_len, hp_len);
}

static int set_read_secret(SSL* ssl, enum ssl_encryption_level_t level,
                            const SSL_CIPHER* cipher, const uint8_t* secret, size_t secret_len) {
    printf("[QUIC] set_read_secret level=%d\n", (int)level);
    quic_set_secret_dir(ssl, level, cipher, secret, secret_len, /*is_write=*/false);
    return 1;
}
static int set_write_secret(SSL* ssl, enum ssl_encryption_level_t level,
                             const SSL_CIPHER* cipher, const uint8_t* secret, size_t secret_len) {
    printf("[QUIC] set_write_secret level=%d\n", (int)level);
    quic_set_secret_dir(ssl, level, cipher, secret, secret_len, /*is_write=*/true);
    return 1;
}

static int add_handshake_data(SSL* ssl, enum ssl_encryption_level_t level,
                              const uint8_t* data, size_t len) {
    QuicConn* qc = get_conn(ssl);
    std::fprintf(stderr, "[QUIC] add_handshake_data level=%d len=%zu\n", (int)level, len);
    switch (level) {
        case ssl_encryption_initial:
            qc->handshake_out_initial.insert(qc->handshake_out_initial.end(), data, data + len);
            break;
        case ssl_encryption_handshake:
            qc->handshake_out_hs.insert(qc->handshake_out_hs.end(), data, data + len);
            break;
        case ssl_encryption_application:
            qc->handshake_out_app.insert(qc->handshake_out_app.end(), data, data + len);
            break;
        default:
            break;
    }
    return 1;
}

static int flush_flight(SSL* /*ssl*/) {
    std::fprintf(stderr, "[QUIC] flush_flight\n");
    return 1;
}

static int send_alert(SSL* /*ssl*/, enum ssl_encryption_level_t level, uint8_t alert) {
    std::fprintf(stderr, "[QUIC] send_alert level=%d alert=%u\n", (int)level, alert);
    return 1;
}

static const SSL_QUIC_METHOD quic_method = {
    set_read_secret,
    set_write_secret,
    add_handshake_data,
    flush_flight,
    send_alert,
};

void attach_quic_method(SSL* ssl, QuicConn* conn) {
    SSL_set_app_data(ssl, conn);
    SSL_set_quic_method(ssl, &quic_method);
}

bool tls_provide_crypto_and_step(SSL* ssl, enum ssl_encryption_level_t level,
                                 const uint8_t* data, size_t len) {
    if (!SSL_provide_quic_data(ssl, level, data, len)) {
        std::fprintf(stderr, "[QUIC] SSL_provide_quic_data failed\n");
        ERR_print_errors_fp(stderr);
        return false;
    }
    const int r = SSL_do_handshake(ssl);
    if (r == 1) {
        QuicConn* qc = get_conn(ssl);
        qc->handshake_complete = true;
        std::fprintf(stderr, "[QUIC] TLS handshake complete\n");
        return true;
    }
    const int err = SSL_get_error(ssl, r);
    if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
        return true;
    }
    std::fprintf(stderr, "[QUIC] SSL_do_handshake error=%d\n", err);
    ERR_print_errors_fp(stderr);
    return false;
}

enum ssl_encryption_level_t tls_current_write_level(SSL* ssl) {
    return SSL_quic_write_level(ssl);
}

std::vector<uint8_t> make_demo_ping_payload() {
    const char* msg = "PING";
    return std::vector<uint8_t>(msg, msg + std::strlen(msg));
}

std::vector<uint8_t> build_alpn_wire_format(const std::vector<std::string>& protos) {
    std::vector<uint8_t> out;
    for (const auto& p : protos) {
        if (p.empty() || p.size() > 255) continue;
        out.push_back(static_cast<uint8_t>(p.size()));
        out.insert(out.end(), p.begin(), p.end());
    }
    return out;
}

bool install_initial_secrets(QuicConn* qc, QuicRole role,
                             const uint8_t* client_dcid, size_t dcid_len) {
    uint8_t client_initial[32], server_initial[32];
    if (!quic_derive_initial_secrets_sha256(client_dcid, dcid_len, client_initial, server_initial)) {
        std::fprintf(stderr, "[QUIC] derive initial secrets failed\n");
        return false;
    }
    QuicLevelKeys& L = qc->level[ssl_encryption_initial];
    L.aead = EVP_aead_aes_128_gcm();
    L.md   = EVP_sha256();

    auto install_dir = [&](bool is_write, const uint8_t* sec, size_t sec_len) {
        uint8_t key[32], iv[12], hp[32];
        const size_t key_len = 16, hp_len = 16;
        if (!quic_derive_keys(L.md, key_len, sizeof(iv), hp_len, sec, sec_len, key, iv, hp)) return false;
        if (is_write) {
            L.write_key_len = key_len; L.write_hp_len = hp_len;
            std::memcpy(L.write_key, key, key_len);
            std::memcpy(L.write_iv,  iv,  sizeof(iv));
            std::memcpy(L.write_hp,  hp,  hp_len);
            L.write_ready = true;
        } else {
            L.read_key_len = key_len; L.read_hp_len = hp_len;
            std::memcpy(L.read_key, key, key_len);
            std::memcpy(L.read_iv,  iv,  sizeof(iv));
            std::memcpy(L.read_hp,  hp,  hp_len);
            L.read_ready = true;
        }
        OPENSSL_cleanse(key, sizeof(key));
        OPENSSL_cleanse(iv, sizeof(iv));
        OPENSSL_cleanse(hp, sizeof(hp));
        return true;
    };

    if (role == QuicRole::Client) {
        // 客户端：写=client_initial，读=server_initial
        if (!install_dir(true,  client_initial, sizeof(client_initial))) return false;
        if (!install_dir(false, server_initial, sizeof(server_initial))) return false;
    } else {
        // 服务端：写=server_initial，读=client_initial
        if (!install_dir(true,  server_initial, sizeof(server_initial))) return false;
        if (!install_dir(false, client_initial, sizeof(client_initial))) return false;
    }

    OPENSSL_cleanse(client_initial, sizeof(client_initial));
    OPENSSL_cleanse(server_initial, sizeof(server_initial));
    std::fprintf(stderr, "[QUIC] Initial secrets installed (role=%s)\n",
                 role == QuicRole::Client ? "client" : "server");
    return true;
}