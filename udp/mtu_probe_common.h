#ifndef MTU_PROBE_COMMON_H
#define MTU_PROBE_COMMON_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

// MTU 探测常量定义
#define MTU_PROBE_MIN          1200    // 最小 MTU (QUIC 要求)
#define MTU_PROBE_MAX_ETH      1500    // 以太网标准 MTU
#define MTU_PROBE_MAX_JUMBO    9000    // 巨型帧 MTU
#define MTU_PROBE_STEP         16      // 探测步进, 增大此值可加快探测速度但会降低精度

#define MTU_PROBE_TIMEOUT_MS   1000    // 探测超时时间 (ms)
#define MTU_PROBE_MAX_RETRY    3       // 最大重试次数
#define MTU_PROBE_INTERVAL_S   600     // 重新探测间隔 (秒)

#define UDP_HEADER_SIZE        8       // UDP 头部大小
#define IP_HEADER_SIZE         20      // IPv4 头部大小 (不含选项)
#define IPV6_HEADER_SIZE       40      // IPv6 头部大小

// 消息类型
typedef enum {
    MSG_TYPE_PROBE_REQ  = 0x01,   // 探测请求
    MSG_TYPE_PROBE_ACK  = 0x02,   // 探测确认
    MSG_TYPE_PROBE_NACK = 0x03,   // 探测失败
    MSG_TYPE_DATA       = 0x04,   // 数据包
} MessageType;

// 探测状态
typedef enum {
    PROBE_STATE_IDLE,             // 空闲
    PROBE_STATE_SEARCHING,        // 搜索中
    PROBE_STATE_COMPLETE,         // 探测完成
    PROBE_STATE_FAILED,           // 探测失败
} ProbeState;

// 消息头部结构
#pragma pack(push, 1)
typedef struct {
    uint8_t  type;                // 消息类型
    uint8_t  flags;               // 标志位
    uint16_t seq;                 // 序列号
    uint32_t probe_size;          // 探测大小
    uint32_t timestamp;           // 时间戳 (ms)
    uint32_t checksum;            // 校验和
} ProbeHeader;
#pragma pack(pop)

#define PROBE_HEADER_SIZE sizeof(ProbeHeader)

// 探测上下文
typedef struct {
    ProbeState state;             // 当前状态
    uint32_t   current_mtu;       // 当前确认的 MTU
    uint32_t   probe_low;         // 探测下界
    uint32_t   probe_high;        // 探测上界
    uint32_t   probe_target;      // 当前探测目标
    uint16_t   probe_seq;         // 探测序列号
    int        retry_count;       // 重试计数
    uint64_t   last_probe_time;   // 上次探测时间
    uint64_t   last_success_time; // 上次成功时间
} MtuProbeContext;

// 工具函数声明
uint32_t calculate_checksum(const void *data, size_t len);
uint64_t get_timestamp_ms(void);
void init_probe_context(MtuProbeContext *ctx, uint32_t max_mtu);
const char* probe_state_to_string(ProbeState state);

#endif // MTU_PROBE_COMMON_H