/*************************************************************************
    > File Name: gzip.hpp
    > Author: hsz
    > Brief:
    > Created Time: 2024年12月16日 星期一 14时24分07秒
 ************************************************************************/

#pragma once
#include <zlib.h>

#include <utils/elapsed_time.h>
#include <utils/buffer.h>

#include "common.hpp"

namespace gzip {

void benchmark(const eular::ByteBuffer &in_buffer)
{
    eular::ByteBuffer out_buffer;
    out_buffer.reserve(compressBound(in_buffer.size()));

    for (int32_t level = 1; level <= Z_BEST_COMPRESSION; ++level) {
        z_stream stream;
        stream.zalloc = Z_NULL;
        stream.zfree = Z_NULL;
        stream.opaque = Z_NULL;
        int32_t status = deflateInit(&stream, level);  // 默认压缩级别

        stream.avail_in = in_buffer.size();
        stream.next_in = (uint8_t *)in_buffer.const_data();
        stream.avail_out = out_buffer.capacity();
        stream.next_out = out_buffer.data();

        eular::ElapsedTime elapsed(eular::ElapsedTimeType::NANOSECOND);
        elapsed.start();
        status = deflate(&stream, Z_FINISH);
        elapsed.stop();

        if (status != Z_STREAM_END) {
            deflateEnd(&stream);
            printf("status(%d) != Z_STREAM_END\n", status);
            return;
        }

        size_t out_size = out_buffer.capacity() - stream.avail_out;
        out_buffer.resize(out_size);

        double ratio = (double)out_buffer.size() / in_buffer.size() * 100.0;

        uint64_t ns = elapsed.elapsedTime();
        double speed = (double)in_buffer.size() / ns;
        printf("gzip version %s: level: %d, ratio: %.3f compress speed: %.2f MB/s\n", ZLIB_VERSION, level, ratio, BPerNS2MBPerS(speed));

        deflateEnd(&stream);
    }
}

} // namespace gzip
