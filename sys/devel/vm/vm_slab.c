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
 * A Simple Slab Allocator.
 * Slabs treats each bucket in kmembucket as pool with "x" slots available.
 *
 * Slots are determined by the bucket index and the object size being allocated.
 * Slab_metadata is responsible for retaining all slot related data and some slab information
 * (i.e. slab state (partial, full or empty) and slab size (large or small))
 *
 * Slab allows for small and large objects by splitting kmembuckets into two.
 * Any objects with a bucket index less than 10 are flagged as small, while objects
 * greater than or equal to 10 are flagged as large.
 */
#include <sys/extent.h>
#include <sys/malloc.h>
#include <devel/sys/malloctypes.h>
#include <devel/vm/include/vm_slab.h>

struct slablist 	slab_list;
simple_lock_data_t	slab_list_lock;

/*
 * Start of Routines for Slab Allocator using extents
 */
void
slab_startup(start, end)
	vm_offset_t	*start;
	vm_offset_t	*end;
{
	slab_t 			slab;
	int				error;
	u_long 			indx;
	size_t 			size;
	u_long			bsize;

    CIRCLEQ_INIT(&slab_cache_list);
    CIRCLEQ_INIT(slab_list);

    simple_lock_init(&slab_list_lock, "slab_list_lock");

    slab_count = 0;

    size = end - start;

	for(indx = 0; indx < MINBUCKET + 16; indx++) {
		bsize = BUCKETSIZE(indx);
		slab_insert(slab, bsize, M_VMSLAB, M_WAITOK);
	}
	slab->s_extent = extent_create("slab extent", start, end, slab->s_size, slab->s_mtype, NULL, NULL, EX_WAITOK | EX_MALLOCOK);
	error = extent_alloc_region(slab->s_extent, start, bsize, EX_WAITOK | EX_MALLOCOK);
	if (error) {
		printf("slab_startup: slab allocator initialized successful");
	} else {
		panic("slab_startup: slab allocator couldn't be initialized");
		slab_destroy(slab);
	}
}

void
slab_malloc(size, alignment, boundary, mtype, flags)
	u_long 	size, alignment, boundary;
	int		mtype, flags;
{
	slab_t 	slab;
	int 	error;
	u_long 	*result;

	slab = slab_lookup(size, mtype);

	error = extent_alloc(slab->s_extent, size, alignment, boundary, flags, &result);
	if(error) {
		printf("slab_malloc: successful");
	} else {
		printf("slab_malloc: unsuccessful");
	}
}

void
slab_free(addr, mtype)
	void 	*addr;
	int		mtype;
{
	slab_t slab;
	size_t start;
	int   error;

	slab = slab_lookup(addr, mtype);
	if (slab) {
		start = slab->s_extent->ex_start;
		error = extent_free(slab->s_extent, start, addr, flags);
		slab_remove(addr);
	}
}

void
slab_destroy(slab)
	slab_t slab;
{
	if(slab->s_extent != NULL) {
		extent_destroy(slab->s_extent);
	}
}
/* End of Routines for Slab Allocator using extents */

/* Start of Routines for Slab Allocator using Kmembuckets */
extern struct kmembuckets bucket[MINBUCKET + 16];

struct slab *
slab_create(slab)
	struct slab *slab;
{
    if(slab == NULL) {
        memset(slab, 0, sizeof(struct slab *));
    }
    return (slab);
}

void
slab_malloc2(size, type, flags)
	unsigned long size;
	int type, flags;
{
	register struct slab *slab;
	register slab_metadata_t meta;
	register struct kmembuckets *kbp;
	long indx;

	indx = BUCKETINDX(size);
	slab = slab_create(&bucket[indx].kb_slab);
	kbp = &bucket[indx];
	slab_insert(slab, size, type, flags);
	meta = slab->s_meta;

	if(slab->s_flags == (SLAB_PARTIAL | SLAB_EMPTY)) {
		if(meta->sm_freeslots >= ALLOCATED_SLOTS(size)) {
			if(kbp->kb_next == NULL) {
				kbp->kb_last = NULL;
				meta->sm_bucket = kbp;
				meta->sm_bucket->kb_next = kbp->kb_next;
				meta->sm_bucket->kb_last = kbp->kb_last;
			}
		}
	}

	if(slab->s_flags == SLAB_FULL) {
		kbp = bucket_search();
		/* update slab */
		slab = slab_create(kbp->kb_slab);
		/* update slab meta here from new bucket */
		if (kbp->kb_next == NULL) {
			kbp->kb_last = NULL;
			meta->sm_bucket = kbp;
			meta->sm_bucket->kb_next = kbp->kb_next;
			meta->sm_bucket->kb_last = kbp->kb_last;
		}
	}
}

/*
 * TODO:
 * - determine space available: to utilize partial slabs
 * - update slab metadata
 *
 * search array for an empty bucket
 */
struct kmembuckets *
bucket_search()
{
	register struct kmembuckets *kbp;
	register long indx;

	for (indx = 0; indx < MINBUCKET + 16; indx++) {
		if(bucket[index].kb_next == NULL) {
			kbp = bucket[index];
			return (kbp);
		}
	}
	return (NULL);
}
/* End of Routines for Slab Allocator using Kmembuckets */

/* slab metadata information */
void
slabmeta(slab, size, mtype, flags)
	slab_t 	slab;
	u_long  size;
	int		mtype, flags;
{
	register slab_metadata_t meta;
	u_long indx;
	u_long bsize;

	indx = BUCKETINDX(size);
	bsize = BUCKETSIZE(indx);
	slab->s_size = size;
	slab->s_mtype = mtype;
	slab->s_flags = flags;

	/* slab metadata */
	meta = slab->s_meta;
	meta->sm_bslots = BUCKET_SLOTS(bsize);
	meta->sm_aslots = ALLOCATED_SLOTS(size);
	meta->sm_freeslots = SLOTSFREE(meta->sm_bslots, meta->sm_aslots);
	if (meta->sm_freeslots < 0) {
		meta->sm_freeslots = 0;
	}
	if (indx < 10) {
		meta->sm_type = SLAB_SMALL;
	} else {
		meta->sm_type = SLAB_LARGE;
	}

	/* test if free bucket slots is between 5% to 95% */
	if((meta->sm_freeslots >= (meta->sm_bslots * 0.05)) && (meta->sm_freeslots <= (meta->sm_bslots * 0.95))) {
		slab->s_flags |= SLAB_PARTIAL;
	/* test if free bucket slots is greater than 95% */
	} else if(meta->sm_freeslots > (meta->sm_bslots * 0.95)) {
		slab->s_flags |= SLAB_FULL;
	} else {
		slab->s_flags |= SLAB_EMPTY;
	}
}

slab_t
slab_small_lookup(size, mtype)
	long    size;
	int 	mtype;
{
    struct slablist   *slabs;
    register slab_t   slab;

    slabs = &slab_list[BUCKETINDX(size)];
    simple_lock(&slab_list_lock);
	for(slab = CIRCLEQ_FIRST(slabs); slab != NULL; slab = CIRCLEQ_NEXT(slab, s_list)) {
		if(slab->s_size == size && slab->s_mtype == mtype) {
			simple_unlock(&slab_list_lock);
			return (slab);
		}
	}
    simple_unlock(&slab_list_lock);
    return (NULL);
}

slab_t
slab_large_lookup(size, mtype)
	long    size;
	int 	mtype;
{
	struct slablist *slabs;
	register slab_t slab;
	slabs = &slab_list[BUCKETINDX(size)];
	simple_lock(&slab_list_lock);
	for (slab = CIRCLEQ_LAST(slabs); slab != NULL; slab = CIRCLEQ_NEXT(slab, s_list)) {
		if (slab->s_size == size && slab->s_mtype == mtype) {
			simple_unlock(&slab_list_lock);
			return (slab);
		}
	}
	simple_unlock(&slab_list_lock);
	return (NULL);
}

void
slab_insert(slab, size, mtype, flags)
    slab_t  slab;
    u_long  size;
    int	    mtype, flags;
{
    register struct slablist  *slabs;
	register slab_metadata_t meta;

    if(slab == NULL) {
        return;
    }

    slabmeta(slab, size, mtype, flags);
    meta = slab->s_meta;

    slabs = &slab_list[BUCKETINDX(size)];
    simple_lock(&slab_list_lock);
    if(meta->sm_type == SLAB_SMALL) {
        CIRCLEQ_INSERT_HEAD(slabs, slab, s_list);
    } else {
        CIRCLEQ_INSERT_TAIL(slabs, slab, s_list);
    }
    simple_unlock(&slab_list_lock);
    slab_count++;
}

void
slab_remove(size)
	u_long    size;
{
    struct slablist  *slabs;
    register slab_t     slab;

    slabs = &slab_list[BUCKETINDX(size)];
    simple_lock(&slab_list_lock);
    CIRCLEQ_REMOVE(slabs, slab, s_list);
    simple_unlock(&slab_list_lock);
    slab_count--;
}
