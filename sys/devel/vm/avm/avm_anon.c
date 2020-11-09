/*	$NetBSD: uvm_anon.c,v 1.28.2.2 2004/09/11 11:00:10 he Exp $	*/

/*
 *
 * Copyright (c) 1997 Charles D. Cranor and Washington University.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Charles D. Cranor and
 *      Washington University.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 * uvm_anon.c: uvm anon ops
 */

#include <sys/cdefs.h>
/* __KERNEL_RCSID(0, "$NetBSD: uvm_anon.c,v 1.28.2.2 2004/09/11 11:00:10 he Exp $"); */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/extent.h>
#include <sys/kernel.h>

#include <devel/vm/include/vm.h>
#include "vm_swap.h"

/*
 * anonblock_list: global list of anon blocks,
 * locked by swap_syscall_lock (since we never remove
 * anything from this list and we only add to it via swapctl(2)).
 */
struct avm_anonblock {
	LIST_ENTRY(avm_anonblock) list;
	int count;
	struct avm_anon *anons;
};
static LIST_HEAD(anonlist, avm_anonblock) anonblock_list;


static boolean_t anon_pagein(struct avm_anon *);

/*
 * allocate anons
 */
void
vm_anon_init()
{
	int nanon = vmexp.free - (vmexp.free / 16); /* XXXCDC ??? */

	simple_lock_init(&vm.afreelock);
	LIST_INIT(&anonblock_list);

	/*
	 * Allocate the initial anons.
	 */
	vm_anon_add(nanon);
}

/*
 * add some more anons to the free pool.  called when we add
 * more swap space.
 *
 * => swap_syscall_lock should be held (protects anonblock_list).
 */
int
vm_anon_add(count)
	int	count;
{
	struct avm_anonblock *anonblock;
	struct avm_anon *anon;
	int lcv, needed;

	simple_lock(&vm.afreelock);
	vmexp.nanonneeded += count;
	needed = vmexp.nanonneeded - vmexp.nanon;
	simple_unlock(&vm.afreelock);

	if (needed <= 0) {
		return 0;
	}
	anon = (void *)uvm_km_alloc(kernel_map, sizeof(*anon) * needed);
	if (anon == NULL) {
		simple_lock(&vm.afreelock);
		vmexp.nanonneeded -= count;
		simple_unlock(&vm.afreelock);
		return ENOMEM;
	}
	MALLOC(anonblock, void *, sizeof(*anonblock), M_AVMMAP, M_WAITOK);

	anonblock->count = needed;
	anonblock->anons = anon;
	LIST_INSERT_HEAD(&anonblock_list, anonblock, list);
	memset(anon, 0, sizeof(*anon) * needed);

	simple_lock(&vm.afreelock);
	vmexp.nanon += needed;
	vmexp.nfreeanon += needed;
	for (lcv = 0; lcv < needed; lcv++) {
		simple_lock_init(&anon[lcv].an_lock);
		anon[lcv].an_u.an_nxt = vm.afree;
		vm.afree = &anon[lcv];
	}
	simple_unlock(&vm.afreelock);
	return 0;
}

/*
 * remove anons from the free pool.
 */
void
vm_anon_remove(count)
	int count;
{
	/*
	 * we never actually free any anons, to avoid allocation overhead.
	 * XXX someday we might want to try to free anons.
	 */

	simple_lock(&vm.afreelock);
	vmexp.nanonneeded -= count;
	simple_unlock(&vm.afreelock);
}

/*
 * allocate an anon
 *
 * => new anon is returned locked!
 */
struct avm_anon *
vm_analloc()
{
	struct avm_anon *a;

	simple_lock(&vm.afreelock);
	a = vm.afree;
	if (a) {
		vm.afree = a->an_u.an_nxt;
		vmexp.nfreeanon--;
		a->an_ref = 1;
		a->an_swslot = 0;
		a->an_page = NULL;		/* so we can free quickly */
		//LOCK_ASSERT(simple_lock_held(&a->an_lock) == 0);
		simple_lock(&a->an_lock);
	}
	simple_unlock(&vm.afreelock);
	return(a);
}

/*
 * uvm_anfree: free a single anon structure
 *
 * => caller must remove anon from its amap before calling (if it was in
 *	an amap).
 * => anon must be unlocked and have a zero reference count.
 * => we may lock the pageq's.
 */

void
vm_anfree(anon)
	struct avm_anon *anon;
{
	struct vm_page *pg;

	//UVMHIST_FUNC("uvm_anfree"); UVMHIST_CALLED(maphist);
	//UVMHIST_LOG(maphist,"(anon=0x%x)", anon, 0,0,0);

	KASSERT(anon->an_ref == 0);
	//LOCK_ASSERT(!simple_lock_held(&anon->an_lock));

	/*
	 * get page
	 */

	pg = anon->an_page;

	/*
	 * if there is a resident page and it is loaned, then anon may not
	 * own it.   call out to uvm_anon_lockpage() to ensure the real owner
 	 * of the page has been identified and locked.
	 */

	if (pg && pg->loan_count) {
		simple_lock(&anon->an_lock);
		pg = vm_anon_lockloanpg(anon);
		simple_unlock(&anon->an_lock);
	}

	/*
	 * if we have a resident page, we must dispose of it before freeing
	 * the anon.
	 */

	if (pg) {

		/*
		 * if the page is owned by a uobject (now locked), then we must
		 * kill the loan on the page rather than free it.
		 */

		if (pg->object) {
			uvm_lock_pageq();
			KASSERT(pg->loan_count > 0);
			pg->loan_count--;
			pg->anon = NULL;
			uvm_unlock_pageq();
			simple_unlock(&pg->object->lock);
		} else {

			/*
			 * page has no uobject, so we must be the owner of it.
			 */

			KASSERT((pg->flags & PG_RELEASED) == 0);
			simple_lock(&anon->an_lock);
			pmap_page_protect(pg, VM_PROT_NONE);

			/*
			 * if the page is busy, mark it as PG_RELEASED
			 * so that uvm_anon_release will release it later.
			 */

			if (pg->flags & PG_BUSY) {
				pg->flags |= PG_RELEASED;
				simple_unlock(&anon->an_lock);
				return;
			}
			uvm_lock_pageq();
			uvm_pagefree(pg);
			uvm_unlock_pageq();
			simple_unlock(&anon->an_lock);
			//UVMHIST_LOG(maphist, "anon 0x%x, page 0x%x: freed now!", anon, pg, 0, 0);
		}
	}
	if (pg == NULL && anon->an_swslot > 0) {
		/* this page is no longer only in swap. */
		simple_lock(&vm.swap_data_lock);
		KASSERT(vmexp.swpgonly > 0);
		vmexp.swpgonly--;
		simple_unlock(&vm.swap_data_lock);
	}

	/*
	 * free any swap resources.
	 */

	vm_anon_dropswap(anon);

	/*
	 * now that we've stripped the data areas from the anon,
	 * free the anon itself.
	 */

	KASSERT(anon->u.an_page == NULL);
	KASSERT(anon->an_swslot == 0);

	simple_lock(&vm.afreelock);
	anon->an_u.an_nxt = vm.afree;
	vm.afree = anon;
	vmexp.nfreeanon++;
	simple_unlock(&vm.afreelock);
	//UVMHIST_LOG(maphist,"<- done!",0,0,0,0);
}

/*
 * uvm_anon_dropswap:  release any swap resources from this anon.
 *
 * => anon must be locked or have a reference count of 0.
 */
void
vm_anon_dropswap(anon)
	struct avm_anon *anon;
{
	//UVMHIST_FUNC("uvm_anon_dropswap"); UVMHIST_CALLED(maphist);

	if (anon->an_swslot == 0)
		return;

	//UVMHIST_LOG(maphist,"freeing swap for anon %p, paged to swslot 0x%x", anon, anon->an_swslot, 0, 0);
	vm_swap_free(anon->an_swslot, 1);
	anon->an_swslot = 0;
}

/*
 * uvm_anon_lockloanpg: given a locked anon, lock its resident page
 *
 * => anon is locked by caller
 * => on return: anon is locked
 *		 if there is a resident page:
 *			if it has a uobject, it is locked by us
 *			if it is ownerless, we take over as owner
 *		 we return the resident page (it can change during
 *		 this function)
 * => note that the only time an anon has an ownerless resident page
 *	is if the page was loaned from a uvm_object and the uvm_object
 *	disowned it
 * => this only needs to be called when you want to do an operation
 *	on an anon's resident page and that page has a non-zero loan
 *	count.
 */
struct vm_page *
vm_anon_lockloanpg(anon)
	struct avm_anon *anon;
{
	struct vm_page *pg;
	boolean_t locked = FALSE;

	//LOCK_ASSERT(simple_lock_held(&anon->an_lock));

	/*
	 * loop while we have a resident page that has a non-zero loan count.
	 * if we successfully get our lock, we will "break" the loop.
	 * note that the test for pg->loan_count is not protected -- this
	 * may produce false positive results.   note that a false positive
	 * result may cause us to do more work than we need to, but it will
	 * not produce an incorrect result.
	 */

	while (((pg = anon->an_page) != NULL) && pg->loan_count != 0) {

		/*
		 * quickly check to see if the page has an object before
		 * bothering to lock the page queues.   this may also produce
		 * a false positive result, but that's ok because we do a real
		 * check after that.
		 */

		if (pg->object) {
			uvm_lock_pageq();
			if (pg->object) {
				locked =
				    simple_lock_try(&pg->object->lock);
			} else {
				/* object disowned before we got PQ lock */
				locked = TRUE;
			}
			uvm_unlock_pageq();

			/*
			 * if we didn't get a lock (try lock failed), then we
			 * toggle our anon lock and try again
			 */

			if (!locked) {
				simple_unlock(&anon->an_lock);

				/*
				 * someone locking the object has a chance to
				 * lock us right now
				 */

				simple_lock(&anon->an_lock);
				continue;
			}
		}

		/*
		 * if page is un-owned [i.e. the object dropped its ownership],
		 * then we can take over as owner!
		 */

		if (pg->object == NULL && (pg->pqflags & PQ_ANON) == 0) {
			uvm_lock_pageq();
			pg->pqflags |= PQ_ANON;
			pg->loan_count--;
			uvm_unlock_pageq();
		}
		break;
	}
	return(pg);
}

/*
 * page in every anon that is paged out to a range of swslots.
 *
 * swap_syscall_lock should be held (protects anonblock_list).
 */

boolean_t
anon_swap_off(startslot, endslot)
	int startslot, endslot;
{
	struct avm_anonblock *anonblock;

	LIST_FOREACH(anonblock, &anonblock_list, list) {
		int i;

		/*
		 * loop thru all the anons in the anonblock,
		 * paging in where needed.
		 */

		for (i = 0; i < anonblock->count; i++) {
			struct avm_anon *anon = &anonblock->anons[i];
			int slot;

			/*
			 * lock anon to work on it.
			 */

			simple_lock(&anon->an_lock);

			/*
			 * is this anon's swap slot in range?
			 */

			slot = anon->an_swslot;
			if (slot >= startslot && slot < endslot) {
				boolean_t rv;

				/*
				 * yup, page it in.
				 */

				/* locked: anon */
				rv = anon_pagein(anon);
				/* unlocked: anon */

				if (rv) {
					return rv;
				}
			} else {

				/*
				 * nope, unlock and proceed.
				 */

				simple_unlock(&anon->an_lock);
			}
		}
	}
	return FALSE;
}


/*
 * fetch an anon's page.
 *
 * => anon must be locked, and is unlocked upon return.
 * => returns TRUE if pagein was aborted due to lack of memory.
 */

static boolean_t
anon_pagein(anon)
	struct avm_anon *anon;
{
	struct vm_page *pg;
	struct vm_object *uobj;
	int rv;

	/* locked: anon */
	LOCK_ASSERT(simple_lock_held(&anon->an_lock));

	rv = uvmfault_anonget(NULL, NULL, anon);

	/*
	 * if rv == 0, anon is still locked, else anon
	 * is unlocked
	 */

	switch (rv) {
	case 0:
		break;

	case EIO:
	case ERESTART:

		/*
		 * nothing more to do on errors.
		 * ERESTART can only mean that the anon was freed,
		 * so again there's nothing to do.
		 */

		return FALSE;

	default:
		return TRUE;
	}

	/*
	 * ok, we've got the page now.
	 * mark it as dirty, clear its swslot and un-busy it.
	 */

	pg = anon->an_page;
	uobj = pg->object;
	if (anon->an_swslot > 0)
		uvm_swap_free(anon->an_swslot, 1);
	anon->an_swslot = 0;
	pg->flags &= ~(PG_CLEAN);

	/*
	 * deactivate the page (to put it on a page queue)
	 */

	pmap_clear_reference(pg);
	uvm_lock_pageq();
	if (pg->wire_count == 0)
		uvm_pagedeactivate(pg);
	uvm_unlock_pageq();

	if (pg->flags & PG_WANTED) {
		wakeup(pg);
		pg->flags &= ~(PG_WANTED);
	}

	/*
	 * unlock the anon and we're done.
	 */

	simple_unlock(&anon->an_lock);
	if (uobj) {
		simple_unlock(&uobj->lock);
	}
	return FALSE;
}

/*
 * uvm_anon_release: release an anon and its page.
 *
 * => caller must lock the anon.
 */

void
uvm_anon_release(anon)
	struct avm_anon *anon;
{
	struct vm_page *pg = anon->an_page;

//	LOCK_ASSERT(simple_lock_held(&anon->an_lock));

	KASSERT(pg != NULL);
	KASSERT((pg->flags & PG_RELEASED) != 0);
	KASSERT((pg->flags & PG_BUSY) != 0);
	KASSERT(pg->uobject == NULL);
	KASSERT(pg->uanon == anon);
	KASSERT(pg->loan_count == 0);
	KASSERT(anon->an_ref == 0);

	uvm_lock_pageq();
	uvm_pagefree(pg);
	uvm_unlock_pageq();
	simple_unlock(&anon->an_lock);

	KASSERT(anon->u.an_page == NULL);

	vm_anfree(anon);
}
