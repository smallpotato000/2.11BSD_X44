/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)null_subr.c	8.7 (Berkeley) 5/14/95
 *
 * $Id: lofs_subr.c,v 1.11 1992/05/30 10:05:43 jsp Exp jsp $
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/malloc.h>

#include <ufs/ufml/ufml.h>
#include <ufs/ufml/ufml_extern.h>
#include <ufs/ufml/ufml_meta.h>
#include <ufs/ufml/ufml_ops.h>

#define LOG2_SIZEVNODE 7		/* log2(sizeof struct vnode) */
#define	NUFMLNODECACHE 16

/*
 * Null layer cache:
 * Each cache entry holds a reference to the lower vnode
 * along with a pointer to the alias vnode.  When an
 * entry is added the lower vnode is VREF'd.  When the
 * alias is removed the lower vnode is vrele'd.
 */

#define	UFML_NHASH(vp) (&ufml_node_hashtbl[(((u_long)vp)>>LOG2_SIZEVNODE) & ufml_node_hash])
LIST_HEAD(ufml_node_hashhead, ufml_node) *ufml_node_hashtbl;
u_long ufml_node_hash;

/*
 * Initialise cache headers
 */
int
ufmlfs_init(vfsp)
	struct vfsconf *vfsp;
{
#ifdef UFMLFS_DIAGNOSTIC
	printf("ufmlfs_init\n");		/* printed during system boot */
#endif

	ufml_node_hashtbl = hashinit(NUFMLNODECACHE, M_CACHE, &ufml_node_hash);

	return (0);
}

/*
 * Return a VREF'ed alias for lower vnode if already exists, else 0.
 */
static struct vnode *
ufml_node_find(mp, lowervp)
	struct mount *mp;
	struct vnode *lowervp;
{
	struct proc *p = curproc;	/* XXX */
	struct ufml_node_hashhead *hd;
	struct ufml_node *a;
	struct vnode *vp;

	/*
	 * Find hash base, and then search the (two-way) linked
	 * list looking for a null_node structure which is referencing
	 * the lower vnode.  If found, the increment the null_node
	 * reference count (but NOT the lower vnode's VREF counter).
	 */
	hd = UFML_NHASH(lowervp);
loop:
	for (a = LIST_FIRST(hd); a != 0; a = LIST_NEXT(a, ufml_cache)) {
		if (a->ufml_lowervp == lowervp && UFMLTOV(a)->v_mount == mp) {
			vp = UFMLTOV(a);
			/*
			 * We need vget for the VXLOCK
			 * stuff, but we don't want to lock
			 * the lower node.
			 */
			if (vget(vp, 0, p)) {
				printf ("ufml_node_find: vget failed.\n");
				goto loop;
			};
			return (vp);
		}
	}

	return (NULL);
}

/*
 * Make a new null_node node.
 * Vp is the alias vnode, ufmlvp is the lower vnode.
 * Maintain a reference to (lowervp).
 */
static int
ufml_node_alloc(mp, lowervp, vpp)
	struct mount *mp;
	struct vnode *lowervp;
	struct vnode **vpp;
{
	struct ufml_node_hashhead *hd;
	struct ufml_node *xp;
	struct vnode *othervp, *vp;
	int error;

	if (error == getnewvnode(VT_UFML, mp, &ufml_vnodeops, vpp))
		return (error);
	vp = *vpp;

	MALLOC(xp, struct ufml_node *, sizeof(struct ufml_node), M_TEMP, M_WAITOK);
	vp->v_type = lowervp->v_type;
	xp->ufml_vnode = vp;
	vp->v_data = xp;
	xp->ufml_lowervp = lowervp;
	/*
	 * Before we insert our new node onto the hash chains,
	 * check to see if someone else has beaten us to it.
	 * (We could have slept in MALLOC.)
	 */
	if (othervp == ufml_node_find(lowervp)) {
		FREE(xp, M_TEMP);
		vp->v_type = VBAD;	/* node is discarded */
		vp->v_usecount = 0;	/* XXX */
		*vpp = othervp;
		return (0);
	};
	VREF(lowervp);   /* Extra VREF will be vrele'd in null_node_create */
	hd = UFML_NHASH(lowervp);
	LIST_INSERT_HEAD(hd, xp, ufml_cache);
	return (0);
}

/*
 * Try to find an existing null_node vnode refering
 * to it, otherwise make a new null_node vnode which
 * contains a reference to the lower vnode.
 */
int
ufml_node_create(mp, lowervp, newvpp)
	struct mount *mp;
	struct vnode *lowervp;
	struct vnode **newvpp;
{
	struct vnode *aliasvp;

	if (aliasvp == ufml_node_find(mp, lowervp)) {
		/*
		 * null_node_find has taken another reference
		 * to the alias vnode.
		 */
#ifdef UFMLFS_DIAGNOSTIC
		vprint("ufml_node_create: exists", UFMLTOV(ap));
#endif
		/* VREF(aliasvp); --- done in null_node_find */
	} else {
		int error;

		/*
		 * Get new vnode.
		 */
#ifdef UFMLFS_DIAGNOSTIC
		printf("ufml_node_create: create new alias vnode\n");
#endif

		/*
		 * Make new vnode reference the null_node.
		 */
		if (error == ufml_node_alloc(mp, lowervp, &aliasvp))
			return error;

		/*
		 * aliasvp is already VREF'd by getnewvnode()
		 */
	}

	vrele(lowervp);

#ifdef DIAGNOSTIC
	if (lowervp->v_usecount < 1) {
		/* Should never happen... */
		vprint ("ufml_node_create: alias ", aliasvp);
		vprint ("ufml_node_create: lower ", lowervp);
		panic ("ufml_node_create: lower has 0 usecount.");
	};
#endif

#ifdef UFMLFS_DIAGNOSTIC
	vprint("ufml_node_create: alias", aliasvp);
	vprint("ufml_node_create: lower", lowervp);
#endif

	*newvpp = aliasvp;
	return (0);
}

#ifdef UFMLFS_DIAGNOSTIC
struct vnode *
ufml_checkvp(vp, fil, lno)
	struct vnode *vp;
	char *fil;
	int lno;
{
	struct ufml_node *a = VTOUFML(vp);
#ifdef notyet
	/*
	 * Can't do this check because vop_reclaim runs
	 * with a funny vop vector.
	 */
	if (vp->v_op != ufml_vnodeops) {
		printf ("ufml_checkvp: on non-null-node\n");
		while (ufml_checkvp_barrier) /*WAIT*/ ;
		panic("ufml_checkvp");
	};
#endif
	if (a->ufml_lowervp == NULL) {
		/* Should never happen */
		int i; u_long *p;
		printf("vp = %x, ZERO ptr\n", vp);
		for (p = (u_long *) a, i = 0; i < 8; i++)
			printf(" %x", p[i]);
		printf("\n");
		/* wait for debugger */
		while (ufml_checkvp_barrier) /*WAIT*/ ;
		panic("ufml_checkvp");
	}
	if (a->ufml_lowervp->v_usecount < 1) {
		int i; u_long *p;
		printf("vp = %x, unref'ed lowervp\n", vp);
		for (p = (u_long *) a, i = 0; i < 8; i++)
			printf(" %x", p[i]);
		printf("\n");
		/* wait for debugger */
		while (ufml_checkvp_barrier) /*WAIT*/ ;
		panic ("ufml with unref'ed lowervp");
	};
#ifdef notyet
	printf("ufml %x/%d -> %x/%d [%s, %d]\n",
	        UFMLTOV(a), UFMLTOV(a)->v_usecount,
		a->ufml_lowervp, a->ufml_lowervp->v_usecount,
		fil, lno);
#endif
	return (a->ufml_lowervp);
}
#endif
