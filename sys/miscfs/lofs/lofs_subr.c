/*
 * Copyright (c) 1992 The Regents of the University of California
 * Copyright (c) 1990, 1992 Jan-Simon Pendry
 * All rights reserved.
 *
 * This code is derived from software donated to Berkeley by
 * Jan-Simon Pendry.
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
 *	@(#)lofs_subr.c	7.1 (Berkeley) 7/12/92
 *
 * $Id: lofs_subr.c,v 1.11 1992/05/30 10:05:43 jsp Exp jsp $
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/malloc.h>
#include <miscfs/lofs/lofs.h>

#define LOG2_SIZEVNODE 	7		/* log2(sizeof struct vnode) */
#define	NLOFSCACHE 		16
#define	LOFS_NHASH(vp) 	((((u_long)vp)>>LOG2_SIZEVNODE) & (NLOFSCACHE-1))

/*
 * Loopback cache:
 * Each cache entry holds a reference to the target vnode
 * along with a pointer to the alias vnode.  When an
 * entry is added the target vnode is VREF'd.  When the
 * alias is removed the target vnode is vrele'd.
 */

/*
 * Cache head
 */
struct lofscache;
static LIST_HEAD(lofscache, lofsnode) lofscache[NLOFSCACHE];

/*
 * Initialise cache headers
 */
void
lofs_init()
{
	struct lofscache *ac;
#ifdef LOFS_DIAGNOSTIC
	printf("lofs_init\n");		/* printed during system boot */
#endif

	int i;
	for(i = 0; i < NLOFSCACHE; i++) {
		LIST_INIT(&lofscache[i]);
	}
}

/*
 * Compute hash list for given target vnode
 */
static struct lofscache *
lofs_hash(targetvp)
	struct vnode *targetvp;
{
	struct lofscache *locache;

	locache = &lofscache[LOFS_NHASH(targetvp)];

	return (locache);
}

/*
 * Make a new lofsnode node.
 * Vp is the alias vnode, lofsvp is the target vnode.
 * Maintain a reference to (targetvp).
 */
static void
lofs_alloc(vp, targetvp)
	struct vnode *vp;
	struct vnode *targetvp;
{
	struct lofscache *hd;
	struct lofsnode *a;

#ifdef LOFS_DIAGNOSTIC
	printf("lofs_alloc(%x, %x)\n", vp, targetvp);
#endif

	MALLOC(a, struct lofsnode *, sizeof(struct lofsnode), M_TEMP, M_WAITOK);
	a->a_vnode = vp;
	vp->v_type = targetvp->v_type;
	vp->v_data = a;
	VREF(targetvp);
	a->a_lofsvp = targetvp;
	hd = lofs_hash(targetvp);

#ifdef LOFS_DIAGNOSTIC
	vprint("alloc vp", vp);
	vprint("alloc targetvp", targetvp);
#endif
	LIST_INSERT_HEAD(hd, a, a_hash);
}

#ifdef LOFS_DIAGNOSTIC
void
lofs_flushmp(mp)
	struct mount *mp;
{
	int i = 0;
	struct lofsnode *ac, *roota;

	printf("lofs_flushmp(%x)\n", mp);

	roota = LOFSP(VFSTOLOFS(mp)->rootvp);

	LIST_FOREACH(ac, &lofscache, a_hash) {
		struct lofsnode *a = LIST_NEXT(ac, a_hash);
		while(a != ac) {
			if (a != roota && a->a_vnode->v_mount == mp) {
				struct vnode *vp = a->a_lofsvp;
				if (vp) {
					a->a_lofsvp = 0;
					vprint("would vrele", vp);
					/*vrele(vp);*/
					i++;
				}
			}
			a = LIST_NEXT(a, a_hash);
		}
	}

	if (i > 0)
		printf("lofsnode: vrele'd %d aliases\n", i);
}
#endif

/*
 * Return alias for target vnode if already exists, else 0.
 */
static struct lofsnode *
lofs_find(mp, targetvp)
	struct mount *mp;
	struct vnode *targetvp;
{
	struct lofscache *hd;
	struct lofsnode *a;

#ifdef LOFS_DIAGNOSTIC
	printf("lofs_find(mp = %x, target = %x)\n", mp, targetvp);
#endif

	/*
	 * Find hash base, and then search the (two-way) linked
	 * list looking for a lofsnode structure which is referencing
	 * the target vnode.  If found, the increment the lofsnode
	 * reference count (but NOT the target vnode's VREF counter).
	 */
	hd = lofs_hash(targetvp);

	for (a = LIST_FIRST(hd); a != (struct lofsnode *) hd; a = LIST_NEXT(a, a_hash)) {
		if (a->a_lofsvp == targetvp && a->a_vnode->v_mount == mp) {
#ifdef LOFS_DIAGNOSTIC
			printf("lofs_find(%x): found (%x,%x)->%x\n",
				targetvp, mp, a->a_vnode, targetvp);
#endif
			return (a);
		}
	}

#ifdef LOFS_DIAGNOSTIC
	printf("lofs_find(%x, %x): NOT found\n", mp, targetvp);
#endif

	return (0);
}

static int
lofs_alias(mp, targetvp, newvpp)
	struct mount *mp;
	struct vnode *targetvp;
	struct vnode **newvpp;
{
	struct lofsnode *ap;
	struct vnode *aliasvp;

	if (targetvp->v_type != VDIR || targetvp->v_op == lofs_vnodeops) {
		*newvpp = targetvp;
		//return (0);
	}

	ap = lofs_find(mp, targetvp);

	if (ap) {
		/*
		 * Take another reference to the alias vnode
		 */
#ifdef LOFS_DIAGNOSTIC
		vprint("lofs_alias: exists", ap->a_vnode);
#endif
		aliasvp = ap->a_vnode;
		VREF(aliasvp);
	} else {
		int error;

		/*
		 * Get new vnode.
		 */
#ifdef LOFS_DIAGNOSTIC
		printf("lofs_alias: create new alias vnode\n");
#endif
		if (error == getnewvnode(VT_LOFS, mp, lofs_vnodeops, &aliasvp))
			return (error);	/* XXX: VT_LOFS above */

		/*
		 * Must be a directory
		 */
		aliasvp->v_type = VDIR;

		/*
		 * Make new vnode reference the lofsnode.
		 */
		lofs_alloc(aliasvp, targetvp);

		/*
		 * aliasvp is already VREF'd by getnewvnode()
		 */
	}

	vrele(targetvp);

#ifdef LOFS_DIAGNOSTIC
	vprint("lofs_alias alias", aliasvp);
	vprint("lofs_alias target", targetvp);
#endif

	*newvpp = aliasvp;
	return (0);
}

/*
 * Try to find an existing lofsnode vnode refering
 * to it, otherwise make a new lofsnode vnode which
 * contains a reference to the target vnode.
 */
int
make_lofs(mp, vpp)
	struct mount *mp;
	struct vnode **vpp;
{
	int error;
	struct vnode *aliasvp;
	struct vnode *targetvp;

#ifdef LOFS_DIAGNOSTIC
	printf("make_lofs(mp = %x, vp = %x\n", mp, *vpp);
#endif

	/*
	 * (targetvp) is locked at this point.
	 */
	targetvp = *vpp;

#ifdef LOFS_DIAGNOSTIC
	if (targetvp == 0)
		panic("make_lofs: null vp");
#endif

	/*
	 * Try to find an existing reference to the target vnodes.
	 */
	return (lofs_alias(mp, targetvp, vpp));
}

#ifdef LOFS_DIAGNOSTIC
struct vnode *
lofs_checkvp(vp, fil, lno)
	struct vnode *vp;
	char *fil;
	int lno;
{
	struct lofsnode *a = LOFSP(vp);
	if (a->a_lofsvp == 0) {
		int i; u_long *p;
		printf("vp = %x, ZERO ptr\n", vp);
#ifdef notdef
		for (p = (u_long *) a, i = 0; i < 8; i++)
			printf(" %x", p[i]);
		printf("\n");
		DELAY(2000000);
		panic("lofs_checkvp");
#endif
	}
	printf("aliasvp %x/%d -> %x/%d [%s, %d]\n",
		a->a_vnode, a->a_vnode->v_usecount,
		a->a_lofsvp, a->a_lofsvp ? a->a_lofsvp->v_usecount : -42,
		fil, lno);
	return a->a_lofsvp;
}
#endif
