/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "async_io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <cstdint>
#include <cstring>

#if !defined(_POSIX_ASYNCHRONOUS_IO) && !defined(_POSIX_ASYNC_IO)
#error "aoi not support"
#endif

int32_t io_setup(uint32_t nr, aio_context_t* ctxp)
{
    return syscall(__NR_io_setup, nr, ctxp);
}

int32_t io_destroy(aio_context_t ctx)
{
    return syscall(__NR_io_destroy, ctx);
}

int32_t io_submit(aio_context_t ctx, long nr, iocb** iocbpp)
{
    return syscall(__NR_io_submit, ctx, nr, iocbpp);
}

/**
 * @brief io_getevents系统调用尝试从ctx指定的AIO上下文的完成队列中读取至少min_nr个事件，最多max_nr个事件
 * 
 * @param ctx aio_context_t句柄
 * @param min_nr 最少读取多少个事件, 如果超时设置为NULL, min_nr设置为1, 则会阻塞直到读取到一个完成事件
 * @param max_nr 最大读取多少个事件
 * @param events 返回io_event, io_event个数应等于max_nr, 
 * @param timeout 最大等待时间
 * @return int32_t 成功返回完成事件个数, 失败返回-1. 如果被信号中断, 设置错误码EINTR
 */
int32_t io_getevents(aio_context_t ctx, long min_nr, long max_nr, io_event* events, timespec* timeout)
{
    return syscall(__NR_io_getevents, ctx, min_nr, max_nr, events, timeout);
}

int32_t io_cancel(aio_context_t ctx, iocb* iocbp, io_event* result)
{
    return syscall(__NR_io_cancel, ctx, iocbp, result);
}

int32_t io_wait_all(aio_context_t ctx, uint32_t nr, iocb* piocb, io_callback_t cb, timespec *timeout)
{
    if (!nr) {
        return 0;
    }

    int32_t status = 0;
    int32_t max_nr = 0;
    uint32_t off = 0;
    struct iocb **ppiocb = (struct iocb **)malloc(nr * sizeof(struct iocb *));
    struct io_event *pevent = (struct io_event *)malloc(nr * sizeof(struct io_event));
    if (ppiocb == nullptr || pevent == nullptr) {
        free(ppiocb);
        free(pevent);
        return -ENOMEM;
    }

retry:
    for (uint32_t i = 0; i < (nr - off); ++i)
    {
        ppiocb[i] = &piocb[i + off];
    }

    status = io_submit(ctx, nr - off, ppiocb);
    if (status < 0) {
        free(ppiocb);
        free(pevent);
        return status;
    }

    if (status == 0) {
        free(ppiocb);
        free(pevent);
        return status + off;
    }

    max_nr = status;
    do {
        int32_t cnr = TEMP_FAILURE_RETRY(io_getevents(ctx, status, status, pevent, timeout));
        if (cnr < 0) {
            free(ppiocb);
            free(pevent);
            return cnr;
        }
        for (int32_t i = 0; i < cnr; ++i)
        {
            if (cb)
            {
                struct iocb *iocb = reinterpret_cast<struct iocb *>(pevent[i].obj);
                cb(ctx, iocb, pevent[i].res, pevent[i].res2);
            }
        }
        max_nr -= cnr;
    } while (max_nr > 0);

    off += status;
    // 有剩余事件
    if (off < nr) {
        goto retry;
    }

    free(ppiocb);
    free(pevent);
    return nr;
}

static inline void io_prep(iocb* iocb, int32_t fd, const void* buf, uint64_t count, int64_t offset, bool read)
{
    memset(iocb, 0, sizeof(*iocb));
    iocb->aio_fildes = fd;
    iocb->aio_lio_opcode = read ? IOCB_CMD_PREAD : IOCB_CMD_PWRITE;
    iocb->aio_reqprio = 0;
    iocb->aio_buf = reinterpret_cast<uint64_t>(buf);
    iocb->aio_nbytes = count;
    iocb->aio_offset = offset;
}

void io_prep_pread(struct iocb* iocb, int32_t fd, void* buf, size_t count, long long offset)
{
    io_prep(iocb, fd, buf, count, offset, true);
}

void io_prep_pwrite(struct iocb* iocb, int32_t fd, void* buf, size_t count, long long offset)
{
    io_prep(iocb, fd, buf, count, offset, false);
}
