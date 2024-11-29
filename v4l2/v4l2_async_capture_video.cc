/*************************************************************************
    > File Name: v4l2_async_capture_video.cc
    > Author: hsz
    > Brief:
    > Created Time: 2024年11月29日 星期五 17时36分56秒
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <vector>
#include <memory>

#include <sched.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>

#define VIDEO_DEVICE "/dev/video0"

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

    while (1) {
        // 等待文件描述符变为可读
        int32_t ret = poll(fds, 1, -1);
        if (ret == -1) {
            perror("poll failed");
            break;
        }

        if (fds[0].revents & POLLIN) {
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

            // TODO 处理一帧图像

            // 可选：将缓冲区放回队列
            if (ioctl(deviceFd, VIDIOC_QBUF, &buf) == -1) {
                perror("VIDIOC_QBUF failed");
                break;
            }
        }
    }

    close(deviceFd);
    return 0;
}
