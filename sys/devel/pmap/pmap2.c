/*
 * pmap2.c
 *
 *  Created on: 13 Mar 2020
 *      Author: marti
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/user.h>

#include <vm/include/vm.h>
#include <vm/include/vm_kern.h>
#include <vm/include/vm_page.h>

#include <arch/i386/include/param.h>
#include <arch/i386/include/pmap.h>
//#include <sys/msgbuf.h>
#include "devel/pmap/cpt.h"

/*
 * All those kernel PT submaps that BSD is so fond of
 */
struct pte			*CMAP1, *CMAP2, *mmap;
caddr_t				CADDR1, CADDR2, vmmap;
struct pte			*msgbufmap;
struct msgbuf		*msgbufp;

#define	pmap_pde(m, v)					(&((m)->pm_pdir[((vm_offset_t)(v) >> PD_SHIFT)&1023]))

#define	LOWPTDI		1 					/* No PAE */

extern u_long 		KERNend;			/* phys addr end of kernel (just after bss) */

/* New For PDE & PTE Mapping */
extern u_long 		physfree;			/* phys addr of next free page */
extern u_long 		KCPTphys;			/* phys addr of kernel clustered page table */
extern u_long 		KCPTsegs;			/* segmented phys addr *
extern u_long 		IdleCPT;
extern cpt_entry_t 	*KCPTmap;			/* Kernel Clustered Page Table Mapping */
extern cpte_entry_t *KCPTEmap;			/* Kernel Clustered Page Table Entry Mapping */

#define NPGPTD		4					/* Num of pages for page directory */

static u_long
allocsegments(u_int cnt, u_long *physfree)
{
	u_long res;

	res = *physfree;
	*physfree += SEGMENT_SIZE * cnt;
	bzero((void *)res, SEGMENT_SIZE * cnt);
	return (res);
}

static u_long
allocpages(u_int cnt, u_long *physfree)
{
	u_long res;

	res = *physfree;
	*physfree += PAGE_SIZE * cnt;
	bzero((void *)res, PAGE_SIZE * cnt);
	return (res);
}

static void
allocate_kern_cpt(cpt, cpte)
	cpt_entry_t *cpt;
	cpte_entry_t *cpte;
{
	u_long a;

	/* Allocate Kernel Page Tables */
	KCPTphys = allocpages(NKPT, &physfree);

	/* Allocate Page Table Directory */
	IdleCPT = allocpages(NPGPTD, &physfree);

	/* Allocate Kernel Segment Tables */
	KCPTsegs = allocsegments(NKST, &physfree);

	allocpages(1, &physfree);
	proc0paddr = allocpages(P0_KSTACK_PAGES, &physfree);

	/* Install page tables into PTD. */
	for (a = 0; a < NKPT; a++) {
		IdleCPT = (KCPTphys + ptoa(a)) | PG_V | PG_RW | PG_A | PG_M;
		cpt_add(cpt, cpte, IdleCPT);
	}
	KCPTmap = cpt_lookup(cpt, IdleCPT);
}

static void
allocate_kern_cpte(pte, entry, boff)
	pt_entry_t *pte;
    unsigned long entry;
    int boff;
{
    /* Install CPTE's */
    if(KCPTmap != NULL) {
    	KCPTEmap = cpt_lookup_cpte(KCPTmap, VPBN(entry));
        cpte_add(KCPTEmap, pte, boff);
    }
}

static void
pmap_cold_map(cpte, pa, va, cnt)
	cpte_entry_t *cpte;
	u_long pa, va, cnt;
{
	pt_entry_t *pt;

	for (pt = (pt_entry_t *)KCPTphys + atop(va); cnt > 0; cnt--, pt++, va += PAGE_SIZE, pa += PAGE_SIZE) {
		*pt = pa | PG_V | PG_RW | PG_A | PG_M;
		allocate_kern_cpte(cpte, pt, cnt);
	}
}

static void
pmap_cold_mapident(cpte, pa, cnt)
	cpte_entry_t *cpte;
	u_long pa, cnt;
{
	pmap_cold_map(cpte, pa, pa, cnt);
}

void
pmap_cold()
{
	cpt_entry_t *cpt;
	cpte_entry_t *cpte;
	pd_entry_t *pd;
	pt_entry_t *pt;
	u_int cr3, ncr4;

	RB_INIT(&cpt_root);
	RB_INIT(&cpte_root);

	cpt = &cpt_base[0];
	cpte = &cpte_base[0];

	/* Kernel Clustered Page Table */
	pd = pmap_get_pde(cpt, 0);
	pt = pmap_get_pte(cpte, 0);

	allocate_kern_cpt(cpt, cpte);

	pmap_cold_mapident(cpte, 0, atop(NBPDR) * LOWPTDI);
	pmap_cold_map(cpte, 0, NBPDR * LOWPTDI, atop(NBPDR) * LOWPTDI);
	pmap_cold_mapident(KERNBASE, atop(KERNend - KERNBASE));

	pmap_cold_mapident(cpte, IdleCPT, NPGPTD);

	/* Map early KPTmap.  It is really pmap_cold_mapident. */
	pmap_cold_map(cpte, KCPTphys, KCPTmap->cpt_pa_addr, KCPTmap->cpt_va_addr, NKPT);

	/* Map proc0addr */
	pmap_cold_mapident(cpte, proc0paddr, P0_KSTACK_PAGES);
}

pd_entry_t
pmap_get_pde(cpt, vpbn)
	cpt_entry_t *cpt;
	u_long vpbn;
{
	pd_entry_t *pd;

	if(cpt != NULL) {
		pd = cpt_to_pde(cpt, vpbn);
		return (pd);
	}
	return (NULL);
}

pt_entry_t
pmap_get_pte(cpte, boff)
	cpte_entry_t *cpte;
	int boff;
{
	pt_entry_t *pt;

	if(cpte != NULL) {
		pt = cpte_to_pte(cpte, boff);
		return (pt);
	}
	return (NULL);
}

/*
 * TODO:
 * - Line 176: Does this address equal IdleCPT?
 * - Will it need a cpt lookup function to find that address?
 * - What does pm_ptab do? (Little to no references in 4.4BSD Kernel)
 */
extern vm_offset_t	atdevbase;
void
pmap_bootstrap(firstaddr, loadaddr)
	vm_offset_t firstaddr;
	vm_offset_t loadaddr;
{
	vm_offset_t va;
	struct pte *pte;
	extern vm_offset_t maxmem, physmem;

	avail_start = firstaddr;
	avail_end = maxmem << PG_SHIFT;

	/* XXX: allow for msgbuf */
	avail_end -= i386_round_page(sizeof(struct msgbuf));

	mem_size = physmem << PG_SHIFT;
	virtual_avail = atdevbase + 0x100000 - 0xa0000 + 10 * NBPG;
	virtual_end = VM_MAX_KERNEL_ADDRESS;
	i386pagesperpage = PAGE_SIZE / I386_PAGE_SIZE;

	/*
	 * Initialize protection array.
	 */
	i386_protection_init();

	kernel_pmap.pm_cpt = cpt_lookup(cpt_base, (0xfe000000 + IdleCPT));

	simple_lock_init(&kernel_pmap->pm_lock);
	kernel_pmap->pm_count = 1;
/*
 * Allocate all the submaps we need
 */
#define	SYSMAP(c, p, v, n)	\
		v = (c)va; va += ((n)*I386_PAGE_SIZE); p = pte; pte += (n);

	va = virtual_avail;
	pte = pmap_pte(kernel_pmap, va);

	SYSMAP(caddr_t, CMAP1, CADDR1, 1);
	SYSMAP(caddr_t, CMAP2, CADDR2, 1);
	SYSMAP(caddr_t, mmap, vmmap, 1);
	SYSMAP(struct msgbuf *, msgbufmap, msgbufp, 1);

	virtual_avail = va;
}


void
pmap_pinit(pmap)
	register struct pmap *pmap;
{

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_CREATE))
		pg("pmap_pinit(%x)\n", pmap);
#endif
	pmap->pm_cpt = (cpt_entry_t *)kmem_alloc(kernel_map, NBPG);

	pmap->pm_count = 1;
	simple_lock_init(&pmap->pm_lock);
}
