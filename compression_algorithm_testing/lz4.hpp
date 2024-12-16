/*************************************************************************
    > File Name: lz4.hpp
    > Author: hsz
    > Brief:
    > Created Time: 2024年12月16日 星期一 14时24分39秒
 ************************************************************************/

#pragma once
#include <lz4.h>

#include <utils/elapsed_time.h>
#include <utils/buffer.h>

#include "common.hpp"

namespace lz4 {
void benchmark(const eular::ByteBuffer &in_buffer)
{
    eular::ByteBuffer out_buffer;
    out_buffer.reserve(LZ4_compressBound(in_buffer.size()));

    for (int32_t level = 1; level <= 12; ++level) {
        eular::ElapsedTime elapsed(eular::ElapsedTimeType::NANOSECOND);
        elapsed.start();
        int32_t compressed_size = LZ4_compress_fast((const char *)in_buffer.const_data(), (char *)out_buffer.data(),
            in_buffer.size(), out_buffer.capacity(), level);
        elapsed.stop();

        if (compressed_size <= 0) {
            printf("LZ4_compress_fast error\n");
            return;
        }
        out_buffer.resize(compressed_size);

        double ratio = (double)out_buffer.size() / in_buffer.size() * 100.0;
        uint64_t ns = elapsed.elapsedTime();
        double speed = (double)in_buffer.size() / ns;
        printf("lz4 version %s: level: %d, ratio: %.3f compress speed: %.2f MB/s\n", LZ4_versionString(), level, ratio, BPerNS2MBPerS(speed));
    }
}
} // namespace lz4
