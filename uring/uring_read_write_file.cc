/*************************************************************************
    > File Name: uring_read_write_file.cc
    > Author: hsz
    > Brief:
    > Created Time: 2024年07月25日 星期四 11时17分06秒
 ************************************************************************/

#include <iostream>
#include <liburing.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

#define QUEUE_DEPTH 1
#define CACHE_SIZE 4096

void setup_io_uring(struct io_uring *ring) {
    if (io_uring_queue_init(QUEUE_DEPTH, ring, 0) < 0) {
        perror("io_uring_queue_init");
        exit(1);
    }
}

void write_file(struct io_uring *ring, const char *filename) {
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open");
        exit(1);
    }

    char buf[CACHE_SIZE];
    snprintf(buf, sizeof(buf), "Hello, IO Uring!");

    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_uring_prep_write(sqe, fd, buf, strlen(buf), 0);

    if (io_uring_submit(ring) < 0) {
        perror("io_uring_submit");
        exit(1);
    }

    struct io_uring_cqe *cqe = nullptr;
    io_uring_wait_cqe(ring, &cqe);
    io_uring_cqe_seen(ring, cqe);

    close(fd);
}

void read_file(struct io_uring *ring, const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("open");
        exit(1);
    }

    char buf[CACHE_SIZE];
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_uring_prep_read(sqe, fd, buf, CACHE_SIZE, 0);

    if (io_uring_submit(ring) < 0) {
        perror("io_uring_submit");
        exit(1);
    }

    struct io_uring_cqe *cqe = nullptr;
    io_uring_wait_cqe(ring, &cqe);

    if (cqe->res < 0) {
        std::cerr << "Read error!" << std::endl;
    } else {
        std::cout << "Read: " << std::string(buf, cqe->res) << std::endl;
    }

    io_uring_cqe_seen(ring, cqe); // 或者调用io_uring_cq_advance(ring, 1);
    close(fd);
}

int main() {
    const char *filename = "example.txt";
    struct io_uring ring;

    setup_io_uring(&ring);
    write_file(&ring, filename);
    read_file(&ring, filename);

    io_uring_queue_exit(&ring);
    return 0;
}
