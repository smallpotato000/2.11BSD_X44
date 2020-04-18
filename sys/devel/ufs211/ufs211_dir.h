/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)dir.h	1.2 (2.11BSD GTE) 11/4/94
 */

#ifndef	_UFS211_DIR_
#define	_UFS211_DIR_

#define UFS211_MAXNAMLEN	63
#define UFS211_DIRBLKSIZ	512

/*
 * inode numbers are ino_t rather than u_long now.  before, when v7direct
 * was used for the kernel, inode numbers were u_short/ino_t anyways, and since
 * everything had to be recompiled when the fs structure was changed it seemed
 * like a good idea to change the "real direct structure".  SMS
*/

struct	ufs211_direct {
	ufs211_ino_t	d_ino;					/* inode number of entry */
	u_short			d_reclen;				/* length of this record */
	u_short			d_namlen;				/* length of string in d_name */
	char			d_name[UFS211_MAXNAMLEN+1];	/* name must be no longer than this */
};

/*
 * A directory consists of some number of blocks of DIRBLKSIZ
 * bytes, where DIRBLKSIZ is chosen such that it can be transferred
 * to disk in a single atomic operation (e.g. 512 bytes on most machines).
 *
 * Each DIRBLKSIZ byte block contains some number of directory entry
 * structures, which are of variable length.  Each directory entry has
 * a struct direct at the front of it, containing its inode number,
 * the length of the entry, and the length of the name contained in
 * the entry.  These are followed by the name padded to a 4 byte boundary
 * with null bytes.  All names are guaranteed null terminated.
 * The maximum length of a name in a directory is MAXNAMLEN.
 *
 * The macro DIRSIZ(dp) gives the amount of space required to represent
 * a directory entry.  Free space in a directory is represented by
 * entries which have dp->d_reclen > DIRSIZ(dp).  All DIRBLKSIZ bytes
 * in a directory block are claimed by the directory entries.  This
 * usually results in the last entry in a directory having a large
 * dp->d_reclen.  When entries are deleted from a directory, the
 * space is returned to the previous entry in the same directory
 * block by increasing its dp->d_reclen.  If the first entry of
 * a directory block is free, then its dp->d_ino is set to 0.
 * Entries other than the first in a directory do not normally have
 * dp->d_ino set to 0.
 */

#undef DIRSIZ
#define DIRSIZ(dp) \
    ((((sizeof (struct direct) - (UFS211_MAXNAMLEN+1)) + (dp)->d_namlen+1) + 3) &~ 3)

/*
 * Definitions for library routines operating on directories.
 */
struct ufs211_dirdesc {
	int						dd_fd;
	long					dd_loc;
	long					dd_size;
	char					dd_buf[UFS211_DIRBLKSIZ];
	struct 	ufs211_direct	dd_cur;
};// DIR;

/*
 * Template for manipulating directories.
 * Should use struct direct's, but the name field
 * is MAXNAMLEN - 1, and this just won't do.
 */
#define dotdot_ino		dtdt_ino
#define dotdot_reclen	dtdt_rec
#define dotdot_name		dtdt_name

struct ufs211_dirtemplate {
	ufs211_ino_t	dot_ino;
	u_short			dot_reclen;
	u_short			dot_namlen;
	char			dot_name[2];		/* must be multiple of 4 */
	ufs211_ino_t	dotdot_ino;
	u_short			dotdot_reclen;
	u_short			dotdot_namlen;
	char			dotdot_name[6];		/* ditto */
};
#endif	_UFS211_DIR_
