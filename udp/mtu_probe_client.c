#include "mtu_probe_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

#define DEFAULT_SERVER_PORT 9999
#define MAX_BUFFER_SIZE 65535
#define DNS_RESOLVE_TIMEOUT_S 10

static volatile int g_running = 1;

// 地址信息结构
typedef struct {
    int family;                          // AF_INET 或 AF_INET6
    union {
        struct sockaddr_in  v4;
        struct sockaddr_in6 v6;
    } addr;
    socklen_t addr_len;
    char ip_str[INET6_ADDRSTRLEN];       // 可读的 IP 地址字符串
} ResolvedAddress;

// 解析结果
typedef struct {
    ResolvedAddress *addresses;           // 地址数组
    int count;                            // 地址数量
    int current_index;                    // 当前使用的地址索引
} ResolveResult;

// 信号处理
void signal_handler(int sig) {
    (void)sig;
    g_running = 0;
    printf("\n[Client] 收到退出信号，正在关闭.. .\n");
}

// 释放解析结果
void free_resolve_result(ResolveResult *result) {
    if (result && result->addresses) {
        free(result->addresses);
        result->addresses = NULL;
        result->count = 0;
    }
}

// 解析域名或 IP 地址 (支持 IPv4/IPv6)
int resolve_host(const char *host, int port, ResolveResult *result, int prefer_ipv4) {
    struct addrinfo hints, *res, *p;
    char port_str[16];
    int status;
    int addr_count = 0;

    memset(result, 0, sizeof(ResolveResult));
    memset(&hints, 0, sizeof(hints));
    
    // 设置地址族偏好
    hints.ai_family = AF_UNSPEC;          // 同时支持 IPv4 和 IPv6
    hints.ai_socktype = SOCK_DGRAM;       // UDP
    hints.ai_protocol = IPPROTO_UDP;
    hints.ai_flags = AI_ADDRCONFIG;       // 只返回本机支持的地址类型

    snprintf(port_str, sizeof(port_str), "%d", port);

    printf("[Client] 正在解析域名: %s\n", host);

    // 执行 DNS 解析
    status = getaddrinfo(host, port_str, &hints, &res);
    if (status != 0) {
        fprintf(stderr, "[Client] DNS 解析失败: %s (错误: %s)\n", 
                host, gai_strerror(status));
        return -1;
    }

    // 统计地址数量
    for (p = res; p != NULL; p = p->ai_next) {
        if (p->ai_family == AF_INET || p->ai_family == AF_INET6) {
            addr_count++;
        }
    }

    if (addr_count == 0) {
        fprintf(stderr, "[Client] 未找到有效的 IP 地址:  %s\n", host);
        freeaddrinfo(res);
        return -1;
    }

    // 分配地址数组
    result->addresses = (ResolvedAddress *)calloc(addr_count, sizeof(ResolvedAddress));
    if (!result->addresses) {
        perror("[Client] 分配内存失败");
        freeaddrinfo(res);
        return -1;
    }

    // 填充地址信息 (优先级排序)
    int ipv4_idx = 0;
    int ipv6_idx = 0;
    
    // 先统计各类型数量
    for (p = res; p != NULL; p = p->ai_next) {
        if (p->ai_family == AF_INET) ipv4_idx++;
        else if (p->ai_family == AF_INET6) ipv6_idx++;
    }

    int v4_count = ipv4_idx;
    int v6_count = ipv6_idx;
    
    // 根据偏好设置插入位置
    int v4_insert = prefer_ipv4 ? 0 : v6_count;
    int v6_insert = prefer_ipv4 ? v4_count : 0;
    
    ipv4_idx = v4_insert;
    ipv6_idx = v6_insert;

    for (p = res; p != NULL; p = p->ai_next) {
        ResolvedAddress *addr = NULL;
        
        if (p->ai_family == AF_INET) {
            addr = &result->addresses[ipv4_idx++];
            addr->family = AF_INET;
            memcpy(&addr->addr.v4, p->ai_addr, sizeof(struct sockaddr_in));
            addr->addr_len = sizeof(struct sockaddr_in);
            inet_ntop(AF_INET, &addr->addr.v4.sin_addr, 
                     addr->ip_str, sizeof(addr->ip_str));
        } else if (p->ai_family == AF_INET6) {
            addr = &result->addresses[ipv6_idx++];
            addr->family = AF_INET6;
            memcpy(&addr->addr.v6, p->ai_addr, sizeof(struct sockaddr_in6));
            addr->addr_len = sizeof(struct sockaddr_in6);
            inet_ntop(AF_INET6, &addr->addr.v6.sin6_addr, 
                     addr->ip_str, sizeof(addr->ip_str));
        }
    }

    result->count = addr_count;
    result->current_index = 0;

    freeaddrinfo(res);

    // 打印解析结果
    printf("[Client] DNS 解析成功，共 %d 个地址:\n", result->count);
    for (int i = 0; i < result->count; i++) {
        const char *type = (result->addresses[i].family == AF_INET) ? "IPv4" : "IPv6";
        printf("  [%d] %s: %s\n", i + 1, type, result->addresses[i].ip_str);
    }

    return 0;
}

// 检测是否为 IP 地址 (而非域名)
int is_ip_address(const char *host) {
    struct in_addr ipv4_addr;
    struct in6_addr ipv6_addr;
    
    // 尝试解析为 IPv4
    if (inet_pton(AF_INET, host, &ipv4_addr) == 1) {
        return AF_INET;
    }
    
    // 尝试解析为 IPv6
    if (inet_pton(AF_INET6, host, &ipv6_addr) == 1) {
        return AF_INET6;
    }
    
    return 0;  // 是域名
}

// 配置 socket (支持 IPv4/IPv6)
int configure_client_socket(int sockfd, int family) {
    // 设置接收缓冲区
    int recv_buf = 1024 * 1024;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &recv_buf, sizeof(recv_buf));

    // 设置发送缓冲区
    int send_buf = 1024 * 1024;
    setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &send_buf, sizeof(send_buf));

    // 设置 Don't Fragment 标志
    if (family == AF_INET) {
#ifdef IP_MTU_DISCOVER
        int mtu_discover = IP_PMTUDISC_DO;
        if (setsockopt(sockfd, IPPROTO_IP, IP_MTU_DISCOVER, 
                       &mtu_discover, sizeof(mtu_discover)) < 0) {
            perror("[Client] 设置 IPv4 MTU 发现失败");
            return -1;
        }
        printf("[Client] 已启用 IPv4 DF (Don't Fragment) 标志\n");
#endif
    } else if (family == AF_INET6) {
#ifdef IPV6_MTU_DISCOVER
        int mtu_discover = IPV6_PMTUDISC_DO;
        if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_MTU_DISCOVER, 
                       &mtu_discover, sizeof(mtu_discover)) < 0) {
            perror("[Client] 设置 IPv6 MTU 发现失败");
            return -1;
        }
        printf("[Client] 已启用 IPv6 DF 标志\n");
#endif
    }

    return 0;
}

// 发送探测包 (通用版本，支持 IPv4/IPv6)
int send_probe_packet(int sockfd, const ResolvedAddress *addr,
                      MtuProbeContext *ctx, uint32_t probe_size) {
    // 分配探测包缓冲区
    uint8_t *buffer = (uint8_t *)malloc(probe_size);
    if (!buffer) {
        perror("[Client] 分配探测包缓冲区失败");
        return -1;
    }

    // 填充探测包
    memset(buffer, 0xAA, probe_size);
    
    ProbeHeader *header = (ProbeHeader *)buffer;
    header->type = MSG_TYPE_PROBE_REQ;
    header->flags = 0;
    header->seq = htons(ctx->probe_seq);
    header->probe_size = htonl(probe_size);
    header->timestamp = htonl((uint32_t)get_timestamp_ms());
    header->checksum = htonl(calculate_checksum(buffer, probe_size - sizeof(uint32_t)));

    // 获取正确的地址指针
    const struct sockaddr *sa = (addr->family == AF_INET) 
        ? (const struct sockaddr *)&addr->addr. v4 
        : (const struct sockaddr *)&addr->addr.v6;

    // 发送探测包
    ssize_t sent = sendto(sockfd, buffer, probe_size, 0, sa, addr->addr_len);
    
    free(buffer);

    if (sent < 0) {
        if (errno == EMSGSIZE) {
            printf("[Client] 探测大小 %u 超过路径 MTU (EMSGSIZE)\n", probe_size);
            return -2;
        }
        perror("[Client] 发送探测包失败");
        return -1;
    }

    printf("[Client] 发送探测包:  序列号=%u, 大小=%u, 目标=%s\n", 
           ctx->probe_seq, probe_size, addr->ip_str);
    
    ctx->last_probe_time = get_timestamp_ms();
    return 0;
}

// 等待探测响应
int wait_for_probe_ack(int sockfd, MtuProbeContext *ctx, int timeout_ms) {
    uint8_t buffer[PROBE_HEADER_SIZE + 64];
    
    fd_set readfds;
    struct timeval tv;
    
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);
    
    tv.tv_sec = timeout_ms / 1000;
    tv. tv_usec = (timeout_ms % 1000) * 1000;

    int ret = select(sockfd + 1, &readfds, NULL, NULL, &tv);
    if (ret < 0) {
        if (errno == EINTR) {
            return -1;
        }
        perror("[Client] select 失败");
        return -1;
    }
    
    if (ret == 0) {
        printf("[Client] 等待 ACK 超时 (序列号=%u)\n", ctx->probe_seq);
        return -2;
    }

    // 接收响应
    struct sockaddr_storage from_addr;
    socklen_t from_len = sizeof(from_addr);
    
    ssize_t recv_len = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                                (struct sockaddr *)&from_addr, &from_len);
    if (recv_len < (ssize_t)PROBE_HEADER_SIZE) {
        printf("[Client] 收到无效响应，大小:  %zd\n", recv_len);
        return -1;
    }

    const ProbeHeader *ack_header = (const ProbeHeader *)buffer;
    
    if (ack_header->type == MSG_TYPE_PROBE_ACK) {
        uint16_t ack_seq = ntohs(ack_header->seq);
        uint32_t ack_size = ntohl(ack_header->probe_size);
        
        if (ack_seq != ctx->probe_seq) {
            printf("[Client] 序列号不匹配:  期望 %u, 收到 %u\n", 
                   ctx->probe_seq, ack_seq);
            return -1;
        }
        
        printf("[Client] 收到 ACK:  序列号=%u, 确认大小=%u\n", ack_seq, ack_size);
        return (int)ack_size;
    } else if (ack_header->type == MSG_TYPE_PROBE_NACK) {
        printf("[Client] 收到 NACK: 探测失败\n");
        return -3;
    }

    printf("[Client] 收到未知响应类型: 0x%02x\n", ack_header->type);
    return -1;
}

// 计算下一个探测大小
uint32_t calculate_next_probe_size(MtuProbeContext *ctx) {
    return (ctx->probe_low + ctx->probe_high) / 2;
}

// 测试服务器连通性
int test_server_connectivity(int sockfd, const ResolvedAddress *addr, 
                             MtuProbeContext *ctx) {
    printf("[Client] 测试与服务器 %s 的连通性.. .\n", addr->ip_str);
    
    ctx->probe_seq = 1;
    
    // 使用最小 MTU 测试连通性
    if (send_probe_packet(sockfd, addr, ctx, MTU_PROBE_MIN) < 0) {
        return -1;
    }
    
    int ack_ret = wait_for_probe_ack(sockfd, ctx, MTU_PROBE_TIMEOUT_MS * 2);
    if (ack_ret > 0) {
        printf("[Client] 服务器连通性测试成功\n");
        return 0;
    }
    
    printf("[Client] 服务器连通性测试失败\n");
    return -1;
}

// 执行 MTU 探测
int perform_mtu_discovery(int sockfd, const ResolvedAddress *addr,
                          MtuProbeContext *ctx) {
    printf("\n[Client] 开始 MTU 探测...\n");
    printf("[Client] 目标地址: %s\n", addr->ip_str);
    printf("[Client] 探测范围: [%u, %u]\n", ctx->probe_low, ctx->probe_high);
    
    ctx->state = PROBE_STATE_SEARCHING;
    
    while (g_running && ctx->state == PROBE_STATE_SEARCHING) {
        ctx->probe_target = calculate_next_probe_size(ctx);
        
        if (ctx->probe_high - ctx->probe_low <= MTU_PROBE_STEP) {
            ctx->current_mtu = ctx->probe_low;
            ctx->state = PROBE_STATE_COMPLETE;
            printf("[Client] 探测完成，确定 MTU = %u\n", ctx->current_mtu);
            break;
        }
        
        printf("\n[Client] 探测目标: %u (范围: [%u, %u])\n", 
               ctx->probe_target, ctx->probe_low, ctx->probe_high);
        
        ctx->retry_count = 0;
        bool probe_success = false;
        
        while (ctx->retry_count < MTU_PROBE_MAX_RETRY && ! probe_success) {
            ctx->probe_seq++;
            
            int send_ret = send_probe_packet(sockfd, addr, ctx, ctx->probe_target);
            if (send_ret == -2) {
                printf("[Client] 本地检测到包过大，缩小探测范围\n");
                ctx->probe_high = ctx->probe_target - 1;
                break;
            } else if (send_ret < 0) {
                ctx->retry_count++;
                continue;
            }

            int ack_ret = wait_for_probe_ack(sockfd, ctx, MTU_PROBE_TIMEOUT_MS);
            if (ack_ret > 0) {
                ctx->probe_low = ctx->probe_target;
                ctx->current_mtu = ctx->probe_target;
                ctx->last_success_time = get_timestamp_ms();
                probe_success = true;
                printf("[Client] 探测成功，更新下界为 %u\n", ctx->probe_low);
            } else if (ack_ret == -2) {
                ctx->retry_count++;
                printf("[Client] 重试 %d/%d\n", ctx->retry_count, MTU_PROBE_MAX_RETRY);
            } else {
                ctx->retry_count++;
            }
        }
        
        if (! probe_success && ctx->retry_count >= MTU_PROBE_MAX_RETRY) {
            printf("[Client] 探测失败，缩小探测范围\n");
            ctx->probe_high = ctx->probe_target - 1;
        }
        
        usleep(100 * 1000);
    }
    
    return ctx->current_mtu;
}

// 验证最终 MTU
int verify_final_mtu(int sockfd, const ResolvedAddress *addr,
                     MtuProbeContext *ctx) {
    printf("\n[Client] 验证最终 MTU:  %u\n", ctx->current_mtu);
    
    ctx->probe_seq++;
    
    if (send_probe_packet(sockfd, addr, ctx, ctx->current_mtu) < 0) {
        return -1;
    }
    
    int ack_ret = wait_for_probe_ack(sockfd, ctx, MTU_PROBE_TIMEOUT_MS * 2);
    if (ack_ret > 0) {
        printf("[Client] MTU 验证成功:  %u bytes\n", ctx->current_mtu);
        return ctx->current_mtu;
    }
    
    printf("[Client] MTU 验证失败\n");
    return -1;
}

// 打印探测结果
void print_probe_result(const MtuProbeContext *ctx, const ResolvedAddress *addr) {
    int ip_header_size = (addr->family == AF_INET) ? IP_HEADER_SIZE : IPV6_HEADER_SIZE;
    
    printf("\n");
    printf("===========================================\n");
    printf("    MTU 探测结果\n");
    printf("===========================================\n");
    printf("  目标地址:        %s\n", addr->ip_str);
    printf("  地址类型:       %s\n", (addr->family == AF_INET) ? "IPv4" :  "IPv6");
    printf("  状态:           %s\n", probe_state_to_string(ctx->state));
    printf("  路径 MTU:        %u bytes\n", ctx->current_mtu);
    printf("  IP 头部大小:    %d bytes\n", ip_header_size);
    printf("  UDP 头部大小:   %d bytes\n", UDP_HEADER_SIZE);
    printf("  最大 UDP 载荷:   %u bytes\n", 
           ctx->current_mtu - ip_header_size - UDP_HEADER_SIZE);
    printf("  探测次数:       %u\n", ctx->probe_seq);
    printf("===========================================\n");
}

void print_usage(const char *prog) {
    printf("用法:  %s [选项] <服务器地址> [端口] [最大MTU]\n", prog);
    printf("\n参数:\n");
    printf("  服务器地址    服务端 IP 地址或域名 (支持 IPv4/IPv6)\n");
    printf("  端口          服务端端口 (默认: %d)\n", DEFAULT_SERVER_PORT);
    printf("  最大MTU       探测最大值 (默认:  %d, 巨型帧:  %d)\n", 
           MTU_PROBE_MAX_ETH, MTU_PROBE_MAX_JUMBO);
    printf("\n选项:\n");
    printf("  -4            优先使用 IPv4 地址\n");
    printf("  -6            优先使用 IPv6 地址\n");
    printf("  -a <index>    使用指定索引的地址 (从 1 开始)\n");
    printf("  -h            显示此帮助信息\n");
    printf("\n示例:\n");
    printf("  %s example.com\n", prog);
    printf("  %s -4 example.com 9999\n", prog);
    printf("  %s -6 example.com 9999 9000\n", prog);
    printf("  %s 192.168.1.100 9999 1500\n", prog);
    printf("  %s :: 1 9999\n", prog);
}

int main(int argc, char *argv[]) {
    int prefer_ipv4 = 1;           // 默认优先 IPv4
    int force_addr_index = -1;     // 强制使用指定地址索引
    int opt;
    
    // 解析命令行选项
    while ((opt = getopt(argc, argv, "46a:h")) != -1) {
        switch (opt) {
            case '4':
                prefer_ipv4 = 1;
                break;
            case '6':
                prefer_ipv4 = 0;
                break;
            case 'a': 
                force_addr_index = atoi(optarg) - 1;
                break;
            case 'h': 
            default:
                print_usage(argv[0]);
                return (opt == 'h') ? EXIT_SUCCESS : EXIT_FAILURE;
        }
    }

    // 检查剩余参数
    if (optind >= argc) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    const char *server_host = argv[optind];
    int server_port = (optind + 1 < argc) ? atoi(argv[optind + 1]) : DEFAULT_SERVER_PORT;
    uint32_t max_mtu = (optind + 2 < argc) ? (uint32_t)atoi(argv[optind + 2]) : MTU_PROBE_MAX_ETH;

    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    printf("===========================================\n");
    printf("    UDP MTU 探测客户端 (域名支持版)\n");
    printf("===========================================\n");
    printf("[Client] 目标主机: %s\n", server_host);
    printf("[Client] 目标端口: %d\n", server_port);
    printf("[Client] 最大探测 MTU: %u\n", max_mtu);
    printf("[Client] 地址偏好: %s\n", prefer_ipv4 ? "IPv4" : "IPv6");
    printf("\n");

    // 解析域名
    ResolveResult resolve_result;
    if (resolve_host(server_host, server_port, &resolve_result, prefer_ipv4) < 0) {
        return EXIT_FAILURE;
    }

    // 确定使用的地址
    int addr_index = 0;
    if (force_addr_index >= 0) {
        if (force_addr_index >= resolve_result.count) {
            fprintf(stderr, "[Client] 无效的地址索引:  %d (共 %d 个地址)\n",
                    force_addr_index + 1, resolve_result.count);
            free_resolve_result(&resolve_result);
            return EXIT_FAILURE;
        }
        addr_index = force_addr_index;
    }

    ResolvedAddress *target_addr = &resolve_result.addresses[addr_index];
    printf("\n[Client] 使用地址: [%d] %s (%s)\n", 
           addr_index + 1, 
           target_addr->ip_str,
           (target_addr->family == AF_INET) ? "IPv4" : "IPv6");

    // 创建对应协议族的 socket
    int sockfd = socket(target_addr->family, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("[Client] 创建 socket 失败");
        free_resolve_result(&resolve_result);
        return EXIT_FAILURE;
    }

    // 配置 socket
    if (configure_client_socket(sockfd, target_addr->family) < 0) {
        close(sockfd);
        free_resolve_result(&resolve_result);
        return EXIT_FAILURE;
    }

    // 初始化探测上下文
    MtuProbeContext ctx;
    init_probe_context(&ctx, max_mtu);

    // 测试连通性
    if (test_server_connectivity(sockfd, target_addr, &ctx) < 0) {
        // 如果有多个地址，尝试下一个
        bool connected = false;
        for (int i = 0; i < resolve_result.count && !connected; i++) {
            if (i == addr_index) continue;
            
            printf("\n[Client] 尝试备用地址 [%d]:  %s\n", 
                   i + 1, resolve_result.addresses[i].ip_str);
            
            // 关闭旧 socket，创建新的
            close(sockfd);
            target_addr = &resolve_result.addresses[i];
            
            sockfd = socket(target_addr->family, SOCK_DGRAM, 0);
            if (sockfd < 0) continue;
            
            if (configure_client_socket(sockfd, target_addr->family) < 0) {
                close(sockfd);
                continue;
            }
            
            init_probe_context(&ctx, max_mtu);
            if (test_server_connectivity(sockfd, target_addr, &ctx) == 0) {
                connected = true;
                addr_index = i;
            }
        }
        
        if (!connected) {
            fprintf(stderr, "[Client] 无法连接到服务器的任何地址\n");
            close(sockfd);
            free_resolve_result(&resolve_result);
            return EXIT_FAILURE;
        }
    }

    // 重新初始化上下文
    init_probe_context(&ctx, max_mtu);

    // 执行 MTU 探测
    int discovered_mtu = perform_mtu_discovery(sockfd, target_addr, &ctx);
    
    if (discovered_mtu > 0 && g_running) {
        verify_final_mtu(sockfd, target_addr, &ctx);
    }

    // 打印结果
    print_probe_result(&ctx, target_addr);

    // 清理
    close(sockfd);
    free_resolve_result(&resolve_result);
    
    return (ctx.state == PROBE_STATE_COMPLETE) ? EXIT_SUCCESS : EXIT_FAILURE;
}