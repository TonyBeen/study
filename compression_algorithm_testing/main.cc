/*************************************************************************
    > File Name: main.cc
    > Author: hsz
    > Brief: g++ main.cc -std=c++11 -lutils -lz -llz4 -lzstd -O2 -g
    > Created Time: 2024年12月16日 星期一 14时24分53秒
 ************************************************************************/

#include <random>

#include "gzip.hpp"
#include "lz4.hpp"
#include "zstd.hpp"

#include <utils/file.h>

#define BIN

int main(int argc, char **argv)
{
    size_t size = 1024 * 16;  // 1MB 数据
    size_t cycle = 1024 * 1024 / size;

    eular::ByteBuffer buffer;
    buffer.reserve(size * cycle);

    eular::ByteBuffer temp;
    temp.reserve(size);
    temp.resize(size);

    std::random_device rd;
    std::mt19937 gen(rd());

#ifdef BIN
    std::uniform_int_distribution<uint8_t> dis(0, 255);

    for (size_t i = 0; i < size; ++i) {
        temp[i] = dis(gen);
    }
#else
    const std::string charset = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()-_=+[]{}|;:,.<>?";
    std::uniform_int_distribution<uint8_t> dis(0, charset.size() - 1);
    for (size_t i = 0; i < size; ++i) {
        temp[i] = charset[dis(gen)];
    }
#endif

    for (size_t i = 0; i < cycle; ++i) {
        buffer.append(temp);
    }
    printf("cycle = %zu, size = %zu\n", cycle, buffer.size());

    // eular::FileOp fileOp("something.out", eular::FileOp::OpenModeFlag::ReadOnly);
    // buffer.reserve(fileOp.fileSize());
    // buffer.resize(fileOp.read(buffer.data(), buffer.capacity()));
    // fileOp.close();

    gzip::benchmark(buffer);
    printf("\n");
    lz4::benchmark(buffer);
    printf("\n");
    zstd::benchmark(buffer);

    return 0;
}
