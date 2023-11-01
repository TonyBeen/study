/*************************************************************************
    > File Name: watch_file.cc
    > Author: hsz
    > Brief:
    > Created Time: Sat 28 Oct 2023 03:51:25 PM CST
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include <sys/inotify.h>

#define EVENT_SIZE  (sizeof(struct inotify_event))
#define BUF_LEN     (1024 * (EVENT_SIZE + 16))

void dump_event(struct inotify_event *ev)
{
    std::string opTotal = "";

    // Supported events suitable for MASK parameter of INOTIFY_ADD_WATCH. 
    if (ev->mask & IN_ACCESS)           opTotal += "IN_ACCESS | ";
    if (ev->mask & IN_MODIFY)           opTotal += "IN_MODIFY | ";
    if (ev->mask & IN_ATTRIB)           opTotal += "IN_ATTRIB | ";
    if (ev->mask & IN_CLOSE_WRITE)      opTotal += "IN_CLOSE_WRITE | ";
    if (ev->mask & IN_CLOSE_NOWRITE)    opTotal += "IN_CLOSE_NOWRITE | ";
    if (ev->mask & IN_OPEN)             opTotal += "IN_OPEN | ";
    if (ev->mask & IN_MOVED_FROM)       opTotal += "IN_MOVED_FROM | ";
    if (ev->mask & IN_MOVED_TO)         opTotal += "IN_MOVED_TO | ";
    if (ev->mask & IN_CREATE)           opTotal += "IN_CREATE | ";
    if (ev->mask & IN_DELETE)           opTotal += "IN_DELETE | ";
    if (ev->mask & IN_DELETE_SELF)      opTotal += "IN_DELETE_SELF | ";
    if (ev->mask & IN_MOVE_SELF)        opTotal += "IN_MOVE_SELF | ";

    // Events sent by the kernel.
    if (ev->mask & IN_UNMOUNT)          opTotal += "IN_UNMOUNT | ";
    if (ev->mask & IN_Q_OVERFLOW)       opTotal += "IN_Q_OVERFLOW | ";
    if (ev->mask & IN_IGNORED)          opTotal += "IN_IGNORED | ";

    // Special flags
    if (ev->mask & IN_ONLYDIR)          opTotal += "IN_ONLYDIR | ";
    if (ev->mask & IN_DONT_FOLLOW)      opTotal += "IN_DONT_FOLLOW | ";
    if (ev->mask & IN_EXCL_UNLINK)      opTotal += "IN_EXCL_UNLINK | ";
    if (ev->mask & IN_MASK_ADD)         opTotal += "IN_MASK_ADD | ";
    if (ev->mask & IN_ISDIR)            opTotal += "IN_ISDIR | ";
    if (ev->mask & IN_ONESHOT)          opTotal += "IN_ONESHOT | ";

    std::string temp(opTotal.c_str(), opTotal.length() - 3);

    printf("wd: %d, mask: %#x(%s), cookie: %#x, len: %u, name: %s\n",
        ev->wd, ev->mask, temp.c_str(), ev->cookie, ev->len, ev->name);
}

/**
 * IN_ACCESS            文件被访问
 * IN_MODIFY            文件被修改
 * IN_ATTRIB            文件属性值被修改
 * IN_CLOSE_WRITE       关闭为了写入而打开的文件
 * IN_CLOSE_NOWRITE     关闭只读打开的文件
 * IN_CLOSE             关闭打开的文件(IN_CLOSE_WRITE | IN_CLOSE_NOWRITE)
 * IN_OPEN              文件被打开
 * IN_MOVED_FROM        文件或目录从被监视的位置移动到其他位置
 * IN_MOVED_TO          文件或目录移动到被监视的位置
 * IN_MOVE              文件移动(IN_MOVED_FROM | IN_MOVED_TO)
 * IN_CREATE            在监听目录内创建了文件/目录
 * IN_DELETE            在监听的目录内删除文件/目录
 * IN_DELETE_SELF       监听的目录或文件被删除
 * IN_MOVE_SELF         监控的文件/目录本身被移动
 * IN_ALL_EVENTS        以上所有事件统称
 * 
 * IN_ISDIR             表示事件所涉及的路径是一个目录
 */

int main() {
    int length, i = 0;
    int fd;
    int wd;
    char buffer[BUF_LEN];

    // 初始化 inotify 实例
    fd = inotify_init();
    if (fd < 0) {
        perror("inotify_init");
        return -1;
    }

    // 添加要监视的文件或目录
    wd = inotify_add_watch(fd, "dir", IN_ALL_EVENTS);
    if (wd < 0) {
        perror("inotify_add_watch");
        return -1;
    }

    printf("Start monitoring...\n");

    // 循环读取事件
    while (1) {
        length = read(fd, buffer, BUF_LEN);
        if (length < 0) {
            perror("read");
            return -1;
        }

        printf("read length: %d\n", length);

        // 处理每个事件
        while (i < length) {
            struct inotify_event* event = (struct inotify_event *)&buffer[i];
            dump_event(event);
            i += EVENT_SIZE + event->len;
        }

        printf("\n\n");

        // 重置索引
        i = 0;
    }

    // 移除监视
    inotify_rm_watch(fd, wd);

    // 关闭 inotify 实例
    close(fd);

    return 0;
}

