/*************************************************************************
    > File Name: helpers.h
    > Author: hsz
    > Brief:
    > Created Time: Thu 14 Nov 2024 02:29:57 PM CST
 ************************************************************************/

/*
 * libstatgrab
 * https://libstatgrab.org
 * Copyright (C) 2003-2004 Peter Saunders
 * Copyright (C) 2003-2019 Tim Bishop
 * Copyright (C) 2003-2013 Adam Sampson
 * Copyright (C) 2012-2019 Jens Rehsack
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#ifndef __SG_EXAMPLES_HELPERS_H__
#define __SG_EXAMPLES_HELPERS_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if defined(WITH_LIBLOG4CPLUS)
#include <log4cplus/clogger.h>
#endif

#include <signal.h>
#include <errno.h>

int register_sig_flagger(int signo, int *flag_ptr);
int sg_warn(const char *prefix);
void sg_die(const char *prefix, int exit_code);
void die(int error, const char *fmt, ...);
int inp_wait(int delay);

#endif /* __SG_EXAMPLES_HELPERS_H__ */