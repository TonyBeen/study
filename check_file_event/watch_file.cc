/*************************************************************************
    > File Name: watch_file.cc
    > Author: hsz
    > Brief:
    > Created Time: Sat 28 Oct 2023 03:51:25 PM CST
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string>
#include <unistd.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <dirent.h>

#include <log/log.h>

#define LOG_TAG "watch_file"

#define EVENT_SIZE  (sizeof(struct inotify_event))
#define BUF_LEN     (1024 * (EVENT_SIZE + 16))


/**
struct inotify_event
{
    int wd;             // inotify_add_watch返回的文件描述符
    uint32_t mask;      // 触发的事件
    uint32_t cookie;    // 将两个事件关联 (cookie > 0时有效)
    uint32_t len;       // name的长度, 为0表示空, 无需做任何处理
    char name[];        // 操作的文件名字
};

*/

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
    if (ev->mask & IN_UNMOUNT)          opTotal += "IN_UNMOUNT | ";     // 当监视的目录或文件所在的文件系统被卸载时, 会触发 IN_UNMOUNT 事件
    if (ev->mask & IN_Q_OVERFLOW)       opTotal += "IN_Q_OVERFLOW | ";  // 当 inotify 事件队列溢出时, 会触发 IN_Q_OVERFLOW 事件. 事件可能会丢失
    if (ev->mask & IN_IGNORED)          opTotal += "IN_IGNORED | ";     // 当添加的监视被移除, 或者监视的目录被删除时, 会触发 IN_IGNORED 事件

    // Special flags
    if (ev->mask & IN_ONLYDIR)          opTotal += "IN_ONLYDIR | ";     // 表示只对目录进行监视. 如果被监视的对象不是一个目录, 则会返回一个错误
    if (ev->mask & IN_DONT_FOLLOW)      opTotal += "IN_DONT_FOLLOW | "; // 指示不要跟随符号链接. 如果被监视的对象是一个符号链接, 且设置了该标志, 则不会跟随符号链接所指向的目标进行监视
    if (ev->mask & IN_EXCL_UNLINK)      opTotal += "IN_EXCL_UNLINK | "; // 指示在监视文件或目录时忽略对其删除操作的事件. 即使文件或目录被删除, 也不会产生 IN_DELETE 或 IN_DELETE_SELF 事件
    if (ev->mask & IN_MASK_ADD)         opTotal += "IN_MASK_ADD | ";    // 指示在监视目录时, 可以自动为新创建的项目添加监视
    if (ev->mask & IN_ISDIR)            opTotal += "IN_ISDIR | ";       // 表示事件发生在一个目录上. 当使用 inotify_add_watch() 函数为目录添加监视时, 会自动设置这个标志
    if (ev->mask & IN_ONESHOT)          opTotal += "IN_ONESHOT | ";     // 仅对一个事件生效. 当设置了该标志时, 在触发一次事件之后, 监视对象会被自动移除.
                                                                        // IN_ONESHOT 标志仅对 inotify_add_watch() 函数有效, 不适用于 inotify_rm_watch() 函数

    std::string temp(opTotal.c_str(), opTotal.length() - 3);

    LOGI("wd: %d, mask: %#x(%s), cookie: %#x, len: %u, name: %s\n",
        ev->wd, ev->mask, temp.c_str(), ev->cookie, ev->len, ev->len > 0 ? ev->name : "(null)");
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

void WatchRecursive(int inotifyFd, const std::string &path)
{
    int32_t wd = inotify_add_watch(inotifyFd, path.c_str(), IN_ALL_EVENTS);
    assert(wd > 0);

    DIR *pDir = opendir(path.c_str());
    if (nullptr == pDir) {
        perror("opendir error");
        return;
    }

    struct dirent *pDirEntry;
    struct stat64 itemStat;
    while ((pDirEntry = readdir(pDir)) != nullptr) {
        if (std::string(".") == pDirEntry->d_name || std::string("..") == pDirEntry->d_name) {
            continue;
        }

        std::string itemPath = path + "/" + pDirEntry->d_name;
        if (-1 == lstat64(itemPath.c_str(), &itemStat))
        {
            perror("lstat64 error");
            closedir(pDir);
            return;
        }

        if (S_ISDIR(itemStat.st_mode) && !S_ISLNK(itemStat.st_mode))
        {
            WatchRecursive(inotifyFd, itemPath);
        }
    }

    closedir(pDir);
}

int main() {
    int length, i = 0;
    int fd;
    int wd;
    char buffer[BUF_LEN];

    eular::log::InitLog();
    eular::log::EnableLogColor(true);
    eular::log::SetPath("/home/eular/VSCode/study/check_file_event/");
    eular::log::addOutputNode(eular::LogWrite::FILEOUT);

    // 初始化 inotify 实例
    fd = inotify_init();
    if (fd < 0) {
        perror("inotify_init");
        return -1;
    }

    WatchRecursive(fd, "dir");

    // 添加要监视的文件或目录
    // wd = inotify_add_watch(fd, "dir", IN_ALL_EVENTS);
    // if (wd < 0) {
    //     perror("inotify_add_watch");
    //     return -1;
    // }

    printf("Start monitoring...\n");

    // 循环读取事件
    while (1) {
        length = read(fd, buffer, BUF_LEN);
        if (length < 0) {
            perror("read");
            return -1;
        }

        LOGI("read length: %d\n", length);

        // 处理每个事件
        while (i < length) {
            struct inotify_event* event = (struct inotify_event *)&buffer[i];
            dump_event(event);
            i += EVENT_SIZE + event->len;
        }

        LOGI("\n");

        // 重置索引
        i = 0;
    }

    // 移除监视
    inotify_rm_watch(fd, wd);

    // 关闭 inotify 实例
    close(fd);

    return 0;
}


// 在dir目录下新建目录, 此时在新目录下操作不会触发事件, 需要调用inotify_add_watch将新目录加到监视器才能继续监视新目录

// NOTE opendir readdir closedir目录操作相关的API会使用的inotify在监控时返回, 携带的信息中len为0, name为空

/**
删除带有文件的监控目录后触发的事件, IN_IGNORED表示表示监测的对象已经被移除或删除, 即使后面在目录恢复, 也不会在进行监测

重要事件: IN_DELETE IN_DELETE_SELF IN_IGNORED

read length: 64
wd: 1, mask: 0x40000020(IN_OPEN | IN_ISDIR), cookie: 0, len: 0, name: 
wd: 1, mask: 0x40000001(IN_ACCESS | IN_ISDIR), cookie: 0, len: 0, name: 
wd: 1, mask: 0x40000010(IN_CLOSE_NOWRITE | IN_ISDIR), cookie: 0, len: 0, name: 
wd: 1, mask: 0x40000020(IN_OPEN | IN_ISDIR), cookie: 0, len: 0, name: 

read length: 48
wd: 1, mask: 0x40000001(IN_ACCESS | IN_ISDIR), cookie: 0, len: 0, name: 
wd: 1, mask: 0x200(IN_DELETE), cookie: 0, len: 16, name: something.txt

read length: 16
wd: 1, mask: 0x40000010(IN_CLOSE_NOWRITE | IN_ISDIR), cookie: 0, len: 0, name: 

read length: 16
wd: 1, mask: 0x400(IN_DELETE_SELF), cookie: 0, len: 0, name: 

read length: 16
wd: 1, mask: 0x8000(IN_IGNORED), cookie: 0, len: 0, name: 

*/

/*
修改dir目录下文件名字, 会依次触发IN_MOVED_FROM和IN_MOVED_TO事件, 并且cookie相同

wd: 1, mask: 0x40(IN_MOVED_FROM), cookie: 0x1cfb, len: 16, name: soming.txt
wd: 1, mask: 0x80(IN_MOVED_TO), cookie: 0x1cfb, len: 16, name: something.txt
*/

/*
mv dir dir_new
mv dir ../
以上操作触发IN_MOVE_SELF事件, 但不会触发IN_IGNORED事件, 即在新的dir目录下的任何操作都会被继续监视

wd: 1, mask: 0x800(IN_MOVE_SELF), cookie: 0, len: 0, name: (null)
*/

/*
rm something.sss

wd: 1, mask: 0x200(IN_DELETE), cookie: 0, len: 16, name: something.sss
*/

/*
cp a.out ./dir

read length: 64
wd: 1, mask: 0x100(IN_CREATE), cookie: 0, len: 16, name: a.out
wd: 1, mask: 0x20(IN_OPEN), cookie: 0, len: 16, name: a.out

read length: 64
wd: 1, mask: 0x2(IN_MODIFY), cookie: 0, len: 16, name: a.out
wd: 1, mask: 0x8(IN_CLOSE_WRITE), cookie: 0, len: 16, name: a.out

cp a.out dir -a
read length: 160
wd: 1, mask: 0x100(IN_CREATE), cookie: 0, len: 16, name: a.out
wd: 1, mask: 0x20(IN_OPEN), cookie: 0, len: 16, name: a.out
wd: 1, mask: 0x2(IN_MODIFY), cookie: 0, len: 16, name: a.out
wd: 1, mask: 0x4(IN_ATTRIB), cookie: 0, len: 16, name: a.out
wd: 1, mask: 0x8(IN_CLOSE_WRITE), cookie: 0, len: 16, name: a.out
*/

/*
mv a.out dir

read length: 32
wd: 1, mask: 0x80(IN_MOVED_TO), cookie: 0x1d19, len: 16, name: a.out
*/

/*
mv dir/a.out .

read length: 32
wd: 1, mask: 0x40(IN_MOVED_FROM), cookie: 0x1d1a, len: 16, name: a.out
*/

/*
mv new_dir/ dir

read length: 32
wd: 1, mask: 0x40000080(IN_MOVED_TO | IN_ISDIR), cookie: 0x209c, len: 16, name: new_dir
*/

/*
mv dir/other_dir/ dir/old_dir

read length: 64
wd: 1, mask: 0x40000040(IN_MOVED_FROM | IN_ISDIR), cookie: 0x209f, len: 16, name: other_dir
wd: 1, mask: 0x40000080(IN_MOVED_TO | IN_ISDIR), cookie: 0x209f, len: 16, name: old_dir
*/

/**
 * 重命名 aaaaa.txt -> abc.txt
 * 
 * read length: 64
 * wd: 1, mask: 0x40(IN_MOVED_FROM), cookie: 0xd85e, len: 16, name: aaaaa.txt
 * wd: 1, mask: 0x80(IN_MOVED_TO), cookie: 0xd85e, len: 16, name: abc.txt
 * 
 */
