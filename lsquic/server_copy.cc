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
#define ALPN "sample"

/* Expected request and response of the siduck protocol */
#define REQUEST "quack"
#define RESPONSE "quack-ack"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

static lsquic_conn_ctx_t *on_new_conn(void *stream_if_ctx, lsquic_conn_t *conn)
{
    std::cout << "Server: new connection" << std::endl;
    return nullptr;
}

static void on_conn_closed(lsquic_conn_t *conn)
{
    std::cout << "Server: connection closed" << std::endl;
    lsquic_conn_set_ctx(conn, nullptr);
}

static ssize_t on_dg_write(lsquic_conn_t *conn, void *buf, size_t sz)
{
    int32_t s = lsquic_conn_want_datagram_write(conn, 0);
    assert(s == 1);

    if (sz >= sizeof(RESPONSE) - 1) {
        LOGI("wrote `%s' in response. buffer size = %zu", RESPONSE, sz);
        memcpy(buf, RESPONSE, sizeof(RESPONSE) - 1);
        lsquic_conn_close(conn);
        return sizeof(RESPONSE) - 1;
    }

    return -1;
}

static void on_datagram(lsquic_conn_t *conn, const void *buf, size_t bufsz)
{
    int s;
    if (bufsz == sizeof(REQUEST) - 1 && 0 == memcmp(buf, REQUEST, sizeof(REQUEST) - 1))
    {
        LOGI("received the expected `%s' request", REQUEST);
        s = lsquic_conn_want_datagram_write(conn, 1);
        assert(s == 0);
    }
    else
    {
        LOGW("unexpected request received, abort connection");
        lsquic_conn_abort(conn);
    }
}

const struct lsquic_stream_if server_stream_if = {
    .on_new_conn            = on_new_conn,
    .on_conn_closed         = on_conn_closed,
    .on_dg_write            = on_dg_write,
    .on_datagram            = on_datagram,
};

static int select_alpn(SSL *ssl, const unsigned char **out, unsigned char *outlen,
                       const unsigned char *in, unsigned int inlen, void *arg)
{
    int32_t status;
    status = SSL_select_next_proto((unsigned char **)out, outlen, in, inlen, (unsigned char *)ALPN, strlen(ALPN));
    if (status == OPENSSL_NPN_NEGOTIATED) {
        return SSL_TLSEXT_ERR_OK;
    }

    LOGW("no supported protocol can be selected from %.*s", (int) inlen, (char *) in);
    return SSL_TLSEXT_ERR_ALERT_FATAL;
}

int lsquic_packets_out(void *ctx, const struct lsquic_out_spec *specs, unsigned count)
{
    struct msghdr msg;
    union {
        #define SIZE1 sizeof(struct in_pktinfo)
        uint8_t buf[CMSG_SPACE(MAX(SIZE1, sizeof(struct in6_pktinfo)))];
        struct cmsghdr cmsg;
    } ancil;
    uintptr_t ancil_key, prev_ancil_key;

    if (0 == count) {
        return 0;
    }

    const unsigned orig_count = count;
    uint32_t n = 0;
    prev_ancil_key = 0;

    do {
        sport = specs[n].peer_ctx;
        if (sport->sp_prog->prog_flags & PROG_SEARCH_ADDRS)
            sport = find_sport(sport->sp_prog, specs[n].local_sa);

        msg.msg_name       = (void *) specs[n].dest_sa;
        msg.msg_namelen    = (AF_INET == specs[n].dest_sa->sa_family ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6)),
        msg.msg_iov        = specs[n].iov;
        msg.msg_iovlen     = specs[n].iovlen;
        msg.msg_flags      = 0;

        if ((sport->sp_flags & SPORT_SERVER) && specs[n].local_sa->sa_family)
        {
            cw = CW_SENDADDR;
            ancil_key = (uintptr_t) specs[n].local_sa;
            assert(0 == (ancil_key & 3));
        }
        else
        {
            cw = 0;
            ancil_key = 0;
        }

        if (cw && prev_ancil_key == ancil_key)
        {
            /* Reuse previous ancillary message */
            ;
        }
        else if (cw)
        {
            prev_ancil_key = ancil_key;
            setup_control_msg(&msg, cw, &specs[n], ancil.buf, sizeof(ancil.buf));
        }
        else
        {
            prev_ancil_key = 0;
            msg.msg_control = NULL;
            msg.msg_controllen = 0;
        }

        s = sendmsg(sport->fd, &msg, 0);
        if (s < 0) {
            LOGI("sendto failed: %s", strerror(errno));
            break;
        }
        char localaddr_str[80];
        char remoteaddr_str[80];
        LOGI("[%s] TX packet %d bytes to: %s", sockaddr2str((struct sockaddr *)&sport->sp_local_addr, localaddr_str, sizeof(localaddr_str)),
                s, sockaddr2str(specs[n].dest_sa, remoteaddr_str, sizeof(remoteaddr_str)));

        ++n;
    }
    while (n < count);

}

int main(int argc, char *argv[])
{
    lsquic_global_init(LSQUIC_GLOBAL_SERVER);

    // 2、解析命令行参数
    CLI::App app{"LSQUIC Server"};
    std::string cert_file = "cert.pem";
    std::string key_file = "private.pem";
    int32_t port = 4433;
    std::string domain = "0.0.0.0";

    app.add_option("-c,--cert", cert_file, "Path to the certificate file");
    app.add_option("-k,--key", key_file, "Path to the private key file");
    app.add_option("-p,--port", port, "Port to listen on");
    app.add_option("-d,--domain", domain, "certificate domain name");
    app.set_help_flag("-h,--help", "Print help");
    CLI11_PARSE(app, argc, argv);

    // 1、加载证书和私钥
    SSL_CTX *ssl_ctx = SSL_CTX_new(TLS_method());
    SSL_CTX_set_min_proto_version(ssl_ctx, TLS1_3_VERSION);
    SSL_CTX_set_max_proto_version(ssl_ctx, TLS1_3_VERSION);
    SSL_CTX_set_default_verify_paths(ssl_ctx);
    SSL_CTX_set_alpn_select_cb(ssl_ctx, select_alpn, NULL);

    /**
     * @brief 2、加载证书链文件
     * SSL_CTX_use_certificate_file 一般用于测试环境中加载证书, 只加载单个证书文件
     * SSL_CTX_use_certificate_chain_file 一般用于生产环境中加载证书, 加载证书链文件
     *  1、把第一张证书当作 服务器端实体证书
     *  2、其余放到内部的“额外证书链”(SSL_CTX_add_extra_chain_cert())中
     *  3、握手时会把整条链（除了根证书）发送给客户端
     * 像Let's Encrypt / GlobalSign / DigiCert 等机构签发的，官方通常会给你两个文件
     *  domain.crt（服务端证书）
     *  CA 提供的 chain.pem（中间证书链）
     * 两个证书应该合并成一个文件，证书链文件的顺序是：
     *  cat domain.crt chain.pem > fullchain.pem
     * 然后加载 fullchain.pem。SSL_CTX_use_certificate_chain_file(ctx, "fullchain.pem");
     */
    if (1 != SSL_CTX_use_certificate_chain_file(ssl_ctx, cert_file.c_str())) {
        LOGE("SSL_CTX_use_certificate_chain_file failed: %s", cert_file.c_str());
        return 0;
    }

    // 仅供自签名/测试场景使用
    // SSL_CTX_use_certificate_file(ssl_ctx, cert_file.c_str(), SSL_FILETYPE_PEM);

    // 3、加载私钥
    if (1 != SSL_CTX_use_PrivateKey_file(ssl_ctx, key_file.c_str(), SSL_FILETYPE_PEM)) {
        LOGE("SSL_CTX_use_PrivateKey_file failed: %s", key_file.c_str());
        return 0;
    }

    /**
     * @brief 4、设置 SSL 会话缓存模式
     * 1、SSL_SESS_CACHE_OFF	            禁用会话缓存（不存储、不查找）
     * 2、SSL_SESS_CACHE_CLIENT	            在客户端启用缓存
     * 3、SSL_SESS_CACHE_SERVER	            在服务端启用缓存（默认对服务器生效）
     * 4、SSL_SESS_CACHE_BOTH	            同时启用客户端和服务器缓存
     * 5、SSL_SESS_CACHE_NO_AUTO_CLEAR	    不自动清理过期会话（需要自己调用清理函数）
     * 6、SSL_SESS_CACHE_NO_INTERNAL_LOOKUP	不自动从内部缓存查找会话（配合自定义回调）
     * 7、SSL_SESS_CACHE_NO_INTERNAL_STORE	不自动存储到内部缓存（配合自定义回调）
     * 8、SSL_SESS_CACHE_NO_INTERNAL	    不用内部缓存，完全依赖自定义 session cache 回调
     */
    int32_t was = SSL_CTX_set_session_cache_mode(ssl_ctx, SSL_SESS_CACHE_SERVER);
    LOGE("set SSL session cache mode to 1 (was: %d)", was);

    // 5、配置 QUIC 引擎参数
    uint8_t ticket_keys[48] = {0};
    ssl_ctx_st *prog_ssl_ctx = SSL_CTX_new(TLS_method());
    SSL_CTX_set_min_proto_version(prog_ssl_ctx, TLS1_3_VERSION);
    SSL_CTX_set_max_proto_version(prog_ssl_ctx, TLS1_3_VERSION);
    SSL_CTX_set_default_verify_paths(prog_ssl_ctx);
    if (1 != SSL_CTX_set_tlsext_ticket_keys(prog_ssl_ctx, ticket_keys, sizeof(ticket_keys))) {
        LOGE("SSL_CTX_set_tlsext_ticket_keys failed");
        return -1;
    }

    lsquic_engine_settings quic_engine_setting;
    lsquic_engine_init_settings(&quic_engine_setting, LSENG_SERVER);
    quic_engine_setting.es_ecn = 0;
    lsquic_engine_api eapi;
    memset(&eapi, 0, sizeof(eapi));
    eapi.ea_settings = &quic_engine_setting;
    eapi.ea_stream_if = &server_stream_if;
    eapi.ea_stream_if_ctx = nullptr;
    eapi.ea_pmi = nullptr;
    eapi.ea_get_ssl_ctx = [=] (void *peer_ctx, const struct sockaddr *unused) -> ssl_ctx_st *{
        return prog_ssl_ctx;
    };

    lsquic_engine_t *engine = lsquic_engine_new(LSENG_SERVER, &eapi);
    if (!engine) { std::cerr << "engine create fail\n"; return 1; }

    // UDP socket绑定
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(12345);
    addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    bind(fd, (sockaddr*)&addr, sizeof(addr));

    // 事件循环
    while (true)
    {
        lsquic_engine_process_conns(engine);
        // 收发 UDP 数据
        unsigned char buf[1500];
        sockaddr_in peer;
        socklen_t peerlen = sizeof(peer);
        ssize_t nr = recvfrom(fd, buf, sizeof(buf), MSG_DONTWAIT,
                              (sockaddr*)&peer, &peerlen);
        if (nr > 0)
        {
            lsquic_engine_packet_in(engine, buf, nr,
                (sockaddr*)&peer, (sockaddr*)&addr, (void*)(uintptr_t)fd, 0);
        }
        lsquic_engine_process_conns(engine);
        usleep(1000);
    }

    lsquic_engine_destroy(engine);
    lsquic_global_cleanup();
    return 0;
}
