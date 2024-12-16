/*************************************************************************
    > File Name: zstd.hpp
    > Author: hsz
    > Brief:
    > Created Time: 2024年12月16日 星期一 14时24分19秒
 ************************************************************************/

#pragma once
#include <zstd.h>

#include <utils/elapsed_time.h>
#include <utils/buffer.h>

#include "common.hpp"

namespace zstd {
void benchmark(const eular::ByteBuffer &in_buffer)
{
    eular::ByteBuffer out_buffer;
    out_buffer.reserve(ZSTD_compressBound(in_buffer.size()));

    for (int32_t level = 1; level <= 22; ++level) {
        eular::ElapsedTime elapsed(eular::ElapsedTimeType::NANOSECOND);
        elapsed.start();
        size_t compressed_size = ZSTD_compress(out_buffer.data(), out_buffer.capacity(), in_buffer.const_data(), in_buffer.size(), level);
        elapsed.stop();

        if (ZSTD_isError(compressed_size)) {
            printf("压缩失败: %s\n", ZSTD_getErrorName(compressed_size));
            return;
        }

        out_buffer.resize(compressed_size);

        double ratio = (double)out_buffer.size() / in_buffer.size() * 100.0;
        uint64_t ns = elapsed.elapsedTime();
        double speed = (double)in_buffer.size() / ns;
        printf("zstd version %s: level: %d, ratio: %.3f compress speed: %.2f MB/s\n", ZSTD_versionString(), level, ratio, BPerNS2MBPerS(speed));
    }
}
} // namespace zstd
