/*************************************************************************
    > File Name: udp_socket.cpp
    > Author: hsz
    > Brief:
    > Created Time: Mon 15 Dec 2025 03:44:04 PM CST
 ************************************************************************/

#include "udp_socket.h"
#include <cstdio>
#include <cstring>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

UdpSocket::UdpSocket() : fd_(-1), peer_port_(0) {
    fd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (fd_ < 0) {
        std::perror("socket");
    }
}

UdpSocket::~UdpSocket() {
    if (fd_ >= 0) ::close(fd_);
}

bool UdpSocket::bind(const std::string& host, uint16_t port) {
    if (fd_ < 0) return false;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (::inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1) {
        std::fprintf(stderr, "inet_pton failed for %s\n", host.c_str());
        return false;
    }
    if (::bind(fd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::perror("bind");
        return false;
    }
    return true;
}

bool UdpSocket::connect(const std::string& host, uint16_t port) {
    if (fd_ < 0) return false;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (::inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1) {
        std::fprintf(stderr, "inet_pton failed for %s\n", host.c_str());
        return false;
    }
    if (::connect(fd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::perror("connect");
        return false;
    }
    peer_host_ = host;
    peer_port_ = port;
    return true;
}

bool UdpSocket::send(const std::vector<uint8_t>& data) {
    if (fd_ < 0) return false;
    ssize_t n = ::send(fd_, data.data(), data.size(), 0);
    if (n < 0) { std::perror("send"); return false; }
    return (size_t)n == data.size();
}

std::optional<UdpSocket::RecvResult> UdpSocket::recv() {
    if (fd_ < 0) return std::nullopt;
    std::vector<uint8_t> buf(65536);
    sockaddr_in peer{};
    socklen_t len = sizeof(peer);
    ssize_t n = ::recvfrom(fd_, buf.data(), buf.size(), 0, (sockaddr*)&peer, &len);
    if (n < 0) { std::perror("recvfrom"); return std::nullopt; }
    char addrstr[INET_ADDRSTRLEN] = {0};
    ::inet_ntop(AF_INET, &peer.sin_addr, addrstr, sizeof(addrstr));
    RecvResult r;
    r.peer_host = addrstr;
    r.peer_port = ntohs(peer.sin_port);
    r.data.assign(buf.begin(), buf.begin() + n);
    return r;
}

bool UdpSocket::sendto(const std::string& host, uint16_t port, const std::vector<uint8_t>& data) {
    if (fd_ < 0) return false;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (::inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1) {
        std::fprintf(stderr, "inet_pton failed for %s\n", host.c_str());
        return false;
    }
    ssize_t n = ::sendto(fd_, data.data(), data.size(), 0, (sockaddr*)&addr, sizeof(addr));
    if (n < 0) { std::perror("sendto"); return false; }
    return (size_t)n == data.size();
}