/*	$NetBSD: pwd.c,v 1.11 1998/11/03 21:38:19 wsanchez Exp $	*/

/*
 * Copyright (c) 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1991, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)pwd.c	8.3 (Berkeley) 4/1/94";
#else
__RCSID("$NetBSD: pwd.c,v 1.11 1998/11/03 21:38:19 wsanchez Exp $");
#endif
#endif /* not lint */

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

char *getcwd_logical (char *, size_t);
void  usage (void);
int   main (int, char *[]);

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int ch;
	int lFlag=0;
	char *p = NULL;

	while ((ch = getopt(argc, argv, "LP")) != -1)
		switch (ch) {
		case 'L':
			lFlag=1;
			break;
		case 'P':
			lFlag=0;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc != 0)
		usage();

	if (lFlag)
		p = getcwd_logical(NULL, 0);
	else
		p = getcwd(NULL, 0);

	if (p == NULL) err(1, "%s", "");

	(void)printf("%s\n", p);

	exit(0);
	/* NOTREACHED */
}

char *
getcwd_logical(pt, size)
        char *pt;
        size_t size;
{
        char *pwd;
        size_t pwdlen;
        dev_t dev;
        ino_t ino;
        struct stat s;

        /* Check $PWD -- if it's right, it's fast. */
        if ((pwd = getenv("PWD")) != NULL && pwd[0] == '/' && stat(pwd, &s) != -1) {
                dev = s.st_dev;
                ino = s.st_ino;
                if (stat(".", &s) != -1 && dev == s.st_dev && ino == s.st_ino) {
                        pwdlen = strlen(pwd);
			if (pt) {
				if (!size) {
                                        errno = EINVAL;
                                        return (NULL);
				}
                                if (pwdlen + 1 > size) {
                                        errno = ERANGE;
                                        return (NULL);
                                }
                        } else if ((pt = malloc(pwdlen + 1)) == NULL)
                                return (NULL);
                        memmove(pt, pwd, pwdlen);
                        pt[pwdlen] = '\0';
                        return (pt);
                }
        }

        return (NULL);
}

void
usage()
{

	(void)fprintf(stderr, "usage: pwd\n");
	exit(1);
	/* NOTREACHED */
}
