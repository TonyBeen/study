/*************************************************************************
    > File Name: fstat.cc
    > Author: hsz
    > Brief: fstat可以通过st_mode判断出是套接字还是普通文件
    > Created Time: 2024年07月26日 星期五 10时00分18秒
 ************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <ctime>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/time.h>

int InitSocket(uint16_t port = 8000)
{
    int sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        perror("socket error");
        return -1;
    }

    sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (::bind(sock, (sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind error");
        goto error_return;
    }

    if (::listen(sock, 128) < 0) {
        perror("listen error.");
        goto error_return;
    }

    return sock;

error_return:
    ::close(sock);
    return -1;
}

int32_t CreateFile()
{
    int32_t fd = ::open("example.out", O_RDWR | O_CREAT | O_TRUNC, 0664);
    if (fd < 0) {
        perror("open error");
    }

    return fd;
}

void print_file_type(mode_t mode)
{
    if (S_ISREG(mode)) printf("常规文件\n");
    else if (S_ISDIR(mode)) printf("目录\n");
    else if (S_ISCHR(mode)) printf("字符设备\n");
    else if (S_ISBLK(mode)) printf("块设备\n");
    else if (S_ISFIFO(mode)) printf("FIFO/管道\n");
    else if (S_ISLNK(mode)) printf("符号链接\n");
    else if (S_ISSOCK(mode)) printf("套接字\n");
    else printf("未知类型\n");
}

void print_stat_info(const struct stat *statbuf) {

    print_file_type(statbuf->st_mode);

    printf("\t设备ID: %ld\n", (long) statbuf->st_dev);
    printf("\t文件inode: %ld\n", (long) statbuf->st_ino);
    printf("\t文件模式: %o\n", statbuf->st_mode);
    printf("\t硬链接数: %ld\n", (long) statbuf->st_nlink);
    printf("\t所有者用户ID: %ld\n", (long) statbuf->st_uid);
    printf("\t所有者组ID: %ld\n", (long) statbuf->st_gid);
    printf("\t设备ID(如果是特殊文件): %ld\n", (long) statbuf->st_rdev);
    printf("\t文件大小: %ld 字节\n", (long) statbuf->st_size);
    printf("\t文件系统块大小: %ld\n", (long) statbuf->st_blksize);
    printf("\t占用块数: %ld\n", (long) statbuf->st_blocks);
    printf("\t上次访问时间: %s", ctime(&statbuf->st_atime));
    printf("\t上次修改时间: %s", ctime(&statbuf->st_mtime));
    printf("\t上次状态改变时间: %s", ctime(&statbuf->st_ctime));
}

void GetStat(int32_t fd)
{
    struct stat statbuf;
    int32_t ret = fstat(fd, &statbuf);
    if (ret != 0) {
        perror("fstat error");
        return;
    }

    print_stat_info(&statbuf);
}

int main(int argc, char **argv)
{
    int32_t sockFd = InitSocket();
    GetStat(sockFd);

    printf("==========================================\n");

    int32_t fileFd = CreateFile();
    GetStat(fileFd);

    ::close(sockFd);
    ::close(fileFd);
    return 0;
}
