/*************************************************************************
    > File Name: udp_server.cc
    > Author: eular
    > Brief:
    > Created Time: Mon 24 Nov 2025 04:00:40 PM CST
 ************************************************************************/

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <errno.h>

#define MAX_BATCH 32           // recvmmsg每批最多收多少包
#define BUF_SIZE  (16 * 1024)  // 每包最大缓存（GRO模式可设大一点）

int main()
{
    // 创建UDP socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket error");
        return 1;
    }

    // 设置地址和端口
    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(9000);
    serv_addr.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0

    // 绑定
    if (bind(sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind error");
        close(sock);
        return 1;
    }
    std::cout << "UDP服务器已启动, 监听 0.0.0.0:9000\n";

    // 批量收包结构体
    struct mmsghdr msgs[MAX_BATCH];
    struct msghdr hdrs[MAX_BATCH];
    struct iovec iovs[MAX_BATCH];
    std::vector<std::vector<char>> bufs(MAX_BATCH, std::vector<char>(BUF_SIZE));
    sockaddr_in peer_addrs[MAX_BATCH];
    memset(msgs, 0, sizeof(msgs));
    memset(hdrs, 0, sizeof(hdrs));
    memset(peer_addrs, 0, sizeof(peer_addrs));

    for (int i = 0; i < MAX_BATCH; ++i) {
        iovs[i].iov_base = bufs[i].data();
        iovs[i].iov_len  = BUF_SIZE;
        hdrs[i].msg_name = &peer_addrs[i];
        hdrs[i].msg_namelen = sizeof(sockaddr_in);
        hdrs[i].msg_iov = &iovs[i];
        hdrs[i].msg_iovlen = 1;
        msgs[i].msg_hdr = hdrs[i];
    }

    // 主循环批量收包
    while (true) {
        // 非阻塞套接字会尽可能的多读取数据包
        // 阻塞套接字则会等到 MAX_BATCH 个才会返回, 故阻塞套接字建议设置合理的超时
        struct timespec timeout{};  
        timeout.tv_sec = 1;
        timeout.tv_nsec = 0;
        int n = recvmmsg(sock, msgs, MAX_BATCH, MSG_DONTWAIT, &timeout);
        if (n < 0) {
            if (errno == EINTR || errno == ETIMEDOUT || errno == EAGAIN) {
                usleep(100000); // 100ms
                continue;
            }
            perror("recvmmsg error");
            break;
        }

        printf("\n本次收到 %d 个数据包\n", n);
        for (int i = 0; i < n; ++i) {
            char addrbuf[64]{};
            inet_ntop(AF_INET, &(peer_addrs[i].sin_addr), addrbuf, sizeof(addrbuf));
            printf("收到数据包，来自 %s:%d, 长度 %u\n",
                   addrbuf, ntohs(peer_addrs[i].sin_port), msgs[i].msg_len);
            // 如需解析内容，可：bufs[i].data(), msgs[i].msg_len
        }
    }

    close(sock);
    return 0;
}