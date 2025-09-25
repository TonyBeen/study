/* Copyright (c) 2017 - 2022 LiteSpeed Technologies Inc.  See LICENSE. */
/*
 * prog.h -- common setup and options for QUIC program
 */

#ifndef PROG_H
#define PROG_H 1

#include "test_config.h"

struct event;
struct event_base;
struct lsquic_hash;
struct sport_head;
struct ssl_ctx_st;

struct prog
{
    struct lsquic_engine_settings prog_settings;
    struct lsquic_engine_api prog_api;
    unsigned prog_engine_flags;
    unsigned prog_packout_max;
    unsigned short prog_max_packet_size;
    int prog_version_cleared;
    unsigned long prog_read_count;
#if HAVE_SENDMMSG
    int prog_use_sendmmsg;
#endif
#if HAVE_RECVMMSG
    int prog_use_recvmmsg;
#endif
    int prog_use_stock_pmi;
    struct event_base *prog_eb;
    struct event *prog_timer;
    struct event *prog_send;
    struct event *prog_usr1;
    struct event *prog_usr2;
    struct ssl_ctx_st *prog_ssl_ctx;
    struct lsquic_hash *prog_certs;
    struct event *prog_event_sni;
    char *prog_susp_sni;
    struct sport_head *prog_sports;
    struct lsquic_engine *prog_engine;
    const char *prog_hostname;
    int prog_ipver; /* 0, 4, or 6 */
    enum
    {
        PROG_FLAG_COOLDOWN = 1 << 0,
        PROG_SEARCH_ADDRS = 1 << 1,
    } prog_flags;
};

int prog_init(struct prog *, unsigned lsquic_engine_flags, struct sport_head *,
              const struct lsquic_stream_if *, void *stream_if_ctx);

struct event_base *prog_eb(struct prog *);

int prog_run(struct prog *);

void prog_cleanup(struct prog *);

void prog_stop(struct prog *);

int prog_prep(struct prog *);

int prog_connect(struct prog *, unsigned char *, size_t);

int prog_is_stopped(void);

void prog_process_conns(struct prog *);

void prog_sport_cant_send(struct prog *, int fd);

#endif
