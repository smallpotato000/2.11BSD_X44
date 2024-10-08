/*
 * Copyright (c) 1980, 1989, 1993
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
 */

#include <sys/cdefs.h>

#ifndef lint
static char sccsid[] = "@(#)mkfs.c	8.11 (Berkeley) 5/3/95";
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ffs/fs.h>
#include <sys/disklabel.h>

#include <err.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stddef.h>

#ifndef STANDALONE
#include <stdio.h>
#endif

#include "extern.h"

/*
 * make file system for cylinder-group style file systems
 */

union dinode {
	struct ufs1_dinode dp1;
	struct ufs2_dinode dp2;
};

/*
 * We limit the size of the inode map to be no more than a
 * third of the cylinder group space, since we must leave at
 * least an equal amount of space for the block map.
 *
 * N.B.: MAXIPG must be a multiple of INOPB(fs).
 */
#define MAXIPG(fs)		roundup((fs)->fs_bsize * NBBY / 3, INOPB(fs))

#define UMASK			0755
#define UFS1_MAXINOPB	(MAXBSIZE / sizeof(struct ufs1_dinode))
#define UFS2_MAXINOPB	(MAXBSIZE / sizeof(struct ufs2_dinode))
#define POWEROF2(num)	(((num) & ((num) - 1)) == 0)

union {
	struct fs fs;
	char pad[SBLOCKSIZE];
} fsun;
#define	sblock	fsun.fs
struct csum *fscs;

union {
	struct cg cg;
	char pad[MAXBSIZE];
} cgun;
#define	acg	cgun.cg

#define DIP(dp, field) \
	((sblock.fs_magic == FS_UFS1_MAGIC) ? \
			(dp)->dp1.di_##field : (dp)->dp2.di_##field)

int	fsi, fso;

void 	fsinit(const struct timeval *, mode_t, uid_t, gid_t);
void 	iput(union dinode *, ino_t);
daddr_t	alloc(int, int);
void 	rdfs(daddr_t, int, char *);
void 	wtfs(daddr_t, int, char *);
long	calcipg(long, long, off_t);
int 	isblock(struct fs *, unsigned char *, int);
void	clrblock(struct fs *, unsigned char *, int);
void 	setblock(struct fs *, unsigned char *, int);

void
mkfs(pp, fsys, fi, fo, mfsmode, mfsuid, mfsgid)
	struct partition *pp;
	char *fsys;
	int fi, fo;
	mode_t mfsmode;
	uid_t mfsuid;
	gid_t mfsgid;
{
	register long i, mincpc, mincpg, inospercg;
	long cylno, rpos, blk, j, warn = 0;
	long used, mincpgcnt, bpcg;
	off_t usedb;
	long mapcramped, inodecramped;
	long postblsize, rotblsize, totalsbsize;
	int ppid, status;
	struct timeval utime;
	quad_t sizepb;

#ifndef STANDALONE
	time(&utime);
#endif
	if (mfs) {
		ppid = getpid();
		(void) signal(SIGUSR1, started);
		if (i == fork()) {
			if (i == -1) {
				perror("mfs");
				exit(10);
			}
			if (waitpid(i, &status, 0) != -1 && WIFEXITED(status))
				exit(WEXITSTATUS(status));
			exit(11);
			/* NOTREACHED */
		}
		(void)malloc(0);
		if (fssize * sectorsize > memleft) {
			fssize = (memleft - 16384) / sectorsize;
		}
		if ((membase = malloc(fssize * sectorsize)) == 0) {
			exit(12);
		}
	}
	fsi = fi;
	fso = fo;
	if (Oflag) {
		sblock.fs_old_inodefmt = FS_42INODEFMT;
		sblock.fs_maxsymlinklen = 0;
	} else {
		sblock.fs_old_inodefmt = FS_44INODEFMT;
		sblock.fs_maxsymlinklen = (Oflag == 1 ? UFS1_MAXSYMLINKLEN : UFS2_MAXSYMLINKLEN);
	}

	/*
	 * Validate the given file system size.
	 * Verify that its last block can actually be accessed.
	 */
	if (fssize <= 0) {
		printf("preposterous size %d\n", fssize), exit(13);
	}
	wtfs(fssize - 1, sectorsize, (char *)&sblock);

	/*
	 * collect and verify the sector and track info
	 */
	sblock.fs_nsect = nsectors;
	sblock.fs_ntrak = ntracks;
	if (sblock.fs_ntrak <= 0) {
		printf("preposterous ntrak %d\n", sblock.fs_ntrak), exit(14);
	}
	if (sblock.fs_nsect <= 0) {
		printf("preposterous nsect %d\n", sblock.fs_nsect), exit(15);
	}

	/*
	 * collect and verify the block and fragment sizes
	 */
	sblock.fs_bsize = bsize;
	sblock.fs_fsize = fsize;
	if (!POWEROF2(sblock.fs_bsize)) {
		printf("block size must be a power of 2, not %d\n",
		    sblock.fs_bsize);
		exit(16);
	}
	if (!POWEROF2(sblock.fs_fsize)) {
		printf("fragment size must be a power of 2, not %d\n",
		    sblock.fs_fsize);
		exit(17);
	}
	if (sblock.fs_fsize < sectorsize) {
		printf("fragment size %d is too small, minimum is %d\n",
		    sblock.fs_fsize, sectorsize);
		exit(18);
	}
	if (sblock.fs_bsize < MINBSIZE) {
		printf("block size %d is too small, minimum is %d\n",
		    sblock.fs_bsize, MINBSIZE);
		exit(19);
	}
	if (sblock.fs_bsize > MAXBSIZE) {
		printf("block size %d is too large, maximum is %d\n",
		    sblock.fs_bsize, MAXBSIZE);
		fserr(19);
	}
	if (sblock.fs_bsize < sblock.fs_fsize) {
		printf("block size (%d) cannot be smaller than fragment size (%d)\n",
		    sblock.fs_bsize, sblock.fs_fsize);
		exit(20);
	}

	if (maxbsize < bsize || !POWEROF2(maxbsize)) {
		sblock.fs_maxbsize = sblock.fs_bsize;
	} else if (sblock.fs_maxbsize > FS_MAXCONTIG * sblock.fs_bsize) {
		sblock.fs_maxbsize = FS_MAXCONTIG * sblock.fs_bsize;
	} else {
		sblock.fs_maxbsize = maxbsize;
	}
	sblock.fs_maxcontig = maxcontig;
	if (sblock.fs_maxcontig < sblock.fs_maxbsize / sblock.fs_bsize) {
		sblock.fs_maxcontig = sblock.fs_maxbsize / sblock.fs_bsize;
	}
	if (sblock.fs_maxcontig > 1) {
		sblock.fs_contigsumsize = MIN(sblock.fs_maxcontig, FS_MAXCONTIG);
	}

	sblock.fs_bmask = ~(sblock.fs_bsize - 1);
	sblock.fs_fmask = ~(sblock.fs_fsize - 1);
	sblock.fs_qbmask = ~sblock.fs_bmask;
	sblock.fs_qfmask = ~sblock.fs_fmask;
	for (sblock.fs_bshift = 0, i = sblock.fs_bsize; i > 1; i >>= 1) {
		sblock.fs_bshift++;
	}
	for (sblock.fs_fshift = 0, i = sblock.fs_fsize; i > 1; i >>= 1) {
		sblock.fs_fshift++;
	}
	sblock.fs_frag = numfrags(&sblock, sblock.fs_bsize);
	for (sblock.fs_fragshift = 0, i = sblock.fs_frag; i > 1; i >>= 1) {
		sblock.fs_fragshift++;
	}
	if (sblock.fs_frag > MAXFRAG) {
		printf("fragment size %d is too small, minimum with block size %d is %d\n",
		    sblock.fs_fsize, sblock.fs_bsize,
		    sblock.fs_bsize / MAXFRAG);
		exit(21);
	}
	sblock.fs_nspf = sblock.fs_fsize / sectorsize;
	for (sblock.fs_fsbtodb = 0, i = NSPF(&sblock); i > 1; i >>= 1) {
		sblock.fs_fsbtodb++;
	}
	sblock.fs_size = dbtofsb(&sblock, fssize);
	if (Oflag <= 1) {
		if ((uint64_t) sblock.fs_size >= 1ull << 31) {
			printf("Too many fragments (0x%" PRIx64
					") for a FFSv1 filesystem\n", sblock.fs_size);
			exit(22);
		}
		sblock.fs_magic = FS_UFS1_MAGIC;
		sblock.fs_sblockloc = SBLOCK_UFS1;
		sblock.fs_nindir = sblock.fs_bsize / sizeof(int32_t);
		sblock.fs_inopb = sblock.fs_bsize / sizeof(struct ufs1_dinode);
		sblock.fs_cgoffset = 0;
		sblock.fs_cgmask = 0xffffffff;
		//sblock.fs_old_size = sblock.fs_size;
		sblock.fs_rotdelay = 0;
		sblock.fs_rps = 60;
		sblock.fs_nspf = sblock.fs_fsize / sectorsize;
		sblock.fs_cpg = 1;
		sblock.fs_interleave = 1;
		sblock.fs_trackskew = 0;
		sblock.fs_cpc = 0;
		sblock.fs_postblformat = FS_DYNAMICPOSTBLFMT;
		sblock.fs_nrpos = nrpos;
	}  else {
		sblock.fs_magic = FS_UFS2_MAGIC;
		sblock.fs_sblockloc = SBLOCK_UFS2;
		sblock.fs_nindir = sblock.fs_bsize / sizeof(int64_t);
		sblock.fs_inopb = sblock.fs_bsize / sizeof(struct ufs2_dinode);
	}

	sblock.fs_sblkno = roundup(howmany(bbsize + sbsize, sblock.fs_fsize), sblock.fs_frag);
	sblock.fs_cblkno = (daddr_t)(sblock.fs_sblkno + roundup(howmany(sbsize, sblock.fs_fsize), sblock.fs_frag));
	sblock.fs_iblkno = sblock.fs_cblkno + sblock.fs_frag;
	sblock.fs_cgoffset = roundup(howmany(sblock.fs_nsect, NSPF(&sblock)), sblock.fs_frag);
	for (sblock.fs_cgmask = 0xffffffff, i = sblock.fs_ntrak; i > 1; i >>= 1) {
		sblock.fs_cgmask <<= 1;
	}
	if (!POWEROF2(sblock.fs_ntrak)) {
		sblock.fs_cgmask <<= 1;
	}
	sblock.fs_maxfilesize = sblock.fs_bsize * NDADDR - 1;
	for (sizepb = sblock.fs_bsize, i = 0; i < NIADDR; i++) {
		sizepb *= NINDIR(&sblock);
		sblock.fs_maxfilesize += sizepb;
	}
	/*
	 * Validate specified/determined secpercyl
	 * and calculate minimum cylinders per group.
	 */
	sblock.fs_spc = secpercyl;
	for (sblock.fs_cpc = NSPB(&sblock), i = sblock.fs_spc;
	     sblock.fs_cpc > 1 && (i & 1) == 0;
	     sblock.fs_cpc >>= 1, i >>= 1) {
		/* void */;
	}
	mincpc = sblock.fs_cpc;
	bpcg = sblock.fs_spc * sectorsize;
	inospercg = roundup(bpcg / sizeof(struct dinode), INOPB(&sblock));
	if (inospercg > MAXIPG(&sblock)) {
		inospercg = MAXIPG(&sblock);
	}
	used = (sblock.fs_iblkno + inospercg / INOPF(&sblock)) * NSPF(&sblock);
	mincpgcnt = howmany(sblock.fs_cgoffset * (~sblock.fs_cgmask) + used, sblock.fs_spc);
	mincpg = roundup(mincpgcnt, mincpc);
	/*
	 * Ensure that cylinder group with mincpg has enough space
	 * for block maps.
	 */
	sblock.fs_cpg = mincpg;
	sblock.fs_ipg = inospercg;
	if (maxcontig > 1) {
		sblock.fs_contigsumsize = MIN(maxcontig, FS_MAXCONTIG);
	}
	mapcramped = 0;
	while (CGSIZE(&sblock) > sblock.fs_bsize) {
		mapcramped = 1;
		if (sblock.fs_bsize < MAXBSIZE) {
			sblock.fs_bsize <<= 1;
			if ((i & 1) == 0) {
				i >>= 1;
			} else {
				sblock.fs_cpc <<= 1;
				mincpc <<= 1;
				mincpg = roundup(mincpgcnt, mincpc);
				sblock.fs_cpg = mincpg;
			}
			sblock.fs_frag <<= 1;
			sblock.fs_fragshift += 1;
			if (sblock.fs_frag <= MAXFRAG) {
				continue;
			}
		}
		if (sblock.fs_fsize == sblock.fs_bsize) {
			printf("There is no block size that");
			printf(" can support this disk\n");
			exit(22);
		}
		sblock.fs_frag >>= 1;
		sblock.fs_fragshift -= 1;
		sblock.fs_fsize <<= 1;
		sblock.fs_nspf <<= 1;
	}
	/*
	 * Ensure that cylinder group with mincpg has enough space for inodes.
	 */
	inodecramped = 0;
	inospercg = calcipg(mincpg, bpcg, &usedb);
	sblock.fs_ipg = inospercg;
	while (inospercg > MAXIPG(&sblock)) {
		inodecramped = 1;
		if (mincpc == 1 || sblock.fs_frag == 1 || sblock.fs_bsize == MINBSIZE) {
			break;
		}
		printf("With a block size of %d %s %d\n", sblock.fs_bsize,
				"minimum bytes per inode is",
				(int) ((mincpg * (off_t) bpcg - usedb) / MAXIPG(&sblock) + 1));
		sblock.fs_bsize >>= 1;
		sblock.fs_frag >>= 1;
		sblock.fs_fragshift -= 1;
		mincpc >>= 1;
		sblock.fs_cpg = roundup(mincpgcnt, mincpc);
		if (CGSIZE(&sblock) > sblock.fs_bsize) {
			sblock.fs_bsize <<= 1;
			break;
		}
		mincpg = sblock.fs_cpg;
		inospercg = calcipg(mincpg, bpcg, &usedb);
		sblock.fs_ipg = inospercg;
	}
	if (inodecramped) {
		if (inospercg > MAXIPG(&sblock)) {
			printf("Minimum bytes per inode is %d\n",
			       (int)((mincpg * (off_t)bpcg - usedb)
				     / MAXIPG(&sblock) + 1));
		} else if (!mapcramped) {
			printf("With %d bytes per inode, ", density);
			printf("minimum cylinders per group is %d\n", mincpg);
		}
	}
	if (mapcramped) {
		printf("With %d sectors per cylinder, ", sblock.fs_spc);
		printf("minimum cylinders per group is %d\n", mincpg);
	}
	if (inodecramped || mapcramped) {
		if (sblock.fs_bsize != bsize) {
			printf("%s to be changed from %d to %d\n",
			    "This requires the block size",
			    bsize, sblock.fs_bsize);
		}
		if (sblock.fs_fsize != fsize) {
			printf("\t%s to be changed from %d to %d\n",
			    "and the fragment size",
			    fsize, sblock.fs_fsize);
		}
		exit(23);
	}
	/* 
	 * Calculate the number of cylinders per group
	 */
	sblock.fs_cpg = cpg;
	if (sblock.fs_cpg % mincpc != 0) {
		printf("%s groups must have a multiple of %d cylinders\n",
			cpgflg ? "Cylinder" : "Warning: cylinder", mincpc);
		sblock.fs_cpg = roundup(sblock.fs_cpg, mincpc);
		if (!cpgflg) {
			cpg = sblock.fs_cpg;
		}
	}
	/*
	 * Must ensure there is enough space for inodes.
	 */
	sblock.fs_ipg = calcipg(sblock.fs_cpg, bpcg, &usedb);
	while (sblock.fs_ipg > MAXIPG(&sblock)) {
		inodecramped = 1;
		sblock.fs_cpg -= mincpc;
		sblock.fs_ipg = calcipg(sblock.fs_cpg, bpcg, &usedb);
	}
	/*
	 * Must ensure there is enough space to hold block map.
	 */
	while (CGSIZE(&sblock) > sblock.fs_bsize) {
		mapcramped = 1;
		sblock.fs_cpg -= mincpc;
		sblock.fs_ipg = calcipg(sblock.fs_cpg, bpcg, &usedb);
	}
	sblock.fs_fpg = (sblock.fs_cpg * sblock.fs_spc) / NSPF(&sblock);
	if ((sblock.fs_cpg * sblock.fs_spc) % NSPB(&sblock) != 0) {
		printf("panic (fs_cpg * fs_spc) % NSPF != 0");
		exit(24);
	}
	if (sblock.fs_cpg < mincpg) {
		printf("cylinder groups must have at least %d cylinders\n",
			mincpg);
		exit(25);
	} else if (sblock.fs_cpg != cpg) {
		if (!cpgflg)
			printf("Warning: ");
		else if (!mapcramped && !inodecramped)
			exit(26);
		if (mapcramped && inodecramped)
			printf("Block size and bytes per inode restrict");
		else if (mapcramped)
			printf("Block size restricts");
		else
			printf("Bytes per inode restrict");
		printf(" cylinders per group to %d.\n", sblock.fs_cpg);
		if (cpgflg)
			exit(27);
	}
	sblock.fs_cgsize = fragroundup(&sblock, CGSIZE(&sblock));
	/*
	 * Now have size for file system and nsect and ntrak.
	 * Determine number of cylinders and blocks in the file system.
	 */
	sblock.fs_size = fssize = dbtofsb(&sblock, fssize);
	sblock.fs_ncyl = fssize * NSPF(&sblock) / sblock.fs_spc;
	if (fssize * NSPF(&sblock) > sblock.fs_ncyl * sblock.fs_spc) {
		sblock.fs_ncyl++;
		warn = 1;
	}
	if (sblock.fs_ncyl < 1) {
		printf("file systems must have at least one cylinder\n");
		exit(28);
	}
	/*
	 * Determine feasability/values of rotational layout tables.
	 *
	 * The size of the rotational layout tables is limited by the
	 * size of the superblock, SBSIZE. The amount of space available
	 * for tables is calculated as (SBSIZE - sizeof (struct fs)).
	 * The size of these tables is inversely proportional to the block
	 * size of the file system. The size increases if sectors per track
	 * are not powers of two, because more cylinders must be described
	 * by the tables before the rotational pattern repeats (fs_cpc).
	 */
	sblock.fs_interleave = interleave;
	sblock.fs_trackskew = trackskew;
	sblock.fs_npsect = nphyssectors;
	sblock.fs_postblformat = FS_DYNAMICPOSTBLFMT;
	sblock.fs_sbsize = fragroundup(&sblock, sizeof(struct fs));
	if (sblock.fs_ntrak == 1) {
		sblock.fs_cpc = 0;
		goto next;
	}
	postblsize = sblock.fs_nrpos * sblock.fs_cpc * sizeof(short);
	rotblsize = sblock.fs_cpc * sblock.fs_spc / NSPB(&sblock);
	totalsbsize = sizeof(struct fs) + rotblsize;
	if (sblock.fs_nrpos == 8 && sblock.fs_cpc <= 16) {
		/* use old static table space */
		sblock.fs_postbloff = (char *)(&sblock.fs_opostbl[0][0]) -
		    (char *)(&sblock.fs_firstfield);
		sblock.fs_rotbloff = &sblock.fs_space[0] -
		    (u_char *)(&sblock.fs_firstfield);
	} else {
		/* use dynamic table space */
		sblock.fs_postbloff = &sblock.fs_space[0] -
		    (u_char *)(&sblock.fs_firstfield);
		sblock.fs_rotbloff = sblock.fs_postbloff + postblsize;
		totalsbsize += postblsize;
	}
	if (totalsbsize > SBSIZE ||
	    sblock.fs_nsect > (1 << NBBY) * NSPB(&sblock)) {
		printf("%s %s %d %s %d.%s",
		    "Warning: insufficient space in super block for\n",
		    "rotational layout tables with nsect", sblock.fs_nsect,
		    "and ntrak", sblock.fs_ntrak,
		    "\nFile system performance may be impaired.\n");
		sblock.fs_cpc = 0;
		goto next;
	}
	sblock.fs_sbsize = fragroundup(&sblock, totalsbsize);
	/*
	 * calculate the available blocks for each rotational position
	 */
	for (cylno = 0; cylno < sblock.fs_cpc; cylno++) {
		for (rpos = 0; rpos < sblock.fs_nrpos; rpos++) {
			fs_postbl(&sblock, cylno)[rpos] = -1;
		}
	}
	for (i = (rotblsize - 1) * sblock.fs_frag; i >= 0; i -= sblock.fs_frag) {
		cylno = cbtocylno(&sblock, i);
		rpos = cbtorpos(&sblock, i);
		blk = fragstoblks(&sblock, i);
		if (fs_postbl(&sblock, cylno)[rpos] == -1) {
			fs_rotbl(&sblock)[blk] = 0;
		} else {
			fs_rotbl(&sblock)[blk] = fs_postbl(&sblock, cylno)[rpos] - blk;
		}
		fs_postbl(&sblock, cylno)[rpos] = blk;
	}
next:
	/*
	 * Compute/validate number of cylinder groups.
	 */
	sblock.fs_ncg = sblock.fs_ncyl / sblock.fs_cpg;
	if (sblock.fs_ncyl % sblock.fs_cpg)
		sblock.fs_ncg++;
	sblock.fs_dblkno = sblock.fs_iblkno + sblock.fs_ipg / INOPF(&sblock);
	i = MIN(~sblock.fs_cgmask, sblock.fs_ncg - 1);
	if (cgdmin(&sblock, i) - cgbase(&sblock, i) >= sblock.fs_fpg) {
		printf("inode blocks/cyl group (%d) >= data blocks (%d)\n",
		    cgdmin(&sblock, i) - cgbase(&sblock, i) / sblock.fs_frag,
		    sblock.fs_fpg / sblock.fs_frag);
		printf("number of cylinders per cylinder group (%d) %s.\n",
		    sblock.fs_cpg, "must be increased");
		exit(29);
	}
	j = sblock.fs_ncg - 1;
	if ((i = fssize - j * sblock.fs_fpg) < sblock.fs_fpg &&
	    cgdmin(&sblock, j) - cgbase(&sblock, j) > i) {
		if (j == 0) {
			printf("Filesystem must have at least %d sectors\n",
			    NSPF(&sblock) *
			    (cgdmin(&sblock, 0) + 3 * sblock.fs_frag));
			exit(30);
		}
		printf("Warning: inode blocks/cyl group (%d) >= data blocks (%d) in last\n",
		    (cgdmin(&sblock, j) - cgbase(&sblock, j)) / sblock.fs_frag,
		    i / sblock.fs_frag);
		printf("    cylinder group. This implies %d sector(s) cannot be allocated.\n",
		    i * NSPF(&sblock));
		sblock.fs_ncg--;
		sblock.fs_ncyl -= sblock.fs_ncyl % sblock.fs_cpg;
		sblock.fs_size = fssize = sblock.fs_ncyl * sblock.fs_spc /
		    NSPF(&sblock);
		warn = 0;
	}
	if (warn && !mfs) {
		printf("Warning: %d sector(s) in last cylinder unallocated\n",
		    sblock.fs_spc -
		    (fssize * NSPF(&sblock) - (sblock.fs_ncyl - 1)
		    * sblock.fs_spc));
	}
	/*
	 * fill in remaining fields of the super block
	 */
	sblock.fs_csaddr = cgdmin(&sblock, 0);
	sblock.fs_cssize = fragroundup(&sblock, sblock.fs_ncg * sizeof(struct csum));
	i = sblock.fs_bsize / sizeof(struct csum);
	sblock.fs_csmask = ~(i - 1);
	for (sblock.fs_csshift = 0; i > 1; i >>= 1) {
		sblock.fs_csshift++;
	}
	fscs = (struct csum *)calloc(1, sblock.fs_cssize);
	sblock.fs_rotdelay = rotdelay;
	sblock.fs_minfree = minfree;
	sblock.fs_maxcontig = maxcontig;
	sblock.fs_headswitch = headswitch;
	sblock.fs_trkseek = trackseek;
	sblock.fs_maxbpg = maxbpg;
	sblock.fs_rps = rpm / 60;
	sblock.fs_optim = opt;
	sblock.fs_cgrotor = 0;
	sblock.fs_cstotal.cs_ndir = 0;
	sblock.fs_cstotal.cs_nbfree = 0;
	sblock.fs_cstotal.cs_nifree = 0;
	sblock.fs_cstotal.cs_nffree = 0;
	sblock.fs_fmod = 0;
	sblock.fs_ronly = 0;
	sblock.fs_clean = 1;

	/*
	 * Dump out summary information about file system.
	 */
	if (!mfs) {
		printf("%s:\t%d sectors in %d %s of %d tracks, %d sectors\n",
		    fsys, sblock.fs_size * NSPF(&sblock), sblock.fs_ncyl,
		    "cylinders", sblock.fs_ntrak, sblock.fs_nsect);
#define B2MBFACTOR (1 / (1024.0 * 1024.0))
		printf("\t%.1fMB in %d cyl groups (%d c/g, %.2fMB/g, %d i/g)\n",
		    (float)sblock.fs_size * sblock.fs_fsize * B2MBFACTOR,
		    sblock.fs_ncg, sblock.fs_cpg,
		    (float)sblock.fs_fpg * sblock.fs_fsize * B2MBFACTOR,
		    sblock.fs_ipg);
#undef B2MBFACTOR
	}
	/*
	 * Now build the cylinders group blocks and
	 * then print out indices of cylinder groups.
	 */
	if (!mfs) {
		printf("super-block backups (for fsck -b #) at:");
	}
	for (cylno = 0; cylno < sblock.fs_ncg; cylno++) {
		initcg(cylno, &utime);
		if (mfs) {
			continue;
		}
		if (cylno % 8 == 0) {
			printf("\n");
		}
		printf(" %d,", fsbtodb(&sblock, cgsblock(&sblock, cylno)));
	}
	if (!mfs) {
		printf("\n");
	}
	if (Nflag && !mfs) {
		exit(0);
	}
	/*
	 * Now construct the initial file system,
	 * then write out the super-block.
	 */
	fsinit(&utime, mfsmode, mfsuid, mfsgid);
	sblock.fs_time = &utime;
	wtfs((int)SBOFF / sectorsize, sbsize, (char *)&sblock);
	for (i = 0; i < sblock.fs_cssize; i += sblock.fs_bsize) {
		wtfs(fsbtodb(&sblock, sblock.fs_csaddr + numfrags(&sblock, i)),
			sblock.fs_cssize - i < sblock.fs_bsize ? sblock.fs_cssize - i : sblock.fs_bsize,
			((char *)fscs) + i);
	}
	/* 
	 * Write out the duplicate super blocks
	 */
	for (cylno = 0; cylno < sblock.fs_ncg; cylno++) {
		wtfs(fsbtodb(&sblock, cgsblock(&sblock, cylno)),
		    sbsize, (char *)&sblock);
	}
	/*
	 * Update information about this partion in pack
	 * label, to that it may be updated on disk.
	 */
	pp->p_fstype = FS_BSDFFS;
	pp->p_fsize = sblock.fs_fsize;
	pp->p_frag = sblock.fs_frag;
	pp->p_cpg = sblock.fs_cpg;
	/*
	 * Notify parent process of success.
	 * Dissociate from session and tty.
	 */
	if (mfs) {
		kill(ppid, SIGUSR1);
		(void) setsid();
		(void) close(0);
		(void) close(1);
		(void) close(2);
		(void) chdir("/");
	}
}

/*
 * Initialize a cylinder group.
 */
void
initcg(cylno, utime)
	uint32_t cylno;
	const struct timeval *utime;
{
	daddr_t cbase, dmax;
	uint32_t d, dlower, dupper, blkno;
	long i, j, s;
	register struct csum *cs;

	/*
	 * Determine block bounds for cylinder group.
	 * Allow space for super block summary information in first
	 * cylinder group.
	 */
	cbase = cgbase(&sblock, cylno);
	dmax = cbase + sblock.fs_fpg;
	if (dmax > sblock.fs_size)
		dmax = sblock.fs_size;
	dlower = cgsblock(&sblock, cylno) - cbase;
	dupper = cgdmin(&sblock, cylno) - cbase;
	if (cylno == 0)
		dupper += howmany(sblock.fs_cssize, sblock.fs_fsize);
	cs = fscs + cylno;
	memset(&acg, 0, sblock.fs_cgsize);
	acg.cg_time = utime;
	acg.cg_magic = CG_MAGIC;
	acg.cg_cgx = cylno;
	if (cylno == sblock.fs_ncg - 1)
		acg.cg_ncyl = sblock.fs_ncyl % sblock.fs_cpg;
	else
		acg.cg_ncyl = sblock.fs_cpg;
	acg.cg_niblk = sblock.fs_ipg;
	acg.cg_ndblk = dmax - cbase;
	if (sblock.fs_contigsumsize > 0)
		acg.cg_nclusterblks = acg.cg_ndblk / sblock.fs_frag;
	acg.cg_btotoff = &acg.cg_space[0] - (u_char *)(&acg.cg_firstfield);
	acg.cg_boff = acg.cg_btotoff + sblock.fs_cpg * sizeof(long);
	acg.cg_iusedoff = acg.cg_boff + 
		sblock.fs_cpg * sblock.fs_nrpos * sizeof(short);
	acg.cg_freeoff = acg.cg_iusedoff + howmany(sblock.fs_ipg, NBBY);
	if (sblock.fs_contigsumsize <= 0) {
		acg.cg_nextfreeoff = acg.cg_freeoff +
		   howmany(sblock.fs_cpg * sblock.fs_spc / NSPF(&sblock), NBBY);
	} else {
		acg.cg_clustersumoff = acg.cg_freeoff + howmany
		    (sblock.fs_cpg * sblock.fs_spc / NSPF(&sblock), NBBY) -
		    sizeof(long);
		acg.cg_clustersumoff =
		    roundup(acg.cg_clustersumoff, sizeof(long));
		acg.cg_clusteroff = acg.cg_clustersumoff +
		    (sblock.fs_contigsumsize + 1) * sizeof(long);
		acg.cg_nextfreeoff = acg.cg_clusteroff + howmany
		    (sblock.fs_cpg * sblock.fs_spc / NSPB(&sblock), NBBY);
	}
	if (acg.cg_nextfreeoff - (long)(&acg.cg_firstfield) > sblock.fs_cgsize) {
		printf("Panic: cylinder group too big\n");
		exit(37);
	}
	acg.cg_cs.cs_nifree += sblock.fs_ipg;
	if (cylno == 0)
		for (i = 0; i < UFS_ROOTINO; i++) {
			setbit(cg_inosused(&acg), i);
			acg.cg_cs.cs_nifree--;
		}
	for (i = 0; i < sblock.fs_ipg / INOPF(&sblock); i += sblock.fs_frag)
		wtfs(fsbtodb(&sblock, cgimin(&sblock, cylno) + i),
		    sblock.fs_bsize, (char *)zino);
	if (cylno > 0) {
		/*
		 * In cylno 0, beginning space is reserved
		 * for boot and super blocks.
		 */
		for (d = 0; d < dlower; d += sblock.fs_frag) {
			blkno = d / sblock.fs_frag;
			setblock(&sblock, cg_blksfree(&acg), blkno);
			if (sblock.fs_contigsumsize > 0)
				setbit(cg_clustersfree(&acg), blkno);
			acg.cg_cs.cs_nbfree++;
			cg_blktot(&acg)[cbtocylno(&sblock, d)]++;
			cg_blks(&sblock, &acg, cbtocylno(&sblock, d))
			    [cbtorpos(&sblock, d)]++;
		}
		sblock.fs_dsize += dlower;
	}
	sblock.fs_dsize += acg.cg_ndblk - dupper;
	if (i == dupper % sblock.fs_frag) {
		acg.cg_frsum[sblock.fs_frag - i]++;
		for (d = dupper + sblock.fs_frag - i; dupper < d; dupper++) {
			setbit(cg_blksfree(&acg), dupper);
			acg.cg_cs.cs_nffree++;
		}
	}
	for (d = dupper; d + sblock.fs_frag <= dmax - cbase; ) {
		blkno = d / sblock.fs_frag;
		setblock(&sblock, cg_blksfree(&acg), blkno);
		if (sblock.fs_contigsumsize > 0)
			setbit(cg_clustersfree(&acg), blkno);
		acg.cg_cs.cs_nbfree++;
		cg_blktot(&acg)[cbtocylno(&sblock, d)]++;
		cg_blks(&sblock, &acg, cbtocylno(&sblock, d))
		    [cbtorpos(&sblock, d)]++;
		d += sblock.fs_frag;
	}
	if (d < dmax - cbase) {
		acg.cg_frsum[dmax - cbase - d]++;
		for (; d < dmax - cbase; d++) {
			setbit(cg_blksfree(&acg), d);
			acg.cg_cs.cs_nffree++;
		}
	}
	if (sblock.fs_contigsumsize > 0) {
		int32_t *sump = cg_clustersum(&acg);
		u_char *mapp = cg_clustersfree(&acg);
		int map = *mapp++;
		int bit = 1;
		int run = 0;

		for (i = 0; i < acg.cg_nclusterblks; i++) {
			if ((map & bit) != 0) {
				run++;
			} else if (run != 0) {
				if (run > sblock.fs_contigsumsize)
					run = sblock.fs_contigsumsize;
				sump[run]++;
				run = 0;
			}
			if ((i & (NBBY - 1)) != (NBBY - 1)) {
				bit <<= 1;
			} else {
				map = *mapp++;
				bit = 1;
			}
		}
		if (run != 0) {
			if (run > sblock.fs_contigsumsize)
				run = sblock.fs_contigsumsize;
			sump[run]++;
		}
	}
	sblock.fs_cstotal.cs_ndir += acg.cg_cs.cs_ndir;
	sblock.fs_cstotal.cs_nffree += acg.cg_cs.cs_nffree;
	sblock.fs_cstotal.cs_nbfree += acg.cg_cs.cs_nbfree;
	sblock.fs_cstotal.cs_nifree += acg.cg_cs.cs_nifree;
	*cs = acg.cg_cs;
	wtfs(fsbtodb(&sblock, cgtod(&sblock, cylno)), sblock.fs_bsize, (char *)&acg);
}

/*
 * initialize the file system
 */


#ifdef LOSTDIR
#define PREDEFDIR 3
#else
#define PREDEFDIR 2
#endif

struct direct root_dir[] = {
	{ ROOTINO, sizeof(struct direct), DT_DIR, 1, "." },
	{ ROOTINO, sizeof(struct direct), DT_DIR, 2, ".." },
#ifdef LOSTDIR
	{ LOSTFOUNDINO, sizeof(struct direct), DT_DIR, 10, "lost+found" },
#endif
};
struct odirect {
	u_long	d_ino;
	u_short	d_reclen;
	u_short	d_namlen;
	u_char	d_name[MAXNAMLEN + 1];
} oroot_dir[] = {
	{ ROOTINO, sizeof(struct direct), 1, "." },
	{ ROOTINO, sizeof(struct direct), 2, ".." },
#ifdef LOSTDIR
	{ LOSTFOUNDINO, sizeof(struct direct), 10, "lost+found" },
#endif
};
#ifdef LOSTDIR
struct direct lost_found_dir[] = {
	{ LOSTFOUNDINO, sizeof(struct direct), DT_DIR, 1, "." },
	{ ROOTINO, sizeof(struct direct), DT_DIR, 2, ".." },
	{ 0, DIRBLKSIZ, 0, 0, 0 },
};
struct odirect olost_found_dir[] = {
	{ LOSTFOUNDINO, sizeof(struct direct), 1, "." },
	{ ROOTINO, sizeof(struct direct), 2, ".." },
	{ 0, DIRBLKSIZ, 0, 0 },
};
#endif

union dinode node;
char buf[MAXBSIZE];

void
fsinit(utime, mfsmode, mfsuid, mfsgid)
	const struct timeval *utime;
	mode_t mfsmode;
	uid_t mfsuid;
	gid_t mfsgid;
{
	int i;

	/*
	 * initialize the node
	 */

#ifdef LOSTDIR
	/*
	 * create the lost+found directory
	 */
	if (Oflag) {
		(void)makedir((struct direct *)olost_found_dir, 2);
		for (i = DIRBLKSIZ; i < sblock.fs_bsize; i += DIRBLKSIZ) {
			memmove(&buf[i], &olost_found_dir[2],
			    DIRSIZ(0, &olost_found_dir[2]));
		}
	} else {
		(void)makedir(lost_found_dir, 2);
		for (i = DIRBLKSIZ; i < sblock.fs_bsize; i += DIRBLKSIZ) {
			memmove(&buf[i], &lost_found_dir[2],
			    DIRSIZ(0, &lost_found_dir[2]));
		}
	}
	if (sblock.fs_magic == FS_UFS1_MAGIC) {
		node.dp1.di_atime = utime->tv_sec;
		node.dp1.di_atimensec = utime->tv_usec * 1000;
		node.dp1.di_mtime = utime->tv_sec;
		node.dp1.di_mtimensec = utime->tv_usec * 1000;
		node.dp1.di_ctime = utime->tv_sec;
		node.dp1.di_ctimensec = utime->tv_usec * 1000;
		node.dp1.di_mode = IFDIR | UMASK;
		node.dp1.di_nlink = 2;
		node.dp1.di_size = sblock.fs_bsize;
		node.dp1.di_db[0] = alloc(node.dp1.di_size, node.dp1.di_mode);
		if (node.dp1.di_db[0] == 0) {
			return;
		}
		node.dp1.di_blocks = btodb(fragroundup(&sblock, node.dp1.di_size));
		node.dp1.di_uid = geteuid();
		node.dp1.di_gid = getegid();
		wtfs(fsbtodb(&sblock, node.dp1.di_db[0]), node.dp1.di_size, buf);
	} else {
		node.dp2.di_atime = utime->tv_sec;
		node.dp2.di_atimensec = utime->tv_usec * 1000;
		node.dp2.di_mtime = utime->tv_sec;
		node.dp2.di_mtimensec = utime->tv_usec * 1000;
		node.dp2.di_ctime = utime->tv_sec;
		node.dp2.di_ctimensec = utime->tv_usec * 1000;
		node.dp2.di_birthtime = utime->tv_sec;
		node.dp2.di_birthnsec = utime->tv_usec * 1000;
		node.dp2.di_mode = IFDIR | UMASK;
		node.dp2.di_nlink = 2;
		node.dp2.di_size = sblock.fs_bsize;
		node.dp2.di_db[0] = alloc(node.dp2.di_size, node.dp2.di_mode);
		if (node.dp2.di_db[0] == 0) {
			return;
		}
		node.dp2.di_blocks = btodb(fragroundup(&sblock, node.dp2.di_size));
		node.dp2.di_uid = geteuid();
		node.dp2.di_gid = getegid();
		wtfs(fsbtodb(&sblock, node.dp2.di_db[0]), node.dp2.di_size, buf);
	}
	iput(&node, LOSTFOUNDINO);
#endif
	/*
	 * create the root directory
	 */
	memset(&node, 0, sizeof(node));
	if (Oflag <= 1) {
		if (mfs) {
			node.dp1.di_mode = IFDIR | mfsmode;
			node.dp1.di_uid = mfsuid;
			node.dp1.di_gid = mfsgid;
		} else {
			node.dp1.di_mode = IFDIR | UMASK;
			node.dp1.di_uid = geteuid();
			node.dp1.di_gid = getegid();
		}
		node.dp1.di_nlink = PREDEFDIR;
		if (Oflag == 0) {
			node.dp1.di_size = makedir(&buf, (struct direct *)oroot_dir, PREDEFDIR);
		} else {
			node.dp1.di_size = makedir(&buf, root_dir, PREDEFDIR);
		}
		node.dp1.di_db[0] = alloc(sblock.fs_fsize, node.dp1.di_mode);
		if (node.dp1.di_db[0] == 0) {
			return;
		}
		node.dp1.di_blocks = btodb(fragroundup(&sblock, node.dp1.di_size));
		wtfs(fsbtodb(&sblock, node.dp1.di_db[0]), sblock.fs_fsize, buf);
	} else {
		if (mfs) {
			node.dp2.di_mode = IFDIR | mfsmode;
			node.dp2.di_uid = mfsuid;
			node.dp2.di_gid = mfsgid;
		} else {
			node.dp2.di_mode = IFDIR | UMASK;
			node.dp2.di_uid = geteuid();
			node.dp2.di_gid = getegid();
		}
		node.dp2.di_atime = utime->tv_sec;
		node.dp2.di_atimensec = utime->tv_usec * 1000;
		node.dp2.di_mtime = utime->tv_sec;
		node.dp2.di_mtimensec = utime->tv_usec * 1000;
		node.dp2.di_ctime = utime->tv_sec;
		node.dp2.di_ctimensec = utime->tv_usec * 1000;
		node.dp2.di_birthtime = utime->tv_sec;
		node.dp2.di_birthnsec = utime->tv_usec * 1000;
		node.dp2.di_nlink = PREDEFDIR;
		node.dp2.di_size = makedir(&buf, root_dir, PREDEFDIR);
		node.dp2.di_db[0] = alloc(sblock.fs_fsize, node.dp2.di_mode);
		if (node.dp2.di_db[0] == 0) {
			return;
		}
		node.dp2.di_blocks = btodb(fragroundup(&sblock, node.dp2.di_size));
		wtfs(fsbtodb(&sblock, node.dp2.di_db[0]), sblock.fs_fsize, &buf);
	}
	iput(&node, ROOTINO);
}

/*
 * construct a set of directory entries in "buf".
 * return size of directory.
 */
int
makedir(protodir, entries)
	register struct direct *protodir;
	int entries;
{
	char *cp;
	int i, spcleft;

	spcleft = UFS_DIRBLKSIZ;
	for (cp = buf, i = 0; i < entries - 1; i++) {
		protodir[i].d_reclen = DIRSIZ(0, &protodir[i]);
		memmove(cp, &protodir[i], protodir[i].d_reclen);
		cp += protodir[i].d_reclen;
		spcleft -= protodir[i].d_reclen;
	}
	protodir[i].d_reclen = spcleft;
	memmove(cp, &protodir[i], DIRSIZ(0, &protodir[i]));
	return (UFS_DIRBLKSIZ);
}

/*
 * allocate a block or frag
 */
daddr_t
alloc(size, mode)
	int size;
	int mode;
{
	int i, frag;
	daddr_t d, blkno;

	rdfs(fsbtodb(&sblock, cgtod(&sblock, 0)), sblock.fs_cgsize,
	    (char *)&acg);
	if (acg.cg_magic != CG_MAGIC) {
		printf("cg 0: bad magic number\n");
		return (0);
	}
	if (acg.cg_cs.cs_nbfree == 0) {
		printf("first cylinder group ran out of space\n");
		return (0);
	}
	for (d = 0; d < acg.cg_ndblk; d += sblock.fs_frag)
		if (isblock(&sblock, cg_blksfree(&acg), d / sblock.fs_frag))
			goto goth;
	printf("internal error: can't find block in cyl 0\n");
	return (0);
goth:
	blkno = fragstoblks(&sblock, d);
	clrblock(&sblock, cg_blksfree(&acg), blkno);
	if (sblock.fs_contigsumsize > 0)
		clrbit(cg_clustersfree(&acg), blkno);
	acg.cg_cs.cs_nbfree--;
	sblock.fs_cstotal.cs_nbfree--;
	fscs[0].cs_nbfree--;
	if (mode & IFDIR) {
		acg.cg_cs.cs_ndir++;
		sblock.fs_cstotal.cs_ndir++;
		fscs[0].cs_ndir++;
	}
	cg_blktot(&acg)[cbtocylno(&sblock, d)]--;
	cg_blks(&sblock, &acg, cbtocylno(&sblock, d))[cbtorpos(&sblock, d)]--;
	if (size != sblock.fs_bsize) {
		frag = howmany(size, sblock.fs_fsize);
		fscs[0].cs_nffree += sblock.fs_frag - frag;
		sblock.fs_cstotal.cs_nffree += sblock.fs_frag - frag;
		acg.cg_cs.cs_nffree += sblock.fs_frag - frag;
		acg.cg_frsum[sblock.fs_frag - frag]++;
		for (i = frag; i < sblock.fs_frag; i++)
			setbit(cg_blksfree(&acg), d + i);
	}
	wtfs(fsbtodb(&sblock, cgtod(&sblock, 0)), sblock.fs_cgsize,
	    (char *)&acg);
	return (d);
}

/*
 * Calculate number of inodes per group.
 */
long
calcipg(cpg, bpcg, usedbp)
	long cpg;
	long bpcg;
	off_t *usedbp;
{
	int i;
	long ipg, new_ipg, ncg, ncyl;
	off_t usedb;

	/*
	 * Prepare to scale by fssize / (number of sectors in cylinder groups).
	 * Note that fssize is still in sectors, not filesystem blocks.
	 */
	ncyl = howmany(fssize, secpercyl);
	ncg = howmany(ncyl, cpg);
	/*
	 * Iterate a few times to allow for ipg depending on itself.
	 */
	ipg = 0;
	for (i = 0; i < 10; i++) {
		usedb = (sblock.fs_iblkno + ipg / INOPF(&sblock))
			* NSPF(&sblock) * (off_t)sectorsize;
		new_ipg = (cpg * (quad_t)bpcg - usedb) / density * fssize
			  / ncg / secpercyl / cpg;
		new_ipg = roundup(new_ipg, INOPB(&sblock));
		if (new_ipg == ipg)
			break;
		ipg = new_ipg;
	}
	*usedbp = usedb;
	return (ipg);
}

/*
 * Allocate an inode on the disk
 */
void
iput(ip, ino)
	register union dinode *ip;
	register ino_t ino;
{
	struct ufs1_dinode dp1[UFS1_MAXINOPB];
	struct ufs2_dinode dp2[UFS2_MAXINOPB];
	daddr_t d;
	int c;

	c = ino_to_cg(&sblock, ino);
	rdfs(fsbtodb(&sblock, cgtod(&sblock, 0)), sblock.fs_cgsize, (char *)&acg);
	if (acg.cg_magic != CG_MAGIC) {
		printf("cg 0: bad magic number\n");
		exit(31);
	}
	acg.cg_cs.cs_nifree--;
	setbit(cg_inosused(&acg), ino);
	wtfs(fsbtodb(&sblock, cgtod(&sblock, 0)), sblock.fs_cgsize, (char *)&acg);
	sblock.fs_cstotal.cs_nifree--;
	fscs[0].cs_nifree--;
	if (ino >= sblock.fs_ipg * sblock.fs_ncg) {
		printf("fsinit: inode value out of range (%d).\n", ino);
		exit(32);
	}
	d = fsbtodb(&sblock, ino_to_fsba(&sblock, ino));
	rdfs(d, sblock.fs_bsize, buf);
	if (sblock.fs_magic == FS_UFS1_MAGIC) {
		/*
		dp1 = (struct ufs1_dinode *)buf;
		dp1 += ino_to_fsbo(&sblock, ino);
		*/
		dp1[ino_to_fsbo(&sblock, ino)] = *ip->dp1;
	} else {
		/*
		dp2 = (struct ufs2_dinode *)buf;
		dp2 += ino_to_fsbo(&sblock, ino);
		*/
		dp2[ino_to_fsbo(&sblock, ino)] = *ip->dp2;
	}
	wtfs(d, sblock.fs_bsize, buf);
}

/*
 * Notify parent process that the filesystem has created itself successfully.
 */
void
started()
{

	exit(0);
}

/*
 * Replace libc function with one suited to our needs.
 */
caddr_t
malloc(size)
	register u_long size;
{
	char *base, *i;
	static u_long pgsz;
	struct rlimit rlp;

	if (pgsz == 0) {
		base = sbrk(0);
		pgsz = getpagesize() - 1;
		i = (char *)((u_long)(base + pgsz) &~ pgsz);
		base = sbrk(i - base);
		if (getrlimit(RLIMIT_DATA, &rlp) < 0)
			perror("getrlimit");
		rlp.rlim_cur = rlp.rlim_max;
		if (setrlimit(RLIMIT_DATA, &rlp) < 0)
			perror("setrlimit");
		memleft = rlp.rlim_max - (u_long)base;
	}
	size = (size + pgsz) &~ pgsz;
	if (size > memleft)
		size = memleft;
	memleft -= size;
	if (size == 0)
		return (0);
	return ((caddr_t)sbrk(size));
}
#ifdef notyet
/*
 * Replace libc function with one suited to our needs.
 */
caddr_t
realloc(ptr, size)
	char *ptr;
	u_long size;
{
	void *p;

	if ((p = malloc(size)) == NULL)
		return (NULL);
	memmove(p, ptr, size);
	free(ptr);
	return (p);
}

/*
 * Replace libc function with one suited to our needs.
 */
char *
calloc(size, numelm)
	u_long size, numelm;
{
	caddr_t base;

	size *= numelm;
	base = malloc(size);
	memset(base, 0, size);
	return (base);
}

/*
 * Replace libc function with one suited to our needs.
 */
free(ptr)
	char *ptr;
{
	
	/* do not worry about it for now */
}
#endif
/*
 * read a block from the file system
 */
void
rdfs(bno, size, bf)
	daddr_t bno;
	int size;
	char *bf;
{
	int n;

	if (mfs) {
		memmove(bf, membase + bno * sectorsize, size);
		return;
	}
	if (lseek(fsi, (off_t)bno * sectorsize, 0) < 0) {
		printf("seek error: %ld\n", bno);
		perror("rdfs");
		exit(33);
	}
	n = read(fsi, bf, size);
	if (n != size) {
		printf("read error: %ld\n", bno);
		perror("rdfs");
		exit(34);
	}
}

/*
 * write a block to the file system
 */
void
wtfs(bno, size, bf)
	daddr_t bno;
	int size;
	char *bf;
{
	int n;

	if (mfs) {
		memmove(membase + bno * sectorsize, bf, size);
		return;
	}
	if (Nflag)
		return;
	if (lseek(fso, (off_t)bno * sectorsize, SEEK_SET) < 0) {
		printf("seek error: %ld\n", bno);
		perror("wtfs");
		exit(35);
	}
	n = write(fso, bf, size);
	if (n != size) {
		printf("write error: %ld\n", bno);
		perror("wtfs");
		exit(36);
	}
}

/*
 * check if a block is available
 */
int
isblock(fs, cp, h)
	struct fs *fs;
	unsigned char *cp;
	int h;
{
	unsigned char mask;

	switch (fs->fs_frag) {
	case 8:
		return (cp[h] == 0xff);
	case 4:
		mask = 0x0f << ((h & 0x1) << 2);
		return ((cp[h >> 1] & mask) == mask);
	case 2:
		mask = 0x03 << ((h & 0x3) << 1);
		return ((cp[h >> 2] & mask) == mask);
	case 1:
		mask = 0x01 << (h & 0x7);
		return ((cp[h >> 3] & mask) == mask);
	default:
#ifdef STANDALONE
		printf("isblock bad fs_frag %d\n", fs->fs_frag);
#else
		fprintf(stderr, "isblock bad fs_frag %d\n", fs->fs_frag);
#endif
		return (0);
	}
}

/*
 * take a block out of the map
 */
void
clrblock(fs, cp, h)
	struct fs *fs;
	unsigned char *cp;
	int h;
{
	switch ((fs)->fs_frag) {
	case 8:
		cp[h] = 0;
		return;
	case 4:
		cp[h >> 1] &= ~(0x0f << ((h & 0x1) << 2));
		return;
	case 2:
		cp[h >> 2] &= ~(0x03 << ((h & 0x3) << 1));
		return;
	case 1:
		cp[h >> 3] &= ~(0x01 << (h & 0x7));
		return;
	default:
#ifdef STANDALONE
		printf("clrblock bad fs_frag %d\n", fs->fs_frag);
#else
		fprintf(stderr, "clrblock bad fs_frag %d\n", fs->fs_frag);
#endif
		return;
	}
}

/*
 * put a block into the map
 */
void
setblock(fs, cp, h)
	struct fs *fs;
	unsigned char *cp;
	int h;
{
	switch (fs->fs_frag) {
	case 8:
		cp[h] = 0xff;
		return;
	case 4:
		cp[h >> 1] |= (0x0f << ((h & 0x1) << 2));
		return;
	case 2:
		cp[h >> 2] |= (0x03 << ((h & 0x3) << 1));
		return;
	case 1:
		cp[h >> 3] |= (0x01 << (h & 0x7));
		return;
	default:
#ifdef STANDALONE
		printf("setblock bad fs_frag %d\n", fs->fs_frag);
#else
		fprintf(stderr, "setblock bad fs_frag %d\n", fs->fs_frag);
#endif
		return;
	}
}
