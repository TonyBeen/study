#pragma once

#include <array>
#include <stdexcept>
#include <cstring>

#include <openssl/curve25519.h>
#include <openssl/rand.h>
#include <openssl/mem.h>

class X25519KeyPair {
public:
    static constexpr size_t PRIVATE_KEY_SIZE = X25519_PRIVATE_KEY_LEN;  // 32
    static constexpr size_t PUBLIC_KEY_SIZE = X25519_PUBLIC_VALUE_LEN;  // 32
    static constexpr size_t SHARED_SECRET_SIZE = X25519_SHARED_KEY_LEN; // 32

    using PrivateKey = std::array<uint8_t, PRIVATE_KEY_SIZE>;
    using PublicKey = std::array<uint8_t, PUBLIC_KEY_SIZE>;
    using SharedSecret = std::array<uint8_t, SHARED_SECRET_SIZE>;

    X25519KeyPair() {
        X25519_keypair(public_key_.data(), private_key_.data());
    }

    // 从已有私钥创建
    explicit X25519KeyPair(const PrivateKey& private_key) 
        : private_key_(private_key) {
        X25519_public_from_private(public_key_.data(), private_key_.data());
    }

    // 禁止拷贝（保护私钥）
    X25519KeyPair(const X25519KeyPair&) = delete;
    X25519KeyPair& operator=(const X25519KeyPair&) = delete;

    // 允许移动
    X25519KeyPair(X25519KeyPair&&) = default;
    X25519KeyPair& operator=(X25519KeyPair&&) = default;

    // 获取公钥（可以安全分享）
    const PublicKey& public_key() const { return public_key_; }

    // 与对方公钥计算共享密钥
    SharedSecret derive_shared_secret(const PublicKey& peer_public_key) const {
        SharedSecret shared_secret;

        if (! X25519(shared_secret.data(), 
                    private_key_.data(), 
                    peer_public_key.data())) {
            throw std:: runtime_error("X25519 密钥交换失败");
        }

        return shared_secret;
    }

    // 析构时清除私钥
    ~X25519KeyPair() {
        secure_zero(private_key_.data(), private_key_.size());
    }

protected:
    inline void secure_zero(void* ptr, size_t len)
    {
        // NOTE 不使用memset的原因是编译器可能会优化掉对不再使用的内存的清零操作. c11标准提供了一个安全清零的函数: memset_s
        // 使用 volatile 指针防止编译器优化掉清零操作
        volatile unsigned char* p = static_cast<volatile unsigned char*>(ptr);
        while (len--) {
            *p++ = 0;
        }
    }

private:
    PrivateKey private_key_;
    PublicKey public_key_;
};