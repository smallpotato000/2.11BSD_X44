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
 * Initialize the MI portions of a struct pcpu.
 * (NOTE: runs in mp_machdep.c (init_secondary))
 */
void
percpu_init(pcpu, cpuid, size)
	struct percpu *pcpu;
	int cpuid;
	size_t size;
{
	bzero(pcpu, size);
	KASSERT(cpuid >= 0 && cpuid < NCPUS ("percpu_init: invalid cpuid %d", cpuid));

	//pcpu->pc_cpuid = cpuid;
	//pcpu->pc_cpumask = 1 << cpuid;

	cpuid_to_percpu[cpuid] = pcpu;
	LIST_INSERT_HEAD(&cpuhead, pcpu, pc_entry);
	cpu_percpu_init(pcpu, cpuid, size);
}

/* allocate percpu structure */
void
percpu_malloc(pcpu, size)
	struct percpu *pcpu;
	size_t size;
{
	pcpu = malloc(sizeof(*pcpu), M_PERCPU, M_WAITOK | M_ZERO);
	percpu_extent(pcpu, PERCPU_START, PERCPU_END);
	percpu_extent_region(pcpu);
	size = roundup(size, PERCPU_SIZE);
	percpu_extent_subregion(pcpu, size);
}

/*
 * Destroy a struct percpu.
 */
void
percpu_destroy(pcpu)
	struct percpu *pcpu;
{
	LIST_REMOVE(pcpu, pc_entry);
	cpuid_to_percpu[pcpu->pc_cpuid] = NULL;
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

/* allocate a percpu extent structure */
void
percpu_extent(pcpu, start, end)
	struct percpu 		*pcpu;
	u_long				start, end;
{
	pcpu->pc_start = start;
	pcpu->pc_end = end;
	pcpu->pc_extent = extent_create("percpu_extent_storage", start, end, M_PERCPU, NULL, NULL, EX_WAITOK | EX_FAST);
}

/* allocate a percpu extent region */
void
percpu_extent_region(pcpu)
	struct percpu *pcpu;
{
	register struct extent *ext;
	int error;

	ext = pcpu->pc_extent;
	if(ext == NULL) {
		panic("percpu_extent_region: no extent");
		return;
	}
	error = extent_alloc_region(ext, pcpu->pc_start, pcpu->pc_end, EX_WAITOK | EX_MALLOCOK | EX_FAST);
	if (error != 0) {
		percpu_extent_free(ext, pcpu->pc_start, pcpu->pc_end, EX_WAITOK | EX_MALLOCOK | EX_FAST);
		panic("percpu_extent_region");
	} else {
		printf("percpu_extent_region: successful");
	}
}

/* allocate a percpu extent subregion */
void
percpu_extent_subregion(pcpu, size)
	struct percpu 	*pcpu;
	size_t 			size;
{
	register struct extent *ext;
	int error;

	ext = pcpu->pc_extent;
	if(ext == NULL) {
		panic("percpu_extent_subregion: no extent");
		return;
	}
	error = extent_alloc(ext, size, PERCPU_ALIGN, PERCPU_BOUNDARY, EX_WAITOK | EX_MALLOCOK | EX_FAST, pcpu->pc_dynamic);
	if (error != 0) {
		percpu_extent_free(ext, pcpu->pc_start, pcpu->pc_end);
		panic("percpu_extent_subregion");
	}  else {
		printf("percpu_extent_subregion: successful");
	}
}

/* free a percpu extent */
void
percpu_extent_free(pcpu, start, end)
	struct percpu *pcpu;
	u_long	start, end;
{
	register struct extent *ext;
	int error;

	ext = pcpu->pc_extent;
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
percpu_free(pcpu)
	struct percpu *pcpu;
{
	if(pcpu->pc_extent != NULL) {
		extent_destroy(pcpu->pc_extent);
	}
	free(pcpu, M_PERCPU);
}

/*
 * TODO:
 * - Fix: ctors & dtors don't serve any purpose in percpu or cpu_info
 * - Only used is in threadpools
 */
struct percpu *
percpu_create(size, ctor, dtor, cookie)
	size_t size;
	percpu_callback_t ctor, dtor;
	void *cookie;
{
	struct percpu *pcpu;

	percpu_malloc(pcpu, size);

	pcpu->pc_size = size;
	pcpu->pc_ctor = ctor;
	pcpu->pc_dtor = dtor;
	pcpu->pc_cookie = cookie;

	if(ctor) {
		void *buf;

		LIST_FOREACH(pcpu, &cpuhead, pc_entry) {
			memset(buf, 0, size);
			(*ctor)(buf, cookie);
			percpu_traverse_enter();
			memcpy(percpu_getptr_remote(pcpu, ci), buf, size);
			percpu_traverse_exit();
		}
		memset(buf, 0, size);
		percpu_extent_free(pcpu, pcpu->pc_start, pcpu->pc_end);
	} else {
		bzero(pcpu, size);
	}
	return (pcpu);
}

struct percpu *
percpu_alloc(size)
	size_t size;
{
	return (percpu_create(size, NULL, NULL, NULL));
}

struct percpu *
cpu_percpu(ci)
	struct cpu_info *ci;
{
	return (&ci->cpu_percpu);
}

void
percpu_init2(ci)
	struct cpu_info *ci;
{
	struct percpu *pcpu;

	pcpu = cpu_percpu(ci);

	bzero(pcpu, ci->cpu_size);
	KASSERT(cpuid >= 0 && cpuid < NCPUS ("percpu_init: invalid cpuid %d", cpuid));

	pcpu->pc_cpuid = cpu_cpuid(ci);
	pcpu->pc_cpumask = cpu_cpumask(ci);

	cpuid_to_percpu[ci->cpu_cpuid] = pcpu;
	LIST_INSERT_HEAD(&cpuhead, pcpu, pc_entry);
	pcpu->pc_acpi_id = cpu_acpi_id(ci);
}

struct percpu *
percpu_lookup(ci)
	struct cpu_info *ci;
{
	struct percpu *pcpu;
	LIST_FOREACH(pcpu, &cpuhead, pc_entry) {
		if(cpu_percpu(ci) == pcpu && cpu_cpuid(ci) == pcpu->pc_cpuid) {
			return (pcpu);
		}
	}
	return (NULL);
}

void
percpu_remove(ci)
	struct cpu_info *ci;
{
	struct percpu *pcpu;
	pcpu = percpu_lookup(ci);

	LIST_REMOVE(pcpu, pc_entry);
	cpuid_to_percpu[ci->cpu_cpuid] = NULL;
}
