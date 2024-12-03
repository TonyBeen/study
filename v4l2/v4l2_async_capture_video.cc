/*************************************************************************
    > File Name: v4l2_async_capture_video.cc
    > Author: hsz
    > Brief:
    > Created Time: 2024年11月29日 星期五 17时36分56秒
 ************************************************************************/

#define __STDC_CONSTANT_MACROS
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <vector>
#include <memory>
#include <chrono>

#include <sched.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>

#include <utils/utils.h>

#include <libyuv.h>

EXTERN_C_BEGIN
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
EXTERN_C_END

#define CAPTURE_DURATION    30 // 采集视频的时长，单位：秒
#define VIDEO_DEVICE        "/dev/video1"

static void errno_exit(const char *s)
{
    fprintf(stderr, "%s: [%d, %s]\n", s, errno, strerror(errno));
    exit(EXIT_FAILURE);
}

static int32_t xioctl(int32_t fh, int32_t request, void *arg)
{
    int32_t r = 0;

    do {
        if (r < 0) {
            sched_yield();
        }
        r = ioctl(fh, request, arg);
    } while (r < 0 && EINTR == errno);

    return r;
}

bool SupportFormat(int32_t deviceFd, int32_t type)
{
    bool support = false;
    struct v4l2_fmtdesc v4l2_fmt_desc;
    v4l2_fmt_desc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_fmt_desc.index = 0;
    do {
        int32_t errorCode = xioctl(deviceFd, VIDIOC_ENUM_FMT, &v4l2_fmt_desc);
        if (errorCode < 0) {
            if (errno != EINVAL) {
                perror("ioctl VIDIOC_ENUM_FMT error");
            }
            break;
        }

        uint8_t *p = (uint8_t *)&v4l2_fmt_desc.pixelformat;
        printf("- %s; %c%c%c%c\n", v4l2_fmt_desc.description, p[0], p[1], p[2], p[3]);
        if (v4l2_fmt_desc.pixelformat == type) {
            support = true;
        }

        struct v4l2_frmsizeenum v4l2_frmsize;
        v4l2_frmsize.index = 0;
        v4l2_frmsize.pixel_format = v4l2_fmt_desc.pixelformat;

        while (0 == xioctl(deviceFd, VIDIOC_ENUM_FRAMESIZES, &v4l2_frmsize)) {
            if (v4l2_frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
                printf("    Resolution: %dx%d\n", v4l2_frmsize.discrete.width, v4l2_frmsize.discrete.height);

                struct v4l2_frmivalenum v4l2_frmival;
                v4l2_frmival.index = 0;
                v4l2_frmival.pixel_format = v4l2_fmt_desc.pixelformat;
                v4l2_frmival.width = v4l2_frmsize.discrete.width;
                v4l2_frmival.height = v4l2_frmsize.discrete.height;

                float maxFPS = 0;
                while (xioctl(deviceFd, VIDIOC_ENUM_FRAMEINTERVALS, &v4l2_frmival) == 0) {
                    if (v4l2_frmival.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
                        float fps = (float)v4l2_frmival.discrete.denominator / v4l2_frmival.discrete.numerator;
                        maxFPS = maxFPS < fps ? fps : maxFPS;
                    }
                    ++v4l2_frmival.index;
                }

                printf("      MAX FPS: %.2f\n", maxFPS);
            }

            ++v4l2_frmsize.index;
        }

        ++v4l2_fmt_desc.index;
    } while (true);

    return support;
}

class MapBuffer
{
public:
    MapBuffer(uint32_t count)
    {
        _buffer_vec.resize(count, nullptr);
        _size_vec.resize(count, 0);
    }

    ~MapBuffer()
    {
        for (auto i = 0; i < _buffer_vec.size(); ++i) {
            if (_buffer_vec[i] != nullptr) {
                ::munmap(_buffer_vec[i], _size_vec[i]);
            }
        }

        _buffer_vec.clear();
        _size_vec.clear();
    }

    std::vector<void *>     _buffer_vec;
    std::vector<uint32_t>   _size_vec;
};

bool MakeMapBuffer(int32_t deviceFd, std::shared_ptr<MapBuffer> spMapBuffer)
{
    bool success = true;
    uint32_t count = spMapBuffer->_buffer_vec.size();

    // 申请内核缓冲区队列
    struct v4l2_requestbuffers v4l2_reqbuffer;
    v4l2_reqbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_reqbuffer.count = count;               // 申请4个缓冲区
    v4l2_reqbuffer.memory = V4L2_MEMORY_MMAP ;  // 映射方式
    int32_t errorCode  = xioctl(deviceFd, VIDIOC_REQBUFS, &v4l2_reqbuffer);
    if (errorCode < 0) {
        errno_exit("ioctl VIDIOC_REQBUFS error");
    }

    struct v4l2_buffer v4l2_mapbuffer;
    v4l2_mapbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_mapbuffer.memory = V4L2_MEMORY_MMAP;

    for (uint32_t i = 0; i < count; ++i) {
        v4l2_mapbuffer.index = i;

        // 从内核空间中查询一个空间做映射
        errorCode = xioctl(deviceFd, VIDIOC_QUERYBUF, &v4l2_mapbuffer);
        if (errorCode < 0) {
            perror("ioctl VIDIOC_QUERYBUF error");
            continue;
        }

        spMapBuffer->_size_vec[i] = v4l2_mapbuffer.length;
        spMapBuffer->_buffer_vec[i] = mmap(NULL, v4l2_mapbuffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, deviceFd, v4l2_mapbuffer.m.offset);
        assert(nullptr != spMapBuffer->_buffer_vec[i]);

        // 通知内核使用完毕
        errorCode = xioctl(deviceFd, VIDIOC_QBUF, &v4l2_mapbuffer);
        if (errorCode < 0) {
            perror("ioctl VIDIOC_QBUF error");
            success = false;
            break;
        }
    }

    return success;
}

int main()
{
    // 初始化 libavformat 和 libavcodec
    av_register_all();
    avformat_network_init();

    // 打开设备
    int32_t deviceFd = ::open(VIDEO_DEVICE, O_RDWR);
    if (deviceFd == -1) {
        perror("Failed to open device");
        return -1;
    }

    if (!SupportFormat(deviceFd, V4L2_PIX_FMT_YUYV)) {
        printf(VIDEO_DEVICE " not support V4L2_PIX_FMT_YUYV\n");
        return 0;
    }

    // 设置文件描述符为非阻塞模式
    int32_t flags = fcntl(deviceFd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl(F_GETFL) failed");
        close(deviceFd);
        return -1;
    }
    if (fcntl(deviceFd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl(F_SETFL) failed");
        close(deviceFd);
        return -1;
    }

    static const uint32_t WIDTH = 640;
    static const uint32_t HEIGHT = 480;

    // 设置采集格式
    struct v4l2_format v4l2_fmt;
    v4l2_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;        // 摄像头采集
    v4l2_fmt.fmt.pix.width = WIDTH;                     // 设置宽(不能任意)
    v4l2_fmt.fmt.pix.height = HEIGHT;                   // 设置高
    v4l2_fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;   // 设置视频采集格式
    v4l2_fmt.fmt.pix.field = V4L2_FIELD_NONE;
    int32_t errorCode = xioctl(deviceFd, VIDIOC_S_FMT, &v4l2_fmt);
    if (errorCode < 0) {
        errno_exit("ioctl VIDIOC_S_FMT error");
    }

    // 获取之前设置的采集格式, 判断是否参数一致
    memset(&v4l2_fmt, 0, sizeof(v4l2_fmt));
    v4l2_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    errorCode  = xioctl(deviceFd, VIDIOC_G_FMT, &v4l2_fmt);
    if (errorCode < 0) {
        errno_exit("ioctl VIDIOC_G_FMT error");
    }

    if (WIDTH != v4l2_fmt.fmt.pix.width &&
        HEIGHT != v4l2_fmt.fmt.pix.height &&
        V4L2_PIX_FMT_YUYV != v4l2_fmt.fmt.pix.pixelformat) {
        printf("Setting width(%u), height(%u), pixelformat(V4L2_PIX_FMT_YUYV) failed\n", WIDTH, HEIGHT);
        close(deviceFd);
        return 1;
    }

    std::shared_ptr<MapBuffer> spMapBuffer = std::make_shared<MapBuffer>(8);
    if (!MakeMapBuffer(deviceFd, spMapBuffer)) {
        return 0;
    }

    // 打开输出文件（目标视频文件）
    AVFormatContext* pFormatContext = nullptr;
    int32_t status = avformat_alloc_output_context2(&pFormatContext, nullptr, nullptr, "output.mp4");
    if (!pFormatContext) {
        char error_buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(status, error_buf, sizeof(error_buf));
        printf("Could not create output context: %s\n", error_buf);
        close(deviceFd);
        return -1;
    }

    // 创建视频流
    AVStream* pVideoStream = avformat_new_stream(pFormatContext, nullptr);
    if (!pVideoStream) {
        std::cerr << "Failed to create video stream!" << std::endl;
        close(deviceFd);
        return -1;
    }

    AVCodec* pCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!pCodec) {
        std::cerr << "Codec not found!" << std::endl;
        close(deviceFd);
        return -1;
    }

    // 设置编码器参数
    AVCodecContext* pCodecContext = avcodec_alloc_context3(pCodec);
    pCodecContext->codec_id = AV_CODEC_ID_H264;  // 使用 H.264 编码
    // pCodecContext->codec_type = AVMEDIA_TYPE_VIDEO;
    pCodecContext->bit_rate = 400000; // 400kbps
    pCodecContext->width = WIDTH;
    pCodecContext->height = HEIGHT;
    pCodecContext->time_base = {1, 30}; // 时基：这是基本的时间单位（以秒为单位） 表示其中的帧时间戳。 对于固定fps内容，时基应为1 / framerate，时间戳增量应为等于1。
    pCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;  // 设置像素格式
    pCodecContext->framerate = (AVRational){30, 1};
    pCodecContext->gop_size = 10; // 最多每十二帧发射一帧内帧

    // 打开编码器
    status = avcodec_open2(pCodecContext, pCodec, nullptr);
    if (status < 0) {
        char error_buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(status, error_buf, sizeof(error_buf));
        printf("Could not open codec: %s\n", error_buf);
        close(deviceFd);
        return -1;
    }

    // 将编码器上下文绑定到流
    status = avcodec_parameters_from_context(pVideoStream->codecpar, pCodecContext);
    if (status < 0) {
        char error_buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(status, error_buf, sizeof(error_buf));
        printf("Error copying codec parameters to stream: %s\n", error_buf);
        return -1;
    }

    av_dump_format(pFormatContext, 0, "output.mp4", 1);

    // 打开输出文件
    if (!(pFormatContext->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&pFormatContext->pb, "output.mp4", AVIO_FLAG_WRITE) < 0) {
            std::cerr << "Could not open output file!" << std::endl;
            close(deviceFd);
            return -1;
        }
    }

    // 写入文件头
    if (avformat_write_header(pFormatContext, nullptr) < 0) {
        std::cerr << "Could not write header!" << std::endl;
        close(deviceFd);
        return -1;
    }

    printf("pix_fmt = %d\n", pCodecContext->pix_fmt);
    // 采集并编码帧
    AVFrame* pFrame = av_frame_alloc();
    pFrame->format = pCodecContext->pix_fmt;
    pFrame->width = pCodecContext->width;
    pFrame->height = pCodecContext->height;
    if (av_frame_get_buffer(pFrame, 32) < 0) {
        std::cerr << "Could not allocate frame buffer!" << std::endl;
        close(deviceFd);
        return -1;
    }

    // 开始采集
    int32_t type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    errorCode = xioctl(deviceFd, VIDIOC_STREAMON, &type);
    if (errorCode < 0) {
        errno_exit("ioctl VIDIOC_STREAMON error");
    }

    // 初始化缓冲区
    struct v4l2_buffer buf;
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    // 设置 pollfd 用于等待设备事件
    struct pollfd fds[1];
    fds[0].fd = deviceFd;
    fds[0].events = POLLIN;  // 等待可读事件

    // 记录开始时间
    auto start_time = std::chrono::steady_clock::now();

    int32_t frame_count = 0;
    int64_t pts = 0;
    while (1) {
        // 等待文件描述符变为可读
        int32_t ret = poll(fds, 1, -1);
        if (ret == -1) {
            perror("poll failed");
            break;
        }

        // auto current_time = std::chrono::steady_clock::now();
        // std::chrono::duration<double> elapsed_time = current_time - start_time;
        // if (elapsed_time.count() >= CAPTURE_DURATION) {
        //     std::cout << "Captured 1 minute of video. Exiting..." << std::endl;
        //     break;
        // }

        if (fds[0].revents & POLLIN) {
            printf("event POLLIN\n");
            // 在非阻塞模式下，尝试从队列中获取数据
            int32_t err = ioctl(deviceFd, VIDIOC_DQBUF, &buf);
            if (err == -1) {
                if (errno == EAGAIN) {
                    // 数据尚未准备好，继续等待
                    printf("No data available yet (EAGAIN).\n");
                    continue;
                } else {
                    perror("VIDIOC_DQBUF failed");
                    break;
                }
            }

            // 从 YUYV 到 YUV420P 转换的函数
            // auto convert_yuyv_to_yuv420p = [] (uint8_t* yuyv_data, AVFrame* pFrame, int width, int height) {
            //     int y_index = 0, u_index = 0, v_index = 0;
            //     for (int i = 0; i < width * height; i++) {
            //         pFrame->data[0][y_index++] = yuyv_data[i * 2]; // Y
            //         if (i % 2 == 0) { // U 和 V 每两个像素共享
            //             pFrame->data[1][u_index++] = yuyv_data[i * 2 + 1]; // U
            //             pFrame->data[2][v_index++] = yuyv_data[i * 2 + 3]; // V
            //         }
            //     }
            // };

            if (av_compare_ts(pts, pCodecContext->time_base, CAPTURE_DURATION, (AVRational){1, 1}) >= 0)
                break;

            av_frame_make_writable(pFrame);

            auto convert_yuyv_to_yuv420p = [] (const uint8_t *in, uint8_t *out, uint32_t width, uint32_t height) {
                unsigned char *y = out;
                unsigned char *u = out + width * height;
                unsigned char *v = out + width * height + width * height / 4;
                unsigned int i, j;
                unsigned int base_h;
                unsigned int is_u = 1;
                unsigned int y_index = 0, u_index = 0, v_index = 0;
                unsigned long yuv422_length = 2 * width * height;
                // 序列为YU YV YU YV，一个yuv422帧的长度 width * height * 2 个字节
                // 丢弃偶数行 u v
                for (i = 0; i < yuv422_length; i += 2)
                {
                    *(y + y_index) = *(in + i);
                    y_index++;
                }
                for (i = 0; i < height; i += 2)
                {
                    base_h = i * width * 2;
                    for (j = base_h + 1; j < base_h + width * 2; j += 2)
                    {
                        if (is_u)
                        {
                            *(u + u_index) = *(in + j);
                            u_index++;
                            is_u = 0;
                        }
                        else
                        {
                            *(v + v_index) = *(in + j);
                            v_index++;
                            is_u = 1;
                        }
                    }
                }
                return 1;
            };

            // YUYV 转 YUV420P
            uint8_t *yuyv_data = static_cast<uint8_t *>(spMapBuffer->_buffer_vec[buf.index]);
            uint8_t *yuv420p_buffer = new uint8_t[WIDTH * HEIGHT * 3 / 2];

            // 转换 YUYV 到 YUV420P
            convert_yuyv_to_yuv420p(yuyv_data, yuv420p_buffer, WIDTH, HEIGHT);

            // 将YUV数据拷贝到缓冲区
            int y_size = WIDTH * HEIGHT;
            memcpy(pFrame->data[0], yuv420p_buffer, y_size);
            memcpy(pFrame->data[1], yuv420p_buffer + y_size, y_size / 4);
            memcpy(pFrame->data[2], yuv420p_buffer + y_size + y_size / 4, y_size / 4);
            pFrame->pts = pts++;

            // 将帧编码成视频并写入文件
            AVPacket pkt;
            av_init_packet(&pkt);
            int got_packet = 0;
            ret = avcodec_encode_video2(pCodecContext, &pkt, pFrame, &got_packet);
            if (got_packet) {
                /*将输出数据包时间戳值从编解码器重新调整为流时基 */
                av_packet_rescale_ts(&pkt, pCodecContext->time_base, pVideoStream->time_base);
                pkt.stream_index = pVideoStream->index;

                // 将压缩的帧写入媒体文件
                av_interleaved_write_frame(pFormatContext, &pkt);
            }
            else
            {
                printf("got_packet = false\n");
                break;
            }

            // ret = avcodec_receive_packet(pCodecContext, &pkt);
            // if (ret == 0) {
            //     pkt.stream_index = pVideoStream->index;
            //     ret = av_write_frame(pFormatContext, &pkt);
            //     av_packet_unref(&pkt);
            // } else if (ret == AVERROR(EAGAIN)) {
            //     if (ioctl(deviceFd, VIDIOC_QBUF, &buf) == -1) {
            //         perror("VIDIOC_QBUF failed");
            //         break;
            //     }
            //     continue;
            // } else if (ret < 0) {
            //     std::cerr << "Error receiving packet!" << std::endl;
            //     break;
            // }

            // 可选：将缓冲区放回队列
            if (ioctl(deviceFd, VIDIOC_QBUF, &buf) == -1) {
                perror("VIDIOC_QBUF failed");
                break;
            }

            ++frame_count;
        }
    }

    // 写入文件尾
    av_write_trailer(pFormatContext);

    // 清理资源
    av_frame_free(&pFrame);
    avcodec_close(pCodecContext);
    avformat_close_input(&pFormatContext);

    spMapBuffer.reset();

    close(deviceFd);
    return 0;
}
