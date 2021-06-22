/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)vm_proc.c	1.2 (2.11BSD GTE) 12/24/92
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/map.h>
#include <sys/user.h>

#include <devel/vm/include/vm.h>
#include <devel/vm/include/vm_segment.h>

/*
 * Change the size of the data+stack regions of the process.
 * If the size is shrinking, it's easy -- just release the extra core.
 * If it's growing, and there is core, just allocate it and copy the
 * image, taking care to reset registers to account for the fact that
 * the system's stack has moved.  If there is no core, arrange for the
 * process to be swapped out after adjusting the size requirement -- when
 * it comes in, enough core will be allocated.  After the expansion, the
 * caller will take care of copying the user's stack towards or away from
 * the data area.  The data and stack segments are separated from each
 * other.  The second argument to expand specifies which to change.  The
 * stack segment will not have to be copied again after expansion.
 */
void
vm_segment_expand(segment, newsize)
	vm_segment_t 	segment;
	vm_size_t 	 	newsize;
{
	register struct proc *p;
	register vm_size_t i, n;
	caddr_t a1, a2;

	p = u->u_procp;
	if (segment->sg_type == PSEG_DATA) {
		n = segment->sg_data.sp_dsize;
		segment->sg_data.sp_dsize = newsize;
		p->p_dsize = segment->sg_data.sp_dsize;
		a1 = segment->sg_data.sp_daddr;
		if (n >= newsize) {
			n -= newsize;
			rmfree(coremap, n, a1 + newsize);
			return;
		}
	} else {
		n = segment->sg_stack.sp_ssize;
		segment->sg_stack.sp_ssize = newsize;
		p->p_ssize = segment->sg_stack.sp_ssize;
		a1 = segment->sg_stack.sp_saddr;
		if (n >= newsize) {
			n -= newsize;
			segment->sg_stack.sp_saddr += n;
			p->p_saddr = segment->sg_stack.sp_saddr;
			rmfree(coremap, n, a1);
			return;
		}
	}
	if (segment->sg_type == PSEG_STACK) {
		a1 = segment->sg_stack.sp_saddr;
		i = newsize - n;
		a2 = a1 + i;
		/*
		 * i is the amount of growth.  Copy i clicks
		 * at a time, from the top; do the remainder
		 * (n % i) separately.
		 */
		while (n >= i) {
			n -= i;
			bcopy(a1 + n, a2 + n, i);
		}
		bcopy(a1, a2, n);
	}
	a2 = rmalloc(coremap, newsize);
	if (a2 == NULL) {
		if (segment->sg_type == PSEG_DATA) {
			//swapout(p);
		} else {
			//swapout(p);
		}
	}
	if (segment->sg_type == PSEG_STACK) {
		segment->sg_stack.sp_saddr = a2;
		p->p_saddr = segment->sg_stack.sp_saddr;
		/*
		 * Make the copy put the stack at the top of the new area.
		 */
		a2 += newsize - n;
	} else {
		segment->sg_data.sp_daddr = a2;
		p->p_daddr = segment->sg_data.sp_daddr;
	}
	bcopy(a1, a2, n);
	rmfree(coremap, n, a1);
}

/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)kern_mman.c	1.3 (2.11BSD) 2000/2/20
 */

int
sbrk(p, uap, retval)
	struct proc *p;
	struct sbrk_args  {
		syscallarg(int) 	type;
		syscallarg(segsz_t) size;
		syscallarg(caddr_t) addr;
		syscallarg(int) 	sep;
		syscallarg(int) 	flags;
		syscallarg(int) 	incr;
	} *uap;
	register_t *retval;
{
	register segsz_t n, d;

	n = btoc(uap->size);
	if (!uap->sep) {

	} else {
		n -= ctos(p->p_tsize) * stoc(1);
	}
	if (n < 0) {
		n = 0;
	}
	p->p_tsize;
	if (estabur(n, p->p_ssize, p->p_tsize, uap->sep, SEG_RO)) {
		return (0);
	}
	expand(n, S_DATA);
	/* set d to (new - old) */
	d = n - p->p_dsize;
	if (d > 0) {
		clear(p->p_daddr + p->p_dsize, d);
	}
	p->p_dsize = n;
	/* Not yet implemented */
	return (EOPNOTSUPP);
}

/*
 * Set up software prototype segmentation registers to implement the 3
 * pseudo text, data, stack segment sizes passed as arguments.  The
 * argument sep specifies if the text and data+stack segments are to be
 * separated.  The last argument determines whether the text segment is
 * read-write or read-only.
 */
int
estabur(pseg, data, stack, text, type, sep, flags)
	vm_psegment_t 	*pseg;
	vm_data_t 		data;
	vm_stack_t 		stack;
	vm_text_t 		text;
	int 			sep, flags;
{

	if(pseg == NULL || data == NULL || stack == NULL || text == NULL) {
		return (1);
	}
	if(!sep && (type = PSEG_DATA | PSEG_STACK | PSEG_TEXT)) {
		if(flags == SEG_RO) {
			vm_psegment_set(pseg, text->sp_tsize, text->sp_taddr, SEG_RO);
		} else {
			vm_psegment_set(pseg, text->sp_tsize, text->sp_taddr, SEG_RW);
		}
		vm_psegment_set(pseg, data->sp_dsize, data->sp_daddr, SEG_RW);
		vm_psegment_set(pseg, stack->sp_ssize, stack->sp_saddr, SEG_RW);
	}
	if(sep && (type == PSEG_DATA | PSEG_STACK)) {
		if (flags == SEG_RO) {
			vm_psegment_set(pseg, text->sp_tsize, text->sp_taddr, SEG_RO);
		} else {
			vm_psegment_set(pseg, text->sp_tsize, text->sp_taddr, SEG_RW);
		}
		vm_psegment_set(pseg, data->sp_dsize, data->sp_daddr, SEG_RW);
		vm_psegment_set(pseg, stack->sp_ssize, stack->sp_saddr, SEG_RW);
	}
	return (0);
}

/*
 * Load the user hardware segmentation registers from the software
 * prototype.  The software registers must have been setup prior by
 * estabur.
 */
void
sureg()
{
	int taddr, daddr, saddr;

	taddr = daddr = u.u_procp->p_daddr;
	saddr = u.u_procp->p_saddr;
}

/*
 * Routine to change overlays.  Only hardware registers are changed; must be
 * called from sureg since the software prototypes will be out of date.
 */
void
choverlay(flags)
	int flags;
{

}
