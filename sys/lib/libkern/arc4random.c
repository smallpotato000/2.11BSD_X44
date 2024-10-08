/*	$NetBSD: arc4random.c,v 1.11.2.1 2004/09/11 10:40:10 he Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Thor Lancelot Simon.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * THE BEER-WARE LICENSE
 *
 * <dan@FreeBSD.ORG> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff.  If we meet some day, and you
 * think this stuff is worth it, you can buy me a beer in return.
 *
 * Dan Moschuk
 *
 * $FreeBSD: src/sys/libkern/arc4random.c,v 1.9 2001/08/30 12:30:58 bde Exp $
 */

#ifdef _KERNEL
#include "rnd.h"
#else
#define NRND 0
#endif

#include <sys/types.h>
#include <sys/time.h>
#include <sys/param.h>
#ifdef _KERNEL
#include <sys/kernel.h>
#endif
#include <sys/systm.h>

#include <lib/libkern/libkern.h>

#if NRND > 0
#include <dev/disk/rnd/rnd.h>
#endif

#define	ARC4_MAXRUNS 		16384
#define	ARC4_RESEED_SECONDS 300
#define	ARC4_KEYBYTES 		32 /* 256 bit key */

struct arc4_data {
	uint8_t			arc4_i;
	uint8_t			arc4_j;
	int				arc4_numruns;
	struct timeval 	arc4_tv_nextreseed;
	uint8_t			arc4_sbox[256];
};

static struct arc4_data arc4_ctx;
static int arc4_initialized = 0;
#ifndef _KERNEL
extern struct timeval mono_time;
#endif

static inline u_int8_t arc4_randbyte(struct arc4_data *);

static __inline void
arc4_swap(u_int8_t *a, u_int8_t *b)
{
	u_int8_t c;

	c = *a;
	*a = *b;
	*b = c;
}	

/*
 * Stir our S-box.
 */
static void
arc4_randrekey(struct arc4_data *ctx)
{
	u_int8_t key[256];
	static int cur_keybytes;
	int n, byteswanted;
#if NRND > 0
	int r;
#endif

	if(!arc4_initialized) {
		/* The first time through, we must take what we can get */
		byteswanted = 0;
	} else {
		/* Don't rekey with less entropy than we already have */
		byteswanted = cur_keybytes;
	}

#if NRND > 0	/* XXX without rnd, we will key from the stack, ouch! */
	r = rnd_extract_data(key, ARC4_KEYBYTES);
	if (r < ARC4_KEYBYTES) {
		if (r >= byteswanted) {
			(void)rnd_extract_data(key + r, ARC4_KEYBYTES - r);
		} else {
			/* don't replace a good key with a bad one! */
			ctx->arc4_tv_nextreseed = mono_time;
			ctx->arc4_tv_nextreseed.tv_sec += ARC4_RESEED_SECONDS;
			ctx->arc4_numruns = 0;
			/* we should just ask rnd(4) to rekey us when
			   it can, but for now, we'll just try later. */
			return;
		}
	}

	cur_keybytes = r;

	for (n = ARC4_KEYBYTES; n < sizeof(key); n++) {
		key[n] = key[n % ARC4_KEYBYTES];
	}
#endif
	for (n = 0; n < 256; n++) {
		ctx->arc4_j = (ctx->arc4_j + ctx->arc4_sbox[n] + key[n]) % 256;
		arc4_swap(&ctx->arc4_sbox[n], &ctx->arc4_sbox[ctx->arc4_j]);
	}

	/*
	 * Throw away the first N words of output, as suggested in the
	 * paper "Weaknesses in the Key Scheduling Algorithm of RC4"
	 * by Fluher, Mantin, and Shamir.  (N = 256 in our case.)
	 */
	for (n = 0; n < 768 * 4; n++) {
		arc4_randbyte(ctx);
	}
	
	/* Reset for next reseed cycle. */
	ctx->arc4_tv_nextreseed = mono_time;
	ctx->arc4_tv_nextreseed.tv_sec += ARC4_RESEED_SECONDS;
	ctx->arc4_numruns = 0;
}

/*
 * Initialize our S-box to its beginning defaults.
 */
static void
arc4_init(struct arc4_data *ctx)
{
	int n;

	ctx->arc4_i = ctx->arc4_j = 0;
	for (n = 0; n < 256; n++) {
		ctx->arc4_sbox[n] = (u_int8_t) n;
	}

	arc4_randrekey(ctx);
	
	for (n = 0; n < 768 * 4; n++) {
		arc4_randbyte(ctx);
	}
	arc4_initialized = 1;
}

/*
 * Generate a random byte.
 */
static __inline u_int8_t
arc4_randbyte(struct arc4_data *ctx)
{
	u_int8_t arc4_t;

	ctx->arc4_i = (ctx->arc4_i + 1) % 256;
	ctx->arc4_j = (ctx->arc4_j + ctx->arc4_sbox[ctx->arc4_i]) % 256;

	arc4_swap(&ctx->arc4_sbox[ctx->arc4_i], &ctx->arc4_sbox[ctx->arc4_j]);

	arc4_t = (ctx->arc4_sbox[ctx->arc4_i] + ctx->arc4_sbox[ctx->arc4_j]) % 256;
	return (ctx->arc4_sbox[arc4_t]);
}

u_int32_t
arc4random(void)
{
	struct arc4_data *ctx;
	u_int32_t ret;
	int i;

	ctx = &arc4_ctx;

	/* Initialize array if needed. */
	if (!arc4_initialized) {
		arc4_init(ctx);
	}

	if ((++ctx->arc4_numruns > ARC4_MAXRUNS) || (mono_time.tv_sec > ctx->arc4_tv_nextreseed.tv_sec)) {
		arc4_randrekey(ctx);
	}
	ret = arc4_randbyte(ctx);
	ret |= arc4_randbyte(ctx) << 8;
	ret |= arc4_randbyte(ctx) << 16;
	ret |= arc4_randbyte(ctx) << 24;
	return (ret);
}

u_int64_t
arc4random64(void)
{
	struct arc4_data *ctx;
	u_int64_t ret;
	int i;

	ctx = &arc4_ctx;

	/* Initialize array if needed. */
	if (!arc4_initialized) {
		arc4_init(ctx);
	}

	if ((++ctx->arc4_numruns > ARC4_MAXRUNS) || (mono_time.tv_sec > ctx->arc4_tv_nextreseed.tv_sec)) {
		arc4_randrekey(ctx);
	}
	
	ret = arc4_randbyte(ctx);
	ret |= arc4_randbyte(ctx) << 8;
	ret |= arc4_randbyte(ctx) << 16;
	ret |= arc4_randbyte(ctx) << 24;
	ret |= (u_int64_t)arc4_randbyte(ctx) << 32;
	ret |= (u_int64_t)arc4_randbyte(ctx) << 40;
	ret |= (u_int64_t)arc4_randbyte(ctx) << 48;
	ret |= (u_int64_t)arc4_randbyte(ctx) << 56;
	return (ret);
}

void
arc4randbytes(void *p, size_t len)
{
	struct arc4_data *ctx;
	u_int8_t *buf;
	size_t i;

	ctx = &arc4_ctx;
	buf = (u_int8_t *)p;

	for (i = 0; i < len; buf[i] = arc4_randbyte(ctx), i++);
		ctx->arc4_numruns += len / sizeof(u_int32_t);
	if ((ctx->arc4_numruns > ARC4_MAXRUNS) || (mono_time.tv_sec > ctx->arc4_tv_nextreseed.tv_sec)) {
		arc4_randrekey(ctx);
	}
}
