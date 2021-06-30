/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)gtty.c	5.2 (Berkeley) 3/9/86";
#endif LIBC_SCCS and not lint

/*
 * Writearound to old gtty system call.
 */

#include <sgtty.h>

gtty(fd, ap)
	int fd;
	struct sgtty *ap;
{
	return(ioctl(fd, TIOCGETP, ap));
}
