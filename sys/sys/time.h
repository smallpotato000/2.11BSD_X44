/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)time.h	1.3 (2.11BSD) 2000/4/21
 */

#ifndef	_SYS_TIME_H_
#define	_SYS_TIME_H_

/*
 * Structure returned by gettimeofday(2) system call,
 * and used in other calls.
 */
struct timeval {
	long	tv_sec;		/* seconds */
	long	tv_usec;	/* and microseconds */
};

/*
 * Structure defined by POSIX.4 to be like a timeval but with nanoseconds
 * instead of microseconds.  Silly on a PDP-11 but keeping the names the
 * same makes life simpler than changing the names.
*/
struct timespec {
	long	tv_sec;		/* seconds */
	long   	tv_nsec;	/* and nanoseconds */
};

#define	TIMEVAL_TO_TIMESPEC(tv, ts) {					\
	(ts)->ts_sec = (tv)->tv_sec;						\
	(ts)->ts_nsec = (tv)->tv_usec * 1000;				\
}
#define	TIMESPEC_TO_TIMEVAL(tv, ts) {					\
	(tv)->tv_sec = (ts)->ts_sec;						\
	(tv)->tv_usec = (ts)->ts_nsec / 1000;				\
}

struct timezone {
	int	tz_minuteswest;	/* minutes west of Greenwich */
	int	tz_dsttime;		/* type of dst correction */
};

#define	DST_NONE	0	/* not on dst */
#define	DST_USA		1	/* USA style dst */
#define	DST_AUST	2	/* Australian style dst */
#define	DST_WET		3	/* Western European dst */
#define	DST_MET		4	/* Middle European dst */
#define	DST_EET		5	/* Eastern European dst */
#define	DST_CAN		6	/* Canada */

/*
 * Operations on timevals.
 *
 * NB: timercmp does not work for >= or <=.
 */
#define	timerisset(tvp)		((tvp)->tv_sec || (tvp)->tv_usec)
#define	timercmp(tvp, uvp, cmp)	\
	((tvp)->tv_sec cmp (uvp)->tv_sec || \
	 (tvp)->tv_sec == (uvp)->tv_sec && (tvp)->tv_usec cmp (uvp)->tv_usec)
#define	timerclear(tvp)		(tvp)->tv_sec = (tvp)->tv_usec = 0

/*
 * Names of the interval timers, and structure
 * defining a timer setting.
 */
#define	ITIMER_REAL		0
#define	ITIMER_VIRTUAL	1
#define	ITIMER_PROF		2

struct	k_itimerval {
	long	it_interval;			/* timer interval */
	long	it_value;				/* current value */
};

struct	itimerval {
	struct	timeval it_interval;	/* timer interval */
	struct	timeval it_value;		/* current value */
};


/*
 * Getkerninfo clock information structure
 */
struct clockinfo {
	int	hz;		/* clock frequency */
	int	tick;		/* micro-seconds per hz tick */
	int	stathz;		/* statistics clock frequency */
	int	profhz;		/* profiling clock frequency */
};

#ifndef KERNEL
#include <time.h>
#endif

#ifdef KERNEL
int		itimerdecr __P((struct itimerval *itp,int usec));
int		itimerfix __P((struct timeval *));
void	microtime __P((struct timeval *));
void	timevaladd __P((struct timeval *, struct timeval *));
void	timevalfix __P((struct timeval *));
void	timevalsub __P((struct timeval *, struct timeval *));
#endif /* !KERNEL */

#include <sys/cdefs.h>

__BEGIN_DECLS
int	adjtime __P((const struct timeval *, struct timeval *));
int	getitimer __P((int, struct itimerval *));
int	gettimeofday __P((struct timeval *, struct timezone *));
int	setitimer __P((int, const struct itimerval *, struct itimerval *));
int	settimeofday __P((const struct timeval *, const struct timezone *));
int	utimes __P((const char *, const struct timeval *));
__END_DECLS




#endif	/* !_SYS_TIME_H_ */
