/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)kern_resource.c	1.4 (2.11BSD GTE) 1997/2/14
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <vm/include/vm.h>

/*
 * Resource controls and accounting.
 */
void
getpriority()
{
	register struct a {
		syscallarg(int)	which;
		syscallarg(int)	who;
	} *uap = (struct a *)u->u_ap;
	register struct proc *p;
	register int low = PRIO_MAX + 1;

	switch (uap->which) {

	case PRIO_PROCESS:
		if (uap->who == 0)
			p = u->u_procp;
		else
			p = pfind(uap->who);
		if (p == 0)
			break;
		low = p->p_nice;
		break;

	case PRIO_PGRP:
		if (uap->who == 0)
			uap->who = u->u_procp->p_pgrp;
		for (p = allproc; p != NULL; p = p->p_nxt) {
			if (p->p_pgrp == uap->who &&
			    p->p_nice < low)
				low = p->p_nice;
		}
		break;

	case PRIO_USER:
		if (uap->who == 0)
			uap->who = u->u_uid;
		for (p = allproc; p != NULL; p = p->p_nxt) {
			if (p->p_uid == uap->who &&
			    p->p_nice < low)
				low = p->p_nice;
		}
		break;

	default:
		u->u_error = EINVAL;
		return;
	}
	if (low == PRIO_MAX + 1) {
		u->u_error = ESRCH;
		return;
	}
	u->u_r.r_val1 = low;
}

void
setpriority()
{
	register struct a {
		syscallarg(int)	which;
		syscallarg(int)	who;
		syscallarg(int)	prio;
	} *uap = (struct a *)u->u_ap;
	register struct proc *p;
	register int found = 0;

	switch (uap->which) {

	case PRIO_PROCESS:
		if (uap->who == 0)
			p = u->u_procp;
		else
			p = pfind(uap->who);
		if (p == 0)
			break;
		donice(p, uap->prio);
		found++;
		break;

	case PRIO_PGRP:
		if (uap->who == 0)
			uap->who = u->u_procp->p_pgrp;
		for (p = allproc; p != NULL; p = p->p_nxt)
			if (p->p_pgrp == uap->who) {
				donice(p, uap->prio);
				found++;
			}
		break;

	case PRIO_USER:
		if (uap->who == 0)
			uap->who = u->u_uid;
		for (p = allproc; p != NULL; p = p->p_nxt)
			if (p->p_uid == uap->who) {
				donice(p, uap->prio);
				found++;
			}
		break;

	default:
		u->u_error = EINVAL;
		return;
	}
	if (found == 0)
		u->u_error = ESRCH;
}

static void
donice(p, n)
	register struct proc *p;
	register int n;
{
	if (u->u_uid && u->u_pcred->p_ruid &&
	    u->u_uid != p->p_uid && u->u_pcred->p_ruid != p->p_uid) {
		u->u_error = EPERM;
		return;
	}
	if (n > PRIO_MAX)
		n = PRIO_MAX;
	if (n < PRIO_MIN)
		n = PRIO_MIN;
	if (n < p->p_nice && !suser()) {
		u->u_error = EACCES;
		return;
	}
	p->p_nice = n;
}

void
setrlimit()
{
	register struct a {
		syscallarg(u_int) which;
		syscallarg(struct rlimit *) lim;
	} *uap = (struct a *)u->u_ap;
	struct rlimit alim;
	register struct rlimit *alimp;
	extern unsigned maxdmap;

	if (uap->which >= RLIM_NLIMITS) {
		u->u_error = EINVAL;
		return;
	}
	alimp = &u->u_rlimit[uap->which];
	u->u_error = copyin((caddr_t)uap->lim, (caddr_t)&alim,
		sizeof (struct rlimit));
	if (u->u_error)
		return;
	if (uap->which == RLIMIT_CPU) {
		/*
		 * 2.11 stores RLIMIT_CPU as ticks to keep from making
		 * hardclock() do long multiplication/division.
		 */
		if (alim.rlim_cur >= RLIM_INFINITY / hz)
			alim.rlim_cur = RLIM_INFINITY;
		else
			alim.rlim_cur = alim.rlim_cur * hz;
		if (alim.rlim_max >= RLIM_INFINITY / hz)
			alim.rlim_max = RLIM_INFINITY;
		else
			alim.rlim_max = alim.rlim_max * hz;
	}
	if (alim.rlim_cur > alimp->rlim_max || alim.rlim_max > alimp->rlim_max)
		if (!suser())
			return;
	*alimp = alim;
}

void
getrlimit()
{
	register struct a {
		syscallarg(u_int) 			which;
		syscallarg(struct rlimit *) rlp;
	} *uap = (struct a *)u->u_ap;

	if (uap->which >= RLIM_NLIMITS) {
		u->u_error = EINVAL;
		return;
	}
	if (uap->which == RLIMIT_CPU) {
		struct rlimit alim;

		alim = u->u_rlimit[uap->which];
		if (alim.rlim_cur != RLIM_INFINITY)
			alim.rlim_cur = alim.rlim_cur / hz;
		if (alim.rlim_max != RLIM_INFINITY)
			alim.rlim_max = alim.rlim_max / hz;
		u->u_error = copyout((caddr_t)&alim,
		    (caddr_t)uap->rlp,sizeof (struct rlimit));
	}
	else u->u_error = copyout((caddr_t)&u->u_rlimit[uap->which],
	    (caddr_t)uap->rlp,sizeof (struct rlimit));
}

void
getrusage()
{
	register struct a {
		syscallarg(int) who;
		syscallarg(struct rusage *) rusage;
	} *uap = (struct a *)u->u_ap;
	register struct k_rusage *rup;
	struct rusage ru;

	switch (uap->who) {

	case RUSAGE_SELF:
		rup = &u->u_ru;
		break;

	case RUSAGE_CHILDREN:
		rup = &u->u_cru;
		break;

	default:
		u->u_error = EINVAL;
		return;
	}
	rucvt(&ru,rup);
	u->u_error = copyout((caddr_t)&ru, (caddr_t)uap->rusage,
		sizeof (struct rusage));
}

void
ruadd(ru, ru2)
	struct k_rusage *ru, *ru2;
{
	register long *ip, *ip2;
	register int i;
	struct rusage *kru, *kru2;

	/*
	 * since the kernel timeval structures are single longs,
	 * fold them into the loop.
	 */
	timevaladd(&ru->ru_utime, &ru2->ru_utime);
	timevaladd(&ru->ru_stime, &ru2->ru_stime);
	if (kru->ru_maxrss < kru2->ru_maxrss)
		kru->ru_maxrss = kru2->ru_maxrss;
	ip = &ru->k_ru_first; ip2 = &ru2->k_ru_first;
	for (i = &ru->k_ru_last - &ru->k_ru_first; i >= 0; i--)
		*ip++ += *ip2++;
}

/*
 * Convert an internal kernel rusage structure into a `real' rusage structure.
 */
void
rucvt(rup, krup)
	register struct rusage		*rup;
	register struct k_rusage	*krup;
{
	bzero((caddr_t)rup, sizeof(*rup));
	rup->ru_utime.tv_sec   = krup->ru_utime / hz;
	rup->ru_utime.tv_usec  = (krup->ru_utime % hz) * mshz;
	rup->ru_stime.tv_sec   = krup->ru_stime / hz;
	rup->ru_stime.tv_usec  = (krup->ru_stime % hz) * mshz;
	rup->ru_ovly = krup->ru_ovly;
	rup->ru_nswap = krup->ru_nswap;
	rup->ru_inblock = krup->ru_inblock;
	rup->ru_oublock = krup->ru_oublock;
	rup->ru_msgsnd = krup->ru_msgsnd;
	rup->ru_msgrcv = krup->ru_msgrcv;
	rup->ru_nsignals = krup->ru_nsignals;
	rup->ru_nvcsw = krup->ru_nvcsw;
	rup->ru_nivcsw = krup->ru_nivcsw;
}

struct plimit *
limcopy(lim)
	struct plimit *lim;
{
	register struct plimit *copy;

	rmalloc(copy, sizeof(struct plimit));
	bcopy(lim->pl_rlimit, copy->pl_rlimit, sizeof(struct rlimit) * RLIM_NLIMITS);
	copy->p_lflags = 0;
	copy->p_refcnt = 1;
	return (copy);
}

/*
 * Transform the running time and tick information in proc p into user,
 * system, and interrupt time usage.
 */
void
calcru(p, up, sp, ip)
	register struct proc *p;
	register struct timeval *up;
	register struct timeval *sp;
	register struct timeval *ip;
{
	register u_quad_t u, st, ut, it, tot;
	register u_long sec, usec;
	register int s;
	struct timeval tv;

	s = splstatclock();
	st = p->p_sticks;
	ut = p->p_uticks;
	it = p->p_iticks;
	splx(s);

	tot = st + ut + it;
	if (tot == 0) {
		up->tv_sec = up->tv_usec = 0;
		sp->tv_sec = sp->tv_usec = 0;
		if (ip != NULL)
			ip->tv_sec = ip->tv_usec = 0;
		return;
	}

	sec = p->p_rtime.tv_sec;
	usec = p->p_rtime.tv_usec;
	if (p == curproc) {
		/*
		 * Adjust for the current time slice.  This is actually fairly
		 * important since the error here is on the order of a time
		 * quantum, which is much greater than the sampling error.
		 */
		microtime(&tv);
		sec += tv.tv_sec - runtime.tv_sec;
		usec += tv.tv_usec - runtime.tv_usec;
	}
	u = sec * 1000000 + usec;
	st = (u * st) / tot;
	sp->tv_sec = st / 1000000;
	sp->tv_usec = st % 1000000;
	ut = (u * ut) / tot;
	up->tv_sec = ut / 1000000;
	up->tv_usec = ut % 1000000;
	if (ip != NULL) {
		it = (u * it) / tot;
		ip->tv_sec = it / 1000000;
		ip->tv_usec = it % 1000000;
	}
}
