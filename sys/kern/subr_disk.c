/*	$NetBSD: subr_disk.c,v 1.60 2004/03/09 12:23:07 yamt Exp $	*/
/*
 * Copyright (c) 1982, 1986, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)ufs_disksubr.c	8.5.5 (2.11BSD GTE) 1998/4/3
 */
/*
 * Copyright (c) 1982, 1986, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)ufs_disksubr.c	8.5 (Berkeley) 1/21/94
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/syslog.h>
#include <sys/time.h>
#include <sys/disklabel.h>
#include <sys/diskslice.h>
#include <sys/disk.h>
#include <sys/user.h>

#include <machine/param.h>

/*
 * Initialize the disklist.  Called by main() before autoconfiguration.
 */
struct disklist_head 	disklist = TAILQ_HEAD_INITIALIZER(disklist);	/* TAILQ_HEAD */
int						disk_count = 0;	/* number of drives in global disklist */

/*
 * Searches the disklist for the disk corresponding to the
 * name provided.
 */
struct dkdevice *
disk_find(name)
	char *name;
{
	struct dkdevice *diskp;

	if ((name == NULL) || (disk_count <= 0))
		return (NULL);

	for (diskp = TAILQ_FIRST(&disklist); diskp != NULL; diskp = TAILQ_NEXT(diskp, dk_link)) {
		if (strcmp(diskp->dk_name, name) == 0) {
			return (diskp);
		}
	}
	return (NULL);
}

/*
 * Attach a disk.
 */
void
disk_attach(diskp)
	struct dkdevice *diskp;
{
	int s;
	diskp->dk_label = (struct disklabel *)malloc(sizeof(struct disklabel *), M_DEVBUF, M_NOWAIT);
	diskp->dk_slices = (struct diskslices *)malloc(sizeof(struct diskslices *), M_DEVBUF, M_NOWAIT);

	if (diskp->dk_label == NULL) {
		panic("disk_attach: can't allocate storage for disklabel");
	} else {
		bzero(diskp->dk_label, sizeof(struct disklabel));
	}
	if (diskp->dk_slices == NULL) {
		panic("disk_attach: can't allocate storage for diskslices");
	} else {
		bzero(diskp->dk_slices, sizeof(struct diskslices));
	}

	/*
	 * Set the attached timestamp.
	 */
	s = splclock();
	diskp->dk_attachtime = mono_time;
	splx(s);

	/*
	 * Link into the disklist.
	 */
	TAILQ_INSERT_TAIL(&disklist, diskp, dk_link);
	++disk_count;
}

/*
 * Detach a disk.
 */
void
disk_detach(diskp)
	struct dkdevice *diskp;
{
	/*
	 * Remove from the disklist.
	 */
	if (--disk_count < 0) {
		panic("disk_detach: disk_count < 0");
	}
	TAILQ_REMOVE(&disklist, diskp, dk_link);

	/*
	 * Free the space used by the disklabel structures.
	 */
	free(diskp->dk_label, M_DEVBUF);
	free(diskp->dk_slices, M_DEVBUF);
}

/*
 * Increment a disk's busy counter.  If the counter is going from
 * 0 to 1, set the timestamp.
 */
void
disk_busy(diskp)
	struct dkdevice *diskp;
{
	int s;

	/*
	 * XXX We'd like to use something as accurate as microtime(),
	 * but that doesn't depend on the system TOD clock.
	 */
	if (diskp->dk_busy++ == 0) {
		s = splclock();
		diskp->dk_timestamp = mono_time;
		splx(s);
	}
}

/*
 * Decrement a disk's busy counter, increment the byte count, total busy
 * time, and reset the timestamp.
 */
void
disk_unbusy(diskp, bcount)
	struct dkdevice *diskp;
	long bcount;
{
	int s;
	struct timeval dv_time, diff_time;

	if (diskp->dk_busy-- == 0) {
		printf("%s: dk_busy < 0\n", diskp->dk_name);
		panic("disk_unbusy");
	}

	s = splclock();
	dv_time = mono_time;
	splx(s);

	timersub(&dv_time, &diskp->dk_timestamp, &diff_time);
	timeradd(&diskp->dk_time, &diff_time, &diskp->dk_time);

	diskp->dk_timestamp = dv_time;
	if (bcount > 0) {
		diskp->dk_bytes += bcount;
		diskp->dk_bps++;
	}
}

int
disk_ioctl(diskp, dev, cmd, data, flag, p)
	struct dkdevice *diskp;
	dev_t			dev;
	u_long			cmd;
	void 			*data;
	int				flag;
	struct proc 	*p;
{
	struct disklabel *lp;
	struct diskslices *ssp;
	int error;

	lp = diskp->dk_label;
	ssp = diskp->dk_slices;

	if (dev == NODEV) {
		return (ENOIOCTL);
	}

	switch (cmd) {
	case DIOCGSECTORSIZE:
		*(u_int *)data = lp->d_secsize;
		return (0);

	case DIOCGMEDIASIZE:
		*(off_t *)data = (off_t)lp->d_secsize * lp->d_secperunit;
		return (0);

	default:
		return (ENOIOCTL);
	}

	return (dsioctl(dev, cmd, data, flag, &ssp));
}

/*
 * Reset the metrics counters on the given disk.  Note that we cannot
 * reset the busy counter, as it may case a panic in disk_unbusy().
 * We also must avoid playing with the timestamp information, as it
 * may skew any pending transfer results.
 */
void
disk_resetstat(diskp)
	struct dkdevice *diskp;
{
	int s = splbio(), t;

	diskp->dk_bps = 0;
	diskp->dk_bytes = 0;

	t = splclock();
	diskp->dk_attachtime = mono_time;
	splx(t);

	timerclear(&diskp->dk_time);

	splx(s);
}

/*
 * Seek sort for disks.  We depend on the driver which calls us using b_resid
 * as the current cylinder number.
 *
 * The argument ap structure holds a b_actf activity chain pointer on which we
 * keep two queues, sorted in ascending cylinder order.  The first queue holds
 * those requests which are positioned after the current cylinder (in the first
 * request); the second holds requests which came in after their cylinder number
 * was passed.  Thus we implement a one way scan, retracting after reaching the
 * end of the drive to the first request on the second queue, at which time it
 * becomes the first queue.
 *
 * A one-way scan is natural because of the way UNIX read-ahead blocks are
 * allocated.
 */

/*
 * For portability with historic industry practice, the
 * cylinder number has to be maintained in the `b_resid'
 * field.
 */
/*
 * bufq: the disk buffer queue.
 * buf: the buffer to sort.
 * flags: additional flags to set. BUFQ_DISKSORT is set by default.
 */
void
disksort(bufq, ap, flags)
	struct bufq_state   *bufq;
	struct buf          *ap;
	int 				flags;
{
	if (bufq == NULL) {
        bufq_alloc(bufq, (BUFQ_DISKSORT | flags));
    }
    if((ap = BUFQ_PEEK(bufq)) != NULL) {
        printf("buffer already allocated");
    } else {
        BUFQ_PUT(bufq, ap);
    }
}

/*
 * Disk error is the preface to plaintive error messages
 * about failing disk transfers.  It prints messages of the form

hp0g: hard error reading fsbn 12345 of 12344-12347 (hp0 bn %d cn %d tn %d sn %d)

 * if the offset of the error in the transfer and a disk label
 * are both available.  blkdone should be -1 if the position of the error
 * is unknown; the disklabel pointer may be null from drivers that have not
 * been converted to use them.  The message is printed with printf
 * if pri is LOG_PRINTF, otherwise it uses log at the specified priority.
 * The message should be completed (with at least a newline) with printf
 * or addlog, respectively.  There is no trailing space.
 */
void
diskerr(bp, dname, what, pri, blkdone, lp)
	register struct buf *bp;
	char *dname, *what;
	int pri, blkdone;
	register struct disklabel *lp;
{
	int unit;
	int part;
	register void (*pr)(const char *, ...);
	char partname;
	int sn;

	unit = dkunit(bp->b_dev);
	part = dkpart(bp->b_dev);
	partname = 'a' + part;

	if (pri != LOG_PRINTF) {
		log(pri, "");
		pr = addlog;
	} else {
		pr = printf;
	}
	(*pr)("%s%d%c: %s %sing fsbn ", dname, unit, partname, what, bp->b_flags & B_READ ? "read" : "writ");
	sn = bp->b_blkno;
	if (bp->b_bcount <= DEV_BSIZE) {
		(*pr)("%d", sn);
	} else {
		if (blkdone >= 0) {
			sn += blkdone;
			(*pr)("%d of ", sn);
		}
		(*pr)("%d-%d", bp->b_blkno,
				bp->b_blkno + (bp->b_bcount - 1) / DEV_BSIZE);
	}
	if (lp && (blkdone >= 0 || bp->b_bcount <= lp->d_secsize)) {
		sn += lp->d_partitions[part].p_offset;
		(*pr)(" (%s%d bn %d; cn %d", dname, unit, sn, sn / lp->d_secpercyl);
		sn %= lp->d_secpercyl;
		(*pr)(" tn %d sn %d)", sn / lp->d_nsectors, sn % lp->d_nsectors);
	}
}

/* disk functions for easy access */
struct dkdriver *
disk_driver(disk, dev)
	struct dkdevice *disk;
	dev_t 			dev;
{
	return (disk[dkunit(dev)].dk_driver);
}

struct disklabel *
disk_label(disk, dev)
	struct dkdevice *disk;
	dev_t 			dev;
{
	return (disk[dkunit(dev)].dk_label);
}

struct diskslices *
disk_slices(disk, dev)
	struct dkdevice *disk;
	dev_t 			dev;
{
	return (disk[dkunit(dev)].dk_slices);
}

struct partition *
disk_partition(disk, dev)
	struct dkdevice *disk;
	dev_t 			dev;
{
	return (disk[dkunit(dev)].dk_parts);
}

struct device
disk_device(disk, dev)
	struct dkdevice *disk;
	dev_t 			dev;
{
	return (disk[dkunit(dev)].dk_dev);
}

struct dkdevice *
disk_find_by_dev(dev)
   dev_t dev;
{
    struct dkdevice *diskp;
    
    for (diskp = TAILQ_FIRST(&disklist); diskp != NULL; diskp = TAILQ_NEXT(diskp, dk_link)) {
        if(&diskp[dkunit(dev)] == diskp) {
            return (diskp);
        }
    }
    return (NULL);
}

struct dkdevice *
disk_find_by_slice(slicep)
   struct diskslices *slicep;
{
    struct dkdevice *diskp;
    
    for (diskp = TAILQ_FIRST(&disklist); diskp != NULL; diskp = TAILQ_NEXT(diskp, dk_link)) {
        if(diskp->dk_slices == slicep) {
            return (diskp);
        }
    }
    return (NULL);
}
