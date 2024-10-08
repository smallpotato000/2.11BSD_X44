/*	$NetBSD: uvm_swap.h,v 1.8 2003/08/11 16:33:31 pk Exp $	*/

/*
 * Copyright (c) 1997 Matthew R. Green
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Id: uvm_swap.h,v 1.1.2.6 1997/12/15 05:39:31 mrg Exp
 */

#ifndef _VM_SWAP_H_
#define _VM_SWAP_H_

#include <sys/buf.h>
#include <sys/bufq.h>

#define	SWSLOT_BAD						(-1)

/*
 * swapdev: describes a single swap partition/file
 *
 * note the following should be true:
 * swd_inuse <= swd_nblks  [number of blocks in use is <= total blocks]
 * swd_nblks <= swd_mapsize [because mapsize includes miniroot+disklabel]
 */
struct swapdev {
	struct swdevt						*swd_swdevt;	/* Swap device table */
	int									swd_priority;	/* our priority */
	char								*swd_path;		/* saved pathname of device */
	int									swd_pathlen;	/* length of pathname */
	int									swd_npages;		/* #pages we can use */
	int									swd_npginuse;	/* #pages in use */
	int									swd_npgbad;		/* #pages bad */
	int									swd_drumoffset;	/* page0 offset in drum */
	int									swd_drumsize;	/* #pages in drum */
	struct extent						*swd_ex;		/* extent for this swapdev */
	char								swd_exname[12];	/* name of extent above */
	CIRCLEQ_ENTRY(swapdev)				swd_next;		/* priority circleq */

	int									swd_bsize;		/* blocksize (bytes) */
	int									swd_maxactive;	/* max active i/o reqs */
	int									swd_active;		/* # of active i/o reqs */
	struct bufq_state					swd_tab;		/* buffer list */
	struct ucred						*swd_cred;		/* cred for file access */
};

/*
 * swap device priority entry; the list is kept sorted on `spi_priority'.
 */
struct swappri {
	int									spi_priority;   /* priority */
	CIRCLEQ_HEAD(spi_swapdev, swapdev)	spi_swapdev; 	/* tailq of swapdevs at this priority */
	LIST_ENTRY(swappri)					spi_swappri;    /* global list of pri's */
};

/*
 * swapbuf, swapbuffer plus async i/o info
 */
struct swapbuf {
	struct buf 							*sw_buf;		/* a buffer structure */
	SIMPLEQ_ENTRY(swapbuf) 				sw_sq;			/* free list pointer */
	TAILQ_HEAD(, buf)					sw_bswlist;		/* Head of swap I/O buffer headers free list. */
	int 								sw_nswbuf;
};

#define SWAP_ON				1		/* begin swapping on device */
#define SWAP_OFF			2		/* stop swapping on device */
#define SWAP_NSWAP			3		/* how many swap devices ? */
#define SWAP_CTL			5		/* change priority on device */
#define SWAP_STATS			6		/* get device info */
#define SWAP_DUMPDEV		7		/* use this device as dump device */
#define SWAP_GETDUMPDEV		8		/* use this device as dump device */

#ifdef _KERNEL
extern
simple_lock_data_t 	swap_data_lock;

extern
lock_data_t 		swap_syscall_lock;

int			vm_swap_alloc(struct swdevt *, int *, bool_t);
void			vm_swap_free(int, int);
void			vm_swap_markbad(int, int);
void			vm_swap_stats(int, struct swdevt *, int, register_t *);

/* swapdrum */
void			swapdrum_init(struct swdevt *);
void			swapdrum_strategy(struct swdevt *, struct buf *, struct vnode *);

/* swapbuf */
void			vm_swapbuf_init(struct buf *, struct proc *);
struct swapbuf 	*vm_swapbuf_get(struct buf *);

#endif /* _KERNEL */

#endif /* _VM_SWAP_H_ */
