/*************************************************************************
    > File Name: capture_image.cc
    > Author: hsz
    > Brief:
    > Created Time: 2024年11月28日 星期四 17时09分11秒
 ************************************************************************/

#define __STDC_CONSTANT_MACROS
#include <stdio.h>

#include <utils/utils.h>

EXTERN_C_BEGIN
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libavutil/error.h>
#include <libavutil/imgutils.h>
EXTERN_C_END

int save_yuyv422_as_png(AVFrame* frame, const char* filename)
{
    // 1. 使用 libswscale 将 YUYV422 转换为 RGB24
    SwsContext* sws_ctx = sws_getContext(frame->width, frame->height, (AVPixelFormat)frame->format, 
                                         frame->width, frame->height, AV_PIX_FMT_RGB24, 
                                         SWS_BICUBIC, NULL, NULL, NULL);
    if (!sws_ctx) {
        std::cerr << "Error initializing the sws context!" << std::endl;
        return -1;
    }

    // 创建一个新的 AVFrame 用于存储 RGB 数据
    AVFrame* rgb_frame = av_frame_alloc();
    if (!rgb_frame) {
        std::cerr << "Error allocating RGB frame!" << std::endl;
        sws_freeContext(sws_ctx);
        return -1;
    }

    // 为 RGB 帧分配内存
    int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, frame->width, frame->height, 1);
    uint8_t* buffer = (uint8_t*)av_malloc(num_bytes * sizeof(uint8_t));
    if (!buffer) {
        std::cerr << "Error allocating buffer!" << std::endl;
        av_frame_free(&rgb_frame);
        sws_freeContext(sws_ctx);
        return -1;
    }

    // 设置 RGB frame 的数据
    av_image_fill_arrays(rgb_frame->data, rgb_frame->linesize, buffer, AV_PIX_FMT_RGB24, frame->width, frame->height, 1);

    // 设置 RGB Frame 的格式和尺寸
    rgb_frame->format = AV_PIX_FMT_RGB24;  // 设置像素格式
    rgb_frame->width = frame->width;       // 设置宽度
    rgb_frame->height = frame->height;     // 设置高度

    // 转换 YUYV422 到 RGB
    sws_scale(sws_ctx, frame->data, frame->linesize, 0, frame->height, rgb_frame->data, rgb_frame->linesize);

    // 2. 使用 FFmpeg 的 PNG 编码器保存图像
    AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_PNG);
    if (!codec) {
        std::cerr << "PNG encoder not found!" << std::endl;
        av_free(buffer);
        av_frame_free(&rgb_frame);
        sws_freeContext(sws_ctx);
        return -1;
    }

    AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        std::cerr << "Error allocating codec context!" << std::endl;
        av_free(buffer);
        av_frame_free(&rgb_frame);
        sws_freeContext(sws_ctx);
        return -1;
    }

    codec_ctx->width = frame->width;
    codec_ctx->height = frame->height;
    codec_ctx->pix_fmt = AV_PIX_FMT_RGB24;
    codec_ctx->time_base = (AVRational){1, 25}; // 设置帧率，假设是25fps

    if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
        std::cerr << "Error opening codec!" << std::endl;
        av_free(buffer);
        av_frame_free(&rgb_frame);
        sws_freeContext(sws_ctx);
        avcodec_free_context(&codec_ctx);
        return -1;
    }

    FILE* out_file = fopen(filename, "wb");
    if (!out_file) {
        std::cerr << "Error opening output file!" << std::endl;
        av_free(buffer);
        av_frame_free(&rgb_frame);
        sws_freeContext(sws_ctx);
        avcodec_free_context(&codec_ctx);
        return -1;
    }

    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    // 发送帧给编码器
    int ret = avcodec_send_frame(codec_ctx, rgb_frame);
    if (ret < 0) {
        char error_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_strerror(ret, error_buf, AV_ERROR_MAX_STRING_SIZE);
        std::cerr << "Error sending frame to encoder: " << error_buf << std::endl;
        fclose(out_file);
        av_free(buffer);
        av_frame_free(&rgb_frame);
        sws_freeContext(sws_ctx);
        avcodec_free_context(&codec_ctx);
        return -1;
    }

    // 接收编码后的数据包
    ret = avcodec_receive_packet(codec_ctx, &pkt);
    if (ret < 0) {
        char error_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_strerror(ret, error_buf, AV_ERROR_MAX_STRING_SIZE);
        std::cerr << "Error receiving packet from encoder: " << error_buf << std::endl;
        fclose(out_file);
        av_free(buffer);
        av_frame_free(&rgb_frame);
        sws_freeContext(sws_ctx);
        avcodec_free_context(&codec_ctx);
        return -1;
    }

    // 将数据包写入文件
    fwrite(pkt.data, 1, pkt.size, out_file);

    // 清理
    fclose(out_file);
    av_packet_unref(&pkt);
    av_free(buffer);
    av_frame_free(&rgb_frame);
    sws_freeContext(sws_ctx);
    avcodec_free_context(&codec_ctx);

    return 0;
}

int main() {
    AVFormatContext *fmt_ctx = NULL;
    AVCodecContext *codec_ctx = NULL;
    AVCodec *codec = NULL;
    AVFrame *frame = NULL;
    AVPacket packet;
    int video_stream_index;
    const char *device_name = "/dev/video0";  // Linux摄像头设备，Windows下可能是"dshow"或"vfwcap"
    const char *output_filename = "output.png";
    FILE *output_file = NULL;

    // 初始化FFmpeg库
    avdevice_register_all();
    avformat_network_init();

    // 打开视频输入设备（摄像头）
    AVInputFormat *input_format = av_find_input_format("video4linux2");  // Linux设备
    if (avformat_open_input(&fmt_ctx, device_name, input_format, NULL) < 0) {
        fprintf(stderr, "无法打开视频输入设备\n");
        return -1;
    }

    // 查找视频流
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        fprintf(stderr, "无法获取流信息\n");
        return -1;
    }

    // 查找视频流的解码器
    video_stream_index = -1;
    for (int i = 0; i < fmt_ctx->nb_streams; i++) {
        printf("type: %d\n", fmt_ctx->streams[i]->codecpar->codec_type);
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
        }
    }

    if (video_stream_index == -1) {
        fprintf(stderr, "未找到视频流\n");
        return -1;
    }

    printf("stream index = %d\n", video_stream_index);

    // 获取解码器
    codec = avcodec_find_decoder(fmt_ctx->streams[video_stream_index]->codecpar->codec_id);
    if (!codec) {
        fprintf(stderr, "未找到视频解码器\n");
        return -1;
    }

    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        fprintf(stderr, "无法分配解码器上下文\n");
        return -1;
    }

    if (avcodec_parameters_to_context(codec_ctx, fmt_ctx->streams[video_stream_index]->codecpar) < 0) {
        fprintf(stderr, "无法初始化解码器上下文\n");
        return -1;
    }

    if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
        fprintf(stderr, "无法打开解码器\n");
        return -1;
    }

    // 分配帧内存
    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "无法分配帧内存\n");
        return -1;
    }

    // 读取视频帧并解码
    while (av_read_frame(fmt_ctx, &packet) >= 0) {
        if (packet.stream_index == video_stream_index) {
            // 解码视频帧
            if (avcodec_send_packet(codec_ctx, &packet) < 0) {
                fprintf(stderr, "发送包失败\n");
                continue;
            }

            if (avcodec_receive_frame(codec_ctx, frame) == 0) {
                // 只有收到第一帧就保存
                printf("成功捕获一帧视频: %s\n", av_get_pix_fmt_name((AVPixelFormat)frame->format)); // AV_PIX_FMT_YUV420P
                save_yuyv422_as_png(frame, output_filename);

                av_packet_unref(&packet);
                break;  // 只捕获一帧
            }
        }
        av_packet_unref(&packet);
    }

    // 清理
    av_frame_free(&frame);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);

    return 0;
}
