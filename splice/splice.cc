/*************************************************************************
    > File Name: splice.cc
    > Author: hsz
    > Brief:
    > Created Time: 2024年06月25日 星期二 13时24分59秒
 ************************************************************************/

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <stdio.h>

#define SOURCE_FILE     "source.out"
#define DEST_FILE       "dest.out"
#define PIPE_SIZE       4096

int main(int argc, char *argv[])
{
    const char *path = SOURCE_FILE;
    if (argc > 1) {
        path = argv[1];
    }

    int source_fd, dest_fd;
    int pipe_fd[2];
    struct stat stat_buf;
    ssize_t bytes;

    // 打开源文件
    source_fd = open(SOURCE_FILE, O_RDONLY);
    if (source_fd < 0) {
        perror("open source file");
        return 1;
    }

    // 获取源文件大小
    if (fstat(source_fd, &stat_buf) < 0) {
        perror("fstat");
        close(source_fd);
        return 1;
    }

    // 打开目标文件
    dest_fd = open(DEST_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0664);
    if (dest_fd < 0) {
        perror("open dest file");
        close(source_fd);
        return 1;
    }

    // 创建管道
    if (pipe(pipe_fd) < 0) {
        perror("pipe");
        close(source_fd);
        close(dest_fd);
        return 1;
    }

    // 使用 splice 进行零拷贝传输
    while ((bytes = splice(source_fd, NULL, pipe_fd[1], NULL, PIPE_SIZE, SPLICE_F_MOVE)) > 0) {
        splice(pipe_fd[0], NULL, dest_fd, NULL, bytes, SPLICE_F_MOVE);
    }

    if (bytes < 0) {
        perror("splice");
        close(source_fd);
        close(dest_fd);
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        return 1;
    }

    printf("File copied successfully using zero-copy splice\n");

    // 关闭文件描述符
    close(source_fd);
    close(dest_fd);
    close(pipe_fd[0]);
    close(pipe_fd[1]);

    return 0;
}
