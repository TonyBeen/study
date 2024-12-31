/*************************************************************************
    > File Name: uring_register_file.cc
    > Author: hsz
    > Brief:
    > Created Time: 2024年12月31日 星期二 16时33分23秒
 ************************************************************************/

#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <liburing.h>
#include <cstring>

#define BUFFER_SIZE 4096

// 文件拷贝函数
int io_uring_file_copy(const char *source, const char *destination)
{
    struct io_uring ring;
    struct io_uring_sqe *rsqe;
    struct io_uring_sqe *wsqe;
    struct io_uring_cqe *cqe;
    int src_fd, dest_fd;
    char *buffer;
    int ret;

    // 初始化 io_uring
    ret = io_uring_queue_init(32, &ring, 0);
    if (ret < 0) {
        std::cerr << "io_uring_queue_init failed: " << strerror(-ret) << std::endl;
        return ret;
    }

    // 打开源文件和目标文件
    src_fd = open(source, O_RDONLY);
    if (src_fd < 0) {
        std::cerr << "Failed to open source file: " << strerror(errno) << std::endl;
        io_uring_queue_exit(&ring);
        return -1;
    }

    dest_fd = open(destination, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_fd < 0) {
        std::cerr << "Failed to open destination file: " << strerror(errno) << std::endl;
        close(src_fd);
        io_uring_queue_exit(&ring);
        return -1;
    }

    // 注册缓冲区
    buffer = (char *)malloc(BUFFER_SIZE);
    if (!buffer) {
        std::cerr << "Failed to allocate buffer" << std::endl;
        close(src_fd);
        close(dest_fd);
        io_uring_queue_exit(&ring);
        return -1;
    }

    // 注册文件描述符
    int files[2] = {src_fd, dest_fd};
    ret = io_uring_register_files(&ring, files, 2);
    if (ret < 0) {
        std::cerr << "Failed to register files: " << strerror(-ret) << std::endl;
        free(buffer);
        close(src_fd);
        close(dest_fd);
        io_uring_queue_exit(&ring);
        return -1;
    }

    struct stat file_stat;
    fstat(src_fd, &file_stat);

    // 链式操作：读取数据并写入目标文件
    for (size_t offset = 0; offset < file_stat.st_size; offset += BUFFER_SIZE) {
        // 提交读取操作
        int32_t read_size = (file_stat.st_size - offset) > BUFFER_SIZE ? BUFFER_SIZE : (file_stat.st_size - offset);
        rsqe = io_uring_get_sqe(&ring);
        io_uring_prep_read(rsqe, src_fd, buffer, read_size, offset);
        io_uring_sqe_set_data(rsqe, 0);

        // 提交写入操作
        wsqe = io_uring_get_sqe(&ring);
        io_uring_prep_write(wsqe, dest_fd, buffer, read_size, offset);
        io_uring_sqe_set_data(rsqe, (void *)1);

        // 等待写入操作完成
        ret = io_uring_submit_and_wait(&ring, 2);
        if (ret < 0) {
            std::cerr << "io_uring_wait_cqe failed: " << strerror(-ret) << std::endl;
            break;
        }

        uint32_t head;
        uint32_t count = 0;
        io_uring_for_each_cqe(&ring, head, cqe) {
            ++count;
            if (cqe->user_data) { // write
                if (cqe->res <= 0) {
                    printf("write error %d\n", cqe->res);
                    return -1;
                }
            } else {
                if (cqe->res <= 0) {
                    printf("read error %d\n", cqe->res);
                    return -1;
                }
            }
        }
        io_uring_cq_advance(&ring, count);
    }

    // 清理资源
    free(buffer);
    close(src_fd);
    close(dest_fd);
    io_uring_queue_exit(&ring);

    return 0;
}

int main()
{
    const char *source = "800MB.out";  // 你要拷贝的源文件
    const char *destination = "800MB-copy.out";  // 目标文件

    int ret = io_uring_file_copy(source, destination);
    if (ret == 0) {
        std::cout << "File copy completed successfully!" << std::endl;
    } else {
        std::cerr << "File copy failed!" << std::endl;
    }

    return 0;
}

