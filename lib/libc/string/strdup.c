/*
 * Copyright (c) 1988 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)strdup.c	5.1 (Berkeley) 12/12/88";
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#ifdef __weak_alias
__weak_alias(strdup,_strdup)
#endif

char *
strdup(str)
	char *str;
{
	int len;
	char *copy;

	len = strlen(str) + 1;
	if (!(copy = malloc((u_int)len)))
		return((char *)NULL);
	bcopy(str, copy, len);
	return(copy);
}
