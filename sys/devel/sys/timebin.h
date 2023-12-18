/*	$NetBSD: time.h,v 1.62 2008/07/15 16:18:09 christos Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)time.h	8.5 (Berkeley) 5/4/95
 */

#ifndef _SYS_TIMEBIN_H_
#define _SYS_TIMEBIN_H_

#include <sys/types.h>

/*
 * hide bintime for _STANDALONE because this header is used for hpcboot.exe,
 * which is built with compilers which don't recognize LL suffix.
 *	http://mail-index.NetBSD.org/tech-userlevel/2008/02/27/msg000181.html
 */
#if !defined(_STANDALONE)
struct bintime {
	time_t	sec;
	uint64_t frac;
};

static __inline void
bintime_addx(struct bintime *bt, uint64_t x)
{
	uint64_t u;

	u = bt->frac;
	bt->frac += x;
	if (u > bt->frac)
		bt->sec++;
}

static __inline void
bintime_add(struct bintime *bt, const struct bintime *bt2)
{
	uint64_t u;

	u = bt->frac;
	bt->frac += bt2->frac;
	if (u > bt->frac)
		bt->sec++;
	bt->sec += bt2->sec;
}

static __inline void
bintime_sub(struct bintime *bt, const struct bintime *bt2)
{
	uint64_t u;

	u = bt->frac;
	bt->frac -= bt2->frac;
	if (u < bt->frac)
		bt->sec--;
	bt->sec -= bt2->sec;
}

#define	bintimecmp(bta, btb, cmp)					\
	(((bta)->sec == (btb)->sec) ?					\
	    ((bta)->frac cmp (btb)->frac) :				\
	    ((bta)->sec cmp (btb)->sec))

/*-
 * Background information:
 *
 * When converting between timestamps on parallel timescales of differing
 * resolutions it is historical and scientific practice to round down rather
 * than doing 4/5 rounding.
 *
 *   The date changes at midnight, not at noon.
 *
 *   Even at 15:59:59.999999999 it's not four'o'clock.
 *
 *   time_second ticks after N.999999999 not after N.4999999999
 */

/*
 * The magic numbers for converting ms/us/ns to fractions
 */

/* 1ms = (2^64) / 1000       */
#define	BINTIME_SCALE_MS	((uint64_t)18446744073709551ULL)

/* 1us = (2^64) / 1000000    */
#define	BINTIME_SCALE_US	((uint64_t)18446744073709ULL)

/* 1ns = (2^64) / 1000000000 */
#define	BINTIME_SCALE_NS	((uint64_t)18446744073ULL)

static __inline void
bintime2timespec(const struct bintime *bt, struct timespec *ts)
{

	ts->tv_sec = bt->sec;
	ts->tv_nsec =
	    (long)((1000000000ULL * (uint32_t)(bt->frac >> 32)) >> 32);
}

static __inline void
timespec2bintime(const struct timespec *ts, struct bintime *bt)
{

	bt->sec = ts->tv_sec;
	bt->frac = (uint64_t)ts->tv_nsec * BINTIME_SCALE_NS;
}

static __inline void
bintime2timeval(const struct bintime *bt, struct timeval *tv)
{

	tv->tv_sec = bt->sec;
	tv->tv_usec =
	    (int)((1000000ULL * (uint32_t)(bt->frac >> 32)) >> 32);
}

static __inline void
timeval2bintime(const struct timeval *tv, struct bintime *bt)
{

	bt->sec = tv->tv_sec;
	bt->frac = (uint64_t)tv->tv_usec * BINTIME_SCALE_US;
}

static __inline struct bintime
ms2bintime(uint64_t ms)
{
	struct bintime bt;

	bt.sec = (time_t)(ms / 1000U);
	bt.frac = (uint64_t)(ms % 1000U) * BINTIME_SCALE_MS;

	return bt;
}

static __inline struct bintime
us2bintime(uint64_t us)
{
	struct bintime bt;

	bt.sec = (time_t)(us / 1000000U);
	bt.frac = (uint64_t)(us % 1000000U) * BINTIME_SCALE_US;

	return bt;
}

static __inline struct bintime
ns2bintime(uint64_t ns)
{
	struct bintime bt;

	bt.sec = (time_t)(ns / 1000000000U);
	bt.frac = (uint64_t)(ns % 1000000000U) * BINTIME_SCALE_NS;

	return bt;
}
#endif /* !defined(_STANDALONE) */

#endif /* _SYS_TIMEBIN_H_ */
