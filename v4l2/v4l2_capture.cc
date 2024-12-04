/*************************************************************************
    > File Name: v4l2_capture.c
    > Author: hsz
    > Brief: gcc v4l2_capture.cc -lavcodec -lavfilter -lswresample -lavdevice -lavformat -lswscale -lavutil -lm -pthread
    > Created Time: 2024年12月02日 星期一 16时51分26秒
 ************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <poll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include <utils/utils.h>

EXTERN_C_BEGIN
#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
EXTERN_C_END


#define STREAM_DURATION     10.0                // 录制10秒的视频
#define STREAM_FRAME_RATE   25                  // 25 images/s    avfilter_get_by_name
#define STREAM_PIX_FMT      AV_PIX_FMT_YUV420P  // default pix_fmt
#define SCALE_FLAGS         SWS_BICUBIC

// 固定摄像头输出画面的尺寸
#define VIDEO_WIDTH     640
#define VIDEO_HEIGHT    480

// 存放从摄像头读出转换之后的数据
uint8_t YUV420P_Buffer[VIDEO_WIDTH * VIDEO_HEIGHT * 3 / 2];
uint8_t YUV420P_Buffer_temp[VIDEO_WIDTH * VIDEO_HEIGHT * 3 / 2];

/*一些摄像头需要使用的全局变量*/
uint8_t *image_buffer[4];
int video_fd;
pthread_mutex_t mutex;
pthread_cond_t cond;

// 单个输出AVStream的包装器
typedef struct OutputStream
{
    AVStream *st;
    AVCodecContext *enc;

    /* 下一帧的点数*/
    int64_t next_pts;

    AVFrame *frame;
    AVFrame *tmp_frame;

    struct SwsContext *sws_ctx;
    struct SwrContext *swr_ctx;
} OutputStream;

static int write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt)
{
    /*将输出数据包时间戳值从编解码器重新调整为流时基 */
    av_packet_rescale_ts(pkt, *time_base, st->time_base);
    pkt->stream_index = st->index;

    /*将压缩的帧写入媒体文件*/
    return av_interleaved_write_frame(fmt_ctx, pkt);
}

/* 添加输出流。 */
static void add_stream(OutputStream *ost, AVFormatContext *oc, AVCodec **codec, AVCodecID codec_id)
{
    AVCodecContext *c;
    int i;

    /* find the encoder */
    *codec = avcodec_find_encoder(codec_id);
    if (!(*codec))
    {
        fprintf(stderr, "Could not find encoder for '%s'\n",
                avcodec_get_name(codec_id));
        exit(1);
    }

    ost->st = avformat_new_stream(oc, NULL);
    if (!ost->st)
    {
        fprintf(stderr, "Could not allocate stream\n");
        exit(1);
    }
    ost->st->id = oc->nb_streams - 1;
    c = avcodec_alloc_context3(*codec);
    if (!c)
    {
        fprintf(stderr, "Could not alloc an encoding context\n");
        exit(1);
    }
    ost->enc = c;

    switch ((*codec)->type) {
    case AVMEDIA_TYPE_VIDEO:
        c->codec_id = codec_id;
        // 码率：影响体积，与体积成正比：码率越大，体积越大；码率越小，体积越小。
        c->bit_rate = 400000; // 设置码率 400kps
        /*分辨率必须是2的倍数。 */
        c->width = VIDEO_WIDTH;
        c->height = VIDEO_HEIGHT;
        /*时基：这是基本的时间单位（以秒为单位）
         *表示其中的帧时间戳。 对于固定fps内容，
         *时基应为1 / framerate，时间戳增量应为
         *等于1。*/
        ost->st->time_base = (AVRational){1, STREAM_FRAME_RATE};
        c->time_base = ost->st->time_base;
        c->gop_size = 12; /* 最多每十二帧发射一帧内帧 */
        c->pix_fmt = STREAM_PIX_FMT;
        c->max_b_frames = 0; // 不要B帧
        if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO)
        {
            c->mb_decision = 2;
        }
        break;

    default:
        break;
    }

    /* 某些格式希望流头分开。 */
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}

static AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height)
{
    AVFrame *picture;
    int ret;
    picture = av_frame_alloc();
    picture->format = pix_fmt;
    picture->width = width;
    picture->height = height;

    /* allocate the buffers for the frame data */
    av_frame_get_buffer(picture, 32);
    return picture;
}

static void open_video(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
{
    AVCodecContext *c = ost->enc;
    AVDictionary *opt = NULL;
    av_dict_copy(&opt, opt_arg, 0);
    /* open the codec */
    avcodec_open2(c, codec, &opt);
    av_dict_free(&opt);
    /* allocate and init a re-usable frame */
    ost->frame = alloc_picture(c->pix_fmt, c->width, c->height);
    ost->tmp_frame = NULL;
    /* 将流参数复制到多路复用器 */
    avcodec_parameters_from_context(ost->st->codecpar, c);
}

/*
准备图像数据
YUV422占用内存空间 = w * h * 2
YUV420占用内存空间 = width*height*3/2
*/
static void fill_yuv_image(AVFrame *pict, int frame_index, int width, int height)
{
    int y_size = width * height;
    /*等待条件成立*/
    pthread_cond_wait(&cond, &mutex);
    memcpy(YUV420P_Buffer_temp, YUV420P_Buffer, sizeof(YUV420P_Buffer));
    /*互斥锁解锁*/
    pthread_mutex_unlock(&mutex);

    // 将YUV数据拷贝到缓冲区  y_size=wXh
    memcpy(pict->data[0], YUV420P_Buffer_temp, y_size);
    memcpy(pict->data[1], YUV420P_Buffer_temp + y_size, y_size / 4);
    memcpy(pict->data[2], YUV420P_Buffer_temp + y_size + y_size / 4, y_size / 4);
}

static AVFrame *get_video_frame(OutputStream *ost)
{
    AVCodecContext *c = ost->enc;

    /* 检查我们是否要生成更多帧---判断是否结束录制 */
    if (av_compare_ts(ost->next_pts, c->time_base, STREAM_DURATION, (AVRational){1, 1}) >= 0)
        return NULL;

    /*当我们将帧传递给编码器时，它可能会保留对它的引用
     *内部； 确保我们在这里不覆盖它*/
    if (av_frame_make_writable(ost->frame) < 0)
        exit(1);

    // 制作虚拟图像
    // DTS（解码时间戳）和PTS（显示时间戳）
    fill_yuv_image(ost->frame, ost->next_pts, c->width, c->height);
    ost->frame->pts = ost->next_pts++;
    return ost->frame;
}

/*
 *编码一个视频帧并将其发送到多路复用器
 *编码完成后返回1，否则返回0
 */
static int write_video_frame(AVFormatContext *oc, OutputStream *ost)
{
    int ret;
    AVCodecContext *c;
    AVFrame *frame;
    int got_packet = 0;
    AVPacket pkt = {0};
    c = ost->enc;
    // 获取一帧数据
    frame = get_video_frame(ost);
    av_init_packet(&pkt);

    /* 编码图像 */
    ret = avcodec_encode_video2(c, &pkt, frame, &got_packet);

    if (got_packet)
    {
        ret = write_frame(oc, &c->time_base, ost->st, &pkt);
    }
    else
    {
        ret = 0;
    }
    return (frame || got_packet) ? 0 : 1;
}

static void close_stream(AVFormatContext *oc, OutputStream *ost)
{
    avcodec_free_context(&ost->enc);
    av_frame_free(&ost->frame);
    av_frame_free(&ost->tmp_frame);
    sws_freeContext(ost->sws_ctx);
    swr_free(&ost->swr_ctx);
}

// 编码视频和音频
int video_audio_encode(char *filename)
{
    OutputStream video_st = {0};
    AVOutputFormat *fmt;
    AVFormatContext *oc;
    AVCodec *video_codec;
    int ret;
    int have_video = 0;
    int encode_video = 0;
    AVDictionary *opt = NULL;
    int i;

    /* 分配输出环境 */
    avformat_alloc_output_context2(&oc, NULL, NULL, filename);
    fmt = oc->oformat;

    /*使用默认格式的编解码器添加音频和视频流，初始化编解码器。 */
    if (fmt->video_codec != AV_CODEC_ID_NONE)
    {
        add_stream(&video_st, oc, &video_codec, fmt->video_codec);
        have_video = 1;
        encode_video = 1;
    }

    /*现在已经设置了所有参数，可以打开音频视频编解码器，并分配必要的编码缓冲区。 */
    if (have_video)
        open_video(oc, video_codec, &video_st, opt);

    av_dump_format(oc, 0, filename, 1);

    /* 打开输出文件（如果需要） */
    if (!(fmt->flags & AVFMT_NOFILE))
    {
        ret = avio_open(&oc->pb, filename, AVIO_FLAG_WRITE);
        if (ret < 0)
        {
            char error_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
            av_strerror(ret, error_buf, AV_ERROR_MAX_STRING_SIZE);
            fprintf(stderr, "无法打开输出文件: '%s': %s\n", filename, error_buf);
            return 1;
        }
    }

    // 编写流头
    ret = avformat_write_header(oc, &opt);
    if (ret < 0) {
        return 1;
    }

    while (encode_video) {
        encode_video = !write_video_frame(oc, &video_st);
    }

    av_write_trailer(oc);

    if (have_video)
        close_stream(oc, &video_st);

    if (!(fmt->flags & AVFMT_NOFILE))
        avio_closep(&oc->pb);

    avformat_free_context(oc);
    return 0;
}

/*
函数功能: 摄像头设备初始化
*/
int VideoDeviceInit(char *DEVICE_NAME)
{
    // 1. 打开摄像头设备
    video_fd = open(DEVICE_NAME, O_RDWR);
    if (video_fd < 0) {
        perror("open error");
        return -1;
    }

    // 2. 设置摄像头支持的颜色格式和输出的图像尺寸
    struct v4l2_format video_formt;
    memset(&video_formt, 0, sizeof(struct v4l2_format));
    video_formt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    video_formt.fmt.pix.height = VIDEO_HEIGHT;      // 480
    video_formt.fmt.pix.width = VIDEO_WIDTH;        // 640
    video_formt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    if (ioctl(video_fd, VIDIOC_S_FMT, &video_formt)) {
        perror("ioctl(VIDIOC_S_FMT) error");
        return -2;
    }
    printf("当前摄像头尺寸: width * height = %u * %u\n", video_formt.fmt.pix.width, video_formt.fmt.pix.height);

    // 3.请求申请缓冲区的数量
    struct v4l2_requestbuffers video_requestbuffers;
    memset(&video_requestbuffers, 0, sizeof(struct v4l2_requestbuffers));
    video_requestbuffers.count = 4;
    video_requestbuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    video_requestbuffers.memory = V4L2_MEMORY_MMAP;
    if (ioctl(video_fd, VIDIOC_REQBUFS, &video_requestbuffers))
        return -3;

    // 4. 获取缓冲区的首地址
    struct v4l2_buffer video_buffer;
    memset(&video_buffer, 0, sizeof(struct v4l2_buffer));
    int32_t i = 0;
    for (i = 0; i < video_requestbuffers.count; i++) {
        video_buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        video_buffer.memory = V4L2_MEMORY_MMAP;
        video_buffer.index = i; /*缓冲区的编号*/
        if (ioctl(video_fd, VIDIOC_QUERYBUF, &video_buffer))
            return -4;

        image_buffer[i] = (uint8_t *)mmap(NULL, video_buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, video_fd, video_buffer.m.offset);
    }

    // 5. 将缓冲区加入到采集队列
    memset(&video_buffer, 0, sizeof(struct v4l2_buffer));
    for (i = 0; i < video_requestbuffers.count; i++) {
        video_buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        video_buffer.memory = V4L2_MEMORY_MMAP;
        video_buffer.index = i; /*缓冲区的编号*/
        if (ioctl(video_fd, VIDIOC_QBUF, &video_buffer))
            return -5;
    }

    // 6. 启动采集队列
    int opt = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(video_fd, VIDIOC_STREAMON, &opt))
        return -6;

    return 0;
}

int yuyv_to_yuv420p(const uint8_t *in, uint8_t *out, uint32_t width, uint32_t height)
{
    uint8_t *y = out;
    uint8_t *u = out + width * height;
    uint8_t *v = out + width * height + width * height / 4;
    uint32_t i, j;
    uint32_t base_h;
    uint32_t is_u = 1;
    uint32_t y_index = 0, u_index = 0, v_index = 0;
    uint64_t yuv422_length = 2 * width * height;
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
}

/*
子线程函数: 采集摄像头的数据
*/
void *pthread_read_video_data(void *arg)
{
    /*1. 循环读取摄像头采集的数据*/
    struct pollfd fds;
    fds.fd = video_fd;
    fds.events = POLLIN;

    /*2. 申请存放JPG的数据空间*/
    struct v4l2_buffer video_buffer;
    while (1)
    {
        /*(1)等待摄像头采集数据*/
        poll(&fds, 1, -1);
        /*(2)取出队列里采集完毕的缓冲区*/
        video_buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; /*视频捕获设备*/
        video_buffer.memory = V4L2_MEMORY_MMAP;
        ioctl(video_fd, VIDIOC_DQBUF, &video_buffer);
        /*(3)处理图像数据*/
        /*YUYV数据转YUV420P*/
        pthread_mutex_lock(&mutex); /*互斥锁上锁*/
        yuyv_to_yuv420p(image_buffer[video_buffer.index], YUV420P_Buffer, VIDEO_WIDTH, VIDEO_HEIGHT);
        pthread_mutex_unlock(&mutex);  /*互斥锁解锁*/
        pthread_cond_broadcast(&cond); /*广播方式唤醒休眠的线程*/

        FILE *fp = fopen("yuyv.yuv", "w+");
        fwrite(image_buffer[video_buffer.index], video_buffer.bytesused, 1, fp);
        fclose(fp);

        /*(4)将缓冲区再放入队列*/
        ioctl(video_fd, VIDIOC_QBUF, &video_buffer);
    }
}

// 运行示例:  ./a.out /dev/video0
int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("./app </dev/videoX>\n");
        return 0;
    }

    int err;
    pthread_t thread_id;

    /*初始化互斥锁*/
    pthread_mutex_init(&mutex, NULL);
    /*初始化条件变量*/
    pthread_cond_init(&cond, NULL);

    /*初始化摄像头设备*/
    err = VideoDeviceInit(argv[1]);
    if (err != 0)
        return err;

    /*创建子线程: 采集摄像头的数据*/
    pthread_create(&thread_id, NULL, pthread_read_video_data, NULL);
    /*设置线程的分离属性: 采集摄像头的数据*/
    pthread_detach(thread_id);

    char filename[128];
    time_t t;
    struct tm *tme;
    // 开始音频、视频编码
    for (int32_t i = 0; i < 3; ++i)
    {
        // 获取本地时间
        t = time(NULL);
        t = t + 8 * 60 * 60; //+上8个小时
        tme = gmtime(&t);
        sprintf(filename, "%d-%d-%d-%d-%d-%d.mp4", tme->tm_year + 1900, tme->tm_mon + 1, tme->tm_mday, tme->tm_hour, tme->tm_min, tme->tm_sec);
        printf("视频名称:%s\n", filename);

        // 开始视频编码
        video_audio_encode(filename);
    }
    return 0;
}