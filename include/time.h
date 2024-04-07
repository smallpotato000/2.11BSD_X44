/*
 * Copyright (c) 1983, 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)time.h	1.2 (Berkeley) 3/4/87
 */

#ifndef _TIME_H_
#define	_TIME_H_

#include <sys/cdefs.h>
#include <machine/ansi.h>

#include <sys/null.h>

#ifdef	_BSD_CLOCK_T_
typedef	_BSD_CLOCK_T_	clock_t;
#undef	_BSD_CLOCK_T_
#endif

#ifdef	_BSD_TIME_T_
typedef	_BSD_TIME_T_	time_t;
#undef	_BSD_TIME_T_
#endif

#ifdef	_BSD_SIZE_T_
typedef	_BSD_SIZE_T_	size_t;
#undef	_BSD_SIZE_T_
#endif

#ifdef	_BSD_CLOCKID_T_
typedef	_BSD_CLOCKID_T_	clockid_t;
#undef	_BSD_CLOCKID_T_
#endif

#ifdef	_BSD_TIMER_T_
typedef	_BSD_TIMER_T_	timer_t;
#undef	_BSD_TIMER_T_
#endif

#define CLOCKS_PER_SEC	100

/*
 * Structure returned by gmtime and localtime calls (see ctime(3)).
 */
struct tm {
	int		tm_sec;		/* seconds after the minute [0-60] */
	int		tm_min;		/* minutes after the hour [0-59] */
	int		tm_hour;	/* hours since midnight [0-23] */
	int		tm_mday;	/* day of the month [1-31] */
	int		tm_mon;		/* months since January [0-11] */
	int		tm_year;	/* years since 1900 */
	int		tm_wday;	/* days since Sunday [0-6] */
	int		tm_yday;	/* days since January 1 [0-365] */
	int		tm_isdst;	/* Daylight Savings Time flag */
	long	tm_gmtoff;	/* offset from CUT in seconds */
	char	*tm_zone;	/* timezone abbreviation */
};

#include <machine/limits.h>	/* Include file containing CLK_TCK. */

__BEGIN_DECLS
char 		*asctime(const struct tm *);
clock_t 	clock(void);
char 		*ctime(const time_t *);
double 		difftime(time_t, time_t);
struct tm 	*gmtime(const time_t *);
struct tm 	*localtime(const time_t *);
time_t 		mktime(struct tm *);
size_t 		strftime(char *, size_t, const char *, const struct tm *);
time_t 		time(time_t *);

#ifndef _ANSI_SOURCE
void 		tzset(void);
#endif /* not ANSI */

#if !defined(_ANSI_SOURCE) && !defined(_POSIX_SOURCE)
char 		*timezone(int, int);
void 		tzsetwall(void);
#endif /* neither ANSI nor POSIX */

/*
 * CLK_TCK uses libc's internal __sysconf() to retrieve the machine's
 * HZ. The value of _SC_CLK_TCK is 39 -- we hard code it so we do not
 * need to include unistd.h
 */
long 		__sysconf(int);
__END_DECLS

#endif /* !_TIME_H_ */
