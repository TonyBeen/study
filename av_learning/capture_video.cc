/*************************************************************************
    > File Name: capture_video.cc
    > Author: hsz
    > Brief:
    > Created Time: 2024年11月29日 星期五 10时18分58秒
 ************************************************************************/

#include <iostream>

#include <utils/utils.h>

EXTERN_C_BEGIN
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
EXTERN_C_END

#define VIDEO_DEVICE    "/dev/video0"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return -1;
    }

    const char *input_filename = argv[1];

    // 初始化FFmpeg库
    av_register_all();
    avformat_network_init();

    // 打开输入视频文件
    AVFormatContext *format_context = nullptr;
    if (avformat_open_input(&format_context, input_filename, nullptr, nullptr) < 0) {
        std::cerr << "Could not open input file: " << input_filename << std::endl;
        return -1;
    }

    // 查找流信息
    if (avformat_find_stream_info(format_context, nullptr) < 0) {
        std::cerr << "Could not find stream information" << std::endl;
        avformat_close_input(&format_context);
        return -1;
    }

    // 找到视频流
    int video_stream_index = -1;
    AVCodecParameters *codec_params = nullptr;
    for (int i = 0; i < format_context->nb_streams; i++) {
        if (format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            codec_params = format_context->streams[i]->codecpar;
            break;
        }
    }
    if (video_stream_index == -1) {
        std::cerr << "Could not find video stream" << std::endl;
        avformat_close_input(&format_context);
        return -1;
    }

    // 找到解码器
    AVCodec *codec = avcodec_find_decoder(codec_params->codec_id);
    if (!codec) {
        std::cerr << "Could not find codec" << std::endl;
        avformat_close_input(&format_context);
        return -1;
    }

    // 打开解码器
    AVCodecContext *codec_context = avcodec_alloc_context3(codec);
    if (!codec_context) {
        std::cerr << "Could not allocate codec context" << std::endl;
        avformat_close_input(&format_context);
        return -1;
    }
    if (avcodec_parameters_to_context(codec_context, codec_params) < 0) {
        std::cerr << "Could not copy codec parameters to codec context" << std::endl;
        avcodec_free_context(&codec_context);
        avformat_close_input(&format_context);
        return -1;
    }
    if (avcodec_open2(codec_context, codec, nullptr) < 0) {
        std::cerr << "Could not open codec" << std::endl;
        avcodec_free_context(&codec_context);
        avformat_close_input(&format_context);
        return -1;
    }

    // 解码视频帧
    AVFrame *frame = av_frame_alloc();
    AVPacket packet;
    int frame_count = 0;
    
    while (av_read_frame(format_context, &packet) >= 0) {
        if (packet.stream_index == video_stream_index) {
            int response = avcodec_send_packet(codec_context, &packet);
            if (response < 0) {
                std::cerr << "Error sending packet to decoder" << std::endl;
                break;
            }

            while (response >= 0) {
                response = avcodec_receive_frame(codec_context, frame);
                if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                    break;
                } else if (response < 0) {
                    std::cerr << "Error receiving frame from decoder" << std::endl;
                    break;
                }

                // 打印帧信息
                std::cout << "Frame " << frame_count++ << ": " 
                          << "width=" << frame->width 
                          << " height=" << frame->height 
                          << " pts=" << frame->pts << std::endl;

                // 在这里可以将帧数据进行处理（例如保存为图片或直接显示）
            }
        }
        av_packet_unref(&packet);
    }

    // 清理资源
    av_frame_free(&frame);
    avcodec_free_context(&codec_context);
    avformat_close_input(&format_context);

    return 0;
}
