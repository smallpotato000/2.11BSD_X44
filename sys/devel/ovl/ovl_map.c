/*
 * ovl_map.c
 *
 *  Created on: 25 Apr 2020
 *      Author: marti
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <ovl.h>
#include <ovl_map.h>

vm_offset_t			kovl_data;
vovl_map_entry_t 	kovl_entry_free;
vovl_map_t 			kovl_free;

void
ovl_map_startup()
{
	register vovl_map_entry_t mep;
	vovl_map_t mp;

	kovl_free = mp (vovl_map_t) kovl_data;
	mp->vovl_header
}

struct vovlspace *
ovlspace_alloc(min, max)
	vm_offset_t min, max;
{
	register struct vovlspace *vovl;

	vovl_map_init(&vovl->vovl_map, min, max);
	vovl->vovl_refcnt = 1;
	return (vovl);
}

ovlspace_free()
{

}

vovl_map_t
ovl_map_create(min, max)
	vm_offset_t	min, max;
{
	extern ovl_map_t kovl_rmap;

}

vovl_map_init()
{

}


/* Kernel Overlay Sub-Regions Structure */
int
kovlr_compare(kovlr1, kovlr2)
	struct koverlay_region *kovlr1, *kovlr2;
{
	if (kovlr1->kovlr_start < kovlr2->kovlr_start) {
		return (-1);
	} else if (kovlr1->kovlr_start > kovlr2->kovlr_start) {
		return (1);
	}
	return (0);
}

RB_PROTOTYPE(kovlrb, koverlay_region, kovl_region, kovlr_compare);
RB_GENERATE(kovlrb, koverlay_region, kovl_region, kovlr_compare);

insert(kovlr, addr)
	struct koverlay_region *kovlr;
	caddr_t addr;
{
	int error;

	/*
	struct koverlay_region *kovlr;
	if (kovlr == NULL) {
		kovlr = (struct koverlay*) kovl;
		kovlr->kovlr_flags = KOVLR_ALLOCATED;
	} else {
		continue;
	}

	kovlr->kovlr_name = name;
	kovlr->kovlr_start = start;
	kovlr->kovlr_end =	end;
	kovlr->kovlr_size = size;

	kovlr->kovlr_addr += ex->ex_start;
	*/


	if(kovlr != NULL) {
		error = koverlay_alloc_subregion(kovl, name, start, end, size, alignment, boundary, result);
	}

	RB_INSERT(kovlrb, kovlr, addr);
}


remove(kovlr, addr)
	struct koverlay_region *kovlr;
	caddr_t addr;
{
	int error;


	RB_REMOVE(kovlrb, kovlr, addr);
}
