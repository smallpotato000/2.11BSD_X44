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

/*
 * vm extents provides a simple extent interface, for initializing and allocating
 * extent maps.
 * As extents can be mostly malloc independent, it provides an easy initialization before
 * the vm is up and running.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/null.h>
#include <sys/tree.h>

#include <devel/vm/include/vm.h>
#include <devel/vm/include/vm_hat.h>

/* vm_extent */
void	vm_exbootinit(struct extent *, char *, u_long, u_long, int, caddr_t, size_t, int);
void	vm_exboot_region(struct extent *, u_long, u_long, int);
void	vm_exboot_subregion(struct extent *, u_long, u_long, u_long, u_long, u_long, int, u_long *);
int		vm_exalloc_region(struct extent *, u_long, u_long, int);
int		vm_exalloc_subregion(struct extent *, u_long, u_long, u_long, u_long, u_long, int, u_long *);
void	vm_exfree(struct extent *, u_long, u_long, int);

/* example vm_map startup using extents */
static struct extent 	*kmap_extent, *kentry_extent, *vmspace_extent;
static struct extent 	kmap_store, kentry_store;
vm_map_t 		kmapex;
vm_map_entry_t 	kentryex;
struct vmspace	vmspaceex;

long vmspace_storage[EXTENT_FIXED_STORAGE_SIZE(sizeof(struct vmspace *))];
long kmap_storage[EXTENT_FIXED_STORAGE_SIZE(sizeof(vm_map_t))];
long kentry_storage[EXTENT_FIXED_STORAGE_SIZE(sizeof(vm_map_entry_t))];

void
vm_map_startup1()
{
	kmap_extent = vm_exinit("KMAP", &kmapex[0], &kmapex[MAX_KMAP], M_VMMAP, kmap_storage, sizeof(kmap_storage), EX_NOWAIT);
	kentry_extent = vm_exinit("KENTRY", &kentryex[0], &kentryex[MAX_KMAPENT], M_VMMAPENT, kentry_storage, sizeof(kentry_storage), EX_NOWAIT);
	vmspace_extent = vm_exinit("VMSPACE", &vmspaceex[0], &vmspaceex[], M_VMMAP, vmspace_storage, sizeof(vmspace_storage), EX_NOWAIT);
}

void
vm_map_boot()
{
	vm_exboot_region(kmap_extent, sizeof(vm_map_t), EX_MALLOCOK);
	vm_exboot_region(kentry_extent, sizeof(vm_map_entry_t), EX_MALLOCOK);
}

vm_extent_init()
{
	struct extent extent_zone;
}

struct extent *
vm_exinit(name, start, end, mtype, storage, storagesize, flags)
	char 	*name;
	u_long 	start, end;
	int 	mtype;
	caddr_t storage;
	size_t 	storagesize;
	int 	flags;
{
	struct extent 	*ex;

	ex = extent_create(name, start, end, mtype, storage, storagesize, flags);
	if(ex == NULL) {
		return (NULL);
	}
	return (ex);
}

void
vm_exbootinit(ex, name, start, end, mtype, storage, storagesize, flags)
	struct extent 	*ex;
	char 	*name;
	u_long 	start, end;
	int 	mtype;
	caddr_t storage;
	size_t 	storagesize;
	int 	flags;
{
	ex = vm_exinit(name, start, end, mtype, storage, storagesize, flags);
}

void
vm_exboot_region(ex, size, flags)
	struct extent 	*ex;
	u_long size;
	int flags;
{
	int error;

	error = vm_exalloc_region(ex, ex->ex_start, size, flags);
	if (error != 0) {
		vm_exfree(ex, ex->ex_start, size, flags);
	}
}

void
vm_exboot_subregion(ex, start, end, size, alignment, boundary, flags, result)
	struct extent 	*ex;
	u_long start, end, size, alignment, boundary;
	int flags;
	u_long *result;
{
	int error;

	error = vm_exalloc_subregion(ex, start, end, size, alignment, boundary, flags, result);
	if (error != 0) {
		vm_exfree(ex, start, size, flags);
	}
}

void
vm_exboot(ex, size, alignment, boundary, flags, result)
	struct extent *ex;
	u_long size, alignment, boundary;
	int flags;
	u_long *result;
{
	int error;

	error = vm_exalloc(ex, size, alignment, boundary, flags, result);
	if (error != 0) {
		vm_exfree(ex, ex->ex_start, size, flags);
	}
}

int
vm_exalloc_region(ex, start, size, flags)
	struct extent 	*ex;
	u_long start, size;
	int flags;
{
	int error;
	error = extent_alloc_region(ex, start, size, flags);
	return (error);
}

int
vm_exalloc_subregion(ex, start, end, size, alignment, boundary, flags, result)
	struct extent *ex;
	u_long start, end, size, alignment, boundary;
	int flags;
	u_long *result;
{
	int error;
	error = extent_alloc_subregion(ex, start, end, size, alignment, boundary, flags, result);
	return (error);
}

int
vm_exalloc(ex, size, alignment, boundary, flags, result)
	struct extent *ex;
	u_long size, alignment, boundary;
	int flags;
	u_long *result;
{
	int error;
	error = extent_alloc(ex, size, alignment, boundary, flags, result);
	return (error);
}

void
vm_exfree(ex, start, size, flags)
	struct extent *ex;
	u_long 		start, size;
	int 		flags;
{
	int error;

	error = extent_free(ex, start, size, flags);
	if (error == 0) {
		printf("vm_hat_exfree: extent freed");
	} else {
		panic("vm_hat_exfree: extent wasn't freed");
	}
}

static void
vm_map_zinit(void *mem, int size)
{
	vm_map_t map;

	map = (vm_map_t)mem;
	map->size = 0;
}

/* vm_kern.c */
vm_offset_t
kmem_alloc(map, size)
	register vm_map_t	map;
	register vm_size_t	size;
{
	vm_offset_t				addr;
	register vm_offset_t	offset;
	vm_segment_t 	segment;
	vm_page_t	 	page;
	vm_size_t		pgsize, sgsize;
	vm_offset_t 	i, j;

	size = round_page(size);
	sgsize = round_segment(size);

	vm_map_lock(map);
	if (vm_map_findspace(map, 0, size, &addr)) {
		vm_map_unlock(map);
		return (0);
	}
	offset = addr - VM_MIN_KERNEL_ADDRESS;
	vm_object_reference(kernel_object);
	vm_map_insert(map, kernel_object, offset, addr, addr + size);
	vm_map_unlock(map);

	vm_object_lock(kernel_object);
	for (i = 0 ; i < sgsize; i+= SEGMENT_SIZE) {
		segment = vm_segment_alloc(kernel_object, offset+i);
		while (segment != NULL) {
			for (j = 0 ; j < size; j+= PAGE_SIZE) {
				page = vm_page_alloc(segment, offset+j);
				while (page == NULL) {
					vm_object_unlock(kernel_object);
					VM_WAIT;
					vm_object_lock(kernel_object);
				}
				vm_page_zero_fill(page);
				page->flags &= ~PG_BUSY;
			}
		}
		while (segment == NULL) {
			vm_object_unlock(kernel_object);
			VM_WAIT;
			vm_object_lock(kernel_object);
		}
		vm_segment_zero_fill(segment);
		segment->sg_flags &= ~SEG_BUSY;
	}
	vm_object_unlock(kernel_object);

	(void) vm_map_pageable(map, (vm_offset_t) addr, addr + size, FALSE);

	vm_map_simplify(map, addr);

	return (addr);
}
