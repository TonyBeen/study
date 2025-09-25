#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <lsquic/lsquic.h>
#include <openssl/ssl.h>

/**
 * Echo Client
 * - 连接到 Echo Server
 * - 在 QUIC Stream 发送一条消息
 * - 接收回显回应
 */

static lsquic_conn_ctx_t *on_new_conn(void *stream_if_ctx, lsquic_conn_t *conn) {
    printf("[client] Connected to server\n");
    lsquic_conn_make_stream(conn); // 主动申请一个 stream
    return NULL;
}

static void on_conn_closed(lsquic_conn_t *conn) {
    printf("[client] Connection closed\n");
}

static lsquic_stream_ctx_t *on_new_stream(void *stream_if_ctx, lsquic_stream_t *stream) {
    printf("[client] New stream ready, sending message...\n");
    const char *msg = "Hello QUIC!";
    lsquic_stream_write(stream, msg, strlen(msg));
    lsquic_stream_flush(stream);
    lsquic_stream_wantread(stream, 1);
    return NULL;
}

static void on_read(lsquic_stream_t *stream, lsquic_stream_ctx_t *h) {
    char buf[1024];
    ssize_t n = lsquic_stream_read(stream, buf, sizeof(buf));
    if (n > 0) {
        printf("[client] Received echo: %.*s\n", (int)n, buf);
        lsquic_stream_shutdown(stream, 0);
    }
    else if (n == 0) {
        printf("[client] Stream end\n");
    }
    else {
        perror("lsquic_stream_read");
    }
}

static void on_write(lsquic_stream_t *stream, lsquic_stream_ctx_t *h) {}
static void on_close(lsquic_stream_t *stream, lsquic_stream_ctx_t *h) {
    printf("[client] Stream closed\n");
}

const struct lsquic_stream_if client_stream_if = {
    .on_new_conn = on_new_conn,
    .on_conn_closed = on_conn_closed,
    .on_new_stream = on_new_stream,
    .on_read = on_read,
    .on_write = on_write,
    .on_close = on_close,
};

static int packets_out(void *ctx, const struct lsquic_out_spec *specs, unsigned count) {
    int fd = (uintptr_t)ctx;
    for (unsigned i = 0; i < count; ++i) {
        struct msghdr msg = {0};
        msg.msg_name = (void *)specs[i].dest_sa;
        msg.msg_namelen = sizeof(struct sockaddr_in);
        msg.msg_iov = (struct iovec *)specs[i].iov;
        msg.msg_iovlen = specs[i].iovlen;
        sendmsg(fd, &msg, 0);
    }
    return count;
}

int main() {
    lsquic_global_init(LSQUIC_GLOBAL_CLIENT);

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    struct sockaddr_in local_addr = {0};
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(0);
    inet_pton(AF_INET, "0.0.0.0", &local_addr.sin_addr);
    bind(fd, (struct sockaddr*)&local_addr, sizeof(local_addr));

    SSL_CTX *ssl_ctx = SSL_CTX_new(TLS_method());
    SSL_CTX_set_min_proto_version(ssl_ctx, TLS1_3_VERSION);

    struct lsquic_engine_settings settings;
    lsquic_engine_init_settings(&settings, 0);

    struct lsquic_engine_api eapi = {0};
    eapi.ea_settings = &settings;
    eapi.ea_packets_out = packets_out;
    eapi.ea_packets_out_ctx = (void *)(uintptr_t)fd;
    eapi.ea_stream_if = &client_stream_if;
    eapi.ea_get_ssl_ctx = [](void *, const struct sockaddr *) -> SSL_CTX* {
        extern SSL_CTX *ssl_ctx;
        return ssl_ctx;
    };

    lsquic_engine_t *engine = lsquic_engine_new(0, &eapi);

    // 发起连接
    lsquic_conn_t *conn = lsquic_engine_connect(engine, N_LSQVER, 
        (struct sockaddr*)&local_addr, (struct sockaddr*)&server_addr,
        (void *)(uintptr_t)fd, NULL, "localhost", 0, NULL, 0, NULL, 0);

    // 循环处理 QUIC 事件
    while (1) {
        unsigned char buf[1500];
        struct sockaddr_in peer;
        socklen_t peerlen = sizeof(peer);
        ssize_t nr = recvfrom(fd, buf, sizeof(buf), MSG_DONTWAIT,
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
