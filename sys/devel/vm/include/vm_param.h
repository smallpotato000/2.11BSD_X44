/* 
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * The Mach Operating System project at Carnegie-Mellon University.
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
 *	@(#)vm_param.h	8.2 (Berkeley) 1/9/95
 *
 *
 * Copyright (c) 1987, 1990 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Authors: Avadis Tevanian, Jr., Michael Wayne Young
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

/*
 *	Machine independent virtual memory parameters.
 */

#ifndef	_VM_PARAM_
#define	_VM_PARAM_

#include <machine/vmparam.h>

/*
 * This belongs in types.h, but breaks too many existing programs.
 */
typedef int	boolean_t;
typedef boolean_t 	bool;
#define	TRUE		1
#define	FALSE		0

/*
 *	The machine independent segments are referred to as SEGMENTS.
 *	Note: All Segment information presented here is derived from 4.4BSD-Lite2 HP300.
 */
#define	DEFAULT_SEGMENT_SIZE	4194304					/* 4 mib segments */

#define SEGMENT_SIZE 			cnt.v_segment_size		/* size of segment */
#define SEGMENT_MASK			segment_mask			/* size of segment - 1 */
#define	SEGMENT_SHIFT			segment_shift			/* bits to shift for segments */
#ifdef KERNEL
extern vm_size_t				segment_mask;
extern int						segment_shift;
#endif

/*
 *	The machine independent pages are referred to as PAGES.  A page
 *	is some number of hardware pages, depending on the target machine.
 */
#define	DEFAULT_PAGE_SIZE		4096					/* 4 kib pages per segment */

/*
 *	All references to the size of a page should be done with PAGE_SIZE
 *	or PAGE_SHIFT.  The fact they are variables is hidden here so that
 *	we can easily make them constant if we so desire.
 */

#define	PAGE_SIZE				cnt.v_page_size			/* size of page */
#define	PAGE_MASK				page_mask				/* size of page - 1 */
#define	PAGE_SHIFT				page_shift				/* bits to shift for pages */
#ifdef KERNEL
extern vm_size_t				page_mask;
extern int						page_shift;
#endif

/*
 * CTL_VM identifiers
 */
#define	VM_METER		1		/* struct vmmeter */
#define	VM_LOADAVG		2		/* struct loadavg */
#define	VM_VMEXP		3		/* struct vmexp */
#define	VM_NKMEMPAGES	4		/* kmem_map pages */
#define	VM_SWAPMAP		5		/* struct mapent _swapmap[] */
#define	VM_COREMAP		6		/* struct mapent _coremap[] */
#define	VM_ANONMIN		7
#define	VM_EXECMIN		8
#define	VM_FILEMIN		9
#define	VM_MAXSLP		10
#define	VM_USPACE		11
#define	VM_ANONMAX		12
#define	VM_EXECMAX		13
#define	VM_FILEMAX		14
#define	VM_MINADDRESS	15
#define	VM_MAXADDRESS	16

#define	VM_MAXID		17		/* number of valid vm ids */

#define	CTL_VM_NAMES { 					\
	{ 0, 0 }, 							\
	{ "vmmeter", CTLTYPE_STRUCT }, 		\
	{ "loadavg", CTLTYPE_STRUCT }, 		\
	{ "vmexp", CTLTYPE_STRUCT }, 		\
	{ "nkmempages", CTLTYPE_STRUCT }, 	\
	{ "swapmap", CTLTYPE_STRUCT }, 		\
	{ "coremap", CTLTYPE_STRUCT }, 		\
	{ "anonmin", CTLTYPE_STRUCT }, 		\
	{ "execmin", CTLTYPE_STRUCT }, 		\
	{ "filemin", CTLTYPE_STRUCT }, 		\
	{ "maxslp", CTLTYPE_STRUCT }, 		\
	{ "uspace", CTLTYPE_STRUCT }, 		\
	{ "maxslp", CTLTYPE_STRUCT }, 		\
	{ "anonmax", CTLTYPE_STRUCT }, 		\
	{ "execmax", CTLTYPE_STRUCT }, 		\
	{ "filemax", CTLTYPE_STRUCT }, 		\
}

/* 
 *	Return values from the VM routines.
 */
#define	KERN_SUCCESS			0
#define	KERN_INVALID_ADDRESS	1
#define	KERN_PROTECTION_FAILURE	2
#define	KERN_NO_SPACE			3
#define	KERN_INVALID_ARGUMENT	4
#define	KERN_FAILURE			5
#define	KERN_RESOURCE_SHORTAGE	6
#define	KERN_NOT_RECEIVER		7
#define	KERN_NO_ACCESS			8

#ifndef ASSEMBLER
#ifdef KERNEL

/*
 *	Convert addresses to segments & pages and vice versa.
 *	No rounding is used.
 */

#define atos(x)		(((vm_offset_t)(x)) >> SEGMENT_SHIFT)
#define	stoa(x)		((vm_offset_t)((x) << SEGMENT_SHIFT))
#define	atop(x)		(((vm_offset_t)(x)) >> PAGE_SHIFT)
#define	ptoa(x)		((vm_offset_t)((x) << PAGE_SHIFT))

/*
 * Round off or truncate to the nearest segment.
 */
#define	round_segment(x) \
	((vm_offset_t)((((vm_offset_t)(x)) + SEGMENT_MASK) & ~SEGMENT_MASK))
#define	trunc_segment(x) \
	((vm_offset_t)(((vm_offset_t)(x)) & ~SEGMENT_MASK))
#define	num_segments(x) \
	((vm_offset_t)((((vm_offset_t)(x)) + SEGMENT_MASK) >> SEGMENT_SHIFT))

/*
 * Round off or truncate to the nearest page.  These will work
 * for either addresses or counts (i.e., 1 byte rounds to 1 page).
 */
#define	round_page(x) \
	((vm_offset_t)((((vm_offset_t)(x)) + PAGE_MASK) & ~PAGE_MASK))
#define	trunc_page(x) \
	((vm_offset_t)(((vm_offset_t)(x)) & ~PAGE_MASK))
#define	num_pages(x) \
	((vm_offset_t)((((vm_offset_t)(x)) + PAGE_MASK) >> PAGE_SHIFT))

extern vm_size_t	mem_size;	/* size of physical memory (bytes) */
extern vm_offset_t	first_addr;	/* first physical page */
extern vm_offset_t	last_addr;	/* last physical page */

#else
/* out-of-kernel versions of round_segment, trunc_segment, round_page & trunc_page */
#define	round_page(x) \
	((((vm_offset_t)(x) + (vm_page_size - 1)) / vm_page_size) * vm_page_size)
#define	trunc_page(x) \
	((((vm_offset_t)(x)) / vm_page_size) * vm_page_size)

#define	round_segment(x) \
	((((vm_offset_t)(x) + (vm_segment_size - 1)) / vm_segment_size) * vm_segment_size)
#define	trunc_segment(x) \
	((((vm_offset_t)(x)) / vm_segment_size) * vm_segment_size)

#endif /* KERNEL */
#endif /* ASSEMBLER */
#endif /* _VM_PARAM_ */
