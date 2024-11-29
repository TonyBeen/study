/*************************************************************************
    > File Name: avdevice_list_video.cc
    > Author: hsz
    > Brief:
    > Created Time: 2024年11月28日 星期四 11时15分53秒
 ************************************************************************/

#define __STDC_CONSTANT_MACROS
#include <stdio.h>
#include <utils/utils.h>

EXTERN_C_BEGIN
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
EXTERN_C_END

int main() {
    // 初始化 libavdevice
    avdevice_register_all();

    // 设备输入上下文
    AVInputFormat *input_format = NULL;
    AVDeviceInfoList *device_list = NULL;

    /** 支持以下输入设备
    alsa
    avfoundation
    bktr
    dshow
    dv1394
    fbdev
    gdigrab
    iec61883
    jack
    lavfi
    libcdio
    libdc1394
    openal
    oss
    pulse
    qtkit
    sndio
    video4linux2, v4l2
    vfwcap
    x11grab
    decklink */

    /**
     * @brief 
     * windows下：VFW(Video for Windows) dshow(Directshow)
     * 
     * linux:
     */

    // 获取视频输入设备列表
    AVFormatContext* pFormatCtx = avformat_alloc_context();
    AVInputFormat *ifmt = av_find_input_format("video4linux2");
    if (avformat_open_input(&pFormatCtx, "/dev/video0", ifmt, NULL)!=0) {
        printf("Couldn't open input stream.\n");
        return -1;
    }

    if (avdevice_list_devices(pFormatCtx, &device_list) < 0) {
        printf("无法列举设备！\n");
        return -1;
    }

    // 遍历设备列表并打印设备信息
    for (int i = 0; i < device_list->nb_devices; i++) {
        printf("设备 %d:\n", i);
        printf("设备名称: %s\n", device_list->devices[i]->device_name);
        printf("设备描述: %s\n", device_list->devices[i]->device_description);
    }

    avformat_free_context(pFormatCtx);
    pFormatCtx = nullptr;
    // 释放设备列表
    avdevice_free_list_devices(&device_list);
    return 0;
}
