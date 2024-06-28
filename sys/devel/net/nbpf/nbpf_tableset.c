/*	$NetBSD: npf_tableset.c,v 1.9.2.8 2013/02/11 21:49:48 riz Exp $	*/

/*-
 * Copyright (c) 2009-2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This material is based upon work partially supported by The
 * NetBSD Foundation under a contract with Mindaugas Rasiukevicius.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * NPF tableset module.
 *
 * Notes
 *
 *	The tableset is an array of tables.  After the creation, the array
 *	is immutable.  The caller is responsible to synchronise the access
 *	to the tableset.  The table can either be a hash or a tree.  Its
 *	entries are protected by a read-write lock.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_tableset.c,v 1.9.2.8 2013/02/11 21:49:48 riz Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/atomic.h>
#include <sys/fnv_hash.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/rwlock.h>
#include <sys/systm.h>
#include <sys/types.h>


#include <sys/tree.h>
#include <sys/rwlock.h>
#include <sys/mutex.h>
#include <sys/stdbool.h>
#include <net/if.h>

#include "lpm.h"

#include "ptree.h"
#include "nbpf.h"
//#include "nbpf_ncode.h"

/*
 * Table structures.
 */

struct nbpf_tblent {
	union {
		LIST_ENTRY(nbpf_tblent) 	hashq;		/* hash */
		LIST_ENTRY(nbpf_tblent) 	listent;	/* lpm */
		pt_node_t				node;
	} te_entry;
	uint16_t				te_preflen;
	int						te_alen;
	nbpf_addr_t				te_addr;
};

LIST_HEAD(nbpf_hashl, nbpf_tblent);

struct nbpf_table {
	char				t_name[16];
	/* Lock and reference count. */
	struct rwlock		t_lock;
	u_int				t_refcnt;
	/* Total number of items. */
	u_int				t_nitems;
	/* Table ID. */
	u_int				t_id;
	/* The storage type can be: a) hash b) tree. */
	int					t_type;
	struct nbpf_hashl 	*t_hashl;
	u_long				t_hashmask;
	/* Separate trees for IPv4 and IPv6. */
	pt_tree_t			t_tree[2];

	struct nbpf_hashl 	t_list;
	lpm_t 				*t_lpm;
};

/* Table types. */
#define	NBPF_TABLE_LPM			1
#define	NBPF_TABLE_HASH			2
#define	NBPF_TABLE_TREE			3

#define	NBPF_TABLE_SLOTS		32

#define	NBPF_ADDRLEN2TREE(alen)	((alen) >> 4)

static size_t hashsize;
static nbpf_tblent_t		tblent_cache;

#define nbpf_tableset_malloc(size)		(nbpf_malloc(size, M_NBPF, M_NOWAIT))
#define nbpf_tableset_free(addr)		(nbpf_free(addr, M_NBPF))

void
nbpf_tableset_sysinit(void)
{
	nbpf_malloc(&tblent_cache, sizeof(nbpf_tblent_t), M_NBPF, M_WAITOK);
	KASSERT(tblent_cache != NULL);
}

void
nbpf_tableset_sysfini(void)
{
	nbpf_free(&tblent_cache, M_NBPF);
}

nbpf_tableset_t *
nbpf_tableset_create(void)
{
	const size_t sz = NBPF_TABLE_SLOTS * sizeof(nbpf_table_t *);

	return nbpf_tableset_malloc(sz);
}

void
nbpf_tableset_destroy(nbpf_tableset_t *tblset)
{
	const size_t sz = NBPF_TABLE_SLOTS * sizeof(nbpf_table_t *);
	nbpf_table_t *t;
	u_int tid;

	/*
	 * Destroy all tables (no references should be held, as ruleset
	 * should be destroyed before).
	 */
	for (tid = 0; tid < NBPF_TABLE_SLOTS; tid++) {
		t = tblset[tid];
		if (t && atomic_dec_uint_nv(&t->t_refcnt) == 0) {
			nbpf_table_destroy(t);
		}
	}
	nbpf_tableset_free(tblset);
}

/*
 * npf_tableset_insert: insert the table into the specified tableset.
 *
 * => Returns 0 on success.  Fails and returns error if ID is already used.
 */
int
nbpf_tableset_insert(nbpf_tableset_t *tblset, nbpf_table_t *t)
{
	const u_int tid = t->t_id;
	int error;

	KASSERT((u_int)tid < NBPF_TABLE_SLOTS);

	if (tblset[tid] == NULL) {
		atomic_inc_uint(&t->t_refcnt);
		tblset[tid] = t;
		error = 0;
	} else {
		error = EEXIST;
	}
	return error;
}

/*
 * npf_tableset_reload: iterate all tables and if the new table is of the
 * same type and has no items, then we preserve the old one and its entries.
 *
 * => The caller is responsible for providing synchronisation.
 */
void
nbpf_tableset_reload(nbpf_tableset_t *ntset, nbpf_tableset_t *otset)
{
	for (int i = 0; i < NBPF_TABLE_SLOTS; i++) {
		nbpf_table_t *t = ntset[i], *ot = otset[i];

		if (t == NULL || ot == NULL) {
			continue;
		}
		if (t->t_nitems || t->t_type != ot->t_type) {
			continue;
		}

		/*
		 * Acquire a reference since the table has to be kept
		 * in the old tableset.
		 */
		atomic_inc_uint(&ot->t_refcnt);
		ntset[i] = ot;

		/* Only reference, never been visible. */
		t->t_refcnt--;
		nbpf_table_destroy(t);
	}
}

/*
 * Few helper routines.
 */

static nbpf_tblent_t *
table_hash_lookup(const nbpf_table_t *t, const nbpf_addr_t *addr, const int alen, struct nbpf_hashl **rhtbl)
{
	const uint32_t hidx = fnv_32_buf(addr, alen, FNV1_32_INIT);
	struct nbpf_hashl *htbl = &t->t_hashl[hidx & t->t_hashmask];
	nbpf_tblent_t *ent;

	/*
	 * Lookup the hash table and check for duplicates.
	 * Note: mask is ignored for the hash storage.
	 */
	LIST_FOREACH(ent, htbl, te_entry.hashq) {
		if (ent->te_alen != alen) {
			continue;
		}
		if (memcmp(&ent->te_addr, addr, alen) == 0) {
			break;
		}
	}
	*rhtbl = htbl;
	return ent;
}

static nbpf_tblent_t *
table_lpm_lookup(const nbpf_table_t *t, const nbpf_addr_t *addr, const int alen, struct nbpf_hashl **rlist)
{
	struct nbpf_hashl *list = &t->t_list;
	nbpf_tblent_t *ent;

	LIST_FOREACH(ent, list, te_entry.listent) {
		if ((&ent->te_addr == addr) && (&ent->te_alen == alen)) {
			if (lpm_lookup(t->t_lpm, addr, alen) != NULL) {
				break;
			}
			continue;
		}
		if (lpm_lookup(t->t_lpm, addr, alen) != NULL) {
			break;
		}
	}
	*rlist = list;
	return ent;
}

static void
table_tree_destroy(pt_tree_t *tree)
{
	nbpf_tblent_t *ent;

	while ((ent = ptree_iterate(tree, NULL, PT_ASCENDING)) != NULL) {
		ptree_remove_node(tree, ent);
		nbpf_tableset_free(ent);
	}
}

static void
table_lpm_destroy(nbpf_table_t *t)
{
	nbpf_tblent_t *ent;

	while ((ent = LIST_FIRST(&t->t_list)) != NULL) {
		LIST_REMOVE(ent, te_entry.listent);
		nbpf_tableset_free(ent);
	}
	lpm_clear(t->t_lpm, NULL, NULL);
	t->t_nitems = 0;
}

/*
 * npf_table_create: create table with a specified ID.
 */
nbpf_table_t *
nbpf_table_create(u_int tid, int type, size_t hsize)
{
	nbpf_table_t *t;

	KASSERT((u_int)tid < NBPF_TABLE_SLOTS);

	t = nbpf_tableset_malloc(sizeof(nbpf_table_t));
	switch (type) {
	case NBPF_TABLE_LPM:
		t->t_lpm = lpm_create(M_NOWAIT);
		if (t->t_lpm == NULL) {
			nbpf_tableset_free(t);
			return (NULL);
		}
		LIST_INIT(&t->t_list);
		break;
	case NBPF_TABLE_TREE:
		ptree_init(&t->t_tree[0], &nbpf_table_ptree_ops,
		    (void *)(sizeof(struct in_addr) / sizeof(uint32_t)),
		    offsetof(nbpf_tblent_t, te_entry.node),
		    offsetof(nbpf_tblent_t, te_addr));
		ptree_init(&t->t_tree[1], &nbpf_table_ptree_ops,
		    (void *)(sizeof(struct in6_addr) / sizeof(uint32_t)),
		    offsetof(nbpf_tblent_t, te_entry.node),
		    offsetof(nbpf_tblent_t, te_addr));
		break;
	case NBPF_TABLE_HASH:
		t->t_hashl = hashinit(hsize, M_NBPF, &t->t_hashmask);
		hashsize = hsize;
		if (t->t_hashl == NULL) {
			nbpf_tableset_free(t);
			return NULL;
		}
		break;
	default:
		KASSERT(false);
	}
	rwlock_simple_init(&t->t_lock, "nbpf_table_rwlock");
	t->t_type = type;
	t->t_id = tid;

	return t;
}

/*
 * npf_table_destroy: free all table entries and table itself.
 */
void
nbpf_table_destroy(nbpf_table_t *t)
{
	KASSERT(t->t_refcnt == 0);

	switch (t->t_type) {
	case NBPF_TABLE_LPM:
		table_lpm_destroy(t);
		lpm_destroy(t->t_lpm);
		break;
	case NBPF_TABLE_HASH:
		for (unsigned n = 0; n <= t->t_hashmask; n++) {
			nbpf_tblent_t *ent;
			while ((ent = LIST_FIRST(&t->t_hashl[n])) != NULL) {
				LIST_REMOVE(ent, te_entry.hashq);
				nbpf_tableset_free(ent);
			}
		}
		hashfree(t->t_hashl, hashsize, M_NBPF, t->t_hashmask);
		break;
	case NBPF_TABLE_TREE:
		table_tree_destroy(&t->t_tree[0]);
		table_tree_destroy(&t->t_tree[1]);
		break;
	default:
		KASSERT(false);
	}
	rwlock_unlock(&t->t_lock);
	free(t, M_NBPF);
}

/*
 * npf_table_check: validate ID and type.
 */
int
nbpf_table_check(const nbpf_tableset_t *tset, u_int tid, int type)
{

	if ((u_int)tid >= NBPF_TABLE_SLOTS) {
		return EINVAL;
	}
	if (tset[tid] != NULL) {
		return EEXIST;
	}
	switch (type) {
	case NBPF_TABLE_LPM:
	case NBPF_TABLE_HASH:
	case NBPF_TABLE_TREE:
	default:
		return EINVAL;
	}
	return 0;
}

static int
table_cidr_check(const u_int aidx, const nbpf_addr_t *addr, const nbpf_netmask_t mask)
{

	if (mask > NBPF_MAX_NETMASK && mask != NBPF_NO_NETMASK) {
		return EINVAL;
	}
	if (aidx > 1) {
		return EINVAL;
	}

	/*
	 * For IPv4 (aidx = 0) - 32 and for IPv6 (aidx = 1) - 128.
	 * If it is a host - shall use NPF_NO_NETMASK.
	 */
	if (mask >= (aidx ? 128 : 32) && mask != NBPF_NO_NETMASK) {
		return EINVAL;
	}
	return 0;
}

/*
 * npf_table_insert: add an IP CIDR entry into the table.
 */
int
nbpf_table_insert(nbpf_tableset_t *tset, u_int tid, const int alen, const nbpf_addr_t *addr, const nbpf_netmask_t mask)
{
	const u_int aidx = NBPF_ADDRLEN2TREE(alen);
	nbpf_tblent_t *ent;
	nbpf_table_t *t;
	int error;

	if ((u_int)tid >= NBPF_TABLE_SLOTS || (t = tset[tid]) == NULL) {
		return EINVAL;
	}

	error = table_cidr_check(aidx, addr, mask);
	if (error) {
		return error;
	}
	ent = nbpf_tableset_malloc(sizeof(&tblent_cache));
	memcpy(&ent->te_addr, addr, alen);
	ent->te_alen = alen;

	/*
	 * Insert the entry.  Return an error on duplicate.
	 */
	rw_enter(&t->t_lock, RW_WRITER);
	switch (t->t_type) {
	case NBPF_TABLE_LPM: {
		const unsigned preflen = (mask == NBPF_NO_NETMASK) ? (alen * 8) : mask;
		ent->te_preflen = preflen;

		if (lpm_lookup(t->t_lpm, addr, alen) == NULL
				&& lpm_insert(t->t_lpm, addr, alen, preflen, ent) == 0) {
			LIST_INSERT_HEAD(&t->t_list, ent, te_entry.listent);
			t->t_nitems++;
			error = 0;
		} else {
			error = EEXIST;
		}
		break;
	}
	case NBPF_TABLE_HASH: {
		struct nbpf_hashl *htbl;

		/*
		 * Hash tables by the concept support only IPs.
		 */
		if (mask != NBPF_NO_NETMASK) {
			error = EINVAL;
			break;
		}
		if (!table_hash_lookup(t, addr, alen, &htbl)) {
			LIST_INSERT_HEAD(htbl, ent, te_entry.hashq);
			t->t_nitems++;
		} else {
			error = EEXIST;
		}
		break;
	}
	case NBPF_TABLE_TREE: {
		pt_tree_t *tree = &t->t_tree[aidx];
		bool ok;

		/*
		 * If no mask specified, use maximum mask.
		 */
		ok = (mask != NBPF_NO_NETMASK) ?
		    ptree_insert_mask_node(tree, ent, mask) :
		    ptree_insert_node(tree, ent);
		if (ok) {
			t->t_nitems++;
			error = 0;
		} else {
			error = EEXIST;
		}
		break;
	}
	default:
		KASSERT(false);
	}
	rw_exit(&t->t_lock);

	if (error) {
		nbpf_tableset_free(ent);
	}
	return error;
}

/*
 * npf_table_remove: remove the IP CIDR entry from the table.
 */
int
nbpf_table_remove(nbpf_tableset_t *tset, u_int tid, const int alen, const nbpf_addr_t *addr, const nbpf_netmask_t mask)
{
	const u_int aidx = NBPF_ADDRLEN2TREE(alen);
	nbpf_tblent_t *ent;
	nbpf_table_t *t;
	int error;

	error = table_cidr_check(aidx, addr, mask);
	if (error) {
		return error;
	}

	if ((u_int)tid >= NBPF_TABLE_SLOTS || (t = tset[tid]) == NULL) {
		return EINVAL;
	}

	rwl_lock(&t->t_lock);
	switch (t->t_type) {
	case NBPF_TABLE_LPM:
		ent = lpm_lookup(t->t_lpm, addr, alen);
		if (__predict_true(ent != NULL)) {
			LIST_REMOVE(ent, te_entry.listent);
			lpm_remove(t->t_lpm, &ent->te_addr, ent->te_alen, ent->te_preflen);
			t->t_nitems--;
		} else {
			error = ENOENT;
		}
		break;
	case NBPF_TABLE_HASH: {
		struct nbpf_hashl *htbl;

		ent = table_hash_lookup(t, addr, alen, &htbl);
		if (__predict_true(ent != NULL)) {
			LIST_REMOVE(ent, te_entry.hashq);
			t->t_nitems--;
		}
		break;
	}
	case NBPF_TABLE_TREE: {
		pt_tree_t *tree = &t->t_tree[aidx];

		ent = ptree_find_node(tree, addr);
		if (__predict_true(ent != NULL)) {
			ptree_remove_node(tree, ent);
			t->t_nitems--;
		}
		break;
	}
	default:
		KASSERT(false);
		ent = NULL;
	}
	rwl_unlock(&t->t_lock);

	if (ent == NULL) {
		return ENOENT;
	}
	nbpf_tableset_free(ent);
	return 0;
}

/*
 * npf_table_lookup: find the table according to ID, lookup and match
 * the contents with the specified IP address.
 */
int
nbpf_table_lookup(nbpf_tableset_t *tset, u_int tid, const int alen, const nbpf_addr_t *addr)
{
	const u_int aidx = NPF_ADDRLEN2TREE(alen);
	nbpf_tblent_t *ent;
	nbpf_table_t *t;

	if (__predict_false(aidx > 1)) {
		return EINVAL;
	}

	if ((u_int)tid >= NBPF_TABLE_SLOTS || (t = tset[tid]) == NULL) {
		return EINVAL;
	}

	rwl_lock(&t->t_lock);
	switch (t->t_type) {
	case NBPF_TABLE_LPM: {
		struct nbpf_hashl *list;
		ent = table_lpm_lookup(t->t_lpm, addr, alen, &list);
		break;
	}
	case NBPF_TABLE_HASH: {
		struct nbpf_hashl *htbl;
		ent = table_hash_lookup(t, addr, alen, &htbl);
		break;
	}
	case NBPF_TABLE_TREE: {
		ent = ptree_find_node(&t->t_tree[aidx], addr);
		break;
	}
	default:
		KASSERT(false);
		ent = NULL;
	}
	rwl_unlock(&t->t_lock);

	return ent ? 0 : ENOENT;
}

struct nbpf_ioctl_table {
	int					nct_cmd;
	u_int				nct_tid;
	int					nct_alen;
	nbpf_addr_t			nct_addr;
	nbpf_netmask_t		nct_mask;

	int 				nct_type;
	size_t				nct_hsize;
};
typedef struct nbpf_ioctl_table nbpf_ioctl_table_t;

static int
table_ent_copyout(nbpf_tblent_t *ent, nbpf_netmask_t mask, void *ubuf, size_t len, size_t *off)
{
	void *ubufp = (uint8_t *)ubuf + *off;
	nbpf_ioctl_table_t uent;

	if ((*off += sizeof(nbpf_ioctl_table_t)) > len) {
		return ENOMEM;
	}
	uent.nct_alen = ent->te_alen;
	memcpy(&uent.nct_addr, &ent->te_addr, sizeof(nbpf_addr_t));
	uent.nct_mask = mask;

	return copyout(&uent, ubufp, sizeof(nbpf_ioctl_table_t));
}

static int
table_tree_list(pt_tree_t *tree, nbpf_netmask_t maxmask, void *ubuf, size_t len, size_t *off)
{
	nbpf_tblent_t *ent = NULL;
	int error = 0;

	while ((ent = ptree_iterate(tree, ent, PT_ASCENDING)) != NULL) {
		pt_bitlen_t blen;

		if (!ptree_mask_node_p(tree, ent, &blen)) {
			blen = maxmask;
		}
		error = table_ent_copyout(ent, blen, ubuf, len, off);
		if (error)
			break;
	}
	return error;
}

/*
 * npf_table_list: copy a list of all table entries into a userspace buffer.
 */
int
nbpf_table_list(nbpf_tableset_t *tset, u_int tid, void *ubuf, size_t len)
{
	nbpf_table_t *t;
	size_t off = 0;
	int error = 0;

	if ((u_int)tid >= NBPF_TABLE_SLOTS || (t = tset[tid]) == NULL) {
		return EINVAL;
	}

	rwl_lock(&t->t_lock);
	switch (t->t_type) {
	case NBPF_TABLE_LPM:
		nbpf_tblent_t *ent;

		LIST_FOREACH(ent, &t->t_list, te_entry.listent) {
			if ((error = table_ent_copyout(ent, 0, ubuf, len, &off)) != 0) {
				break;
			}
		}
		break;
	case NBPF_TABLE_HASH:
		for (unsigned n = 0; n <= t->t_hashmask; n++) {
			nbpf_tblent_t *ent;

			LIST_FOREACH(ent, &t->t_hashl[n], te_entry.hashq) {
				if ((error = table_ent_copyout(ent, 0, ubuf, len, &off)) != 0) {
					break;
				}
			}
		}
		break;
	case NBPF_TABLE_TREE:
		error = table_tree_list(&t->t_tree[0], 32, ubuf, len, &off);
		if (error)
			break;
		error = table_tree_list(&t->t_tree[1], 128, ubuf, len, &off);
		break;
	default:
		KASSERT(false);
	}
	rwl_unlock(&t->t_lock);

	return error;
}

/*
#define CREATE	0
#define DESTROY	1
#define LOOKUP	2
#define INSERT	3
#define REMOVE	4
#define CHECK	5

int
nbpf_ioctl()
{

	switch (cmd) {
	case CREATE:
		nbpf_table_create(tid, type, hsize);
		break;
	case DESTROY:
		nbpf_table_destroy(tbl);
		break;
	case LOOKUP:
		nbpf_table_lookup(tset, tid, alen, addr);
		break;
	case INSERT:
		nbpf_table_insert(tset, tid, alen, addr, mask);
		break;
	case REMOVE:
		nbpf_table_remove(tset, tid, alen, addr, mask);
		break;
	case CHECK:
		nbpf_table_check(tset, tid, type);
		break;
	}
}
*/

int
nbpf_mktable(tblset, tid, type, hsize)
	nbpf_tableset_t *tblset;
	u_int tid;
	int type;
	size_t hsize;
{
	nbpf_table_t *tbl;
	int error;

	error = nbpf_table_check(tblset, tid, type);
	if (error != 0) {
		return (error);
	}
	tbl = nbpf_table_create(tid, type, hsize);
	if (tbl == NULL) {
		return (ENOMEM);
	}
	error = nbpf_tableset_insert(tblset, tbl);
	if (error != 0) {
		return (error);
	}
	return (0);
}

nbpf_tableset_t *
nbpf_init_table(tid, type, hsize)
	u_int tid;
	int type;
	size_t hsize;
{
	nbpf_tableset_t *tset;
	int error;

	nbpf_tableset_sysinit();

	tset = nbpf_tableset_create();
	if (tset == NULL) {
		return (NULL);
	}
	error = nbpf_mktable(tset, tid, type, hsize);
	if (error != 0) {
		return (NULL);
	}
	return (tset);
}

nbpf_tcp(tid, alen, addr, mask)
	u_int tid;
	const int alen;
	const nbpf_addr_t *addr;
	const nbpf_netmask_t mask;
{
	nbpf_tableset_t *tblset;
	nbpf_table_t 	*t;

	tblset = nbpf_init_table(tid, NBPF_TABLE_LPM, 1024);

	t = nbpf_table_insert(tblset, tid, alen, addr, mask);
	if (t == NULL) {
		nbpf_table_remove(tblset, tid, alen, addr, mask);
	}
}

struct nbpf_program {
	u_int 				bf_len;
	struct nbpf_insn 	*bf_insns; /* ncode */
};

struct nbpf_d {
	nbpf_tableset_t		*bd_table;

	nbpf_state_t		*bd_state;
	struct nbpf_insn 	*bd_filter;
	int 				bd_layer;
};

#define	NBPF_CMD_TABLE_LOOKUP	1
#define	NBPF_CMD_TABLE_ADD		2
#define	NBPF_CMD_TABLE_REMOVE	3

int
nbpf_table_ioctl(cmd, tid, alen, addr, mask)
{
	npf_tableset_t *tblset;
	int error;

	tblset = nbpf_init_table(tid, NBPF_TABLE_LPM, 1024);

	switch (cmd) {
	case NBPF_CMD_TABLE_LOOKUP:
		error = nbpf_table_lookup(tblset, tid, alen, addr);
		break;
	case NBPF_CMD_TABLE_ADD:
		error = nbpf_table_insert(tblset, tid, alen, addr, mask);
		break;
	case NBPF_CMD_TABLE_REMOVE:
		error = nbpf_table_remove(tblset, tid, alen, addr, mask);
		break;
	default:
		error = EINVAL;
		break;
	}
	return (error);
}

struct bpf_program {
	u_int 				bf_len;
	struct bpf_insn 	*bf_insns;
	struct nbpf_insn 	*bf_nc_insns; /* ncode */
};

struct bpf_d {
	caddr_t				bd_sbuf;		/* store slot */
	struct bpf_insn 	*bd_filter;

	nbpf_state_t		*bd_nc_state;
	struct nbpf_insn 	*bd_nc_filter; 	/* ncode filter */
	int 				bd_nc_layer;
};

struct bpf_if {
	struct bpf_if 		*bif_next;
};

void
nbpf_init_state(d, layer)
	struct bpf_d *d;
	int layer;
{
	nbpf_state_t *state;

	state = (nbpf_state_t *)malloc(sizeof(*state), M_DEVBUF, M_DONTWAIT);

	/* TODO: if no tags set, disable */
	nbpf_set_tag(state, tag);

	d->bd_nc_state = state;
	d->bd_nc_layer = layer;

	/* place in bpf_allocbufs: */
	nbpf_cache_all(state, d->bd_sbuf);
}

int
nbpf_setf(d, fp)
	struct bpf_d *d;
	struct bpf_program *fp;
{
	struct nbpf_insn *ncode, *old;
	u_int nlen, size;
	int error, s;

	old = d->bd_nc_filter;
	if (fp->bf_nc_insns == 0) {
		if (fp->bf_len != 0) {
			return (EINVAL);
		}
		s = splnet();
		d->bd_nc_filter = 0;
		reset_d(d);
		splx(s);
		if (old != 0) {
			free((caddr_t)old, M_DEVBUF);
		}
		return (0);
	}
	nlen = fp->bf_len;

	size = nlen * sizeof(*fp->bf_nc_insns);
	ncode = (struct nbpf_insn *)malloc(size, M_DEVBUF, M_WAITOK);
	if (copyin((caddr_t)fp->bf_nc_insns, (caddr_t)ncode, size) == 0 && nbpf_validate(ncode, (int)nlen, &error)) {
		s = splnet();
		d->bd_nc_filter = ncode;
		reset_d(d);
		splx(s);
		if (old != 0) {
			free((caddr_t)old, M_DEVBUF);
		}
		return (0);
	}
	free((caddr_t)ncode, M_DEVBUF);
	return (EINVAL);
}

void
nbpf_tap(arg, pkt, pktlen)
	caddr_t arg;
	u_char *pkt;
	u_int pktlen;
{
	struct bpf_if *bp;
	struct bpf_d *d;
	u_int slen;

	bp = (struct bpf_if *)arg;
	for (d = bp->bif_dlist; d != 0; d = d->bd_next) {
		++d->bd_rcount;

		slen = nbpf_filter(d->bd_nc_state, d->bd_nc_filter, pktlen, d->bd_nc_layer);
		if (slen != 0) {
			catchpacket(d, pkt, pktlen, slen, memcpy);
		}
	}
}

void
nbpf_mtap(arg, m)
	caddr_t arg;
	struct mbuf *m;
{
	void *(*cpfn)(void *, const void *, size_t);
	struct bpf_if *bp = (struct bpf_if *)arg;
	struct bpf_d *d;
	u_int pktlen, slen, buflen;
	struct mbuf *m0;
	void *marg;

	pktlen = 0;
	for (m0 = m; m0 != 0; m0 = m0->m_next)
		pktlen += m0->m_len;

	if (pktlen == m->m_len) {
		cpfn = memcpy;
		marg = mtod(m, void *);
		buflen = pktlen;
	} else {
		cpfn = bpf_mcpy;
		marg = m;
		buflen = 0;
	}

	for (d = bp->bif_dlist; d != 0; d = d->bd_next) {
		if (!d->bd_seesent && (m->m_pkthdr.rcvif == NULL))
			continue;
		++d->bd_rcount;
		slen = nbpf_filter(d->bd_nc_state, d->bd_nc_filter, pktlen, d->bd_nc_layer);
		if (slen != 0)
			catchpacket(d, marg, pktlen, slen, cpfn);
	}
}
