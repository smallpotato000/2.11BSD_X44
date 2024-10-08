/*	$NetBSD: fsutil.h,v 1.8 2003/03/28 08:12:38 perseant Exp $	*/

/*
 * Copyright (c) 1996 Christos Zoulas.  All rights reserved.
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
 *	This product includes software developed by Christos Zoulas.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdarg.h>

void errexit(const char *, ...)
    __attribute__((__noreturn__,__format__(__printf__,1,2)));  
void pfatal(const char *, ...)
    __attribute__((__format__(__printf__,1,2)));  
void pwarn(const char *, ...)
    __attribute__((__format__(__printf__,1,2)));  
void panic(const char *, ...)
    __attribute__((__noreturn__,__format__(__printf__,1,2)));  
void vmsg(int, const char *, va_list)
     __attribute((__format__(__printf__,2,0)));
const char *rawname(const char *);
const char *unrawname(const char *);
const char *blockcheck(const char *);
const char *cdevname(void);
void setcdevname(const char *, int);
int  hotroot(void);
void *emalloc(size_t);
void *erealloc(void *, size_t);
char *estrdup(const char *);

#define CHECK_PREEN	1
#define	CHECK_VERBOSE	2
#define	CHECK_DEBUG	4
#define	CHECK_FORCE	8

struct fstab;
int checkfstab(int, int, void *(*)(struct fstab *), 
    int (*) (const char *, const char *, const char *, void *, pid_t *));
