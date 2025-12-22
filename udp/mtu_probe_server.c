#include "mtu_probe_common.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_PORT 9999
#define MAX_BUFFER_SIZE 65535

static volatile int g_running = 1;

void signal_handler(int sig) {
    (void)sig;
    g_running = 0;
    printf("\n[Server] 收到退出信号，正在关闭.. .\n");
}

// 获取客户端地址字符串
void get_client_address_string(const struct sockaddr_storage *addr, 
                               char *buf, size_t buf_len, int *port) {
    if (addr->ss_family == AF_INET) {
        const struct sockaddr_in *v4 = (const struct sockaddr_in *)addr;
        inet_ntop(AF_INET, &v4->sin_addr, buf, buf_len);
        *port = ntohs(v4->sin_port);
    } else if (addr->ss_family == AF_INET6) {
        const struct sockaddr_in6 *v6 = (const struct sockaddr_in6 *)addr;
        inet_ntop(AF_INET6, &v6->sin6_addr, buf, buf_len);
        *port = ntohs(v6->sin6_port);
    } else {
        snprintf(buf, buf_len, "unknown");
        *port = 0;
    }
}

int handle_probe_request(int sockfd, const struct sockaddr_storage *client_addr,
                         socklen_t client_len, const uint8_t *buffer, ssize_t recv_len) {
    if (recv_len < (ssize_t)PROBE_HEADER_SIZE) {
        printf("[Server] 收到无效的探测包，大小: %zd\n", recv_len);
        return -1;
    }

    const ProbeHeader *req_header = (const ProbeHeader *)buffer;
    
    if (req_header->type != MSG_TYPE_PROBE_REQ) {
        printf("[Server] 非探测请求消息，类型:  0x%02x\n", req_header->type);
        return -1;
    }

    uint32_t claimed_size = ntohl(req_header->probe_size);
    
    char client_ip[INET6_ADDRSTRLEN];
    int client_port;
    get_client_address_string(client_addr, client_ip, sizeof(client_ip), &client_port);

    if ((uint32_t)recv_len != claimed_size) {
        printf("[Server] 探测大小不匹配: 声明 %u, 实际 %zd (客户端: %s:%d)\n", 
               claimed_size, recv_len, client_ip, client_port);
        
        ProbeHeader nack_header;
        memset(&nack_header, 0, sizeof(nack_header));
        nack_header.type = MSG_TYPE_PROBE_NACK;
        nack_header.seq = req_header->seq;
        nack_header.probe_size = htonl(recv_len);
        nack_header. timestamp = htonl((uint32_t)get_timestamp_ms());
        
        sendto(sockfd, &nack_header, sizeof(nack_header), 0,
               (const struct sockaddr *)client_addr, client_len);
        return -1;
    }

    printf("[Server] 收到探测请求:  客户端=%s:%d, 序列号=%u, 大小=%u\n",
           client_ip, client_port, ntohs(req_header->seq), claimed_size);

    ProbeHeader ack_header;
    memset(&ack_header, 0, sizeof(ack_header));
    ack_header.type = MSG_TYPE_PROBE_ACK;
    ack_header.seq = req_header->seq;
    ack_header.probe_size = req_header->probe_size;
    ack_header.timestamp = htonl((uint32_t)get_timestamp_ms());
    ack_header.checksum = htonl(calculate_checksum(&ack_header, 
                                sizeof(ack_header) - sizeof(uint32_t)));

    ssize_t sent = sendto(sockfd, &ack_header, sizeof(ack_header), 0,
                          (const struct sockaddr *)client_addr, client_len);
    if (sent < 0) {
        perror("[Server] 发送 ACK 失败");
        return -1;
    }

    printf("[Server] 已发送 ACK: 序列号=%u, 确认大小=%u\n",
           ntohs(ack_header.seq), claimed_size);
    
    return 0;
}

int configure_socket(int sockfd, int family) {
    int recv_buf = 1024 * 1024;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &recv_buf, sizeof(recv_buf));

    int send_buf = 1024 * 1024;
    setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &send_buf, sizeof(send_buf));

    int reuse = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    // 对于 IPv6 socket，禁用 IPV6_V6ONLY 以同时支持 IPv4
    if (family == AF_INET6) {
        int v6only = 0;
        if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &v6only, sizeof(v6only)) < 0) {
            perror("[Server] 设置 IPV6_V6ONLY 失败");
        }
    }

    if (family == AF_INET) {
#ifdef IP_MTU_DISCOVER
        int mtu_discover = IP_PMTUDISC_DO;
        setsockopt(sockfd, IPPROTO_IP, IP_MTU_DISCOVER, &mtu_discover, sizeof(mtu_discover));
#endif
    } else {
#ifdef IPV6_MTU_DISCOVER
        int mtu_discover = IPV6_PMTUDISC_DO;
        setsockopt(sockfd, IPPROTO_IPV6, IPV6_MTU_DISCOVER, &mtu_discover, sizeof(mtu_discover));
#endif
    }

    return 0;
}

int main(int argc, char *argv[]) {
    int port = SERVER_PORT;
    int use_ipv6 = 1;  // 默认使用 IPv6 (可以同时接受 IPv4)
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-4") == 0) {
            use_ipv6 = 0;
        } else if (strcmp(argv[i], "-6") == 0) {
            use_ipv6 = 1;
        } else if (strcmp(argv[i], "-h") == 0) {
            printf("用法: %s [-4|-6] [端口]\n", argv[0]);
            printf("  -4     仅使用 IPv4\n");
            printf("  -6     使用 IPv6 (同时支持 IPv4，默认)\n");
            printf("  端口   监听端口 (默认: %d)\n", SERVER_PORT);
            return EXIT_SUCCESS;
        } else {
            port = atoi(argv[i]);
        }
    }

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    int family = use_ipv6 ? AF_INET6 :  AF_INET;
    int sockfd = socket(family, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("[Server] 创建 socket 失败");
        return EXIT_FAILURE;
    }

    configure_socket(sockfd, family);

    // 绑定地址
    struct sockaddr_storage server_addr;
    socklen_t addr_len;
    
    memset(&server_addr, 0, sizeof(server_addr));
    
    if (use_ipv6) {
        struct sockaddr_in6 *v6 = (struct sockaddr_in6 *)&server_addr;
        v6->sin6_family = AF_INET6;
        v6->sin6_addr = in6addr_any;
        v6->sin6_port = htons(port);
        addr_len = sizeof(struct sockaddr_in6);
    } else {
        struct sockaddr_in *v4 = (struct sockaddr_in *)&server_addr;
        v4->sin_family = AF_INET;
        v4->sin_addr.s_addr = INADDR_ANY;
        v4->sin_port = htons(port);
        addr_len = sizeof(struct sockaddr_in);
    }

    if (bind(sockfd, (struct sockaddr *)&server_addr, addr_len) < 0) {
        perror("[Server] 绑定失败");
        close(sockfd);
        return EXIT_FAILURE;
    }

    printf("===========================================\n");
    printf("    UDP MTU 探测服务端 (IPv4/IPv6)\n");
    printf("===========================================\n");
    printf("[Server] 地址族: %s\n", use_ipv6 ? "IPv6 (双栈)" : "IPv4");
    printf("[Server] 监听端口: %d\n", port);
    printf("[Server] 等待探测请求...\n\n");

    uint8_t *buffer = (uint8_t *)malloc(MAX_BUFFER_SIZE);
    if (!buffer) {
        perror("[Server] 分配缓冲区失败");
        close(sockfd);
        return EXIT_FAILURE;
    }

    while (g_running) {
        struct sockaddr_storage client_addr;
        socklen_t client_len = sizeof(client_addr);

        ssize_t recv_len = recvfrom(sockfd, buffer, MAX_BUFFER_SIZE, 0,
                                    (struct sockaddr *)&client_addr, &client_len);
        
        if (recv_len < 0) {
            if (errno == EINTR) continue;
            perror("[Server] 接收数据失败");
            continue;
        }

        if (recv_len == 0) continue;

        handle_probe_request(sockfd, &client_addr, client_len, buffer, recv_len);
    }

    free(buffer);
    close(sockfd);
    printf("[Server] 服务端已关闭\n");
    
    return EXIT_SUCCESS;
}