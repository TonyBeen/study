/*************************************************************************
    > File Name: v4l2.cc
    > Author: hsz
    > Brief:
    > Created Time: Thu 11 Jan 2024 04:15:11 PM CST
 ************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <string>
#include <sstream>

#include <getopt.h>

#include <fcntl.h>
#include <unistd.h>
#include <sched.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <jpeglib.h>
#include <linux/videodev2.h>

// #include <log/log.h>

#define LOG_TAG "v4l2"

// https://linuxtv.org/downloads/v4l-dvb-apis/userspace-api/v4l/capture.c.html

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

static inline int32_t process_in_scope(int32_t v, int32_t maxV = 255, int32_t minV = 0)
{
    if (v > maxV) {
        return maxV;
    } else if (v < minV) {
        return minV;
    }

    return v;
}

void yuyv_to_rgb(uint8_t *yuyvdata, uint8_t *rgbdata, int32_t w, int32_t h)
{
    // 码流Y0 U0 Y1 V1 Y2 U2 Y3 V3 --> YUYV像素[Y0 U0 V1] [Y1 U0 V1] [Y2 U2 V3] [Y3 U2 V3]-> RGB像素
    int32_t r1, g1, b1; 
    int32_t r2, g2, b2;
    for(int32_t i = 0; i < w * h / 2; ++i)
    {
        char data[4];
        memcpy(data, yuyvdata + i * 4, 4);
        uint8_t Y0 = data[0];
        uint8_t U0 = data[1];
        uint8_t Y1 = data[2];
        uint8_t V1 = data[3];
        // Y0U0Y1V1 --> [Y0 U0 V1] [Y1 U0 V1]
        r1 = Y0 + 1.4075 * (V1 - 128);
        r1 = process_in_scope(r1);

        g1 = Y0 - 0.3455 * (U0 - 128) - 0.7169*(V1-128);
        g1 = process_in_scope(g1);

        b1 = Y0 + 1.779  * (U0 - 128);
        b1 = process_in_scope(b1);

        r2 = Y1+1.4075*(V1-128);
        r2 = process_in_scope(r2);

        g2 = Y1- 0.3455 * (U0-128) - 0.7169*(V1-128);
        g2 = process_in_scope(g2);

        b2 = Y1 + 1.779 * (U0-128);
        b2 = process_in_scope(b2);

        rgbdata[i*6+0]=r1;
        rgbdata[i*6+1]=g1;
        rgbdata[i*6+2]=b1;
        rgbdata[i*6+3]=r2;
        rgbdata[i*6+4]=g2;
        rgbdata[i*6+5]=b2;
    }
}

void save_jpeg(const char *filename, const void *data, int32_t size, int32_t width, int32_t height)
{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPROW row_pointer[1];

    FILE *outfile = fopen(filename, "wb");
    if (!outfile) {
        perror("Failed to open output file");
        return;
    }

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, outfile);

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_YCbCr;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, 80, TRUE);

    jpeg_start_compress(&cinfo, TRUE);

    while (cinfo.next_scanline < cinfo.image_height) {
        row_pointer[0] = (JSAMPLE *)((uint8_t *)data + cinfo.next_scanline * width * 3);
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    fclose(outfile);
    jpeg_destroy_compress(&cinfo);
}

int32_t main(int32_t argc, char *argv[])
{
    const char *devicePath = "/dev/video0";

    if (argc > 1) {
        devicePath = argv[1];
    }

    // 1、打开设备
    int32_t deviceFd = ::open(devicePath, O_RDWR); // O_NONBLOCK 可以配合select/poll/epoll使用
    if (deviceFd < 0) {
        perror("open error");
        return 1;
    }

    // 2、获取摄像头支持的格式ioctl(文件描述符, 命令, 与命令对应的结构体)
    struct v4l2_capability cap;
    if (0 > xioctl(deviceFd, VIDIOC_QUERYCAP, &cap)) {
        perror("Failed to query capability");
        close(deviceFd);
        return 1;
    }
    printf("Driver: %s\nCard: %s\nBus info: %s\n", cap.driver, cap.card, cap.bus_info);

    struct v4l2_fmtdesc v4l2_fmt_desc;
    v4l2_fmt_desc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_fmt_desc.index = 0;

    printf("Supported formats:\n");
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

    static const uint32_t WIDTH = 1280;
    static const uint32_t HEIGHT = 720;

    // 3、设置采集格式
    struct v4l2_format v4l2_fmt;
    v4l2_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;        // 摄像头采集
    v4l2_fmt.fmt.pix.width = WIDTH;                     // 设置宽(不能任意)
    v4l2_fmt.fmt.pix.height = HEIGHT;                   // 设置高
    v4l2_fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;  // 设置视频采集格式

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

    printf("Set successfully\n");

    static const uint32_t MMAP_COUNT = 4;

    // 4、申请内核缓冲区队列
    struct v4l2_requestbuffers v4l2_reqbuffer;
    v4l2_reqbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_reqbuffer.count = MMAP_COUNT;                  // 申请4个缓冲区
    v4l2_reqbuffer.memory = V4L2_MEMORY_MMAP ;          // 映射方式
    errorCode  = xioctl(deviceFd, VIDIOC_REQBUFS, &v4l2_reqbuffer);
    if (errorCode < 0) {
        errno_exit("ioctl VIDIOC_REQBUFS error");
    }

    // 5、把内核的缓冲区队列映射到用户空间
    uint8_t *mmapPtrVec[MMAP_COUNT] = {nullptr}; // 保存映射后用户空间的首地址
    uint32_t size[MMAP_COUNT];

    struct v4l2_buffer v4l2_mapbuffer;
    v4l2_mapbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    for (uint32_t i = 0; i < MMAP_COUNT; ++i) {
        v4l2_mapbuffer.index = i;

        // 从内核空间中查询一个空间做映射
        errorCode = xioctl(deviceFd, VIDIOC_QUERYBUF, &v4l2_mapbuffer);
        if (errorCode < 0) {
            perror("ioctl VIDIOC_QUERYBUF error");
            continue;
        }

        size[i] = v4l2_mapbuffer.length;
        mmapPtrVec[i] = (uint8_t *)mmap(NULL, v4l2_mapbuffer.length, PROT_READ | PROT_WRITE, 
                                        MAP_SHARED, deviceFd, v4l2_mapbuffer.m.offset);
        assert(nullptr != mmapPtrVec[i]);

        // 通知内核使用完毕
        errorCode = xioctl(deviceFd, VIDIOC_QBUF, &v4l2_mapbuffer);
        if (errorCode < 0) {
            errno_exit("ioctl VIDIOC_QBUF error");
        }
    }

    // 6、开始采集
    printf("Start capturing...\n");
    int32_t type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    errorCode = xioctl(deviceFd, VIDIOC_STREAMON, &type);
    if (errorCode < 0) {
        errno_exit("ioctl VIDIOC_STREAMON error");
    }

    // 7、获取数据, 从队列中提取一帧数据
    printf("read data\n");
    struct v4l2_buffer  v4l2_read_buffer;
    v4l2_read_buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    errorCode = xioctl(deviceFd, VIDIOC_DQBUF, &v4l2_read_buffer);
    if (errorCode < 0) {
        errno_exit("ioctl VIDIOC_DQBUF error");
    }

    FILE *file = fopen("my.jpg", "w+");
    fwrite(mmapPtrVec[v4l2_read_buffer.index], v4l2_read_buffer.bytesused, 1, file);
    fclose(file);

    printf("%p bytesused: %u, length: %u\n",
        mmapPtrVec[v4l2_read_buffer.index], v4l2_read_buffer.bytesused, v4l2_read_buffer.length);
    // save_jpeg("output.jpg", mmapPtrVec[v4l2_read_buffer.index], v4l2_read_buffer.bytesused, WIDTH, HEIGHT);

    // 通知内核使用完毕
    errorCode = ioctl(deviceFd, VIDIOC_QBUF, &v4l2_read_buffer);
    if (errorCode < 0) {
        errno_exit("ioctl VIDIOC_QBUF error");
    }

    // 8、停止采集
    xioctl(deviceFd, VIDIOC_STREAMOFF, &type);

    // 9、释放映射
    for (uint32_t i = 0; i < MMAP_COUNT; ++i) {
        munmap(mmapPtrVec[i], size[i]);
    }

    // 10、关闭设备
    close(deviceFd);
    return 0;
}