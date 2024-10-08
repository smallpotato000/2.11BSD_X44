/*
 * Copyright (c) 1982, 1986, 1989, 1991, 1993, 1995
 *	The Regents of the University of California.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)ufs_ihash.c	8.7 (Berkeley) 5/17/95
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/user.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufs_extern.h>

/*
 * Structures associated with inode cacheing.
 */
LIST_HEAD(ihashhead, inode) 	*ihashtbl;
u_long							ihash;		/* size of hash table - 1 */
#define	INOHASH(device, inum)	(&ihashtbl[((device) + (inum)) & ihash])
struct lock_object 				ufs_ihash_slock;

/*
 * Initialize inode hash table.
 */
void
ufs_ihashinit()
{
	ihashtbl = hashinit(desiredvnodes, M_UFSMNT, &ihash);
	simple_lock_init(&ufs_ihash_slock, "ufs_ihash_slock");
}

/*
 * Use the device/inum pair to find the incore inode, and return a pointer
 * to it. If it is in core, return it, even if it is locked.
 */
struct vnode *
ufs_ihashlookup(dev, inum)
	dev_t dev;
	ino_t inum;
{
	struct inode *ip;

	simple_lock(&ufs_ihash_slock);
	for (ip = LIST_FIRST(INOHASH(dev, inum)); ip; ip = LIST_NEXT(ip, i_hash))
		if (inum == ip->i_number && dev == ip->i_dev)
			break;
	simple_unlock(&ufs_ihash_slock);

	if (ip)
		return (ITOV(ip));
	return (NULLVP);
}

/*
 * Use the device/inum pair to find the incore inode, and return a pointer
 * to it. If it is in core, but locked, wait for it.
 */
struct vnode *
ufs_ihashget(dev, inum)
	dev_t dev;
	ino_t inum;
{
	struct proc *p = curproc;	/* XXX */
	struct inode *ip;
	struct vnode *vp;

loop:
	simple_lock(&ufs_ihash_slock);
	for (ip = LIST_FIRST(INOHASH(dev, inum)); ip; ip = LIST_NEXT(ip, i_hash)) {
		if (inum == ip->i_number && dev == ip->i_dev) {
			vp = ITOV(ip);
			simple_lock(&vp->v_interlock);
			simple_unlock(&ufs_ihash_slock);
			if (vget(vp, LK_EXCLUSIVE | LK_INTERLOCK, p))
				goto loop;
			return (vp);
		}
	}
	simple_unlock(&ufs_ihash_slock);
	return (NULL);
}

/*
* Insert the inode into the hash table, and return it locked.
 */
void
ufs_ihashins(ip)
	struct inode *ip;
{
	struct proc *p = curproc;		/* XXX */
	struct ihashhead *ipp;

	/* lock the inode, then put it on the appropriate hash list */
	lockmgr(&ip->i_lock, LK_EXCLUSIVE, (struct lock_object *)0, p->p_pid);

	simple_lock(&ufs_ihash_slock);
	ipp = INOHASH(ip->i_dev, ip->i_number);
	LIST_INSERT_HEAD(ipp, ip, i_hash);
	simple_unlock(&ufs_ihash_slock);
}

/*
 * Remove the inode from the hash table.
 */
void
ufs_ihashrem(ip)
	struct inode *ip;
{
	struct inode *iq;

	simple_lock(&ufs_ihash_slock);
	LIST_REMOVE(ip, i_hash);
#ifdef DIAGNOSTIC
	LIST_NEXT(ip, i_hash) = NULL;
	LIST_PREV(ip, i_hash) = NULL;
#endif
	simple_unlock(&ufs_ihash_slock);
}
