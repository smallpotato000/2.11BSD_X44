/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)ufs_alloc.c	1.3 (2.11BSD GTE) 1996/9/19
 */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/mount.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include "devel/ufs211/ufs211_dir.h"
#include "devel/ufs211/ufs211_fs.h"
#include "devel/ufs211/ufs211_inode.h"
#include "devel/ufs211/ufs211_quota.h"

typedef	struct ufs211_fblk *FBLKP;

/*
 * Allocate a block in the file system.
 *
 * alloc will obtain the next available free disk block from the
 * free list of the specified device.  The super block has up to
 * NICFREE remembered free blocks; the last of these is read to
 * obtain NICFREE more...
 */
struct buf *
balloc(ip, flags)
	struct ufs211_inode *ip;
	int flags;
{
	register struct ufs211_fs *fs;
	register struct buf *bp;
	int async;
	ufs211_daddr_t bno;

	fs = ip->i_fs;
	async = fs->fs_flags & MNT_ASYNC;

	while (fs->fs_flock)
		sleep((caddr_t) &fs->fs_flock, PINOD);
	do {
		if (fs->fs_nfree <= 0)
			goto nospace;
		if (fs->fs_nfree > UFS211_NICFREE) {
			fserr(fs, "bad free count");
			goto nospace;
		}
		bno = fs->fs_free[--fs->fs_nfree];
		if (bno == 0)
			goto nospace;
	} while (badblock(fs, bno));
	if (fs->fs_nfree <= 0) {
		fs->fs_flock++;
		bp = bread(ip->i_dev, bno);
		if (((bp->b_flags & B_ERROR) == 0) && (bp->b_resid == 0)) {
			register struct ufs211_fblk *fbp;
			ufs211_mapin(bp);
			fbp = (FBLKP) bp;
			*((FBLKP) &fs->fs_nfree) = *fbp;
			ufs211_mapout(bp);
		}
		brelse(bp);
		/*
		 * Write the superblock back, synchronously if requested,
		 * so that the free list pointer won't point at garbage.
		 * We can still end up with dups in free if we then
		 * use some of the blocks in this freeblock, then crash
		 * without a sync.
		 */
		bp = getblk(ip->i_dev, UFS211_SUPERB);
		fs->fs_fmod = 0;
		fs->fs_time = time.tv_sec;
		{
			register struct fs *fps;

			ufs211_mapin(bp);
			fps = (struct fs *)bp;
			*fps = *fs;
		}
		ufs211_mapout(bp);
		if (!async)
			bwrite(bp);
		else
			bdwrite(bp);
		fs->fs_flock = 0;
		wakeup((caddr_t) &fs->fs_flock);
		if (fs->fs_nfree <= 0)
			goto nospace;
	}
	bp = getblk(ip->i_dev, bno);
	bp->b_resid = 0;
	if (flags & B_CLRBUF)
		clrbuf(bp);
	fs->fs_fmod = 1;
	fs->fs_tfree--;
	return (bp);

nospace:
	fs->fs_nfree = 0;
	fs->fs_tfree = 0;
	fserr(fs, "file system full");
	/*
	 * THIS IS A KLUDGE...
	 * SHOULD RATHER SEND A SIGNAL AND SUSPEND THE PROCESS IN A
	 * STATE FROM WHICH THE SYSTEM CALL WILL RESTART
	 */
	uprintf("\n%s: write failed, file system full\n", fs->fs_fsmnt);
	{
		register int i;

		for (i = 0; i < 5; i++)
			sleep((caddr_t) &lbolt, PRIBIO);
	}
	u->u_error = ENOSPC;
	return (NULL);
}

/*
 * Allocate an inode in the file system.
 *
 * Allocate an unused I node on the specified device.  Used with file
 * creation.  The algorithm keeps up to NICINOD spare I nodes in the
 * super block.  When this runs out, a linear search through the I list
 * is instituted to pick up NICINOD more.
 */
struct inode *
ialloc(pip)
	struct ufs211_inode *pip;
{
	register struct ufs211_fs *fs;
	register struct buf *bp;
	register struct ufs211_inode *ip;
	int i;
	struct ufs211_dinode *dp;
	ufs211_ino_t ino;
	ufs211_daddr_t adr;
	ufs211_ino_t inobas;
	int first;
	struct vnode *vp;
	char	*emsg = "no inodes free";

	vp = UFS211_ITOV(ip);
	fs = pip->i_fs;
	while (fs->fs_ilock)
		sleep((caddr_t)&fs->fs_ilock, PINOD);
#ifdef QUOTA
	QUOTAMAP();
	u->u_error = chkiq(pip->i_dev, NULL, u->u_uid, 0);
	QUOTAUNMAP();
	if (u->u_error)
		return(NULL);
#endif
loop:
	if (fs->fs_ninode > 0) {
		ino = fs->fs_inode[--fs->fs_ninode];
		if (ino <= UFS211_ROOTINO)
			goto loop;
		ip = iget(pip->i_dev, fs, ino);
		if (ip == NULL)
			return(NULL);
		if (ip->i_mode == 0) {
			bzero((caddr_t)ip->i_addr,sizeof(ip->i_addr));
			ip->i_flags = 0;
			fs->fs_fmod = 1;
			fs->fs_tinode--;
			return(ip);
		}
		/*
		 * Inode was allocated after all.
		 * Look some more.
		 */
		vput(vp);
		goto loop;
	}
	fs->fs_ilock++;
	if (fs->fs_nbehind < 4 * UFS211_NICINOD) {
		first = 1;
		ino = fs->fs_lasti;
#ifdef DIAGNOSTIC
		if (itoo(ino))
			panic("ialloc");
#endif DIAGNOSTIC
		adr = itod(ino);
	} else {
fromtop:
		first = 0;
		ino = 1;
		adr = UFS211_SUPERB + 1;
		fs->fs_nbehind = 0;
	}
	for (; adr < fs->fs_isize; adr++) {
		inobas = ino;
		bp = bread(pip->i_dev, adr);
		if ((bp->b_flags & B_ERROR) || bp->b_resid) {
			brelse(bp);
			ino += INOPB;
			continue;
		}
		ufs211_mapin(bp);
		dp = (struct ufs211_dinode *) bp;
		for (i = 0; i < INOPB; i++) {
			if (dp->di_icom1.ic_mode != 0)
				goto cont;
			if (ifind(pip->i_dev, ino))
				goto cont;
			fs->fs_inode[fs->fs_ninode++] = ino;
			if (fs->fs_ninode >= UFS211_NICINOD)
				break;
			cont: ino++;
			dp++;
		}
		ufs211_mapout(bp);
		brelse(bp);
		if (fs->fs_ninode >= UFS211_NICINOD)
			break;
	}
	if (fs->fs_ninode < UFS211_NICINOD && first)
		goto fromtop;
	fs->fs_lasti = inobas;
	fs->fs_ilock = 0;
	wakeup((caddr_t) &fs->fs_ilock);
	if (fs->fs_ninode > 0)
		goto loop;
	fserr(fs, emsg);
	uprintf("\n%s: %s\n", fs->fs_fsmnt, emsg);
	u->u_error = ENOSPC;
	return (NULL);
}

/*
 * Free a block or fragment.
 *
 * Place the specified disk block back on the free list of the
 * specified device.
 */
free(ip, bno)
	struct ufs211_inode *ip;
	ufs211_daddr_t bno;
{
	register struct ufs211_fs *fs;
	register struct buf *bp;
	struct ufs211_fblk *fbp;

	fs = ip->i_fs;
	if (badblock(fs, bno)) {
		printf("bad block %D, ino %d\n", bno, ip->i_number);
		return;
	}
	while (fs->fs_flock)
		sleep((caddr_t)&fs->fs_flock, PINOD);
	if (fs->fs_nfree <= 0) {
		fs->fs_nfree = 1;
		fs->fs_free[0] = 0;
	}
	if (fs->fs_nfree >= UFS211_NICFREE) {
		fs->fs_flock++;
		bp = getblk(ip->i_dev, bno);
		ufs211_mapin(bp);
		fbp = (FBLKP) bp;
		*fbp = *((FBLKP)&fs->fs_nfree);
		ufs211_mapout(bp);
		fs->fs_nfree = 0;
		if (fs->fs_flags & MNT_ASYNC)
			bdwrite(bp);
		else
			bwrite(bp);
		fs->fs_flock = 0;
		wakeup((caddr_t)&fs->fs_flock);
	}
	fs->fs_free[fs->fs_nfree++] = bno;
	fs->fs_tfree++;
	fs->fs_fmod = 1;
}

/*
 * Free an inode.
 *
 * Free the specified I node on the specified device.  The algorithm
 * stores up to NICINOD I nodes in the super block and throws away any more.
 */
void
ifree(ip, ino)
	struct ufs211_inode *ip;
	ufs211_ino_t ino;
{
	register struct ufs211_fs *fs;

	fs = ip->i_fs;
	fs->fs_tinode++;
	if (fs->fs_ilock)
		return;
	if (fs->fs_ninode >= UFS211_NICINOD) {
		if (fs->fs_lasti > ino)
			fs->fs_nbehind++;
		return;
	}
	fs->fs_inode[fs->fs_ninode++] = ino;
	fs->fs_fmod = 1;
}

/*
 * Fserr prints the name of a file system with an error diagnostic.
 *
 * The form of the error message is:
 *	fs: error message
 */
void
fserr(fp, cp)
	struct ufs211_fs *fp;
	char *cp;
{
	printf("%s: %s\n", fp->fs_fsmnt, cp);
}
