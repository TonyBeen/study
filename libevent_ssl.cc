#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <event2/event.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/evp.h>

#define PORT 4433

// 定义 SSL 连接结构体
typedef struct {
    SSL *ssl;
    struct event *read_event;
    struct event *write_event;
} ssl_connection_t;

// 初始化 OpenSSL
void init_openssl() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

// 创建 SSL_CTX
SSL_CTX *create_context() {
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = TLS_server_method();
    ctx = SSL_CTX_new(method);
    if (!ctx) {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(1);
    }

    return ctx;
}

// 加载证书和私钥
void configure_context(SSL_CTX *ctx) {
    // 加载服务器证书和私钥
    if (SSL_CTX_use_certificate_file(ctx, "server_cert.pem", SSL_FILETYPE_PEM) <= 0) {
        perror("Unable to load certificate");
        ERR_print_errors_fp(stderr);
        exit(1);
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, "server_key.pem", SSL_FILETYPE_PEM) <= 0) {
        perror("Unable to load private key");
        ERR_print_errors_fp(stderr);
        exit(1);
    }
}

// 处理客户端 SSL 握手
void ssl_handshake(int fd, short events, void *arg) {
    ssl_connection_t *conn = (ssl_connection_t *)arg;
    int ret = SSL_do_handshake(conn->ssl);
    
    if (ret == 1) {
        // 握手完成，添加写事件
        event_del(conn->read_event);
        event_add(conn->write_event, NULL);
    } else {
        int err = SSL_get_error(conn->ssl, ret);
        if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE) {
            perror("SSL handshake failed");
            event_del(conn->read_event);
            event_del(conn->write_event);
        }
    }
}

// 处理读事件
void ssl_read(int fd, short events, void *arg) {
    ssl_connection_t *conn = (ssl_connection_t *)arg;
    char buf[1024];
    int bytes;

    bytes = SSL_read(conn->ssl, buf, sizeof(buf));
    if (bytes > 0) {
        printf("Received: %.*s\n", bytes, buf);
    } else {
        int err = SSL_get_error(conn->ssl, bytes);
        if (err == SSL_ERROR_WANT_READ) {
            return;
        }
        if (err == SSL_ERROR_ZERO_RETURN) {
            // 关闭连接
            printf("SSL connection closed\n");
            event_del(conn->read_event);
            event_del(conn->write_event);
            SSL_free(conn->ssl);
            free(conn);
            return;
        }
        perror("SSL read error");
    }
}

// 处理写事件
void ssl_write(int fd, short events, void *arg) {
    ssl_connection_t *conn = (ssl_connection_t *)arg;
    const char *msg = "Hello from SSL server!";
    int bytes;

    bytes = SSL_write(conn->ssl, msg, strlen(msg));
    if (bytes > 0) {
        printf("Sent: %s\n", msg);
    } else {
        int err = SSL_get_error(conn->ssl, bytes);
        if (err == SSL_ERROR_WANT_WRITE) {
            return;
        }
        perror("SSL write error");
    }

    // 处理完数据后关闭连接
    event_del(conn->read_event);
    event_del(conn->write_event);
    SSL_free(conn->ssl);
    free(conn);
}

// 接受客户端连接
void accept_connection(int listener, short events, void *arg) {
    SSL_CTX *ctx = (SSL_CTX *)arg;
    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    ssl_connection_t *conn;

    client_fd = accept(listener, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0) {
        perror("Accept failed");
        return;
    }

    // 创建 SSL 连接
    conn = (ssl_connection_t *)malloc(sizeof(ssl_connection_t));
    conn->ssl = SSL_new(ctx);
    SSL_set_fd(conn->ssl, client_fd);

    // 设置读事件来处理 SSL 握手
    conn->read_event = event_new(NULL, client_fd, EV_READ | EV_PERSIST, ssl_handshake, conn);
    event_add(conn->read_event, NULL);
}

int main() {
    SSL_CTX *ctx;
    struct event_base *base;
    struct sockaddr_in sin;
    int listener;

    // 初始化 OpenSSL
    init_openssl();

    // 创建 SSL_CTX 上下文
    ctx = create_context();
    configure_context(ctx);

    // 创建 libevent 事件循环
    base = event_base_new();
    if (!base) {
        perror("Unable to create event base");
        exit(1);
    }

    // 创建服务器套接字
    listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        perror("Unable to create socket");
        exit(1);
    }

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(PORT);

    if (bind(listener, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        perror("Unable to bind");
        exit(1);
    }

    if (listen(listener, 16) < 0) {
        perror("Unable to listen");
        exit(1);
    }

    // 设置 listener 事件，接收客户端连接
    struct event *listener_event = event_new(base, listener, EV_READ | EV_PERSIST, accept_connection, ctx);
    event_add(listener_event, NULL);

    // 运行事件循环
    event_base_dispatch(base);

    // 清理资源
    event_free(listener_event);
    event_base_free(base);
    SSL_CTX_free(ctx);

    return 0;
}
