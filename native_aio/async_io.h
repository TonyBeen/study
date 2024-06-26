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

#ifndef _ASYNCIO_H
#define _ASYNCIO_H


// 通过 _POSIX_ASYNCHRONOUS_IO _POSIX_ASYNC_IO判断支不支持异步IO

#include <linux/aio_abi.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*io_callback_t)(aio_context_t ctx, struct iocb *iocb, long res, long res2);

int32_t io_setup(uint32_t nr, aio_context_t* ctxp);
int32_t io_destroy(aio_context_t ctx);
int32_t io_submit(aio_context_t ctx, long nr, struct iocb** iocbpp);
int32_t io_getevents(aio_context_t ctx, long min_nr, long max_nr, struct io_event* events, struct timespec* timeout);
int32_t io_cancel(aio_context_t ctx, struct iocb*, struct io_event* result);
int32_t io_wait_all(aio_context_t ctx, uint32_t nr, struct iocb* piocb, io_callback_t cb, struct timespec* timeout);

void io_prep_pread(struct iocb* iocb, int32_t fd, void* buf, size_t count, long long offset);
void io_prep_pwrite(struct iocb* iocb, int32_t fd, void* buf, size_t count, long long offset);

#ifdef __cplusplus
};
#endif

#endif  // ASYNCIO_H
