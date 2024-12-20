#include <liburing.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#define PORT            8080
#define MAX_CONNECTIONS 512
#define BUFFER_COUNT    128
#define BUFFER_SIZE     4096

struct conn_info {
    int     fd;
    int16_t buffer_index;
};

struct io_uring ring;
char registered_buffers[BUFFER_COUNT][BUFFER_SIZE];
struct iovec iovecs[BUFFER_COUNT];

// 添加 accept 请求
void add_accept(struct io_uring *ring, int listen_fd, struct sockaddr *client_addr, socklen_t *client_len) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_uring_prep_accept(sqe, listen_fd, client_addr, client_len, 0);
    struct conn_info conn_i = { .fd = listen_fd, .buffer_index = -1 };
    io_uring_sqe_set_data(sqe, &conn_i);
}

// 添加 read 请求
void add_read_fixed(struct io_uring *ring, int fd, int buffer_index) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_uring_prep_read_fixed(sqe, fd, registered_buffers[buffer_index], BUFFER_SIZE, 0, buffer_index);
    struct conn_info conn_i = { .fd = fd, .buffer_index = buffer_index };
    io_uring_sqe_set_data(sqe, &conn_i);
}

// 添加 write 请求
void add_write_fixed(struct io_uring *ring, int fd, int buffer_index, int size) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_uring_prep_write_fixed(sqe, fd, registered_buffers[buffer_index], size, 0, buffer_index);
    struct conn_info conn_i = { .fd = fd, .buffer_index = buffer_index };
    io_uring_sqe_set_data(sqe, &conn_i);
}

// 主函数
int main() {
    // 创建 socket
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 设置地址复用
    int optval = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // 绑定端口并监听
    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }
    if (listen(listen_fd, MAX_CONNECTIONS) < 0) {
        perror("listen failed");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    // 初始化 io_uring
    io_uring_queue_init(1024, &ring, 0);

    // 注册缓冲区
    for (int i = 0; i < BUFFER_COUNT; i++) {
        iovecs[i].iov_base = registered_buffers[i];
        iovecs[i].iov_len = BUFFER_SIZE;
    }

    if (io_uring_register_buffers(&ring, iovecs, BUFFER_COUNT) < 0) {
        perror("io_uring_register_buffers failed");
        io_uring_queue_exit(&ring);
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    // 添加第一个 accept
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    add_accept(&ring, listen_fd, (struct sockaddr *)&client_addr, &client_len);
    io_uring_submit(&ring);

    // 事件循环
    while (1) {
        struct io_uring_cqe *cqe;
        io_uring_wait_cqe(&ring, &cqe);

        struct conn_info *conn_i = (conn_info *)io_uring_cqe_get_data(cqe);
        int res = cqe->res;

        if (res < 0) {
            fprintf(stderr, "Operation failed: %s\n", strerror(-res));
            close(conn_i->fd);
        } else if (conn_i->buffer_index == -1) {
            // accept 完成，启动读请求
            int client_fd = res;
            printf("Accepted connection on fd %d\n", client_fd);
            add_read_fixed(&ring, client_fd, client_fd % BUFFER_COUNT);
        } else {
            if (cqe->opcode == IORING_OP_READ_FIXED) {
                if (res == 0) {
                    // 客户端关闭连接
                    printf("Client disconnected on fd %d\n", conn_i->fd);
                    close(conn_i->fd);
                } else {
                    // 读取数据，准备写回
                    printf("Read %d bytes on fd %d\n", res, conn_i->fd);
                    add_write_fixed(&ring, conn_i->fd, conn_i->buffer_index, res);
                }
            } else if (cqe->opcode == IORING_OP_WRITE_FIXED) {
                // 写完成，重新读取
                add_read_fixed(&ring, conn_i->fd, conn_i->buffer_index);
            }
        }

        io_uring_cqe_seen(&ring, cqe);
        io_uring_submit(&ring);
    }

    // 退出前清理资源
    io_uring_unregister_buffers(&ring);
    io_uring_queue_exit(&ring);
    close(listen_fd);
    return 0;
}
