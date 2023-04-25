/*
 * The 3-Clause BSD License:
 * Copyright (c) 2023 Martin Kelly
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#include <devel/vm/hat.h>

/* Overlays */
void
ovl_hat_init(hatlist, phys_start, phys_end)
	hat_list_t hatlist;
	vm_offset_t phys_start, phys_end;
{
	hat_t hat;
	vm_offset_t addr;
	vm_size_t pvsize, size, npg;

	ovl_object_reference(overlay_object);
	ovl_map_find(overlay_map, overlay_object, addr, &addr, 2*NBPG, FALSE);

	pvsize = (vm_size_t)(sizeof(struct pv_entry));
	npg = atop(phys_end - phys_start);
	size = (pvsize * npg + npg);

	hat = LIST_FIRST(hatlist);
	if (hat == NULL) {
		hat = (struct hat *)omem_alloc(overlay_map, sizeof(struct hat));
	}
	hat->h_table = (struct pv_entry *)omem_alloc(overlay_map, size);
	hat_attach(&ovlhat_list, hat, overlay_map, overlay_object, HAT_OVL);
}

vm_offset_t
ovl_hat_pa_index(pa)
	vm_offset_t pa;
{
	return (hat_pa_index(pa, ovl_first_phys));
}

pv_entry_t
ovl_hat_to_pvh(hatlist, pa)
	hat_list_t hatlist;
	vm_offset_t pa;
{
	return (hat_to_pvh(hatlist, overlay_map, overlay_object, pa, ovl_first_phys, HAT_OVL));
}

void
ovl_hat_pv_enter(hatlist, pmap, va, pa)
	hat_list_t hatlist;
	pmap_t pmap;
	vm_offset_t va;
	vm_offset_t pa;
{
	hat_pv_enter(hatlist, overlay_map, overlay_object, pmap, va, pa, ovl_first_phys, ovl_last_phys, HAT_OVL);
}

void
ovl_hat_pv_remove(hatlist, pmap, sva, eva)
	hat_list_t hatlist;
	pmap_t pmap;
	vm_offset_t sva, eva;
{
	hat_pv_remove(hatlist, overlay_map, overlay_object, pmap, sva, eva, ovl_first_phys, ovl_last_phys, HAT_OVL);
}
