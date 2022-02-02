/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)quota_ufs.c	7.1.1 (2.11BSD GTE) 12/31/93
 */

/*
 * MELBOURNE QUOTAS
 *
 * Routines used in checking limits on file system usage.
 */

#include <sys/cdefs.h>
#include <sys/errno.h>
#include <sys/user.h>
#include <sys/param.h>

#include <ufs/ufs211/ufs211_extern.h>
#include <ufs/ufs211/ufs211_fs.h>
#include <ufs/ufs211/ufs211_inode.h>
#include <ufs/ufs211/ufs211_mount.h>
#include <ufs/ufs211/ufs211_quota.h>

#ifdef QUOTA

/*
 * Find the dquot structure that should
 * be used in checking i/o on inode ip.
 */
struct ufs211_dquot *
inoquota(ip)
	register struct ufs211_inode *ip;
{
	register struct ufs211_quota *q;
	register struct ufs211_dquot **dqq;
	register struct ufs211_mount *mp;
	int index;

 top:
	q = qfind(ip->i_uid);
	if (q == NOQUOTA) {
		for (mp = mount; mp < &mount[NMOUNT]; mp++)
#ifdef pdp11
			if (mp->m_inodp && mp->m_dev == ip->i_dev)
#else
			if (mp->m_bufp && mp->m_dev == ip->i_dev)
#endif
				return (discquota(ip->i_uid, mp->m_qinod));
		panic("inoquota");
	}

	/*
	 * We have a quota struct in core (most likely our own) that
	 * belongs to the same user as the inode
	 */
	if (q->q_flags & Q_NDQ)
		return (NODQUOT);
	if (q->q_flags & Q_LOCK) {
		q->q_flags |= Q_WANT;
#ifdef pdp11
		QUOTAUNMAP();
		sleep((caddr_t)q, PINOD+1);
		QUOTAMAP();
#else
		sleep((caddr_t)q, PINOD+1);
#endif
		goto top;		/* might just have been freed */
	}
	index = getfsx(ip->i_dev);
	dqq = &q->q_dq[index];
	if (*dqq == LOSTDQUOT) {
		q->q_flags |= Q_LOCK;
		*dqq = discquota(q->q_uid, mount[index].m_qinod);
		if (*dqq != NODQUOT)
			(*dqq)->dq_own = q;
		if (q->q_flags & Q_WANT)
			wakeup((caddr_t) q);
		q->q_flags &= ~(Q_LOCK | Q_WANT);
	}
	if (*dqq != NODQUOT)
		(*dqq)->dq_cnt++;
	return (*dqq);
}

/*
 * Update disc usage, and take corrective action.
 */
int
chkdq(ip, change, force)
	register struct ufs211_inode *ip;
	long change;
	int force;
{
	register struct ufs211_dquot *dq;

	if (change == 0)
		return (0);
#ifdef pdp11
	dq = ix_dquot[ip - inode];
#else
	dq = ip->i_dquot;
#endif
	if (dq == NODQUOT)
		return (0);
	if (dq->dq_bsoftlimit == 0)
		return (0);
	dq->dq_flags |= DQ_MOD;
	/*
	 * reset warnings if below disk quota.
	 */
        if (dq->dq_bwarn == 0 && dq->dq_bsoftlimit &&
	    (dq->dq_curblocks + change) < dq->dq_bsoftlimit) {
		dq->dq_bwarn = MAX_DQ_WARN;
		if (dq->dq_own == u->u_quota) {
			uprintf("\nUNDER DISC QUOTA: (%s) by %d Kbytes\n",
				ip->i_fs->fs_fsmnt,
#ifdef pdp11
				(dq->dq_bsoftlimit + 1023L - (dq->dq_curblocks
				 + change)) / 1024);
#else
				dbtob(dq->dq_bsoftlimit -
				(dq->dq_curblocks + change)) / 1024);
#endif
		}
	}
	if (change < 0) {
		if (dq->dq_curblocks + change >= 0)
			dq->dq_curblocks += change;
		else
			dq->dq_curblocks = 0;
		dq->dq_flags &= ~DQ_BLKS;
		return (0);
	}

	/*
	 * If user is over quota, or has run out of warnings, then
	 * disallow space allocation (except su's are never stopped).
	 */
	if (u->u_uid == 0)
		force = 1;
	if (!force && dq->dq_bwarn == 0) {
		if ((dq->dq_flags & DQ_BLKS) == 0 && dq->dq_own == u->u_quota) {
		     uprintf("\nOVER DISC QUOTA: (%s) NO MORE DISC SPACE\n",
			ip->i_fs->fs_fsmnt);
		     dq->dq_flags |= DQ_BLKS;
		}
		return (EDQUOT);
	}
	if (dq->dq_curblocks < dq->dq_bsoftlimit) {
		dq->dq_curblocks += change;
		if (dq->dq_curblocks < dq->dq_bsoftlimit)
			return (0);
		if (dq->dq_own == u->u_quota)
			uprintf("\nWARNING: disc quota (%s) exceeded\n",
			   ip->i_fs->fs_fsmnt);
		return (0);
	}
	if (!force && dq->dq_bhardlimit &&
	    dq->dq_curblocks + change >= dq->dq_bhardlimit) {
		if ((dq->dq_flags & DQ_BLKS) == 0 && dq->dq_own == u->u_quota) {
			uprintf("\nDISC LIMIT REACHED (%s) - WRITE FAILED\n",
			   ip->i_fs->fs_fsmnt);
			dq->dq_flags |= DQ_BLKS;
		}
		return (EDQUOT);
	}
	/*
	 * User is over quota, but not over limit
	 * or is over limit, but we have been told
	 * there is nothing we can do.
	 */
	dq->dq_curblocks += change;
	return (0);
}

/*
 * Check the inode limit, applying corrective action.
 */
int
chkiq(dev, ip, uid, force)
	dev_t dev;
	uid_t uid;
	register struct ufs211_inode *ip;
	int force;
{
	register struct ufs211_dquot *dq;
	register struct ufs211_quota *q;

	if (ip == NULL) { /* allocation */
		q = qfind(uid);
		if (q != NOQUOTA)
			dq = dqp(q, dev);
		else
			dq = discquota(uid, mount[getfsx(dev)].m_qinod);
	} else { /* free */
#ifdef pdp11
		dq = ix_dquot[ip - inode];
#else
		dq = ip->i_dquot;
#endif
		if (dq != NODQUOT)
			dq->dq_cnt++;
	}
	if (dq == NODQUOT)
		return (0);
	if (dq->dq_isoftlimit == 0) {
		dqrele(dq);
		return (0);
	}
	dq->dq_flags |= DQ_MOD;
	if (ip) { /* a free */
		if (dq->dq_curinodes)
			dq->dq_curinodes--;
		dq->dq_flags &= ~DQ_INODS;
		dqrele(dq);
		return (0);
	}

	/*
	 * The following shouldn't be necessary, as if u.u_uid == 0
	 * then dq == NODQUOT & we wouldn't get here at all, but
	 * then again, its not going to harm anything ...
	 */
	if (u->u_uid == 0) /* su's musn't be stopped */
		force = 1;
	if (!force && dq->dq_iwarn == 0) {
		if ((dq->dq_flags & DQ_INODS) == 0 && dq->dq_own == u->u_quota) {
			uprintf("\nOVER FILE QUOTA - NO MORE FILES (%s)\n",
					getfs(dq->dq_dev)->fs_fsmnt);
			dq->dq_flags |= DQ_INODS;
		}
		dqrele(dq);
		return (EDQUOT);
	}
	if (dq->dq_curinodes < dq->dq_isoftlimit) {
		if (++dq->dq_curinodes >= dq->dq_isoftlimit && dq->dq_own == u->u_quota)
			uprintf("\nWARNING - too many files (%s)\n",
					getfs(dq->dq_dev)->fs_fsmnt);
		dqrele(dq);
		return (0);
	}
	if (!force && dq->dq_ihardlimit && dq->dq_curinodes + 1 >= dq->dq_ihardlimit) {
		if ((dq->dq_flags & DQ_INODS) == 0 && dq->dq_own == u->u_quota) {
			uprintf("\nFILE LIMIT REACHED - CREATE FAILED (%s)\n",
					getfs(dq->dq_dev)->fs_fsmnt);
			dq->dq_flags |= DQ_INODS;
		}
		dqrele(dq);
		return (EDQUOT);
	}
	/*
	 * Over quota but not at limit;
	 * or over limit, but we aren't
	 * allowed to stop it.
	 */
	dq->dq_curinodes++;
	dqrele(dq);
	return (0);
}
#endif
