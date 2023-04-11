/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and William Jolitz of UUNET Technologies Inc.
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
 *	@(#)pmap.c	8.1 (Berkeley) 6/11/93
 */
#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/malloc.h>
#include <sys/msgbuf.h>
#include <sys/memrange.h>
#include <sys/sysctl.h>
#include <sys/cputopo.h>
#include <sys/queue.h>
#include <sys/lock.h>

#include <vm/include/vm.h>
#include <vm/include/vm_kern.h>
#include <vm/include/vm_page.h>

#include <i386/include/pte.h>

#include <amd64/include/param.h>
#include <amd64/include/pmap.h>

/*
 * Get PDEs and PTEs for user/kernel address space
 */
#define ptlvlshift(lvl)         ((((lvl) * NPDLVL_SHIFT) + PGSHIFT) - NPDLVL_SHIFT)
#define	pmap_pde(m, v, lvl)		(&((m)->pm_pdir[((vm_offset_t)(v) >> ptlvlshift(lvl))]))

static pt_entry_t 	*pmap_pte_to_va(pmap_t, vm_offset_t);
static pt_entry_t 	*pmap_apte_to_va(pmap_t, vm_offset_t);
static pt_entry_t 	*pmap_pte(pmap_t, vm_offset_t);

static pt_entry_t *
pmap_pte(pmap, va)
	register pmap_t pmap;
	vm_offset_t va;
{
	pd_entry_t newpf;
	pd_entry_t *pde, *apde;

	pde = pmap_pte_to_va(pmap, va);
	apde = pmap_apte_to_va(pmap, va);

	if (pde != 0) {
		if (pmap->pm_pdir == pde || pmap == kernel_pmap) {
			return (pde);
		}
	}
	if (apde != 0) {
		if (pmap->pm_pdir != apde) {
			tlbflush();
		}
		newpf = *pde & PG_FRAME;
		if (newpf) {
			//pmap_invalidate_page(kernel_pmap, PADDR2);
		}
		return (apde);
	}
	return (0);
}

vm_offset_t
pmap_extract(pmap, va)
	register pmap_t	pmap;
	vm_offset_t va;
{
	register vm_offset_t pa;
	int i;

	pa = 0;
	if (pmap && pmap_pde_v(pmap_pte(pmap, va))) {
		pa = *(int *)pmap_pte(pmap, va);
	}
	if (pa) {
		pa = (pa & PG_FRAME) | (va & ~PG_FRAME);
	}
	return (pa);
}

static pt_entry_t *
pmap_pte_to_va(pmap, va)
	register pmap_t pmap;
	vm_offset_t va;
{
    pd_entry_t *pde;
    int lvl;

    for (lvl = 0; lvl < NPDLVL; lvl++) {
    	pde = pmap_pde(pmap, va, lvl);
    	if (pde && pmap_pde_v(pde)) {
    		if (pde == PTD_PDE) {
    			return ((pt_entry_t *) vtopte(va));
    		}
    	}
    }
	return (0);
}

static pt_entry_t *
pmap_apte_to_va(pmap, va)
	register pmap_t pmap;
	vm_offset_t va;
{
	pd_entry_t *apde;
	int lvl;

	for (lvl = 0; lvl < NPDLVL; lvl++) {
		apde = pmap_pde(pmap, va, lvl);
		if (apde && pmap_pde_v(apde)) {
			if (apde == APTD_PDE) {
				return ((pt_entry_t *)avtopte(va));
			}
		}
	}
	return (0);
}
