/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)string.h	5.1 (Berkeley) 85/08/05
 */
/*	$NetBSD: string.h,v 1.8 1997/10/16 23:26:26 christos Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
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
 *
 *	@(#)string.h	5.10 (Berkeley) 3/9/91
 */

#ifndef _STRING_H_
#define	_STRING_H_
#include <machine/ansi.h>

#ifdef	_BSD_SIZE_T_
typedef	_BSD_SIZE_T_	size_t;
#undef	_BSD_SIZE_T_
#endif

#ifndef	NULL
#define	NULL	0
#endif

#include <sys/cdefs.h>
#include <strings.h>

/*
 * these next few are obsolete trash
 */

extern char *strcpyn();
extern char *strcatn();
extern int 	strcmpn();

/*
 * and the rest are Sys5 functions supported just so
 * Sys5 progs will compile easily.
 */

extern char *strchr();
extern char *strrchr();
extern char *strpbrk();
extern int 	strspn();
extern int 	strcspn();
extern char *strtok();

__BEGIN_DECLS
void	*memchr __P((const void *, int, size_t));
int	 	memcmp __P((const void *, const void *, size_t));
void	*memcpy __P((void *, const void *, size_t));
void	*memmove __P((void *, const void *, size_t));
void	*memset __P((void *, int, size_t));
char	*strcat __P((char *, const char *));
char	*strchr __P((const char *, int));
int	 	strcmp __P((const char *, const char *));
int	 	strcoll __P((const char *, const char *));
char	*strcpy __P((char *, const char *));
size_t	 strcspn __P((const char *, const char *));
char	*strerror __P((int));
size_t	 strlen __P((const char *));
char	*strncat __P((char *, const char *, size_t));
int	 	strncmp __P((const char *, const char *, size_t));
char	*strncpy __P((char *, const char *, size_t));
char	*strpbrk __P((const char *, const char *));
char	*strrchr __P((const char *, int));
size_t	 strspn __P((const char *, const char *));
char	*strstr __P((const char *, const char *));
char	*strtok __P((char *, const char *));
char	*strtok_r __P((char *, const char *, char **));
size_t	 strxfrm __P((char *, const char *, size_t));

/* Nonstandard routines */
#if !defined(_ANSI_SOURCE) && !defined(_POSIX_SOURCE)
int	 	bcmp (const void *, const void *, size_t);
void	bcopy (const void *, void *, size_t);
void	bzero (void *, size_t);
int	 	ffs (int);
char	*index (const char *, int);
void	*memccpy (void *, const void *, int, size_t);
char	*rindex (const char *, int);
int		strcasecmp (const char *, const char *);
char	*strdup (const char *);
int	 	strncasecmp (const char *, const char *, size_t);
char	*strsep (char **, const char *);
void	swab (const void *, void *, size_t);
#endif
__END_DECLS

#endif /* _STRING_H_ */
