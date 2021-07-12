/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)times.c	5.2 (Berkeley) 3/9/86";
#endif LIBC_SCCS and not lint

#include <sys/param.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/resource.h>

#include <assert.h>
#include <errno.h>
#include <time.h>

long scale60(struct timeval *tvp);

clock_t
times(tmsp)
	register struct tms *tmsp;
{
	struct rusage ru;
	struct timeval t;

	if (getrusage(RUSAGE_SELF, &ru) < 0)
		return (-1);
	tmsp->tms_utime = scale60(&ru.ru_utime);
	tmsp->tms_stime = scale60(&ru.ru_stime);
	if (getrusage(RUSAGE_CHILDREN, &ru) < 0)
		return (-1);
	tmsp->tms_cutime = scale60(&ru.ru_utime);
	tmsp->tms_cstime = scale60(&ru.ru_stime);
	if (gettimeofday(&t, (struct timezone *)0))
		return (-1);
	return (0);
}

static long
scale60(tvp)
	register struct timeval *tvp;
{
	return (tvp->tv_sec * 60 + tvp->tv_usec / 16667);
}
