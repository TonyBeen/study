/*************************************************************************
    > File Name: server.cpp
    > Author: hsz
    > Brief:
    > Created Time: Mon 15 Dec 2025 03:45:23 PM CST
 ************************************************************************/

#include "common/udp_socket.h"
#include "common/boringssl_glue.h"
#include "common/quic_crypto.h"
#include "common/quic_bp_builder.h"
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <vector>
#include <cstdio>
#include <cstring>

// 帧类型
enum : uint8_t {
    FT_CRYPTO = 0x01,
    FT_PING   = 0x02,
};

// 加密层枚举与 ssl_encryption_* 对齐
static enum ssl_encryption_level_t enc_from_u8(uint8_t v) {
    switch (v) {
        case 0: return ssl_encryption_initial;
        case 1: return ssl_encryption_early_data;
        case 2: return ssl_encryption_handshake;
        case 3: return ssl_encryption_application;
        default: return ssl_encryption_initial;
    }
}

static uint8_t u8_from_enc(enum ssl_encryption_level_t lvl) {
    switch (lvl) {
        case ssl_encryption_initial:     return 0;
        case ssl_encryption_early_data:  return 1;
        case ssl_encryption_handshake:   return 2;
        case ssl_encryption_application: return 3;
        default: return 0;
    }
}

int main() {
    // 初始化 BoringSSL
    SSL_library_init();
    ERR_load_crypto_strings();
    ERR_load_SSL_strings();

    // 创建 TLS 上下文（服务端）
    const SSL_METHOD* method = TLS_server_method();
    SSL_CTX* ctx = SSL_CTX_new(method);
    if (!ctx) { std::fprintf(stderr, "SSL_CTX_new failed\n"); return 1; }

    // 仅启用 TLS1.3（可选）
    SSL_CTX_set_min_proto_version(ctx, TLS1_3_VERSION);
    SSL_CTX_set_max_proto_version(ctx, TLS1_3_VERSION);

    // 加载证书与私钥
    SSL_CTX_use_certificate_file(ctx, "../server.crt", SSL_FILETYPE_PEM);
    SSL_CTX_use_PrivateKey_file(ctx, "../server.key", SSL_FILETYPE_PEM);
    if (!SSL_CTX_check_private_key(ctx)) {
        std::fprintf(stderr, "Private key does not match the certificate public key\n");
        SSL_CTX_free(ctx);
        return 1;
    }

    SSL* ssl = SSL_new(ctx);
    if (!ssl) { std::fprintf(stderr, "SSL_new failed\n"); return 1; }

    // 设置接受状态：作为服务端
    SSL_set_accept_state(ssl);

    // 服务端注册 ALPN 选择回调：从客户端提供的列表中选择我们支持的协议（例如 "h3"）
    auto alpn_select_cb = [](SSL* /*ssl*/,
                             const unsigned char** out, unsigned char* outlen,
                             const unsigned char* in, unsigned int inlen, void* /*arg*/) -> int {
        // 遍历客户端的 wire-format 列表，寻找 "h3"
        const char* want = "h3";
        size_t want_len = std::strlen(want);
        unsigned int i = 0;
        while (i < inlen) {
            unsigned int len = in[i];
            ++i;
            if (i + len > inlen) break;
            if (len == want_len && std::memcmp(in + i, want, len) == 0) {
                *out = in + i;
                *outlen = (unsigned char)len;
                return SSL_TLSEXT_ERR_OK;
            }
            i += len;
        }
        // 没有找到匹配的 ALPN
        return SSL_TLSEXT_ERR_NOACK;
    };
    SSL_CTX_set_alpn_select_cb(ctx, alpn_select_cb, nullptr);

    // 绑定 QUIC 方法与连接状态
    QuicConn qc;
    attach_quic_method(ssl, &qc);

    // 携带 QUIC 传输参数
    auto tp = build_min_quic_transport_params();
    if (!tp.empty()) {
        SSL_set_quic_transport_params(ssl, tp.data(), tp.size());
    } else {
        std::fprintf(stderr, "[Server] TP build failed\n");
    }

    // 安装 Initial 密钥（与客户端相同的 client_dcid）
    static const uint8_t kClientDCID[] = { 0x83,0x94,0xc8,0xf0,0x3e,0x51,0x57,0x08 };
    if (!install_initial_secrets(&qc, QuicRole::Server, kClientDCID, sizeof(kClientDCID))) {
        std::fprintf(stderr, "[Server] install_initial_secrets failed\n");
        return 1;
    }

    // UDP 服务器侦听
    UdpSocket sock;
    if (!sock.bind("0.0.0.0", 9000)) {
        std::fprintf(stderr, "bind failed\n"); return 1;
    }
    std::fprintf(stderr, "Server listening on 0.0.0.0:9000\n");

    // 简单事件循环：接收包 => 解析 => 投递给 TLS => 发送响应
    while (true) {
        auto r = sock.recv();
        printf("[Server] Received packet len=%zu from %s:%u\n", r ? r->data.size() : 0,
               r ? r->peer_host.c_str() : "N/A", r ? r->peer_port : 0);
        if (!r) continue;

        const auto& data = r->data;
        if (data.size() < 1 + 1 + 8 + 2) {
            continue;
        }

        uint8_t ft = data[0];
        uint8_t el = data[1];
        uint64_t pn = 0;
        std::memcpy(&pn, &data[2], 8);
        uint16_t plen = 0;
        std::memcpy(&plen, &data[10], 2);
        if (data.size() < 12 + plen) continue;
        const uint8_t* payload = &data[12];

        printf("[Server] Frame type=%u, enc_level=%u, pn=%llu, payload_len=%u\n",
               ft, el, (unsigned long long)pn, plen);

        if (ft == FT_CRYPTO) {
            // 根据层选择读密钥，解密得到 CRYPTO 片段，并投递给 TLS
            enum ssl_encryption_level_t lvl = enc_from_u8(el);
            QuicLevelKeys& L = qc.level[lvl];
            if (!L.read_ready) {
                std::fprintf(stderr, "[Server] Level %d not ready for read\n", (int)lvl);
                continue;
            }
            uint8_t nonce[12]; make_nonce_from_pn(L.read_iv, pn, nonce);
            std::vector<uint8_t> plaintext;
            if (!aead_open(L.aead, L.read_key, L.read_key_len, nonce, sizeof(nonce), nullptr, 0, payload, plen, plaintext)) {
                std::fprintf(stderr, "[Server] AEAD open failed\n");
                continue;
            }
            bool ok = tls_provide_crypto_and_step(ssl, lvl, plaintext.data(), plaintext.size());
            if (!ok) {
                // 会打印 SSL 错误；如果这里失败，就不会有握手层 add_handshake_data
                continue;
            }

            // 发送 TLS 产生的握手数据（如果有）——从 qc.handshake_out_* 缓存中取
            auto send_handshake = [&](enum ssl_encryption_level_t olvl, std::vector<uint8_t>& buf, uint64_t& pn_counter) {
                printf("[Server] Sending handshake data for level %d, len=%zu\n", (int)olvl, buf.size());
                if (buf.empty()) {
                    return;
                }
                QuicLevelKeys& LL = qc.level[olvl];
                if (!LL.write_ready) {
                    std::fprintf(stderr, "[Server] Level %d not ready for write\n", (int)olvl);
                    return;
                }
                uint8_t nonce2[12]; make_nonce_from_pn(LL.write_iv, pn_counter++, nonce2);
                std::vector<uint8_t> ct;
                if (!aead_seal(LL.aead, LL.write_key, LL.write_key_len, nonce2, sizeof(nonce2),
                               nullptr, 0, buf.data(), buf.size(), ct)) {
                    std::fprintf(stderr, "[Server] AEAD seal failed\n");
                    return;
                }
                // 封装为 UDP 帧并发回客户端
                std::vector<uint8_t> out;
                out.reserve(12 + ct.size());
                out.push_back(FT_CRYPTO);
                out.push_back(u8_from_enc(olvl));
                uint64_t opn = pn_counter - 1;
                out.insert(out.end(), (uint8_t*)&opn, (uint8_t*)&opn + 8);
                uint16_t clen = (uint16_t)ct.size();
                out.insert(out.end(), (uint8_t*)&clen, (uint8_t*)&clen + 2);
                out.insert(out.end(), ct.begin(), ct.end());
                sock.sendto(r->peer_host, r->peer_port, out);
                buf.clear();
            };

            send_handshake(ssl_encryption_initial, qc.handshake_out_initial, qc.pn_initial);
            send_handshake(ssl_encryption_handshake, qc.handshake_out_hs, qc.pn_hs);
            send_handshake(ssl_encryption_application, qc.handshake_out_app, qc.pn_app);

            // 如果握手完成，回一个应用层 PING 作为演示
            if (qc.handshake_complete) {
                printf("[Server] Handshake complete!\n");
                QuicLevelKeys& AL = qc.level[ssl_encryption_application];
                uint8_t nonce3[12];
                make_nonce_from_pn(AL.write_iv, qc.pn_app++, nonce3);
                auto ping = make_demo_ping_payload();
                std::vector<uint8_t> ct;
                if (aead_seal(AL.aead, AL.write_key, AL.write_key_len, nonce3, sizeof(nonce3), nullptr, 0, ping.data(), ping.size(), ct)) {
                    printf("[Server] Sending application PING\n");
                    std::vector<uint8_t> out;
                    out.push_back(FT_PING);
                    out.push_back(u8_from_enc(ssl_encryption_application));
                    uint64_t opn = qc.pn_app - 1;
                    out.insert(out.end(), (uint8_t*)&opn, (uint8_t*)&opn + 8);
                    uint16_t clen = (uint16_t)ct.size();
                    out.insert(out.end(), (uint8_t*)&clen, (uint8_t*)&clen + 2);
                    out.insert(out.end(), ct.begin(), ct.end());
                    sock.sendto(r->peer_host, r->peer_port, out);
                }
            }
        } else if (ft == FT_PING) {
            std::fprintf(stderr, "[Server] Received PING (encrypted) len=%u\n", plen);
            // 可选：解密并打印
        }
    }

    SSL_free(ssl);
    SSL_CTX_free(ctx);
    return 0;
}