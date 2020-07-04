/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)ufs_subr.c	1.5 (2.11BSD GTE) 1996/9/13
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/mount.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include "vfs/ufs211/ufs211_fs.h"
#include "vfs/ufs211/ufs211_inode.h"
#include "vfs/ufs211/ufs211_mount.h"
#include "vfs/ufs211/ufs211_quota.h"
#include "vfs/ufs211/ufs211_extern.h"

/*
 * Flush all the blocks associated with an inode.
 * There are two strategies based on the size of the file;
 * large files are those with more than (nbuf / 2) blocks.
 * Large files
 * 	Walk through the buffer pool and push any dirty pages
 *	associated with the device on which the file resides.
 * Small files
 *	Look up each block in the file to see if it is in the 
 *	buffer pool writing any that are found to disk.
 *	Note that we make a more stringent check of
 *	writing out any block in the buffer pool that may
 *	overlap the inode. This brings the inode up to
 *	date with recent mods to the cooked device.
 */
void
syncip(ip)
	struct ufs211_inode *ip;
{
	register struct buf *bp;
	register struct buf *lastbufp;
	long lbn, lastlbn;
	register int s;
	ufs211_daddr_t blkno;

	lastlbn = howmany(ip->i_size, DEV_BSIZE);
	if (lastlbn < nbuf / 2) {
		for (lbn = 0; lbn < lastlbn; lbn++) {
			blkno = fsbtodb(bmap(ip, lbn, B_READ, 0));
			blkflush(ip->i_dev, blkno);
		}
	} else {
		lastbufp = &buf[nbuf];
		for (bp = buf; bp < lastbufp; bp++) {
			if (bp->b_dev != ip->i_dev ||
			    (bp->b_flags & B_DELWRI) == 0)
				continue;
			s = splbio();
			if (bp->b_flags & B_BUSY) {
				bp->b_flags |= B_WANTED;
				sleep((caddr_t)bp, PRIBIO+1);
				splx(s);
				bp--;
				continue;
			}
			splx(s);
			notavail(bp);
			bwrite(bp);
		}
	}
	ip->i_flag |= UFS211_ICHG;
	iupdat(ip, &time, &time, 1);
}

/*
 * Check that a specified block number is in range.
 */
int
badblock(fp, bn)
	register struct ufs211_fs *fp;
	ufs211_daddr_t bn;
{

	if (bn < fp->fs_isize || bn >= fp->fs_fsize) {
		printf("bad block %D, ",bn);
		fserr(fp, "bad block");
		return (1);
	}
	return (0);
}

/*
 * Getfs maps a device number into a pointer to the incore super block.
 *
 * The algorithm is a linear search through the mount table. A
 * consistency check of the super block magic number is performed.
 *
 * panic: no fs -- the device is not mounted.
 *	this "cannot happen"
 */
struct ufs211_fs *
getfs(dev)
	ufs211_dev_t dev;
{
	register struct ufs211_mount *mp;
	register struct ufs211_fs *fs;

	for (mp = &mount[0]; mp < &mount[NMOUNT]; mp++) {
		if (mp->m_inodp == NULL || mp->m_dev != dev)
			continue;
		fs = &mp->m_filsys;
		if (fs->fs_nfree > UFS211_NICFREE || fs->fs_ninode > UFS211_NICINOD) {
			fserr(fs, "bad count");
			fs->fs_nfree = fs->fs_ninode = 0;
		}
		return(fs);
	}
	printf("no fs on dev %u/%u\n",major(dev), minor(dev));
	return((struct ufs211_fs *) NULL);
}

#ifdef QUOTA
/*
 * Getfsx returns the index in the file system
 * table of the specified device.  The swap device
 * is also assigned a pseudo-index.  The index may
 * be used as a compressed indication of the location
 * of a block, recording
 *	<getfsx(dev),blkno>
 * rather than
 *	<dev, blkno>
 * provided the information need remain valid only
 * as long as the file system is mounted.
 */
int
getfsx(dev)
	ufs211_dev_t dev;
{
	register struct ufs211_mount *mp;

	if (dev == swapdev)
		return (0377);
	for(mp = &mount[0]; mp < &mount[NMOUNT]; mp++)
		if (mp->m_dev == dev)
			return (mp - &mount[0]);
	return (-1);
}
#endif
