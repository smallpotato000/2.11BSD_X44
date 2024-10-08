/* 	$NetBSD: wsfont.h,v 1.17 2003/02/09 10:29:38 jdolecek Exp $	*/

/*-
 * Copyright (c) 1999, 2000, 2001, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _WSFONT_H_
#define _WSFONT_H_ 1

/*
 * Example:
 *
 *	struct wsdisplay_font *font;
 *	int cookie;
 *
 *	cookie = wsfont_find(NULL, 8, 16, 0, WSDISPLAY_FONTORDER_L2R,
 *          WSDISPLAY_FONTORDER_R2L);
 *      if (cookie <= 0)
 *		panic("unable to get 8x16 font");
 *
 *	if (wsfont_lock(cookie, &font))
 *		panic("unable to lock font");
 *
 *	... do stuff ...
 *
 *	wsfont_unlock(cookie);
 */

struct wsdisplay_font;

void	wsfont_init(void);
int	wsfont_matches(struct wsdisplay_font *, const char *, int, int, int);
int	wsfont_find(const char *, int, int, int, int, int);

void	wsfont_walk(void (*)(struct wsdisplay_font *, void *, int), void *);

int	wsfont_add(struct wsdisplay_font *, int);
int	wsfont_remove(int);
void	wsfont_enum(void (*)(const char *, int, int, int));
int	wsfont_lock(int, struct wsdisplay_font **);
int	wsfont_unlock(int);
int	wsfont_map_unichar(struct wsdisplay_font *, int);

#endif	/* !_WSFONT_H_ */
