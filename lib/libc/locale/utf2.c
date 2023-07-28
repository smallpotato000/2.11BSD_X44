/*-
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Paul Borman at Krystal Technologies.
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
#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)utf2.c	8.1 (Berkeley) 6/4/93";
#endif /* LIBC_SCCS and not lint */

#include <errno.h>
#include <rune.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <citrus/citrus_ctype.h>
#include <citrus/citrus_stdenc.h>

typedef _Encoding_Info				_UTF2EncodingInfo;
typedef _Encoding_TypeInfo 			_UTF2CTypeInfo;
typedef _Encoding_State				_UTF2State;

#define _FUNCNAME(m)				_UTF2_##m
#define _ENCODING_MB_CUR_MAX(_ei_)	3

rune_t	_UTF2_sgetrune(const char *, size_t, char const **);
int		_UTF2_sputrune(rune_t, char *, size_t, char **);
int		_UTF2_sgetmbrune(_UTF2EncodingInfo *, wchar_t *, const char **, size_t, _UTF2State *, size_t *);
int 	_UTF2_sputmbrune(_UTF2EncodingInfo *, char *, wchar_t, _UTF2State *, size_t *);
int		_UTF2_sgetcsrune(_UTF2EncodingInfo * __restrict, wchar_t * __restrict, _csid_t, _index_t);
int		_UTF2_sputcsrune(_UTF2EncodingInfo * __restrict, _csid_t * __restrict, _index_t * __restrict, wchar_t);

static int _UTF2_encoding_module_init(_UTF2EncodingInfo * __restrict, const void * __restrict, size_t);

_RuneOps _utf2_runeops = {
		.ro_sgetrune 	=  	_UTF2_sgetrune,
		.ro_sgetrune 	=  	_UTF2_sgetrune,
		.ro_sgetmbrune 	=  	_UTF2_sgetmbrune,
		.ro_sgetmbrune 	=  	_UTF2_sgetmbrune,
		.ro_sgetcsrune  =	_UTF2_sgetcsrune,
		.ro_sputcsrune	= 	_UTF2_sputcsrune,
};

static _utf_count[16] = {
	1, 1, 1, 1, 1, 1, 1, 1,
	0, 0, 0, 0, 2, 2, 3, 0,
};

int
_UTF2_init(rl)
	_RuneLocale *rl;
{
	_UTF2EncodingInfo 	*info;
	int ret;

	rl->ops = &_utf2_runeops;
	ret = _citrus_ctype_open(&rl, rl->variable, rl->variable_len, _UTF2_encoding_module_init);
	if (ret != 0) {
		return (ret);
	}
	ret = _citrus_stdenc_open(&rl, rl->variable, rl->variable_len, _UTF2_encoding_module_init);
	if (ret != 0) {
		return (ret);
	}
/*
	ret = _citrus_ctype_init(&rl);
	if (ret != 0) {
		return (ret);
	}
	ret = _citrus_stdenc_init(info);
	if (ret != 0) {
		return (ret);
	}
*/
	_CurrentRuneLocale = rl;

	return (0);
}

int
_UTF2_sgetmbrune(_UTF2EncodingInfo *ei, wchar_t *pwc, const char **s, size_t n, _UTF2State *psenc, size_t *nresult)
{
	wchar_t wchar;
	const char *s0;

	_DIAGASSERT(nresult != 0);
	_DIAGASSERT(ei != NULL);
	_DIAGASSERT(psenc != NULL);
	_DIAGASSERT(s != NULL);

	s0 = *s;

	if (s0 == NULL) {
		_citrus_ctype_init_state(ei, psenc);
		*nresult = 0; /* state independent */
		return 0;
	}

	wchar = (wchar_t) _UTF2_sgetrune(s, n, nresult);

	if (pwc != NULL) {
		*pwc = wchar;
	}
	*nresult = (wchar == 0) ? 0 : s0 - *s;
	*s = s0;

	return (0);
}

int
_UTF2_sputmbrune(_UTF2EncodingInfo  *ei, char *s, size_t n, wchar_t wc, _UTF2State *psenc, size_t *nresult)
{
	_DIAGASSERT(ei != NULL);
	_DIAGASSERT(nresult != 0);
	_DIAGASSERT(s != NULL);

	return (_UTF2_sputrune(wc, s, n, nresult));
}

rune_t
_UTF2_sgetrune(string, n, result)
	const char *string;
	size_t n;
	char const **result;
{
	int c;

	if (n < 1 || (c = _utf_count[(*string >> 4) & 0xf]) > n) {
		if (result)
			*result = string;
		return (_INVALID_RUNE);
	}
	switch (c) {
	case 1:
		if (result)
			*result = string + 1;
		return (*string & 0xff);
	case 2:
		if ((string[1] & 0xC0) != 0x80)
			goto encoding_error;
		if (result)
			*result = string + 2;
		return (((string[0] & 0x1F) << 6) | (string[1] & 0x3F));
	case 3:
		if ((string[1] & 0xC0) != 0x80 || (string[2] & 0xC0) != 0x80)
			goto encoding_error;
		if (result)
			*result = string + 3;
		return (((string[0] & 0x1F) << 12) | ((string[1] & 0x3F) << 6)
				| (string[2] & 0x3F));
	default:
encoding_error:
		if (result)
			*result = string + 1;
		return (_INVALID_RUNE);
	}
}

int
_UTF2_sputrune(c, string, n, result)
	rune_t c;
	char *string, **result;
	size_t n;
{
	if (c & 0xF800) {
		if (n >= 3) {
			if (string) {
				string[0] = 0xE0 | ((c >> 12) & 0x0F);
				string[1] = 0x80 | ((c >> 6) & 0x3F);
				string[2] = 0x80 | ((c) & 0x3F);
			}
			if (result)
				*result = string + 3;
		} else if (result)
			*result = NULL;

		return (3);
	} else if (c & 0x0780) {
		if (n >= 2) {
			if (string) {
				string[0] = 0xC0 | ((c >> 6) & 0x1F);
				string[1] = 0x80 | ((c) & 0x3F);
			}
			if (result)
				*result = string + 2;
		} else if (result)
			*result = NULL;
		return (2);
	} else {
		if (n >= 1) {
			if (string)
				string[0] = c;
			if (result)
				*result = string + 1;
		} else if (result)
			*result = NULL;
		return (1);
	}
}

int
_UTF2_sgetcsrune(_UTF2EncodingInfo * __restrict ei, wchar_t * __restrict wc, _csid_t csid, _index_t idx)
{
	return (0);
}

int
_UTF2_sputcsrune(_UTF2EncodingInfo * __restrict ei, _csid_t * __restrict csid, _index_t * __restrict idx, wchar_t wc)
{
	return (0);
}

static int
_UTF2_encoding_module_init(_UTF2EncodingInfo * __restrict ei, const void * __restrict var, size_t lenvar)
{
	return (0);
}
