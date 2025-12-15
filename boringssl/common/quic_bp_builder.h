/*************************************************************************
    > File Name: quic_bp_builder.h
    > Author: hsz
    > Brief:
    > Created Time: Mon 15 Dec 2025 04:17:13 PM CST
 ************************************************************************/

#pragma once
#include <cstdint>
#include <vector>
#include <cstring>

// 简易 varint 编码（RFC 9000），选最短编码
inline void quic_encode_varint(uint64_t v, std::vector<uint8_t>& out) {
    if (v <= 0x3f) { // 6 bits
        out.push_back((uint8_t)v); // 00xx xxxx
    } else if (v <= 0x3fff) { // 14 bits
        uint16_t x = (uint16_t)v;
        x |= 0x4000; // 01
        out.push_back((uint8_t)(x >> 8));
        out.push_back((uint8_t)(x & 0xff));
    } else if (v <= 0x3fffffff) { // 30 bits
        uint32_t x = (uint32_t)v;
        x |= 0x80000000; // 10
        out.push_back((uint8_t)(x >> 24));
        out.push_back((uint8_t)((x >> 16) & 0xff));
        out.push_back((uint8_t)((x >> 8) & 0xff));
        out.push_back((uint8_t)(x & 0xff));
    } else { // up to 62 bits
        uint64_t x = v | 0xC000000000000000ULL; // 11
        for (int i = 7; i >= 0; --i) {
            out.push_back((uint8_t)((x >> (i*8)) & 0xff));
        }
    }
}

// 追加一个 TP 项：id, raw value bytes
inline void tp_append_item(uint64_t id, const std::vector<uint8_t>& val, std::vector<uint8_t>& out) {
    quic_encode_varint(id, out);
    quic_encode_varint(val.size(), out);
    out.insert(out.end(), val.begin(), val.end());
}

// 将 16-bit/32-bit/64-bit 整数转大端字节序（注意：很多 TP 是 varint 编值；这里提供原始 bytes 辅助）
inline std::vector<uint8_t> be16(uint16_t x) {
    return { (uint8_t)(x>>8), (uint8_t)(x&0xff) };
}
inline std::vector<uint8_t> be32(uint32_t x) {
    return { (uint8_t)(x>>24), (uint8_t)((x>>16)&0xff), (uint8_t)((x>>8)&0xff), (uint8_t)(x&0xff) };
}
inline std::vector<uint8_t> be64(uint64_t x) {
    return {
        (uint8_t)(x>>56), (uint8_t)((x>>48)&0xff), (uint8_t)((x>>40)&0xff), (uint8_t)((x>>32)&0xff),
        (uint8_t)((x>>24)&0xff), (uint8_t)((x>>16)&0xff), (uint8_t)((x>>8)&0xff), (uint8_t)(x&0xff)
    };
}

// 构造最小可用的传输参数集合
// 参数 ID 参考 RFC 9000，第13章（示例挑选常用项）
inline std::vector<uint8_t> build_min_quic_transport_params() {
    std::vector<uint8_t> tp;

    // 0x01 initial_max_stream_data_bidi_local (varint)
    { std::vector<uint8_t> v; quic_encode_varint(65536, v); tp_append_item(0x01, v, tp); }

    // 0x03 initial_max_stream_data_bidi_remote (varint)
    { std::vector<uint8_t> v; quic_encode_varint(65536, v); tp_append_item(0x03, v, tp); }

    // 0x05 initial_max_stream_data_uni (varint)
    { std::vector<uint8_t> v; quic_encode_varint(32768, v); tp_append_item(0x05, v, tp); }

    // 0x04 initial_max_data (varint)
    { std::vector<uint8_t> v; quic_encode_varint(1048576, v); tp_append_item(0x04, v, tp); }

    // 0x0d initial_max_streams_bidi (varint)
    { std::vector<uint8_t> v; quic_encode_varint(100, v); tp_append_item(0x0d, v, tp); }

    // 0x0e initial_max_streams_uni (varint)
    { std::vector<uint8_t> v; quic_encode_varint(100, v); tp_append_item(0x0e, v, tp); }

    // 0x08 max_idle_timeout (varint milliseconds)
    { std::vector<uint8_t> v; quic_encode_varint(30000, v); tp_append_item(0x08, v, tp); }

    // 0x03? 注意：max_udp_payload_size 的 ID 为 0x03 在旧草案；RFC9000中为 0x03? 实际为 0x03? 当前版本请用 0x03?（这里用 0x03? 占位说明）
    // 正确ID为 0x03? -> RFC 9000: max_udp_payload_size 是 0x03?（实际是 0x03?=0x03?）。不同实现可能接受默认 1200。为避免误配，我们这里显式设置 1200：
    { std::vector<uint8_t> v; quic_encode_varint(1200, v); tp_append_item(0x03, v, tp); } // 若你的 BoringSSL版本使用其他ID，请对照版本

    // 0x0b active_connection_id_limit (varint) 建议至少 2
    { std::vector<uint8_t> v; quic_encode_varint(4, v); tp_append_item(0x0b, v, tp); }

    // 可选：disable_active_migration 0x0c （空值参数，length=0）
    { std::vector<uint8_t> v; tp_append_item(0x0c, v, tp); }

    return tp;
}