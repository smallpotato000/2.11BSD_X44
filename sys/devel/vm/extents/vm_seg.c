/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
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

#include <sys/user.h>
#include <sys/tree.h>
#include "vm_seg.h"

/* TODO:
 * - Allocate vm_segment
 */

void
vm_segment_init(void)
{
	register struct vm_segment *seg;
	RB_INIT(&seg->seg_rbroot);
	seg = (struct vm_segment *) rmalloc(seg, sizeof(struct vm_segment *));
}

/* vm segment extent memory management */
struct extent *
vm_segment_create(seg, name, start, end, mtype, storage, storagesize, flags)
	struct vm_segment *seg;
	char *name;
	vm_offset_t start, end;
	int mtype, flags;
	caddr_t	storage;
	vm_size_t storagesize;
{
	seg->seg_name = name;
	seg->seg_start = start;
	seg->seg_end = end;
	seg->seg_addr += start;
	seg->seg_vsize = start - end;

	seg->seg_extent = extent_create(seg->seg_extent, name, start, end, mtype, storage, storagesize, flags);

	return (seg->seg_extent);
}

int
vm_segment_allocate_region(seg, start, size, flags)
	struct vm_segment *seg;
	u_long start, size;
	int flags;
{
	int error;

	error = extent_alloc_region(seg->seg_extent, start, size, flags);

	if(error) {
		return (error);
	}
	return (0);
}

int
vm_segment_allocate_subregion(seg, substart, subend, size, alignment, boundary, flags, result)
	struct vm_segment *seg;
	u_long substart, subend, size, alignment, boundary;
	int flags;
	u_long *result;
{
	int error;

	error = extent_alloc_subregion(seg->seg_extent, substart, subend, size, alignment, boundary, flags, result);

	if(error) {
		return (error);
	}
	return (0);
}

int
vm_segment_free(seg, start, size, flags)
	struct vm_segment *seg;
	u_long start, size;
	int flags;
{
	int error;

	error = extent_free(seg->seg_extent, start, size, flags);

	if(error) {
		return (error);
	}
	return (0);
}

int
vm_segment_destroy(seg)
	struct vm_segment *seg;
{
	int error;

	error = extent_destroy(seg->seg_extent);

	if(error) {
		return (error);
	}
	return (0);
}
