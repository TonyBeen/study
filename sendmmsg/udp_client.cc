
#include <cstring>
#include <iostream>
#include <vector>
#include <algorithm>

#include <unistd.h>
#include <linux/udp.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef UDP_SEGMENT
#define UDP_SEGMENT 103
#endif

#ifndef SOL_UDP
#define SOL_UDP 17
#endif


int main()
{
    // 配置参数
    const size_t TOTAL_SIZE = 128 * 1024 - 1400; // 128KB
    const int MTU = 1500;
    const int IP_UDP_HDR = 20 + 8 + 8;
    const int GSO_PAYLOAD = MTU - IP_UDP_HDR; // 1472
    const size_t BATCH_SIZE = 32; // 每批最多发多少mmsghdr
    const char* DST_IP = "82.157.8.46";
    const int DST_PORT = 9000;

    // 准备数据缓存，内容全置'A'
    std::vector<char> send_buf(TOTAL_SIZE, 'A');

    // 目标地址
    sockaddr_in dst_addr{};
    dst_addr.sin_family = AF_INET;
    dst_addr.sin_port = htons(DST_PORT);
    inet_pton(AF_INET, DST_IP, &dst_addr.sin_addr);

    // 建立UDP socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    size_t pos = 0;
    size_t msg_count = 0;
    while (pos < TOTAL_SIZE) {
        size_t remain = TOTAL_SIZE - pos;
        size_t segment_size = std::min(remain, size_t(GSO_PAYLOAD * BATCH_SIZE));
        size_t batch_msgs = (segment_size + GSO_PAYLOAD - 1) / GSO_PAYLOAD;

        // mmsghdr批次
        std::vector<mmsghdr> mms(batch_msgs);
        std::vector<msghdr> hdrs(batch_msgs);
        std::vector<iovec> iovs(batch_msgs);
        std::vector<char> cmsg_bufs(batch_msgs * CMSG_SPACE(sizeof(uint16_t)));

        for (size_t i = 0; i < batch_msgs; ++i) {
            size_t this_size = std::min(remain, size_t(GSO_PAYLOAD));
            iovs[i].iov_base = send_buf.data() + pos;
            iovs[i].iov_len  = this_size;

            hdrs[i].msg_name    = &dst_addr;
            hdrs[i].msg_namelen = sizeof(dst_addr);
            hdrs[i].msg_iov     = &iovs[i];
            hdrs[i].msg_iovlen  = 1;

            // UDP_SEGMENT设置
            memset(&cmsg_bufs[i * CMSG_SPACE(sizeof(uint16_t))], 0, CMSG_SPACE(sizeof(uint16_t)));
            hdrs[i].msg_control = &cmsg_bufs[i * CMSG_SPACE(sizeof(uint16_t))];
            hdrs[i].msg_controllen = CMSG_SPACE(sizeof(uint16_t));
            cmsghdr* cmsg = CMSG_FIRSTHDR(&hdrs[i]);
            cmsg->cmsg_level = SOL_UDP;
            cmsg->cmsg_type  = UDP_SEGMENT;
            cmsg->cmsg_len   = CMSG_LEN(sizeof(uint16_t));
            uint16_t gso_val = GSO_PAYLOAD;
            memcpy(CMSG_DATA(cmsg), &gso_val, sizeof(uint16_t));

            mms[i].msg_hdr = hdrs[i];
            mms[i].msg_len = 0;

            pos += this_size;
            remain -= this_size;
        }

        // 批量发送
        int ret = sendmmsg(sock, mms.data(), batch_msgs, 0);
        if (ret < 0) {
            perror("sendmmsg");
            close(sock);
            return 1;
        }
        msg_count += ret;
        for (int i = 0; i < ret; ++i) {
            printf("Sent message %d, size = %d\n", i, mms[i].msg_len);
        }
        printf("This time sending the fragmented GSO packages: %d\n", ret);
    }
    std::cout << "全部分片包批量发送完成，共计 GSO 包数：" << msg_count << std::endl;

    close(sock);
    return 0;
}