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
 * This module provides MI support for per-cpu data.
 *
 * Each architecture determines the mapping of logical CPU IDs to physical
 * CPUs.  The requirements of this mapping are as follows:
 *  - Logical CPU IDs must reside in the range 0 ... MAXCPU - 1.
 *  - The mapping is not required to be dense.  That is, there may be
 *    gaps in the mappings.
 *  - The platform sets the value of MAXCPU in <machine/param.h>.
 *  - It is suggested, but not required, that in the non-SMP case, the
 *    platform define MAXCPU to be 1 and define the logical ID of the
 *    sole CPU as 0.
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/user.h>
#include <sys/extent.h>

#include <devel/sys/percpu.h>
#include <devel/sys/malloctypes.h>

#include <devel/arch/i386/include/cpu.h>

struct percpu *cpuid_to_percpu[NCPUS];
struct cpuhead cpuhead = LIST_HEAD_INITIALIZER(cpuhead);

/*
 * Initialize the MI portions of a struct percpu.
 *  (NOTE: runs in mp_machdep.c (init_secondary))
 */
void
percpu_init(pc, ci, size)
	struct percpu 		*pc;
	struct cpu_info 	*ci;
	size_t 				size;
{
	bzero(pc, size);
	KASSERT(ci->cpu_cpuid >= 0 && ci->cpu_cpuid < NCPUS ("percpu_init: invalid cpuid %d", ci->cpu_cpuid));

	pc->pc_cpuid = cpu_cpuid(ci);
	pc->pc_cpumask = cpu_cpumask(ci);
	cpuid_to_percpu[ci->cpu_cpuid] = pc;
	LIST_INSERT_HEAD(&cpuhead, pc, pc_entry);
	pc->pc_acpi_id = cpu_acpi_id(ci);
	pc->pc_offset = percpu_offset_cpu(cpu_cpuid(ci));
}

/* lookup percpu from cpu_info */
struct percpu *
percpu_lookup(ci)
	struct cpu_info *ci;
{
	struct percpu *pc;
	LIST_FOREACH(pc, &cpuhead, pc_entry) {
		if(cpu_percpu(ci) == pc) {
			return (pc);
		}
	}
	return (NULL);
}

/* Destroys a percpu struct if cpuid matches cpu_info's cpuid */
void
percpu_remove(ci)
	struct cpu_info *ci;
{
	struct percpu *pc;
	pc = percpu_lookup(ci);
	if(pc->pc_cpuid == ci->cpu_cpuid) {
		percpu_destroy(pc);
	}
}

/*
 * Destroy a struct percpu.
 */
void
percpu_destroy(pc)
	struct percpu *pc;
{
	LIST_REMOVE(pc, pc_entry);
	cpuid_to_percpu[pc->pc_cpuid] = NULL;
}

/*
 * Locate a struct percpu by cpu id.
 */
struct percpu *
percpu_find(cpuid)
	int cpuid;
{
	return (cpuid_to_percpu[cpuid]);
}

/* allocate percpu structures */
void
percpu_malloc(pc, size)
	struct percpu *pc;
	size_t size;
{
	pc = malloc(sizeof(*pc), M_PERCPU, M_WAITOK | M_ZERO);
	percpu_extent(pc, PERCPU_START, PERCPU_END);
	percpu_extent_region(pc);
	size = roundup(size, PERCPU_SIZE);
	percpu_extent_subregion(pc, size);
}

/* allocate a percpu extent structure */
void
percpu_extent(pc, start, end)
	struct percpu 		*pc;
	u_long				start, end;
{
	pc->pc_start = start;
	pc->pc_end = end;
	pc->pc_extent = extent_create("percpu_extent_storage", start, end, M_PERCPU, NULL, NULL, EX_WAITOK | EX_FAST);
}

/* allocate a percpu extent region */
void
percpu_extent_region(pc)
	struct percpu *pc;
{
	register struct extent *ext;
	int error;

	ext = pc->pc_extent;
	if(ext == NULL) {
		panic("percpu_extent_region: no extent");
		return;
	}
	error = extent_alloc_region(ext, pc->pc_start, pc->pc_end, EX_WAITOK | EX_MALLOCOK | EX_FAST);
	if (error != 0) {
		percpu_extent_free(ext, pc->pc_start, pc->pc_end, EX_WAITOK | EX_MALLOCOK | EX_FAST);
		panic("percpu_extent_region");
	} else {
		printf("percpu_extent_region: successful");
	}
}

/* allocate a percpu extent subregion */
void
percpu_extent_subregion(pc, size)
	struct percpu 	*pc;
	size_t 			size;
{
	register struct extent *ext;
	int error;

	ext = pc->pc_extent;
	if(ext == NULL) {
		panic("percpu_extent_subregion: no extent");
		return;
	}
	error = extent_alloc(ext, size, PERCPU_ALIGN, PERCPU_BOUNDARY, EX_WAITOK | EX_MALLOCOK | EX_FAST, pc->pc_dynamic);
	if (error != 0) {
		percpu_extent_free(ext, pc->pc_start, pc->pc_end);
		panic("percpu_extent_subregion");
	}  else {
		printf("percpu_extent_subregion: successful");
	}
}

/* free a percpu extent */
void
percpu_extent_free(pc, start, end)
	struct percpu *pc;
	u_long	start, end;
{
	register struct extent *ext;
	int error;

	ext = pc->pc_extent;
	if(ext == NULL) {
		printf("percpu_extent_free: no extent to free");
	}

	error = extent_free(ext, start, end, NULL);
	if (error != 0) {
		panic("percpu_extent_free: failed to free extent region");
	} else {
		printf("percpu_extent_free: successfully freed");
	}
}

/* free percpu and destroy all percpu extents */
void
percpu_free(pc)
	struct percpu *pc;
{
	if(pc->pc_extent != NULL) {
		extent_destroy(pc->pc_extent);
	}
	free(pc, M_PERCPU);
}

/* start a percpu struct from valid cpu_info */
struct percpu *
percpu_start(ci, size)
	struct cpu_info *ci;
	size_t 			size;
{
	struct percpu *pc;

	pc = cpu_percpu(ci);
	pc->pc_size = size;

	return (pc);
}

struct percpu *
percpu_create(ci, size, count , ncpus)
	struct cpu_info *ci;
	size_t size;
	int count, ncpus;
{
	struct percpu *pc;
	int i, error, cpu;

	percpu_malloc(pc, size);

	cpu = ncpus;

	for (i = 0; i < count; i++) {
		if ((ncpus <= -1) && (count > 1)) {
			cpu = i%ncpus;
			if (count == 1) {
				pc = percpu_start(ci, size);
			} else {
				pc = percpu_start(ci, size);
			}
		}
	}
	return (pc);
}
