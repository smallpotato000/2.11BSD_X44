/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#if !HAVE_NBTOOL_CONFIG_H || !HAVE_MKSTEMP || !HAVE_MKDTEMP

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)mktemp.c	5.4 (Berkeley) 9/14/87";
#endif LIBC_SCCS and not lint

#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <ctype.h>

#define	YES	1
#define	NO	0

#ifdef __weak_alias
__weak_alias(mkdtemp,_mkdtemp)
__weak_alias(mkstemp,_mkstemp)
__weak_alias(mktemp,_mktemp)
#endif

int _gettemp(char *, int *, int);

char *
mkdtemp(as)
	char *as;
{
	_DIAGASSERT(as != NULL);

	return (_gettemp(as, (int *)NULL, 1) ? as : (char *)NULL);
}

char *
mkstemp(as)
	char *as;
{
	int	fd;

	_DIAGASSERT(as != NULL);

	return (_gettemp(as, &fd, 0) ? fd : -1);
}

char *
mktemp(as)
	char	*as;
{
	_DIAGASSERT(as != NULL);

	return(_gettemp(as, (int *)NULL, 0) ? as : (char *)NULL);
}

int
_gettemp(as, doopen, domkdir)
	char	*as;
	register int *doopen;
	int domkdir;
{
	extern int	errno;
	register char	*start, *trv;
	struct stat	sbuf;
	u_int	pid;

	_DIAGASSERT(as != NULL);

	pid = getpid();

	/* extra X's get set to 0's */
	for (trv = as; *trv; ++trv);
	while (*--trv == 'X') {
		*trv = (pid % 10) + '0';
		pid /= 10;
	}

	/*
	 * check for write permission on target directory; if you have
	 * six X's and you can't write the directory, this will run for
	 * a *very* long time.
	 */
	for (start = ++trv; trv > as && *trv != '/'; --trv);
	if (*trv == '/') {
		*trv = '\0';
		if (stat(as, &sbuf) || !(sbuf.st_mode & S_IFDIR)) {
			return(NO);
		}
		*trv = '/';
	} else if (stat(".", &sbuf) == -1) {
		return(NO);
	}

	for (;;) {
		if (doopen) {
			if ((*doopen = open(as, O_CREAT|O_EXCL|O_RDWR, 0600)) >= 0) {
				return(YES);
			}
			if (errno != EEXIST) {
				return(NO);
			}
		}  else if (domkdir) {
			if (mkdir(as, 0700) >= 0) {
				return(YES);
			}
			if (errno != EEXIST) {
				return(NO);
			}
		} else if (stat(as, &sbuf)) {
			return(errno == ENOENT ? YES : NO);
		}

		/* tricky little algorithm for backward compatibility */
		for (trv = start;;) {
			if (!*trv) {
				return(NO);
			}
			if (*trv == 'z') {
				*trv++ = 'a';
			} else {
				if (isdigit(*trv)) {
					*trv = 'a';
				} else {
					++*trv;
				}
				break;
			}
		}
	}
	/*NOTREACHED*/
}

#endif /* !HAVE_NBTOOL_CONFIG_H || !HAVE_MKSTEMP || !HAVE_MKDTEMP */
