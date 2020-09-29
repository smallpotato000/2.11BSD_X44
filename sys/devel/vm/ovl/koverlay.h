/*
 * koverlay.h
 *
 *  Created on: 25 Apr 2020
 *      Author: marti
 *	3-Clause BSD License
 */

/* Current Kernel Overlays:
 * - Akin to a more advanced & versatile overlay from original 2.11BSD
 * - Allocates memory from kernelspace (thus vm allocation)
 * - While useful (in some situations), for extending memory management
 * - This version would not provide any benefit to vm paging, if the overlay is a page in itself.
 * - would likely be slower than using just paging.
 */

#ifndef SYS_KOVERLAY_H_
#define SYS_KOVERLAY_H_


#include <sys/queue.h>

struct ovlstats {
	long			os_inuse;		/* # of packets of this type currently in use */
	long			os_calls;		/* total packets of this type ever allocated */
	long 			os_memuse;		/* total memory held in bytes */
	u_short			os_limblocks;	/* number of times blocked for hitting limit */
	u_short			os_mapblocks;	/* number of times blocked for kernel map */
	long			os_maxused;		/* maximum number ever used */
	long			os_limit;		/* most that are allowed to exist */
	long			os_size;		/* sizes of this thing that are allocated */
	long			os_spare;

	int 			slotsfilled;
	int 			slotsfree;
	int 			nslots;
};

struct ovlusage {
	short 			ou_indx;		/* bucket index */
	u_short 		ou_kovlcnt;		/* kernel overlay count */
	u_short 		ou_vovlcnt;		/* vm overlay count */
};

CIRCLEQ_HEAD(ovlbucket_head, ovlbuckets);
struct ovlbuckets {
	struct ovltree          		*ob_ztree;			/* Pointer ovltree */
	unsigned long            		ob_bsize;			/* bucket size */
	long                     		ob_bindx;			/* bucket indx */
	boolean_t                		ob_bspace;			/* bucket contains a tree: Default = False or 0 */

	caddr_t 						ob_next;			/* list of free blocks */
	caddr_t 						ob_last;			/* last free block */

	long							ob_calls;			/* total calls to allocate this size */
	long							ob_total;			/* total number of blocks allocated */
	long							ob_totalfree;		/* # of free elements in this bucket */
	long							ob_elmpercl;		/* # of elements in this sized allocation */
	long							ob_highwat;			/* high water mark */
	long							ob_couldfree;		/* over high water mark and could free */
};

struct asl {
	struct asl 						*asl_next;
	struct asl 						*asl_prev;
	unsigned long  					asl_size;
	caddr_t 						asl_addr;
	long							asl_spare0;
	long							asl_spare1;
	short							asl_type;
};

/* Overlay Flags */
#define OVL_ALLOCATED  (1 < 0) 						/* kernel overlay region allocated */

extern struct ovlstats 	ovlstats[];
extern struct ovlusage 	*ovlusage;
extern char *vovlbase, 	*kovlbase;
extern struct ovlbuckets bucket[];

#endif /* SYS_KOVERLAY_H_ */
