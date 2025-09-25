#include <iostream>
#include <string>
#include <cstring>
#include <cassert>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/queue.h>
#include <fcntl.h>

#include <lsquic/lsquic_types.h>
#include <lsquic/lsquic.h>

#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/ssl.h>

#include <utils/CLI11.hpp>

#include <log/log.h>

#define LOG_TAG "LSQUIC"

// 新连接回调
static lsquic_conn_ctx_t *on_new_conn(void *stream_if_ctx, lsquic_conn_t *conn) {
    printf("[server] New QUIC connection\n");
    return NULL;
}

// 连接关闭回调
static void on_conn_closed(lsquic_conn_t *conn) {
    printf("[server] QUIC connection closed\n");
}

// 新 stream 创建回调
static lsquic_stream_ctx_t *on_new_stream(void *stream_if_ctx, lsquic_stream_t *stream) {
    printf("[server] New stream created\n");
    lsquic_stream_wantread(stream, 1); // 允许读取
    return NULL;
}

// stream 可读回调
static void on_read(lsquic_stream_t *stream, lsquic_stream_ctx_t *h) {
    char buf[1024];
    ssize_t n = lsquic_stream_read(stream, buf, sizeof(buf));
    if (n > 0) {
        printf("[server] Received: %.*s\n", (int)n, buf);
        lsquic_stream_write(stream, buf, n); // 回写
        lsquic_stream_flush(stream);
    }
    else if (n == 0) {
        printf("[server] End of stream input\n");
        lsquic_stream_shutdown(stream, 1); // 关闭写端
    }
    else {
        perror("lsquic_stream_read");
    }
}

// stream 可写回调（没用到，这里留空）
static void on_write(lsquic_stream_t *stream, lsquic_stream_ctx_t *h) {}

// stream 关闭回调
static void on_close(lsquic_stream_t *stream, lsquic_stream_ctx_t *h) {
    printf("[server] Stream closed\n");
}

// Server 端 stream 回调结构
const struct lsquic_stream_if server_stream_if = {
    on_new_conn,
    NULL,
    on_conn_closed,
    on_new_stream,
    on_read,
    on_write,
    on_close,
    NULL, NULL, NULL, NULL, NULL,
};

// ============ 发包回调 ============

static int packets_out(void *ctx, const struct lsquic_out_spec *specs, unsigned count) {
    int fd = (uintptr_t)ctx;
    for (unsigned i = 0; i < count; ++i) {
        struct msghdr msg = {0};
        msg.msg_name = (void *)specs[i].dest_sa;
        msg.msg_namelen = sizeof(struct sockaddr_in);
        msg.msg_iov = (struct iovec *)specs[i].iov;
        msg.msg_iovlen = specs[i].iovlen;
        if (sendmsg(fd, &msg, 0) < 0) {
            perror("sendmsg");
        }
    }
    return count;
}

SSL_CTX *ssl_ctx = NULL;

int main() {
    // 初始化 LSQUIC
    lsquic_global_init(LSQUIC_GLOBAL_SERVER);

    // 创建 UDP socket
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in local_addr = {0};
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(12345);
    inet_pton(AF_INET, "0.0.0.0", &local_addr.sin_addr);
    bind(fd, (struct sockaddr*)&local_addr, sizeof(local_addr));

    // 创建 SSL_CTX，加载证书和私钥
    ssl_ctx = SSL_CTX_new(TLS_method());
    SSL_CTX_set_min_proto_version(ssl_ctx, TLS1_3_VERSION);
    SSL_CTX_use_certificate_chain_file(ssl_ctx, "fullchain.pem");
    SSL_CTX_use_PrivateKey_file(ssl_ctx, "privkey.pem", SSL_FILETYPE_PEM);

    // 配置 QUIC 引擎
    struct lsquic_engine_settings settings;
    lsquic_engine_init_settings(&settings, LSENG_SERVER);

    struct lsquic_engine_api eapi = {0};
    eapi.ea_settings = &settings;
    eapi.ea_packets_out = packets_out;
    eapi.ea_packets_out_ctx = (void *)(uintptr_t)fd;
    eapi.ea_stream_if = &server_stream_if;
    eapi.ea_get_ssl_ctx = [](void *, const struct sockaddr *) -> SSL_CTX* {
        return ssl_ctx;
    };

    lsquic_engine_t *engine = lsquic_engine_new(LSENG_SERVER, &eapi);

    printf("[server] Listening on UDP port 12345\n");

    // 主循环
    while (1) {
        unsigned char buf[1500];
        struct sockaddr_in peer;
        socklen_t peerlen = sizeof(peer);
        ssize_t nr = recvfrom(fd, buf, sizeof(buf), 0,
                              (struct sockaddr*)&peer, &peerlen);
        if (nr > 0) {
            lsquic_engine_packet_in(engine, buf, nr,
                                    (struct sockaddr*)&peer, (struct sockaddr*)&local_addr,
                                    (void *)(uintptr_t)fd, 0);
        }
        lsquic_engine_process_conns(engine);
        usleep(1000);
    }

    lsquic_engine_destroy(engine);
    lsquic_global_cleanup();
    return 0;
}
