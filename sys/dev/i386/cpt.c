/*
 * cpt.c
 *
 *  Created on: 13 Mar 2020
 *      Author: marti
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/fnv_hash.h>
#include <sys/tree.h>

#include "cpt.h"

struct cpt cpt_base[NBPG];
struct cpte cpte_base[NCPTE];

/*
 * Clustered Page Table & Entries (Red-Black Tree) Computation & Hash Function
 */
int
cpt_cmp(cpt1, cpt2)
    struct cpt *cpt1, *cpt2;
{
    if(cpt1->cpt_hindex > cpt2->cpt_hindex) {
        return (1);
    } else if(cpt1->cpt_hindex < cpt2->cpt_hindex) {
        return (-1);
    } else {
        return (0);
    }
}

int
cpte_cmp(cpte1, cpte2)
    struct cpte *cpte1, *cpte2;
{
    if(cpte1->cpte_boff > cpte2->cpte_boff) {
        return (1);
    } else if(cpte1->cpte_boff < cpte2->cpte_boff) {
        return (-1);
    } else {
        return (0);
    }
}

/* Virtual Page Block Number */
unsigned int
VPBN(entry)
    vm_offset_t entry;
{
	char val[] = { (char) entry };
    unsigned int hash1 = (fnv_32_str(val, NBPG) % NBPG);
    unsigned int hash2 = (jenkins(val, NBPG) % NBPG);

    if(hash1 != hash2) {
        return (hash1);
    } else if(hash1 == hash2) {
        return ((hash1 + hash2) % NBPG);
    } else {
        return (hash2);
    }
}

RB_PROTOTYPE(cpt_rbtree, cpt, cpt_entry, cpt_cmp);
RB_GENERATE(cpt_rbtree, cpt, cpt_entry, cpt_cmp);
RB_PROTOTYPE(cpte_rbtree, cpte, cpte_entry, cpte_cmp);
RB_GENERATE(cpte_rbtree, cpte, cpte_entry, cpte_cmp);

/*
 * Clustered Page Table (Red-Black Tree) Functions
 */
/* Add a Clustered Page Table Entry */
void
cpt_add(cpt, cpte, vpbn)
    struct cpt *cpt;
    struct cpte *cpte;
    vm_offset_t vpbn;
{
    cpt = &cpt_base[VPBN(vpbn)];
    cpt->cpt_addr = vpbn;
    cpt->cpt_hindex = VPBN(vpbn);
    cpt->cpt_cpte = cpte;
    RB_INSERT(cpt_rbtree, &cpt_root, cpt);
}

/* Search Clustered Page Table for an Entry */
struct cpt *
cpt_lookup(cpt, vpbn)
    struct cpt *cpt;
    vm_offset_t vpbn;
{
    struct cpt *result;
    if(&cpt[VPBN(vpbn)] != NULL) {
        result = &cpt[VPBN(vpbn)];
        return (RB_FIND(cpt_rbtree, &cpt_root, result));
    } else {
        return (NULL);
    }
}

/* Remove an Entry from the Clustered Page Table */
void
cpt_remove(cpt, vpbn)
    struct cpt *cpt;
	vm_offset_t vpbn;
{
    struct cpt *result = cpt_lookup(cpt, vpbn);
    RB_REMOVE(cpt_rbtree, &cpt_root, result);
}

struct cpte *
cpt_lookup_cpte(cpt, vpbn)
    struct cpt *cpt;
	vm_offset_t vpbn;
{
    return (cpt_lookup(cpt, vpbn)->cpt_cpte);
}

/* (WIP) Clustered Page Table Superpage Support */
void
cpt_add_superpage(cpt, cpte, vpbn, sz, pad)
    struct cpt *cpt;
    struct cpte *cpte;
    vm_offset_t vpbn;
    unsigned long sz, pad;
{
    cpt = &cpt_base[VPBN(vpbn)];
    cpt->cpt_sz = sz;
    cpt->cpt_pad = pad;

    cpt_add(cpt, cpte, vpbn);
}

/* (WIP) Clustered Page Table Partial-Subblock Support */
void
cpt_add_partial_subblock(cpt, cpte, vpbn, pad)
    struct cpt *cpt;
    struct cpte *cpte;
    vm_offset_t vpbn;
    unsigned long pad;
{
    cpt = &cpt_base[VPBN(vpbn)];
    cpt->cpt_pad = pad;

    cpt_add(cpt, cpte, vpbn);
}

/*
 * Clustered Page Table (Red-Black Tree) Entry Functions
 */
void
cpte_add(cpte, pte, boff)
    struct cpte *cpte;
    struct pte *pte;
    int boff;
{
    if(boff <= NCPTE) {
        cpte = &cpte_base[boff];
        cpte->cpte_boff = boff;
        cpte->cpte_pte = pte;
        RB_INSERT(cpte_rbtree, &cpte_root, cpte);
    } else {
        printf("%s\n", "This Clustered Page Table Entry is Full");
    }
}

struct cpte *
cpte_lookup(cpte, boff)
    struct cpte *cpte;
    int boff;
{
    struct cpte *result;
    if(&cpte[boff] != NULL) {
        result = &cpte[boff];
        return (RB_FIND(cpte_rbtree, &cpte_root, result));
    } else {
        return (NULL);
    }
}

void
cpte_remove(cpte, boff)
    struct cpte *cpte;
    int boff;
{
    struct cpte *result = cpte_lookup(cpte, boff);
    RB_REMOVE(cpte_rbtree, &cpte_root, result);
}

struct pte *
cpte_lookup_pte(cpte, boff)
    struct cpte *cpte;
    int boff;
{
    return (cpte_lookup(cpte, boff)->cpte_pte);
}
