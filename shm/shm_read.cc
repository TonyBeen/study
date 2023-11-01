/*************************************************************************
    > File Name: shared_shm.cc
    > Author: hsz
    > Brief:
    > Created Time: Sun 29 Oct 2023 03:24:59 PM CST
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define SHM_SIZE 1024

int main() {
    key_t key = ftok(".", 'A');  // 使用与进程A相同的键值
    int shmid, semid;
    char *shm;
    struct sembuf sops;

    // 获取共享内存段的标识符
    shmid = shmget(key, SHM_SIZE, 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }

    // 获取信号量标识符
    semid = semget(key, 1, 0666);
    if (semid == -1) {
        perror("semget");
        exit(1);
    }

    // 等待信号量的值变为0
    sops.sem_num = 0;
    sops.sem_op = -1;
    sops.sem_flg = SEM_UNDO;
    semop(semid, &sops, 1);

    // 将共享内存连接到当前进程的地址空间
    shm = (char *)shmat(shmid, NULL, 0);
    if (shm == (char *) -1) {
        perror("shmat");
        exit(1);
    }

    printf("Process B read: %s\n", shm);  // 从共享内存读取数据

    // 释放信号量
    sops.sem_op = 1;
    semop(semid, &sops, 1);

    // 和共享内存断开连接
    if (shmdt(shm) == -1) {
        perror("shmdt");
        exit(1);
    }

    // 由读进程删除共享内存
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}
