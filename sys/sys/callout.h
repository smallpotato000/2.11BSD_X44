/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)callout.h	1.1 (2.10BSD Berkeley) 12/1/86
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

struct callout_circq;
CIRCLEQ_HEAD(callout_circq, callout);
struct callout {
	CIRCLEQ_ENTRY(callout) 	c_list;
	int						c_time;				/* incremental time */
	caddr_t					c_arg;				/* argument to routine */
	int						(*c_func)(void *);	/* routine */
	int						c_flags;			/* state of this entry */

	struct	callout 		*c_next;
};

#define	CALLOUT_PENDING		0x0002	/* callout is on the queue */
#define	CALLOUT_FIRED		0x0004	/* callout has fired */
#define	CALLOUT_INVOKING	0x0008	/* callout function is being invoked */

#ifdef KERNEL
struct	callout *callfree, callout[], calltodo;
int	ncallout;
#endif
