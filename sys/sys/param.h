/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)param.h	1.6 (2.11BSD) 1999/9/5
 */

#ifndef	_SYS_PARAM_H_
#define	_SYS_PARAM_H_

/*
 * Historic BSD #defines -- probably will remain untouched for all time.
 */
#define	BSD	211		/* 2.11 * 10, as cpp doesn't do floats */

/*
 *	#define __211BSD_Version__ MMmmrrpp00
 *
 *	M = major version
 *	m = minor version; a minor number of 99 indicates current.
 *	r = 0 (*)
 *	p = patchlevel
 *
 * When new releases are made, src/gnu/usr.bin/groff/tmac/mdoc.local
 * needs to be updated and the changes sent back to the groff maintainers.
 *
 * (*)	Up to 2.0I "release" used to be "",A-Z,Z[A-Z] but numeric
 *	    	e.g. 211BSD-1.2D  = 102040000 ('D' == 4)
 *	211BSD-2.0H 	(200080000) was changed on 20041001 to:
 *	2.99.9		(299000900)
 */

#define	__211BSD_Version__	0000000100			/* 211BSD 0.00.01 */

#define __211BSD_Prereq__(M,m,p) (((((M) * 100000000) + \
    (m) * 1000000) + (p) * 100) <= __211BSD_Version__)

#ifndef	NULL
#define	NULL	0
#endif

#ifndef LOCORE
#include <sys/types.h>
#endif
#include <sys/ansi.h>

#include <sys/null.h>
/*
 * Machine-independent constants
 */
#include <sys/syslimits.h>

#define	MAXCOMLEN		16			/* max command name remembered */
#define	MAXINTERP		32			/* max interpreter file name length */
#define	MAXLOGNAME		16			/* max login name length */
#define MAXHOSTNAMELEN	256			/* max hostname size */
#define	NMOUNT			6			/* number of mountable file systems */
#define	MAXUPRC			CHILD_MAX	/* max processes per user */
#define	NOFILE			OPEN_MAX	/* max open files per process */
#define	CANBSIZ			256			/* max size of typewriter line */
#define	NCARGS			ARG_MAX		/* # characters in exec arglist */
#define	NGROUPS			NGROUPS_MAX	/* max number groups */
#define	NOGROUP			65535		/* marker for empty group set member */

/* More types and definitions used throughout the kernel. */
#ifdef KERNEL
#include <sys/cdefs.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/uio.h>
#include <sys/ucred.h>
#include <sys/user.h>
#include <vm/include/vm_param.h>
#endif

/* Signals */
#include <sys/signal.h>

/* Machine type dependent parameters.*/
#include <machine/param.h>
#include <machine/limits.h>

/* alignment */
#define	ALIGNBYTES				_ALIGNBYTES
#define	ALIGN(p)				_ALIGN(p)
#define	ALIGNED_POINTER(p,t)	_ALIGNED_POINTER(p,t)

/*
 * Priorities
 */
#define	PSWP	0
#define	PVM		5
#define	PINOD	10
#define	PRIBIO	20
#define	PRIUBA	24
#define	PZERO	25
#define	PVFS	26
#define	PSOCK	26
#define	PWAIT	30
#define	PLOCK	35
#define	PPAUSE	40
#define	PUSER	50
#define	MAXPRI	127				/* Priorities range from 0 through MAXPRI. */

#define	NZERO	0				/* default "nice" */

#define	PRIMASK	0xff
#define	PCATCH	0x100 			/* OR'd with pri for tsleep to check signals */

#define	NBPW	sizeof(int)		/* number of bytes in an integer */

#define	CMASK	026				/* default mask for file creation */
#define	NODEV	(dev_t)(-1)		/* non-existent device */

/*
 * Clustering of hardware pages on machines with ridiculously small
 * page sizes is done here.  The paging subsystem deals with units of
 * CLSIZE pte's describing NBPG (from machine/machparam.h) pages each.
 */
#define	CLBYTES			(CLSIZE*NBPG)
#define	CLOFSET			(CLBYTES-1) /* for clusters, like PGOFSET */
#define	claligned(x)	((((int)(x))&CLOFSET)==0)
#define	CLOFF			CLOFSET
#define	CLSHIFT			(PGSHIFT + CLSIZELOG2)

#if CLSIZE==1
#define	clbase(i)		(i)
#define	clrnd(i)		(i)
#else
/* Give the base virtual address (first of CLSIZE). */
#define	clbase(i)		((i) &~ (CLSIZE-1))
/* round a number of clicks up to a whole cluster */
#define	clrnd(i)		(((i) + (CLSIZE-1)) &~ ((long)(CLSIZE-1)))
#endif

/* CBLOCK is the size of a clist block, must be power of 2 */
#define	CBLOCK			64									/* Clist block size, must be a power of 2. */
#define CBQSIZE			(CBLOCK/NBBY)						/* Quote bytes/cblock - can do better. */
															/* Data chars/clist. */
#define	CBSIZE			(CBLOCK - sizeof(struct cblock *))	/* data chars/clist */
#define	CROUND			(CBLOCK - 1)						/* clist rounding */

/*
 * File system parameters and macros.
 *
 * The file system is made out of blocks of most MAXBSIZE units.
 */
#define	MAXBSIZE		MAXPHYS
#define MAXFRAG 		8

/*
 * MAXPATHLEN defines the longest permissable path length
 * after expanding symbolic links. It is used to allocate
 * a temporary buffer from the buffer pool in which to do the
 * name expansion, hence should be a power of two, and must
 * be less than or equal to MAXBSIZE.
 * MAXSYMLINKS defines the maximum number of symbolic links
 * that may be expanded in a path name. It should be set high
 * enough to allow all legitimate uses, but halt infinite loops
 * reasonably quickly.
 */
#define MAXPATHLEN	PATH_MAX
#define MAXSYMLINKS	8


/* Bit map related macros. */
#define	setbit(a,i)	((a)[(i)/NBBY] |= 1<<((i)%NBBY))
#define	clrbit(a,i)	((a)[(i)/NBBY] &= ~(1<<((i)%NBBY)))
#define	isset(a,i)	((a)[(i)/NBBY] & (1<<((i)%NBBY)))
#define	isclr(a,i)	(((a)[(i)/NBBY] & (1<<((i)%NBBY))) == 0)

/*
 * Macros for counting and rounding.
 */
#ifndef howmany
#define	howmany(x, y)	(((x)+((y)-1))/(y))
#endif
#define	roundup(x, y)	((((x)+((y)-1))/(y))*(y))
#define roundup2(x, y)  (((x)+((y)-1))&(~((y)-1))) 	/* if y is powers of two */
#define powerof2(x)		((((x)-1)&(x))==0)			/* is x a powerof2 */
#define percent(x, y)	(((x) / 100) * (y)) 		/* calculate the percentage of a value */
#define powertwo(x) 	(1 << (x))					/* returns x to the power of 2 */

/*
 * Macros for fast min/max.
 */
#ifndef KERNEL
#define	MIN(a,b) (((a)<(b))?(a):(b))
#define	MAX(a,b) (((a)>(b))?(a):(b))
#endif

/*
 * MAXMEM is the maximum core per process is allowed.  First number is Kb.
*/
#define	MAXMEM		(300*16)

/*
 * Constants for setting the parameters of the kernel memory allocator.
 *
 * 2 ** MINBUCKET is the smallest unit of memory that will be
 * allocated. It must be at least large enough to hold a pointer.
 *
 * Units of memory less or equal to MAXALLOCSAVE will permanently
 * allocate physical memory; requests for these size pieces of
 * memory are quite fast. Allocations greater than MAXALLOCSAVE must
 * always allocate and free physical memory; requests for these
 * size allocations should be done infrequently as they will be slow.
 *
 * Constraints: CLBYTES <= MAXALLOCSAVE <= 2 ** (MINBUCKET + 14), and
 * MAXALLOCSIZE must be a power of two.
 */
#define MINBUCKET		4				/* 4 => min allocation of 16 bytes */
#define MAXALLOCSAVE	(2 * CLBYTES)

/*
 * Scale factor for scaled integers used to count %cpu time and load avgs.
 *
 * The number of CPU `tick's that map to a unique `%age' can be expressed
 * by the formula (1 / (2 ^ (FSHIFT - 11))).  The maximum load average that
 * can be calculated (assuming 32 bits) can be closely approximated using
 * the formula (2 ^ (2 * (16 - FSHIFT))) for (FSHIFT < 15).
 *
 * For the scheduler to maintain a 1:1 mapping of CPU `tick' to `%age',
 * FSHIFT must be at least 11; this gives us a maximum load avg of ~1024.
 */
#define	FSHIFT	11		/* bits to right of fixed binary point */
#define FSCALE	(1<<FSHIFT)

#ifdef _KERNEL
extern int hz;

/*
 * macro to convert from milliseconds to hz without integer overflow
 * Default version using only 32bits arithmetics.
 * 64bit port can define 64bit version in their <machine/param.h>
 * 0x20000 is safe for hz < 20000
 */
#ifndef mstohz
# ifdef _LP64
#  define mstohz(ms) ((unsigned int)((ms + 0ul) * hz / 1000ul))
# else
static __inline unsigned int
mstohz(unsigned int ms)
{
	return __predict_false(ms >= 0x20000u) ?
	    (ms / 1000u) * hz : (ms * hz) / 1000u;
}
# endif
#endif
#endif /* _KERNEL */

#endif /* _SYS_PARAM_H_ */
