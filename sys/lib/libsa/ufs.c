/*-
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * The Mach Operating System project at Carnegie-Mellon University.
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
 *	@(#)ufs.c	8.2 (Berkeley) 11/30/93
 *
 *
 * Copyright (c) 1990, 1991 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Author: David Golub
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

/*
 *	Stand-alone file reading package.
 */

#include <sys/param.h>
#include <sys/time.h>
#include <ufs/ffs/fs.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/dir.h>

#include <libsa/stand.h>
#include <libsa/ufs.h>

/*
 * In-core open file.
 */
struct file {
	off_t			f_seekp;				/* seek pointer */
	struct fs		*f_fs;					/* pointer to super-block */
	struct inode	*f_ip;					/* copy of on-disk inode */

	int				f_nindir[NIADDR];		/* number of blocks mapped by indirect block at level i */
	char			*f_blk[NIADDR];			/* buffer for indirect block at level i */
	u_int			f_blksize[NIADDR]; 		/* size of buffer */
	daddr_t			f_blkno[NIADDR];		/* disk address of block in buffer */
	char			*f_buf;					/* buffer for data block */
	u_int			f_buf_size;				/* size of data block */
	daddr_t			f_buf_blkno;			/* block number of data block */
};

/*
 * Read a new inode into a file structure.
 */
static int
read_inode(inumber, f)
	ino_t inumber;
	struct open_file *f;
{
	register struct file *fp = (struct file *)f->f_fsdata;
	register struct fs *fs = fp->f_fs;
	char *buf;
	u_int rsize;
	int rc;

	/*
	 * Read inode and save it.
	 */
	buf = alloc(fs->fs_bsize);
	rc = (f->f_dev->dv_strategy)(f->f_devdata, F_READ,
		fsbtodb(fs, ino_to_fsba(fs, inumber)), fs->fs_bsize,
		buf, &rsize);
	if (rc)
		goto out;
	if (rsize != fs->fs_bsize) {
		rc = EIO;
		goto out;
	}

	{
		if(I_IS_UFS1_MOUNTED(fp->f_ip)) {
			struct ufs1_dinode *dp = (struct ufs1_dinode *)buf;
			DIP(fp->f_ip, dp[ino_to_fsbo(fs, inumber)]);
		} else {
			struct ufs2_dinode *dp = (struct ufs2_dinode *)buf;
			DIP(fp->f_ip, dp[ino_to_fsbo(fs, inumber)]);
		}
	}

	/*
	 * Clear out the old buffers
	 */
	{
		register int level;

		for (level = 0; level < NIADDR; level++)
			fp->f_blkno[level] = -1;
		fp->f_buf_blkno = -1;
	}
out:
	free(buf, fs->fs_bsize);
	return (0);
}

/*
 * Given an offset in a file, find the disk block number that
 * contains that block.
 */
static int
block_map(f, file_block, disk_block_p)
	struct open_file *f;
	daddr_t file_block;
	daddr_t *disk_block_p;	/* out */
{
	register struct file *fp = (struct file *)f->f_fsdata;
	register struct fs *fs = fp->f_fs;
	int level;
	int idx;
	daddr_t ind_block_num;
	daddr_t *ind_p;
	int rc;

	/*
	 * Index structure of an inode:
	 *
	 * di_db[0..NDADDR-1]	hold block numbers for blocks
	 *			0..NDADDR-1
	 *
	 * di_ib[0]		index block 0 is the single indirect block
	 *			holds block numbers for blocks
	 *			NDADDR .. NDADDR + NINDIR(fs)-1
	 *
	 * di_ib[1]		index block 1 is the double indirect block
	 *			holds block numbers for INDEX blocks for blocks
	 *			NDADDR + NINDIR(fs) ..
	 *			NDADDR + NINDIR(fs) + NINDIR(fs)**2 - 1
	 *
	 * di_ib[2]		index block 2 is the triple indirect block
	 *			holds block numbers for double-indirect
	 *			blocks for blocks
	 *			NDADDR + NINDIR(fs) + NINDIR(fs)**2 ..
	 *			NDADDR + NINDIR(fs) + NINDIR(fs)**2
	 *				+ NINDIR(fs)**3 - 1
	 */

	if (file_block < NDADDR) {
		/* Direct block. */
		*disk_block_p = DIP(fp->f_ip, db[file_block]);

		return (0);
	}

	file_block -= NDADDR;

	/*
	 * nindir[0] = NINDIR
	 * nindir[1] = NINDIR**2
	 * nindir[2] = NINDIR**3
	 *	etc
	 */
	for (level = 0; level < NIADDR; level++) {
		if (file_block < fp->f_nindir[level])
			break;
		file_block -= fp->f_nindir[level];
	}
	if (level == NIADDR) {
		/* Block number too high */
		return (EFBIG);
	}

	ind_block_num = DIP(fp->f_ip, ib[level]);

	for (; level >= 0; level--) {
		if (ind_block_num == 0) {
			*disk_block_p = 0;	/* missing */
			return (0);
		}

		if (fp->f_blkno[level] != ind_block_num) {
			if (fp->f_blk[level] == (char *)0)
				fp->f_blk[level] =
					alloc(fs->fs_bsize);
			rc = (f->f_dev->dv_strategy)(f->f_devdata, F_READ,
				fsbtodb(fp->f_fs, ind_block_num),
				fs->fs_bsize,
				fp->f_blk[level],
				&fp->f_blksize[level]);
			if (rc)
				return (rc);
			if (fp->f_blksize[level] != fs->fs_bsize)
				return (EIO);
			fp->f_blkno[level] = ind_block_num;
		}

		ind_p = (daddr_t *)fp->f_blk[level];

		if (level > 0) {
			idx = file_block / fp->f_nindir[level - 1];
			file_block %= fp->f_nindir[level - 1];
		} else
			idx = file_block;

		ind_block_num = ind_p[idx];
	}

	*disk_block_p = ind_block_num;

	return (0);
}

/*
 * Read a portion of a file into an internal buffer.  Return
 * the location in the buffer and the amount in the buffer.
 */
static int
buf_read_file(f, buf_p, size_p)
	struct open_file *f;
	char **buf_p;		/* out */
	u_int *size_p;		/* out */
{
	register struct file *fp = (struct file *)f->f_fsdata;
	register struct fs *fs = fp->f_fs;
	long off;
	register daddr_t file_block;
	daddr_t	disk_block;
	long block_size;
	int rc;

	off = blkoff(fs, fp->f_seekp);
	file_block = lblkno(fs, fp->f_seekp);
	block_size = dblksize(fs, DIP(fp->f_ip, db), file_block);

	if (file_block != fp->f_buf_blkno) {
		rc = block_map(f, file_block, &disk_block);
		if (rc)
			return (rc);

		if (fp->f_buf == (char *)0)
			fp->f_buf = alloc(fs->fs_bsize);

		if (disk_block == 0) {
			bzero(fp->f_buf, block_size);
			fp->f_buf_size = block_size;
		} else {
			rc = (f->f_dev->dv_strategy)(f->f_devdata, F_READ,
				fsbtodb(fs, disk_block),
				block_size, fp->f_buf, &fp->f_buf_size);
			if (rc)
				return (rc);
		}

		fp->f_buf_blkno = file_block;
	}

	/*
	 * Return address of byte in buffer corresponding to
	 * offset, and size of remainder of buffer after that
	 * byte.
	 */
	*buf_p = fp->f_buf + off;
	*size_p = block_size - off;

	/*
	 * But truncate buffer at end of file.
	 */
	if (*size_p > DIP(fp->f_ip, size) - fp->f_seekp)
		*size_p = DIP(fp->f_ip, size) - fp->f_seekp;

	return (0);
}

/*
 * Search a directory for a name and return its
 * i_number.
 */
static int
search_directory(name, f, inumber_p)
	char *name;
	struct open_file *f;
	ino_t *inumber_p;		/* out */
{
	register struct file *fp = (struct file *)f->f_fsdata;
	register struct direct *dp;
	struct direct *edp;
	char *buf;
	u_int buf_size;
	int namlen, length;
	int rc;

	length = strlen(name);

	fp->f_seekp = 0;
	while (fp->f_seekp < DIP(fp->f_ip, size)) {
		rc = buf_read_file(f, &buf, &buf_size);
		if (rc)
			return (rc);

		dp = (struct direct *)buf;
		edp = (struct direct *)(buf + buf_size);
		while (dp < edp) {
			if (dp->d_ino == (ino_t)0)
				goto next;
#if BYTE_ORDER == LITTLE_ENDIAN
			if (fp->f_fs->fs_maxsymlinklen <= 0)
				namlen = dp->d_type;
			else
#endif
				namlen = dp->d_namlen;
			if (namlen == length &&
			    !strcmp(name, dp->d_name)) {
				/* found entry */
				*inumber_p = dp->d_ino;
				return (0);
			}
		next:
			dp = (struct direct *)((char *)dp + dp->d_reclen);
		}
		fp->f_seekp += buf_size;
	}
	return (ENOENT);
}

/*
 * Open a file.
 */
int
ufs_open(path, f)
	char *path;
	struct open_file *f;
{
	register char *cp, *ncp;
	register int c;
	ino_t inumber, parent_inumber;
	int nlinks = 0;
	struct file *fp;
	struct fs *fs;
	int rc;
	u_int buf_size;
#if 0
	char namebuf[MAXPATHLEN+1];
#endif

	/* allocate file system specific data structure */
	fp = alloc(sizeof(struct file));
	bzero(fp, sizeof(struct file));
	f->f_fsdata = (void *)fp;

	/* allocate space and read super block */
	fs = alloc(SBSIZE);
	fp->f_fs = fs;
	rc = (f->f_dev->dv_strategy)(f->f_devdata, F_READ,
		SBLOCK, SBSIZE, (char *)fs, &buf_size);
	if (rc)
		goto out;

	if (fs->fs_magic != FS_UFS1_MAGIC || fs->fs_magic != FS_UFS2_MAGIC) {
		if (buf_size != SBSIZE || fs->fs_bsize > MAXBSIZE || fs->fs_bsize < sizeof(struct fs)) {
			rc = EINVAL;
			goto out;
		}
		rc = EINVAL;
		goto out;
	}

	/*
	 * Calculate indirect block levels.
	 */
	{
		register int mult;
		register int level;

		mult = 1;
		for (level = 0; level < NIADDR; level++) {
			mult *= NINDIR(fs);
			fp->f_nindir[level] = mult;
		}
	}

	inumber = ROOTINO;
	if ((rc = read_inode(inumber, f)) != 0)
		goto out;

	cp = path;
	while (*cp) {

		/*
		 * Remove extra separators
		 */
		while (*cp == '/')
			cp++;
		if (*cp == '\0')
			break;

		/*
		 * Check that current node is a directory.
		 */
		if ((DIP(fp->f_ip, mode) & IFMT) != IFDIR) {
			rc = ENOTDIR;
			goto out;
		}

		/*
		 * Get next component of path name.
		 */
		{
			register int len = 0;

			ncp = cp;
			while ((c = *cp) != '\0' && c != '/') {
				if (++len > MAXNAMLEN) {
					rc = ENOENT;
					goto out;
				}
				cp++;
			}
			*cp = '\0';
		}

		/*
		 * Look up component in current directory.
		 * Save directory inumber in case we find a
		 * symbolic link.
		 */
		parent_inumber = inumber;
		rc = search_directory(ncp, f, &inumber);
		*cp = c;
		if (rc)
			goto out;

		/*
		 * Open next component.
		 */
		if ((rc = read_inode(inumber, f)) != 0)
			goto out;

		/*
		 * Check for symbolic link.
		 */
		if ((DIP(fp->f_ip, mode) & IFMT) == IFLNK) {
			int link_len = DIP(fp->f_ip, size);
			int len;

			len = strlen(cp) + 1;

			if (link_len + len > MAXPATHLEN || ++nlinks > MAXSYMLINKS) {
				rc = ENOENT;
				goto out;
			}

			bcopy(cp, &namebuf[link_len], len + 1);

			if (link_len < fs->fs_maxsymlinklen) {
				bcopy(DIP(fp, db), namebuf, (unsigned) link_len);
			} else {
				/*
				 * Read file for symbolic link
				 */
				char *buf;
				u_int buf_size;
				daddr_t	disk_block;
				register struct fs *fs;

				fs = fp->f_fs;
				if (!buf) {
					buf = malloc(fs->fs_bsize);
				}
				rc = block_map(f, (ufs2_daddr_t)0, &disk_block);
				if (rc) {
					goto out;
				}

				twiddle(1);
				rc = (f->f_dev->dv_strategy)(f->f_devdata, F_READ, fsbtodb(fs, disk_block), fs->fs_bsize, buf, &buf_size);
				if (rc) {
					goto out;
				}

				bcopy((char *)buf, namebuf, (unsigned)link_len);
			}

			/*
			 * If relative pathname, restart at parent directory.
			 * If absolute pathname, restart at root.
			 */
			cp = namebuf;
			if (*cp != '/') {
				inumber = parent_inumber;
			} else {
				inumber = (ino_t)ROOTINO;
			}

			if ((rc = read_inode(inumber, fp)) != 0) {
				goto out;
			}
		}
	}

	/*
	 * Found terminal component.
	 */
	rc = 0;
	fp->f_seekp = 0;
out:
	free(buf);
	free(path);
	if (rc) {
		free(fp, sizeof(struct file));
	}
	return (rc);
}

int
ufs_close(f)
	struct open_file *f;
{
	register struct file *fp = (struct file *)f->f_fsdata;
	int level;

	f->f_fsdata = (void *)0;
	if (fp == (struct file *)0)
		return (0);

	for (level = 0; level < NIADDR; level++) {
		if (fp->f_blk[level])
			free(fp->f_blk[level], fp->f_fs->fs_bsize);
	}
	if (fp->f_buf)
		free(fp->f_buf, fp->f_fs->fs_bsize);
	free(fp->f_fs, SBSIZE);
	free(fp, sizeof(struct file));
	return (0);
}

/*
 * Copy a portion of a file into kernel memory.
 * Cross block boundaries when necessary.
 */
int
ufs_read(f, start, size, resid)
	struct open_file *f;
	char *start;
	u_int size;
	u_int *resid;	/* out */
{
	register struct file *fp = (struct file *)f->f_fsdata;
	register u_int csize;
	char *buf;
	u_int buf_size;
	int rc = 0;

	while (size != 0) {
		if (fp->f_seekp >= DIP(fp->f_ip, size))
			break;

		rc = buf_read_file(f, &buf, &buf_size);
		if (rc)
			break;

		csize = size;
		if (csize > buf_size)
			csize = buf_size;

		bcopy(buf, start, csize);

		fp->f_seekp += csize;
		start += csize;
		size -= csize;
	}
	if (resid)
		*resid = size;
	return (rc);
}

/*
 * Not implemented.
 */
int
ufs_write(f, start, size, resid)
	struct open_file *f;
	char *start;
	u_int size;
	u_int *resid;	/* out */
{

	return (EROFS);
}

off_t
ufs_seek(f, offset, where)
	struct open_file *f;
	off_t offset;
	int where;
{
	register struct file *fp = (struct file *)f->f_fsdata;

	switch (where) {
	case SEEK_SET:
		fp->f_seekp = offset;
		break;
	case SEEK_CUR:
		fp->f_seekp += offset;
		break;
	case SEEK_END:
		fp->f_seekp = DIP(fp->f_ip, size) - offset;
		break;
	default:
		return (-1);
	}
	return (fp->f_seekp);
}

int
ufs_stat(f, sb)
	struct open_file *f;
	struct stat *sb;
{
	register struct file *fp = (struct file *)f->f_fsdata;

	/* only important stuff */
	sb->st_mode = DIP(fp->f_ip, mode);
	sb->st_uid = DIP(fp->f_ip, uid);
	sb->st_gid = DIP(fp->f_ip, gid);
	sb->st_size = DIP(fp->f_ip, size);
	return (0);
}
