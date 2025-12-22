#include "mtu_probe_common.h"

#include <string.h>
#include <sys/time.h>

// 计算简单校验和 (CRC32 简化版)
uint32_t calculate_checksum(const void *data, size_t len) {
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t checksum = 0;
    
    for (size_t i = 0; i < len; i++) {
        checksum = (checksum << 8) ^ bytes[i];
        checksum ^= (checksum >> 4);
    }
    
    return checksum;
}

// 获取当前时间戳 (毫秒)
uint64_t get_timestamp_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

// 初始化探测上下文
void init_probe_context(MtuProbeContext *ctx, uint32_t max_mtu) {
    memset(ctx, 0, sizeof(MtuProbeContext));
    ctx->state = PROBE_STATE_IDLE;
    ctx->current_mtu = MTU_PROBE_MIN;
    ctx->probe_low = MTU_PROBE_MIN;
    ctx->probe_high = max_mtu;
    ctx->probe_target = MTU_PROBE_MIN;
    ctx->probe_seq = 0;
    ctx->retry_count = 0;
    ctx->last_probe_time = 0;
    ctx->last_success_time = get_timestamp_ms();
}

// 状态转字符串
const char* probe_state_to_string(ProbeState state) {
    switch (state) {
        case PROBE_STATE_IDLE:       return "IDLE";
        case PROBE_STATE_SEARCHING: return "SEARCHING";
        case PROBE_STATE_COMPLETE:  return "COMPLETE";
        case PROBE_STATE_FAILED:    return "FAILED";
        default:                    return "UNKNOWN";
    }
}