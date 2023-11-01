/*************************************************************************
    > File Name: test_shm.cc
    > Author: hsz
    > Brief:
    > Created Time: Sun 29 Oct 2023 03:20:05 PM CST
 ************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>

// 共享内存创建及使用

int main(int argc, char **argv)
{
    // 1、生成唯一的key值, 也可以自己指定, 但是易冲突
    // const char *filePath = "shm.dat";
    // int fileHandle = ::open(filePath, O_RDWR | O_CREAT | O_TRUNC, 0755);
    // if (fileHandle < 0) {
    //     perror("create shm.dat error");
    //     return 0;
    // }

    // close(fileHandle);
    
    // getuid不会失败
    uint32_t uid = getuid();
    printf("uid = %d\n", uid);

    // 第一个参数: 一个存在的文件路径, 第二个参数: 大于0的8位整数, 虽然是int类型, 但是只有低8位有效
    key_t shmKeyID = ftok(".", 'A');
    if (shmKeyID < 0) {
        perror("ftok error");
        return 0;
    }

    // 2、创建或打开共享内存
    /**
     * IPC_CREAT: 创建共享内存标志
     * IPC_EXCL: 只只能和IPC_CREAT一起使用, 若共享内存存在, 则报错, 并且errno == EEXIST
     * 
     * 0755 所有者具有读写执行权限, 所有组和其他用户具有读和执行权限
     *  一般和文件权限一致
     */
    int32_t shmId = shmget(shmKeyID, 1024, IPC_CREAT | IPC_EXCL | 0755);
    if (shmId < 0) {
        if (errno != EEXIST) {
            perror("shm error");
            return 0;
        } else {
            shmId = shmget(shmKeyID, 0, 0);
            assert(shmId > 0);
        }
    }

    // 3、关联共享内存
    void *shmBuffer = shmat(shmId, NULL, 0);

    // 获取信号量标识符
    int32_t semId = semget(shmKeyID, 1, IPC_CREAT | 0666);
    if (semId == -1) {
        perror("semget");
        exit(1);
    }

    // 初始化信号量的值为1
    semctl(semId, 0, SETVAL, 1);

    struct sembuf semops;
    semops.sem_num = 0;         // 表示要操作的信号量在信号量集中的索引, 从0开始计数. 如果信号量集中只有一个信号量, 通常将其设置为0.
    /**
     * sem_op > 0:  表示对信号量的值进行增加操作 (V操作)
     * sem_op == 0: 表示等待信号量的值变为0     (P操作)
     * sem_op < 0:  表示对信号量的值进行减少操作 (P操作)
     */
    semops.sem_op = -1;

    /**
     * SEM_UNDO:    在进程退出时自动撤销对信号量的操作，以防止死锁
     * IPC_NOWAIT:  如果无法立即进行操作，则不等待，直接返回错误
     */
    semops.sem_flg = SEM_UNDO;
    semop(semId, &semops, 1);

    // 4、操作数据
    const char *data = "Hello World From Process A";
    strcpy((char *)shmBuffer, data);

    // 发送信号
    semops.sem_op = 1;
    semop(semId, &semops, 1);

    // 5、和共享内存断开连接
    if (shmdt(shmBuffer) < 0) {
        perror("shmdt error");
    }

_error:
    return 0;
}
