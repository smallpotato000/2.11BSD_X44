/*	$NetBSD: reallocarray.c,v 1.11 2021/02/26 19:25:12 christos Exp $	*/
/*	$OpenBSD: reallocarray.c,v 1.1 2014/05/08 21:43:49 deraadt Exp $	*/

/*-
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif /* HAVE_NBTOOL_CONFIG_H */

#include <sys/cdefs.h>
__RCSID("$NetBSD: reallocarray.c,v 1.11 2021/02/26 19:25:12 christos Exp $");

#include "namespace.h"

#define _OPENBSD_SOURCE
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#if !HAVE_REALLOCARRAY || !HAVE_DECL_REALLOCARRAY
void *
reallocarray(void *optr, size_t nmemb, size_t size)
{
	int e;

	if (nmemb == 0 || size == 0)
		return realloc(optr, 0);

	e = reallocarr(&optr, nmemb, size);
	if (e == 0)
		return optr;
	errno = e;
	return NULL;
}
#endif
