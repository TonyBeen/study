/*************************************************************************
    > File Name: udp_socket.h
    > Author: hsz
    > Brief:
    > Created Time: Mon 15 Dec 2025 03:43:35 PM CST
 ************************************************************************/

#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <optional>

// 一个极简的 UDP 封装，提供绑定、收发、连接（设置默认目的地址）
class UdpSocket {
public:
    UdpSocket();
    ~UdpSocket();

    // 绑定到本地地址与端口，例如 "0.0.0.0", 9000
    bool bind(const std::string& host, uint16_t port);

    // 设置默认的远端地址（类似 connect，但仍是 UDP）
    bool connect(const std::string& host, uint16_t port);

    // 发送（使用 connect 设置的远端地址）
    bool send(const std::vector<uint8_t>& data);

    // 接收（阻塞），返回对端地址信息与数据
    struct RecvResult {
        std::string peer_host;
        uint16_t peer_port;
        std::vector<uint8_t> data;
    };
    std::optional<RecvResult> recv();

    // 发送到指定对端（服务器端常用）
    bool sendto(const std::string& host, uint16_t port, const std::vector<uint8_t>& data);

private:
    int fd_;
    std::string peer_host_;
    uint16_t peer_port_;
};