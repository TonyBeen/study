/*************************************************************************
    > File Name: client.cpp
    > Author: hsz
    > Brief:
    > Created Time: Mon 15 Dec 2025 03:45:39 PM CST
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

enum : uint8_t {
    FT_CRYPTO = 0x01,
    FT_PING   = 0x02,
};

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
    SSL_library_init();
    ERR_load_crypto_strings();
    ERR_load_SSL_strings();

    const SSL_METHOD* method = TLS_client_method();
    SSL_CTX* ctx = SSL_CTX_new(method);
    if (!ctx) { std::fprintf(stderr, "SSL_CTX_new failed\n"); return 1; }

    // 仅启用 TLS1.3（可选）
    SSL_CTX_set_min_proto_version(ctx, TLS1_3_VERSION);
    SSL_CTX_set_max_proto_version(ctx, TLS1_3_VERSION);

    SSL* ssl = SSL_new(ctx);
    if (!ssl) { std::fprintf(stderr, "SSL_new failed\n"); return 1; }

    // SNI（可选）
    SSL_set_tlsext_host_name(ssl, "example.com");

    // 设置连接状态：客户端
    SSL_set_connect_state(ssl);

    QuicConn qc;
    attach_quic_method(ssl, &qc);

    // 设置 ALPN：例如选择 "h3"（与服务端一致）
    {
        auto alpn = build_alpn_wire_format({"MyQuicProtocol"});
        // 重要：wire-format，需要传入原始字节串
        if (SSL_set_alpn_protos(ssl, alpn.data(), (unsigned)alpn.size()) != 0) {
            std::fprintf(stderr, "[Client] SSL_set_alpn_protos failed\n");
            return 1;
        }
    }

    // 携带 QUIC 传输参数
    auto tp = build_min_quic_transport_params();
    if (!tp.empty()) {
        SSL_set_quic_transport_params(ssl, tp.data(), tp.size());
    } else {
        std::fprintf(stderr, "[Client] TP build failed\n");
    }

    // 安装 Initial 层密钥：使用双方约定的 client_dcid（演示用固定值；真实应从包头解析）
    static const uint8_t kClientDCID[] = { 0x83,0x94,0xc8,0xf0,0x3e,0x51,0x57,0x08 };
    if (!install_initial_secrets(&qc, QuicRole::Client, kClientDCID, sizeof(kClientDCID))) {
        std::fprintf(stderr, "[Client] install_initial_secrets failed\n");
        return 1;
    }

    // UDP 连接到服务器
    UdpSocket sock;
    if (!sock.connect("127.0.0.1", 9000)) {
        std::fprintf(stderr, "connect failed\n"); return 1;
    }

    // 第一步：调用 SSL_do_handshake，BoringSSL 会通过 add_handshake_data 产生 ClientHello 等握手字节
    int r = SSL_do_handshake(ssl);
    if (r != 1) {
        int err = SSL_get_error(ssl, r);
        if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE) {
            std::fprintf(stderr, "[Client] initial SSL_do_handshake failed: %d\n", err);
            ERR_print_errors_fp(stderr);
            return 1;
        }
    }

    // 发送握手数据（initial/handshake/app 缓冲区）
    auto send_handshake = [&](enum ssl_encryption_level_t lvl, std::vector<uint8_t>& buf, uint64_t& pn_counter) {
        printf("[Client] Trying send handshake data at level %d, %zu bytes\n", (int)lvl, buf.size());
        if (buf.empty()) {
            return;
        }

        QuicLevelKeys& LL = qc.level[lvl];
        if (!LL.write_ready) {
            std::fprintf(stderr, "[Client] Level %d not ready for encryption\n", (int)lvl);
            return;
        }
        uint8_t nonce2[12]; make_nonce_from_pn(LL.write_iv, pn_counter++, nonce2);
        std::vector<uint8_t> ct;
        if (!aead_seal(LL.aead, LL.write_key, LL.write_key_len, nonce2, sizeof(nonce2),
                        nullptr, 0, buf.data(), buf.size(), ct)) {
            std::fprintf(stderr, "[Client] AEAD seal failed\n");
            return;
        }

        std::vector<uint8_t> out;
        out.push_back(FT_CRYPTO);
        out.push_back(u8_from_enc(lvl));
        uint64_t opn = pn_counter - 1;
        out.insert(out.end(), (uint8_t*)&opn, (uint8_t*)&opn + 8);
        uint16_t clen = (uint16_t)ct.size();
        out.insert(out.end(), (uint8_t*)&clen, (uint8_t*)&clen + 2);
        out.insert(out.end(), ct.begin(), ct.end());
        sock.send(out);
        buf.clear();
    };

    // 可能已生成 Initial 层握手数据，尝试发送
    send_handshake(ssl_encryption_initial, qc.handshake_out_initial, qc.pn_initial);
    send_handshake(ssl_encryption_handshake, qc.handshake_out_hs, qc.pn_hs);
    send_handshake(ssl_encryption_application, qc.handshake_out_app, qc.pn_app);

    // 接收服务器响应并推进握手
    while (true) {
        auto rr = sock.recv();
        if (!rr) continue;

        printf("[Client] Received %zu bytes from server\n", rr->data.size());
        const auto& data = rr->data;
        if (data.size() < 12) continue;
        uint8_t ft = data[0];
        uint8_t el = data[1];
        uint64_t pn = 0;
        std::memcpy(&pn, &data[2], 8);
        uint16_t plen = 0;
        std::memcpy(&plen, &data[10], 2);
        if (data.size() < 12 + plen) continue;
        const uint8_t* payload = &data[12];

        enum ssl_encryption_level_t lvl = enc_from_u8(el);
        QuicLevelKeys& L = qc.level[lvl];
        if (!L.read_ready) {
            std::fprintf(stderr, "[Client] Level %d not ready for decryption\n", (int)lvl);
            continue;
        }

        if (ft == FT_CRYPTO) {
            uint8_t nonce[12]; make_nonce_from_pn(L.read_iv, pn, nonce);
            std::vector<uint8_t> plaintext;
            if (!aead_open(L.aead, L.read_key, L.read_key_len, nonce, sizeof(nonce), nullptr, 0, payload, plen, plaintext)) {
                std::fprintf(stderr, "[Client] AEAD open failed\n");
                continue;
            }

            // 投递握手片段并推进 TLS
            if (!tls_provide_crypto_and_step(ssl, lvl, plaintext.data(), plaintext.size())) {
                std::fprintf(stderr, "[Client] TLS step failed\n");
                continue;
            }

            // TLS 若产生新的握手数据，发送之
            send_handshake(ssl_encryption_initial,   qc.handshake_out_initial, qc.pn_initial);
            send_handshake(ssl_encryption_handshake, qc.handshake_out_hs,      qc.pn_hs);
            send_handshake(ssl_encryption_application, qc.handshake_out_app,   qc.pn_app);

            if (qc.handshake_complete) {
                std::fprintf(stderr, "[Client] Handshake complete. Waiting PING...\n");
            }
        } else if (ft == FT_PING) {
            // 演示：解密应用层 PING
            enum ssl_encryption_level_t app = ssl_encryption_application;
            QuicLevelKeys& AL = qc.level[app];
            uint8_t nonce2[12]; make_nonce_from_pn(AL.read_iv, pn, nonce2);
            std::vector<uint8_t> pt;
            if (aead_open(AL.aead, AL.read_key, AL.read_key_len, nonce2, sizeof(nonce2), nullptr, 0, payload, plen, pt)) {
                std::string s(pt.begin(), pt.end());
                std::fprintf(stderr, "[Client] Got app PING: %s\n", s.c_str());
                break; // 示例结束
            }
        }
    }

    SSL_free(ssl);
    SSL_CTX_free(ctx);
    return 0;
}