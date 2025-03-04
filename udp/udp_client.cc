/*************************************************************************
    > File Name: upd_client.cc
    > Author: hsz
    > Brief:
    > Created Time: Sat 18 Jun 2022 11:45:03 AM CST
 ************************************************************************/

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/errqueue.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#define PORT 8080

int main() {
    int udp_socket;
    struct sockaddr_in server_addr, client_addr;
    char buffer[1024];
    socklen_t addr_len = sizeof(client_addr);

    // 创建 UDP 套接字
    if ((udp_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 设置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 绑定套接字
    if (bind(udp_socket, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(udp_socket);
        exit(EXIT_FAILURE);
    }

    // 启用接收 ICMP 错误报文
    int enable = 1;
    if (setsockopt(udp_socket, IPPROTO_IP, IP_RECVERR, &enable, sizeof(enable)) < 0) {
        perror("setsockopt IP_RECVERR failed");
        close(udp_socket);
        exit(EXIT_FAILURE);
    }

    while (true) {
        // 接收数据或 ICMP 错误报文
        struct msghdr msg;
        struct iovec iov;
        char cmsgbuf[512];
        struct sockaddr_in remote;
        struct cmsghdr *cmsg;
        struct sock_extended_err *sock_err;

        char data[1024];
        iov.iov_base = data;
        iov.iov_len = sizeof(data);

        msg.msg_name = &remote;
        msg.msg_namelen = sizeof(remote);
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_control = cmsgbuf;
        msg.msg_controllen = sizeof(cmsgbuf);
        msg.msg_flags = 0;

        ssize_t len = recvmsg(udp_socket, &msg, 0);
        if (len < 0) {
            perror("recvmsg failed");
            close(udp_socket);
            exit(EXIT_FAILURE);
        }

        // 检查是否为 ICMP 错误数据
        bool is_icmp_error = false;
        for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
            if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_RECVERR) {
                is_icmp_error = true;
                sock_err = (struct sock_extended_err *)CMSG_DATA(cmsg);
                if (sock_err->ee_origin == SO_EE_ORIGIN_ICMP) {
                    std::cout << "Received ICMP error message" << std::endl;
                    std::cout << "ICMP Type: " << static_cast<int>(sock_err->ee_type) << std::endl;
                    std::cout << "ICMP Code: " << static_cast<int>(sock_err->ee_code) << std::endl;
                }
            }
        }

        if (!is_icmp_error) {
            // 普通数据
            data[len] = '\0';
            std::cout << "Received message: " << data << std::endl;
        }
    }

    close(udp_socket);
    return 0;
}