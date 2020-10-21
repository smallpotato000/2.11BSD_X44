/*	$NetBSD: pool.h,v 1.14 1999/03/31 23:23:47 thorpej Exp $	*/

/*-
 * Copyright (c) 1997, 1998, 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg; by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#ifndef _SYS_POOL_H_
#define _SYS_POOL_H_

#ifdef _KERNEL
#define	__POOL_EXPOSE
#endif

#ifdef __POOL_EXPOSE
#include <sys/lock.h>
#include <sys/queue.h>
#include <sys/time.h>
#endif

#define PR_HASHTABSIZE		8

struct pool;
typedef struct pool *pool_handle_t;

#ifdef __POOL_EXPOSE
struct pool {
	TAILQ_ENTRY(pool) 				pr_poollist;
	TAILQ_HEAD(,pool_item_header) 	pr_pagelist;	/* Allocated pages */
	struct pool_item_header			*pr_curpage;
	unsigned int					pr_size;		/* Size of item */
	unsigned int					pr_align;		/* Requested alignment, must be 2^n */
	unsigned int					pr_itemoffset;	/* Align this offset in item */
	unsigned int					pr_minitems;	/* minimum # of items to keep */
	unsigned int					pr_minpages;	/* same in page units */
	unsigned int					pr_maxpages;	/* maximum # of pages to keep */
	unsigned int					pr_npages;		/* # of pages allocated */
	unsigned int					pr_pagesz;		/* page size, must be 2^n */
	unsigned long					pr_pagemask;	/* abbrev. of above */
	unsigned int					pr_pageshift;	/* shift corr. to above */
	unsigned int					pr_itemsperpage;/* # items that fit in a page */
	unsigned int					pr_slack;		/* unused space in a page */
	unsigned int					pr_nitems;		/* number of available items in pool */
	unsigned int					pr_nout;		/* # items currently allocated */
	unsigned int					pr_hardlimit;	/* hard limit to number of allocated items */
	void							*(*pr_alloc) (unsigned long, int, int);
	void							(*pr_free) (void *, unsigned long, int);
	int								pr_mtype;		/* memory allocator tag */
	const char						*pr_wchan;		/* tsleep(9) identifier */
	unsigned int					pr_flags;		/* r/w flags */
	unsigned int					pr_roflags;		/* r/o flags */

	/*
	 * `pr_slock' protects the pool's data structures when removing
	 * items from or returning items to the pool, or when reading
	 * or updating read/write fields in the pool descriptor.
	 *
	 * We assume back-end page allocators provide their own locking
	 * scheme.  They will be called with the pool descriptor _unlocked_,
	 * since the page allocators may block.
	 */
	struct simplelock				pr_slock;

	LIST_HEAD(,pool_item_header)	pr_hashtab[PR_HASHTABSIZE];/* Off-page page headers */

	int								pr_maxcolor;	/* Cache colouring */
	int								pr_curcolor;
	int								pr_phoffset;	/* Offset in page of page header */

	/*
	 * Warning message to be issued, and a per-time-delta rate cap,
	 * if the hard limit is reached.
	 */
	const char						*pr_hardlimit_warning;
	int								pr_hardlimit_ratecap;		/* in seconds */
	struct timeval					pr_hardlimit_warning_last;

	/*
	 * Instrumentation
	 */
	unsigned long					pr_nget;		/* # of successful requests */
	unsigned long					pr_nfail;		/* # of unsuccessful requests */
	unsigned long					pr_nput;		/* # of releases */
	unsigned long					pr_npagealloc;	/* # of pages allocated */
	unsigned long					pr_npagefree;	/* # of pages released */
	unsigned int					pr_hiwat;		/* max # of pages in pool */
	unsigned long					pr_nidle;		/* # of idle pages */

#ifdef POOL_DIAGNOSTIC
	struct pool_log					*pr_log;
	int								pr_curlogentry;
	int								pr_logsize;
#endif

#define PR_MALLOCOK					1
#define	PR_NOWAIT					0				/* for symmetry */
#define PR_WAITOK					2
#define PR_WANTED					4
#define PR_STATIC					8
#define PR_FREEHEADER				16
#define PR_URGENT					32
#define PR_PHINPAGE					64
#define PR_LOGGING					128
};
#endif /* __POOL_EXPOSE */

#ifdef _KERNEL
pool_handle_t	pool_create (size_t, u_int, u_int, int, const char *, size_t, void *(*)(unsigned long, int, int), void  (*)(void *, unsigned long, int), int);
void			pool_init (struct pool *, size_t, u_int, u_int, int, const char *, size_t, void *(*)(unsigned long, int, int), void(*)(void *, unsigned long, int), int);
void			pool_destroy (pool_handle_t);
#ifdef POOL_DIAGNOSTIC
void			*_pool_get (pool_handle_t, int, const char *, long);
void			_pool_put (pool_handle_t, void *, const char *, long);
#define			pool_get(h, f)	_pool_get((h), (f), __FILE__, __LINE__)
#define			pool_put(h, v)	_pool_put((h), (v), __FILE__, __LINE__)
#else
void			*pool_get (pool_handle_t, int);
void			pool_put (pool_handle_t, void *);
#endif
int				pool_prime (pool_handle_t, int, caddr_t);
void			pool_setlowat (pool_handle_t, int);
void			pool_sethiwat (pool_handle_t, int);
void			pool_sethardlimit (pool_handle_t, int, const char *, int);
void			pool_reclaim (pool_handle_t);
void			pool_drain (void *);
#if defined(POOL_DIAGNOSTIC) || defined(DEBUG)
void			pool_print (struct pool *, const char *);
int				pool_chk (struct pool *, char *);
#endif

/*
 * Alternate pool page allocator, provided for pools that know they
 * will never be accessed in interrupt context.
 */
void			*pool_page_alloc_nointr (unsigned long, int, int);
void			pool_page_free_nointr (void *, unsigned long, int);
#endif /* _KERNEL */
#endif /* _SYS_POOL_H_ */
