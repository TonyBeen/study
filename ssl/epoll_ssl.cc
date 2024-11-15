/*************************************************************************
    > File Name: epoll_ssl.cc
    > Author: hsz
    > Brief:
    > Created Time: Tue 15 Mar 2022 04:58:28 PM CST
 ************************************************************************/

#include <utils/utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <map>
#include <log/log.h>

#define LOG_TAG "epoll ssl"

#define LOCAL_IP "10.0.8.3"
#define LOCAL_PORT 8000

#define CA_CERT_FILE        "ca.crt"
#define SERVER_KEY_FILE     "server.key"
#define SERVER_CERT_FILE    "server.pem"

int CreateSocket()
{
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    assert(fd > 0 && "socket error");

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(LOCAL_IP);
    addr.sin_port = htons(LOCAL_PORT);

    int flag = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

    int code = ::bind(fd, (sockaddr *)&addr, sizeof(addr));
    assert(code == 0 && "bind error");

    code = ::listen(fd, 1024);
    assert(code == 0 && "listen error");

    timeval tv = {0, 500 * 1000};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(timeval));
    return fd;
}

void catch_signal(int sig)
{
    LOG_ASSERT(false, "sig = %d", sig);
}

int main(int argc, char **argv)
{
    signal(SIGSEGV, catch_signal);
    int srvFd = CreateSocket();
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    int efd = epoll_create(1024);
    assert(efd > 0);
    printf("epoll fd %d\n", efd);

    epoll_event events[1024];
    epoll_event ev;
    ev.data.fd = srvFd;
    ev.events = EPOLLET | EPOLLIN;
    epoll_ctl(efd, EPOLL_CTL_ADD, srvFd, &ev);

    std::map<int, SSL *> sslMap;
    SSL_CTX *ctx = nullptr;

    ctx = SSL_CTX_new(SSLv23_server_method());
    assert(ctx);
    // 不校验客户端证书
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);
    // 加载CA的证书  
    if(!SSL_CTX_load_verify_locations(ctx, CA_CERT_FILE, nullptr)) {
        printf("SSL_CTX_load_verify_locations error!\n");
        return -1;
    }
    // 加载自己的证书  
    if(SSL_CTX_use_certificate_file(ctx, SERVER_CERT_FILE, SSL_FILETYPE_PEM) <= 0) {
        printf("SSL_CTX_use_certificate_file error!\n");
        return -1;
    }

    // 加载私钥
    if(SSL_CTX_use_PrivateKey_file(ctx, SERVER_KEY_FILE, SSL_FILETYPE_PEM) <= 0) {
        printf("SSL_CTX_use_PrivateKey_file error!\n");
        return -1;
    }

    // 判定私钥是否正确  
    if(!SSL_CTX_check_private_key(ctx)) {
        printf("SSL_CTX_check_private_key error!\n");
        return -1;
    }

    printf("CA证书、本地证书、私钥均已加载完毕\n");
    printf("server is listening...\n");

    static const char *https_response = 
        "HTTP/1.1 200 OK\r\nServer: httpd\r\nContent-Length: %d\r\nConnection: keep-alive\r\n\r\n";

    while (true) {
        printf("epoll wait...\n");
        int nev = epoll_wait(efd, events, sizeof(events) / sizeof(epoll_event), -1);
        if (nev < 0) {
            printf("epoll_wait error. [%d,%s]", errno, strerror(errno));
            break;
        }

        for (size_t i = 0; i < nev; ++i) {
            auto &event = events[i];
            if (event.data.fd == srvFd) {  // accept
                sockaddr_in addr;
                socklen_t len = sizeof(addr);
                int cfd = ::accept(srvFd, (sockaddr *)&addr, &len);
                if (cfd > 0) {
                    printf("accept client %d [%s:%d]\n", cfd, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
                    SSL *ssl = SSL_new(ctx);
                    bool isSSLAccept = true;
                    if (ssl == nullptr) {
                        printf("SSL_new error.\n");
                        continue;
                    }
                    // static struct timeval tv = {0, 500 * 1000};
                    // setsockopt(cfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(timeval));
                    // setsockopt(cfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(timeval));
                    int flags = ::fcntl(cfd, F_GETFL, 0);
                    ::fcntl(cfd, F_SETFL, flags | O_NONBLOCK);

                    SSL_set_fd(ssl, cfd);
                    int code;
                    int retryTimes = 0;
                    uint64_t begin = Time::SystemTime();
                    // 防止客户端连接了但不进行ssl握手, 单纯的增大循环次数无法解决问题，本地大概循环4000次，chrome连接会循环20000多次
                    while ((code = SSL_accept(ssl)) <= 0 && retryTimes++ < 100) {
                        if (SSL_get_error(ssl, code) != SSL_ERROR_WANT_READ) {
                            printf("ssl accept error. %d\n", SSL_get_error(ssl, code));
                            break;
                        }
                        msleep(1);
                    }
                    uint64_t end = Time::SystemTime();

                    printf("code %d, retry times %d\n", code, retryTimes);
                    if (code != 1) {
                        isSSLAccept = false;
                        ::close(cfd);
                        SSL_free(ssl);
                        continue;
                    }

                    printf("ssl accept success. cost %lu ms\n", end - begin);
                    ev.data.fd = cfd;
                    ev.events = EPOLLET | EPOLLIN;
                    epoll_ctl(efd, EPOLL_CTL_ADD, cfd, &ev);
                    sslMap.insert(std::make_pair(cfd, ssl));
                } else {
                    perror("accept error");
                }
                continue;
            }

            auto it = sslMap.find(event.data.fd);
            assert(it != sslMap.end());

            if (event.events & (EPOLLRDHUP | EPOLLHUP)) {
                printf("client %d quit!\n", event.data.fd);
                close(event.data.fd);
                SSL_shutdown(it->second);
                SSL_free(it->second);
                sslMap.erase(it);
                epoll_ctl(efd, EPOLL_CTL_DEL, event.data.fd, nullptr);
                continue;
            }

            if (event.events & EPOLLIN) {
                char buf[1024] = {0};
                int readSize = SSL_read(it->second, buf, sizeof(buf));
                if (readSize <= 0) {
                    printf("SSL_read error. %d\n", SSL_get_error(it->second, readSize));
                    continue;
                }
                printf("read: %d\n%s\n", readSize, buf);

                char sendBuf[1024] = {0};
                int fmtSize = sprintf(sendBuf, https_response, readSize);

                printf("*********************\n%s*********************\n", sendBuf);
                int writeSize = SSL_write(it->second, sendBuf, strlen(sendBuf));    // 发送响应头
                printf("format size %d, write size %d\n", fmtSize, writeSize);
                if (writeSize <= 0) {
                    printf("SSL_write error. %d\n", SSL_get_error(it->second, writeSize));
                }
                writeSize = SSL_write(it->second, buf, readSize);   // 发送响应主体
                if (writeSize <= 0) {
                    printf("SSL_write error. %d\n", SSL_get_error(it->second, writeSize));
                }
                printf("format size %d, write size %d\n", fmtSize, writeSize);
            }
        }
    }

    for (auto it : sslMap) {
        close(it.first);
        SSL_free(it.second);
    }

    SSL_CTX_free(ctx);
    close(srvFd);
    close(efd);
    return 0;
}

