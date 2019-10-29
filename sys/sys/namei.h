/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)namei.h	1.3 (2.11BSD) 1997/1/18
 */

#ifndef _NAMEI_
#define	_NAMEI_

/*
 * Encapsulation of namei parameters.
 * One of these is located in the u. area to
 * minimize space allocated on the kernel stack.
 */
struct nameidata {
	caddr_t	ni_dirp;			/* pathname pointer */
	short	ni_nameiop;			/* see below */
	short	ni_error;			/* error return if any */
	off_t	ni_endoff;			/* end of useful stuff in directory */
	struct	inode *ni_pdir;		/* inode of parent directory of dirp */
	struct	inode *ni_ip;		/* inode of dirp */
	enum	uio_seg	ni_segflg;	/* segment flag */
	off_t	ni_offset;			/* offset in directory */
	u_short	ni_count;			/* offset of open slot (off_t?) */
	struct	direct ni_dent;		/* current directory entry */

	/* Arguments to lookup */
	struct	vnode *ni_startdir;	/* starting directory */
	struct	vnode *ni_rootdir;	/* logical root directory */

	/* Results: returned from/manipulated by lookup  */
	struct	vnode *ni_vp;		/* vnode of result */
	struct	vnode *ni_dvp;		/* vnode of intermediate directory */

	struct componentname {
		/* Arguments to lookup */
		u_long	cn_nameiop;		/* namei operation */
		u_long	cn_flags;		/* flags to namei */
		struct	proc *cn_proc;	/* process requesting lookup */
		struct	ucred *cn_cred;	/* credentials */

		/* Shared between lookup and commit routines */
		char	*cn_pnbuf;		/* pathname buffer */
		char	*cn_nameptr;	/* pointer to looked up name */
		long	cn_namelen;		/* length of looked up component */
	} ni_cnd;
};

#ifdef KERNEL
/*
 * namei operations and modifiers, stored in ni_cnd.flags
 */
#define	LOOKUP		0		/* perform name lookup only */
#define	CREATE		1		/* setup for file creation */
#define	DELETE		2		/* setup for file deletion */
#define	RENAME		3		/* setup for file renaming */
#define	OPMASK		3		/* mask for operation */


#define	LOCKLEAF	0x0004	/* lock inode on return */
#define	LOCKPARENT	0x0008	/* want parent vnode returned locked */
#define	WANTPARENT	0x0010	/* want parent vnode returned unlocked */
#define NOCACHE		0x0020	/* name must not be left in cache */
#define FOLLOW		0x0040	/* follow symbolic links */
#define	NOFOLLOW	0x0000	/* don't follow symbolic links (pseudo) */
#define	MODMASK		0x00fc	/* mask of operational modifiers */


#define	NDINIT(ndp, op, flags, segflg, namep) {\
	(ndp)->ni_nameiop = op | flags; \
	(ndp)->ni_segflg = segflg; \
	(ndp)->ni_dirp = namep; \
	}
#ifdef VNODE
#define VNDINIT(ndp, op, flags, segflg, namep, p) { \
	(ndp)->ni_cnd.cn_nameiop = op; \
	(ndp)->ni_cnd.cn_flags = flags; \
	(ndp)->ni_segflg = segflg; \
	(ndp)->ni_dirp = namep; \
	(ndp)->ni_cnd.cn_proc = p; \
}
#endif
#endif

/*
 * This structure describes the elements in the cache of recent
 * names looked up by namei.
 */

#define	NCHNAMLEN	31	/* maximum name segment length we bother with */

struct	namecache {
	struct	namecache *nc_forw;	/* hash chain, MUST BE FIRST */
	struct	namecache *nc_back;	/* hash chain, MUST BE FIRST */
	struct	namecache *nc_nxt;	/* LRU chain */
	struct	namecache **nc_prev;	/* LRU chain */
	struct	inode *nc_ip;		/* inode the name refers to */
	ino_t	nc_ino;			/* ino of parent of name */
	dev_t	nc_dev;			/* dev of parent of name */
	dev_t	nc_idev;		/* dev of the name ref'd */
	u_short	nc_id;			/* referenced inode's id */
	char	nc_nlen;		/* length of name */
	char	nc_name[NCHNAMLEN];	/* segment name */

	struct	vnode *nc_dvp;		/* vnode of parent of name */
	u_long	nc_dvpid;			/* capability number of nc_dvp */
	struct	vnode *nc_vp;		/* vnode the name refers to */
	u_long	nc_vpid;			/* capability number of nc_vp */
};

/*
 * Stats on usefulness of namei caches.
 */
struct	nchstats {
	long	ncs_goodhits;		/* hits that we can reall use */
	long	ncs_badhits;		/* hits we must drop */
	long	ncs_falsehits;		/* hits with id mismatch */
	long	ncs_miss;			/* misses */
	long	ncs_long;			/* long names that ignore cache */
	long	ncs_pass2;			/* names found with passes == 2 */
	long	ncs_2passes;		/* number of times we attempt it */

	struct	vnode *nc_dvp;		/* vnode of parent of name */
	struct	vnode *nc_vp;		/* vnode the name refers to */
};
#endif
