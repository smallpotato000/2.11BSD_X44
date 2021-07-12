/*-
 * Copyright (c) 1998 Michael Smith <msmith@freebsd.org>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/boot/i386/libi386/biossmap.c,v 1.2 2003/08/25 23:28:31 obrien Exp $
 */

#include <sys/cdefs.h>

/*
 * Obtain memory configuration information from the BIOS
 */
#include <sys/param.h>

#include <machine/bios.h>
#include <machine/psl.h>

#include <lib/libsa/stand.h>
#include "bootstrap.h"
#include "libi386.h"
#include "btxv86.h"

static struct bios_smap smap;

static struct bios_smap *smapbase;
static int smaplen;
static void	*heapbase;

#define	MODINFOMD_SMAP		0x1001

void
bios_getsmap(void)
{
	int n;

	heapbase = (void *)(((uintptr_t)base + 15) & ~15);
	n = 0;
	smaplen = 0;
	/* Count up segments in system memory map */
	v86.ebx = 0;
	do {
		v86.ctl = V86_FLAGS;
		v86.addr = 0x15; /* int 0x15 function 0xe820*/
		v86.eax = 0xe820;
		v86.ecx = sizeof(struct bios_smap);
		v86.edx = SMAP_SIG;
		v86.es = VTOPSEG(&smap);
		v86.edi = VTOPOFF(&smap);
		v86int();
		if ((v86.efl & PSL_C) || (v86.eax != SMAP_SIG))
			break;
		n++;
	} while (v86.ebx != 0);
	if (n == 0)
		return;
	n += 10; /* spare room */
	smapbase = malloc(n * sizeof(*smapbase));

	/* Save system memory map */
	v86.ebx = 0;
	do {
		v86.ctl = V86_FLAGS;
		v86.addr = 0x15; /* int 0x15 function 0xe820*/
		v86.eax = 0xe820;
		v86.ecx = sizeof(struct bios_smap);
		v86.edx = SMAP_SIG;
		v86.es = VTOPSEG(&smap);
		v86.edi = VTOPOFF(&smap);
		v86int();

		/*
		 * Our heap is now in high memory and must be removed from
		 * the smap so the kernel does not blow away passed-in
		 * arguments, smap, kenv, etc.
		 *
		 * This wastes a little memory.
		 */
		if (smap.type == SMAP_TYPE_MEMORY && smap.base + smap.length > heapbase
				&& smap.base < memtop) {
			if (smap.base <= heapbase) {
				if (heapbase - smap.base) {
					smapbase[smaplen] = smap;
					smapbase[smaplen].length = heapbase - smap.base;
					++smaplen;
				}
			}
			if (smap.base + smap.length >= memtop) {
				if (smap.base + smap.length - memtop) {
					smapbase[smaplen] = smap;
					smapbase[smaplen].base = memtop;
					smapbase[smaplen].length = smap.base + smap.length - memtop;
					++smaplen;
				}
			}
		} else {
			smapbase[smaplen] = smap;
			++smaplen;
		}
		if ((v86.efl & PSL_C) || (v86.eax != SMAP_SIG))
			break;
	} while (v86.ebx != 0 && smaplen < n);
}

void
bios_addsmapdata(struct preloaded_file *kfp)
{
	int len;

	if (smapbase == NULL || smaplen == 0) {
		return;
	}
	len = smaplen * sizeof(*smapbase);
	file_addmetadata(kfp, MODINFOMD_SMAP, len, smapbase);
	/* Temporary compatibility with older development kernels */
	file_addmetadata(kfp, 0x0009, len, smapbase);
}
