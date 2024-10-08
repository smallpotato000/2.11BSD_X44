/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)callout.h	1.1 (2.10BSD Berkeley) 12/1/86
 */

/*	$NetBSD: callout.h,v 1.20 2003/10/30 04:32:56 thorpej Exp $	*/
/*-
 * Copyright (c) 2000, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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

/*
 * Copyright (c) 2000-2001 Artur Grabowski <art@openbsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL  DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * The callout structure is for
 * a routine arranging
 * to be called by the clock interrupt
 * (clock.c) with a specified argument,
 * in a specified amount of time.
 * Used, for example, to time tab
 * delays on typewriters.
 *
 * The c_time field is stored in terms of ticks.  Therefore, no callout
 * may be scheduled past around 8 minutes on a 60 HZ machine.  This is
 * good as it avoids long operations on clock ticks.  If you are ever
 * forced to use a long, you might as well start doing the real-time
 * timer as a timeout like 4.3BSD.
 */

#ifndef _SYS_CALLOUT_H_
#define	_SYS_CALLOUT_H_

#include <sys/types.h>

struct callout;
struct callout_circq {
	/* next element */
	union {
		struct callout			*elem;
		struct callout_circq	*list;
	} cq_next;
	/* previous element */
	union {
		struct callout			*elem;
		struct callout_circq	*list;
	} cq_prev;
};

#define	cq_next_e				cq_next.elem
#define	cq_prev_e				cq_prev.elem
#define	cq_next_l				cq_next.list
#define	cq_prev_l				cq_prev.list

struct callout {
	struct callout_circq 		c_list;				/* linkage on queue */
	int							c_time;				/* incremental time */
	void						*c_arg;				/* argument to routine */
	void						(*c_func)(void *);	/* routine */
	int							c_flags;			/* state of this entry */
};

#define	CALLOUT_PENDING			0x0002				/* callout is on the queue */
#define	CALLOUT_FIRED			0x0004				/* callout has fired */
#define	CALLOUT_INVOKING		0x0008				/* callout function is being invoked */

#define	CALLOUT_INITIALIZER_SETFUNC(func, arg)	\
	{ {{NULL}, {NULL}}, func, arg, 0, 0 }

#define	CALLOUT_INITIALIZER		CALLOUT_INITIALIZER_SETFUNC(NULL, NULL)

#define CIRCQ_FIRST(list)		((list)->cq_next_e)
#define CIRCQ_NEXT(elem)		((elem)->cq_next_e)
#define CIRCQ_LAST(elem, list)	((elem)->cq_next_l == (list))
#define CIRCQ_EMPTY(list)		((list)->cq_next_l == (list))

#ifdef _KERNEL
void	callout_startup(void);
void	callout_init(struct callout *);
void	callout_setfunc(struct callout *, void (*)(void *), void *);
void	callout_reset(struct callout *, int, void (*)(void *), void *);
void	callout_schedule(struct callout *, int);
void	callout_stop(struct callout *);
int		callout_hardclock(int);
void	callout_softclock(void);
void	callout_setfunc(struct callout *, void (*)(void *), void *);

#define	callout_pending(c)	((c)->c_flags & CALLOUT_PENDING)
#define	callout_expired(c)	((c)->c_flags & CALLOUT_FIRED)
#define	callout_invoking(c)	((c)->c_flags & CALLOUT_INVOKING)
#define	callout_ack(c)		((c)->c_flags &= ~CALLOUT_INVOKING)

/* kern_clock.c */
extern struct callout *callfree, *calltodo;
#endif
#endif /* _SYS_CALLOUT_H_ */
