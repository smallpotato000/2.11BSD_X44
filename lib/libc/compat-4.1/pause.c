/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)pause.c	5.2.1 (2.11BSD) 1997/9/9";
#endif
#endif /* LIBC_SCCS and not lint */

#include <signal.h>

#include "compat_41.h"

/*
 * Backwards compatible pause.
 */

void
#if __STDC__
pause(void)
#else
pause()
#endif
{
	sigset_t set;

	sigemptyset(&set);
	sigsuspend(&set);
}
