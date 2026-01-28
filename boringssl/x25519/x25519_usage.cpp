// g++ x25519_usage.cpp -std=c++17 -I/home/eular/Github/lsquic/boringssl/include /home/eular/Github/lsquic/boringssl/lib/libcrypto.a /home/eular/Github/lsquic/boringssl/lib/libssl.a -pthread


#include <algorithm>
#include <iostream>
#include <iomanip>

#include "x25519_wrapper.hpp"

void print_key(const std::string& label, const auto& key) {
    std::cout << label << ": ";
    for (uint8_t byte :  key) {
        std::cout << std::hex << std:: setfill('0') << std::setw(2) 
                  << static_cast<int>(byte);
    }
    std::cout << std::dec << std:: endl;
}

int main() {
    try {
        // 创建双方密钥对
        X25519KeyPair alice;
        X25519KeyPair bob;

        std::cout << "=== 公钥交换 ===" << std::endl;
        print_key("Alice 公钥", alice. public_key());
        print_key("Bob 公钥  ", bob.public_key());

        // 计算共享密钥
        auto alice_secret = alice.derive_shared_secret(bob.public_key());
        auto bob_secret = bob.derive_shared_secret(alice.public_key());

        // AES-128
        std::array<uint8_t, 16> alice_secret_2;
        std::array<uint8_t, 16> bob_secret_2;
        std::copy_n(alice_secret.begin(), 16, alice_secret_2.begin());
        std::copy_n(bob_secret.begin(), 16, bob_secret_2.begin());

        std::cout << "\n=== 共享密钥 ===" << std:: endl;
        print_key("Alice 端", alice_secret_2);
        print_key("Bob 端  ", bob_secret_2);

        if (alice_secret_2 == bob_secret_2) {
            std::cout << "\n✓ 密钥交换成功！" << std::endl;
        }

    } catch (const std::exception& e) {
        std:: cerr << "错误: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}