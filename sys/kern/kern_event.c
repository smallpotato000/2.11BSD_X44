/*	$NetBSD: kern_event.c,v 1.19 2004/02/14 11:56:28 jdolecek Exp $	*/
/*-
 * Copyright (c) 1999,2000,2001 Jonathan Lemon <jlemon@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/kern/kern_event.c,v 1.27 2001/07/05 17:10:44 rwatson Exp $
 */

#include <sys/cdefs.h>
/* __KERNEL_RCSID(0, "$NetBSD: kern_event.c,v 1.19 2004/02/14 11:56:28 jdolecek Exp $"); */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/unistd.h>
#include <sys/file.h>
#include <sys/fcntl.h>
#include <sys/select.h>
#include <sys/queue.h>
#include <sys/poll.h>
#include <sys/user.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/mount.h>
#include <sys/filedesc.h>
#include <sys/event.h>
#include <sys/eventvar.h>

int     kqueue_rw(struct file *, struct uio *, struct ucred *);
int     kqueue_read(struct file *, struct uio *, struct ucred *);
int     kqueue_write(struct file *, struct uio *, struct ucred *);
int     kqueue_ioctl(struct file *, u_long, void *, struct proc *);
int     kqueue_poll(struct file *, int, struct proc *);
int		kqueue_stat(struct file *, struct stat *, struct proc *);
int     kqueue_close(struct file *, struct proc *);
int     kqueue_kqfilter(struct file *, struct knote *);


struct fileops kqueueops = {
		.fo_rw = kqueue_rw,
		.fo_read = kqueue_read,
		.fo_write = kqueue_write,
		.fo_ioctl = kqueue_ioctl,
		.fo_poll = kqueue_poll,
		.fo_stat = kqueue_stat,
		.fo_close = kqueue_close,
		.fo_kqfilter = kqueue_kqfilter
};

static int	kqueue_scan(struct file *fp, size_t maxevents, struct kevent *ulistp, const struct timespec *timeout, struct proc *p, register_t *retval);
static void	kqueue_wakeup(struct kqueue *kq);
static void	knote_attach(struct knote *kn, struct filedesc *fdp);
static void	knote_drop(struct knote *kn, struct proc *p, struct filedesc *fdp);
static void	knote_enqueue(struct knote *kn);
static void	knote_dequeue(struct knote *kn);

static void	filt_kqdetach(struct knote *kn);
static int	filt_kqueue(struct knote *kn, long hint);
static int	filt_procattach(struct knote *kn);
static void	filt_procdetach(struct knote *kn);
static int	filt_proc(struct knote *kn, long hint);
static int	filt_fileattach(struct knote *kn);
static void	filt_timerexpire(void *knx);
static int	filt_timerattach(struct knote *kn);
static void	filt_timerdetach(struct knote *kn);
static int	filt_timer(struct knote *kn, long hint);

static const struct filterops file_filtops = {
		.f_isfd = 1,
		.f_attach = filt_fileattach,
};

static const struct filterops kqread_filtops = {
		.f_isfd = 1,
		.f_detach = filt_kqdetach,
		.f_event = filt_kqueue,
};

/* XXX - move to kern_proc.c?  */
static const struct filterops proc_filtops = {
		.f_isfd = 0,
		.f_attach = filt_procattach,
		.f_detach = filt_procdetach,
		.f_event = filt_proc,
};
static const struct filterops timer_filtops = {
		.f_isfd = 0,
		.f_attach = filt_timerattach,
		.f_detach = filt_timerdetach,
		.f_event = filt_timer,
};

#define	KNOTE_ACTIVATE(kn) do {									\
	kn->kn_status |= KN_ACTIVE;									\
	if ((kn->kn_status & (KN_QUEUED | KN_DISABLED)) == 0)		\
		knote_enqueue(kn);										\
} while(0)

#define	KN_HASHSIZE			64		/* XXX should be tunable */
#define	KN_HASH(val, mask)	(((val) ^ (val >> 8)) & (mask))

extern const struct filterops sig_filtops;

/*
 * Table for for all system-defined filters.
 * These should be listed in the numeric order of the EVFILT_* defines.
 * If filtops is NULL, the filter isn't implemented in NetBSD.
 * End of list is when name is NULL.
 */
struct kfilter {
	const char	 			*name;		/* name of filter */
	uint32_t	  			filter;		/* id of filter */
	const struct filterops 	*filtops;	/* operations for filter */
};

/* System defined filters */
static const struct kfilter sys_kfilters[] = {
		{ "EVFILT_READ",	EVFILT_READ,	&file_filtops },
		{ "EVFILT_WRITE",	EVFILT_WRITE,	&file_filtops },
		{ "EVFILT_AIO",		EVFILT_AIO,		NULL },
		{ "EVFILT_VNODE",	EVFILT_VNODE,	&file_filtops },
		{ "EVFILT_PROC",	EVFILT_PROC,	&proc_filtops },
		{ "EVFILT_SIGNAL",	EVFILT_SIGNAL,	&sig_filtops },
		{ "EVFILT_TIMER",	EVFILT_TIMER,	&timer_filtops },
		{ NULL,				0,				NULL },	/* end of list */
};

/* User defined kfilters */
static struct kfilter	*user_kfilters;		/* array */
static int				user_kfilterc;		/* current offset */
static int				user_kfiltermaxc;	/* max size so far */

struct kqueue 			kern_kqueue;		/* kernel allocated kqueue structure */
struct knote  			kern_knote;			/* kernel allocated knote structure */
static int				kq_ncallouts = 0;
static int				kq_calloutmax = (4 * 1024);

/*
 * kqueue_alloc:
 *
 *	Allocate the systems kqueue/knote structures.
 */
static void
kqueue_alloc(kqueue, knote)
	struct kqueue *kqueue;
	struct knote *knote;
{
	MALLOC(kqueue, struct kqueue *, sizeof(struct kqueue), M_KEVENT, M_WAITOK);
	MALLOC(knote, struct knote *, sizeof(struct knote), M_KEVENT, M_WAITOK);
}

/*
 * kqueue_init:
 *
 *	Initialize the kqueue/knote facility.
 */
void
kqueue_init(void)
{
	kqueue_alloc(&kern_kqueue, &kern_knote);
}

/*
 * Find kfilter entry by name, or NULL if not found.
 */
static const struct kfilter *
kfilter_byname_sys(name)
	const char *name;
{
	int i;

	for (i = 0; sys_kfilters[i].name != NULL; i++) {
		if (strcmp(name, sys_kfilters[i].name) == 0)
			return (&sys_kfilters[i]);
	}
	return (NULL);
}

static struct kfilter *
kfilter_byname_user(name)
	const char *name;
{
	int i;

	/* user_kfilters[] could be NULL if no filters were registered */
	if (!user_kfilters)
		return (NULL);

	for (i = 0; user_kfilters[i].name != NULL; i++) {
		if (strcmp(user_kfilters[i].name,'\0') != 0 && strcmp(name, user_kfilters[i].name) == 0)
			return (&user_kfilters[i]);
	}
	return (NULL);
}

static const struct kfilter *
kfilter_byname(name)
	const char *name;
{
	const struct kfilter *kfilter;

	if ((kfilter = kfilter_byname_sys(name)) != NULL)
		return (kfilter);

	return (kfilter_byname_user(name));
}

/*
 * Find kfilter entry by filter id, or NULL if not found.
 * Assumes entries are indexed in filter id order, for speed.
 */
static const struct kfilter *
kfilter_byfilter(filter)
	uint32_t filter;
{
	const struct kfilter *kfilter;

	if (filter < EVFILT_SYSCOUNT) /* it's a system filter */
		kfilter = &sys_kfilters[filter];
	else if (user_kfilters != NULL && filter < EVFILT_SYSCOUNT + user_kfilterc)
		/* it's a user filter */
		kfilter = &user_kfilters[filter - EVFILT_SYSCOUNT];
	else
		return (NULL); /* out of range */
	KASSERT(kfilter->filter == filter); /* sanity check! */
	return (kfilter);
}

/*
 * Register a new kfilter. Stores the entry in user_kfilters.
 * Returns 0 if operation succeeded, or an appropriate errno(2) otherwise.
 * If retfilter != NULL, the new filterid is returned in it.
 */
int
kfilter_register(name, filtops, retfilter)
	const char *name;
	const struct filterops *filtops;
    int *retfilter;
{
	struct kfilter *kfilter;
	void *space;
	int len;

	if (name == NULL || name[0] == '\0' || filtops == NULL)
		return (EINVAL); /* invalid args */
	if (kfilter_byname(name) != NULL)
		return (EEXIST); /* already exists */
	if (user_kfilterc > 0xffffffff - EVFILT_SYSCOUNT)
		return (EINVAL); /* too many */

	/* check if need to grow user_kfilters */
	if (user_kfilterc + 1 > user_kfiltermaxc) {
		/*
		 * Grow in KFILTER_EXTENT chunks. Use malloc(9), because we
		 * want to traverse user_kfilters as an array.
		 */
		user_kfiltermaxc += KFILTER_EXTENT;
		kfilter = malloc(user_kfiltermaxc * sizeof(struct filter*),
		M_KEVENT, M_WAITOK);

		/* copy existing user_kfilters */
		if (user_kfilters != NULL)
			memcpy((caddr_t) kfilter, (caddr_t) user_kfilters,
					user_kfilterc * sizeof(struct kfilter*));
		/* zero new sections */
		memset((caddr_t) kfilter + user_kfilterc * sizeof(struct kfilter*), 0,
				(user_kfiltermaxc - user_kfilterc) * sizeof(struct kfilter*));
		/* switch to new kfilter */
		if (user_kfilters != NULL)
			free(user_kfilters, M_KEVENT);
		user_kfilters = kfilter;
	}
	len = strlen(name) + 1; /* copy name */
	space = malloc(len, M_KEVENT, M_WAITOK);
	memcpy(space, name, len);
	user_kfilters[user_kfilterc].name = space;

	user_kfilters[user_kfilterc].filter = user_kfilterc + EVFILT_SYSCOUNT;

	len = sizeof(struct filterops); /* copy filtops */
	space = malloc(len, M_KEVENT, M_WAITOK);
	memcpy(space, filtops, len);
	user_kfilters[user_kfilterc].filtops = space;

	if (retfilter != NULL)
		*retfilter = user_kfilters[user_kfilterc].filter;
	user_kfilterc++; /* finally, increment count */
	return (0);
}

/*
 * Unregister a kfilter previously registered with kfilter_register.
 * This retains the filter id, but clears the name and frees filtops (filter
 * operations), so that the number isn't reused during a boot.
 * Returns 0 if operation succeeded, or an appropriate errno(2) otherwise.
 */
int
kfilter_unregister(name)
	const char *name;
{
	struct kfilter *kfilter;

	if (name == NULL || name[0] == '\0')
		return (EINVAL); /* invalid name */

	if (kfilter_byname_sys(name) != NULL)
		return (EINVAL); /* can't detach system filters */

	kfilter = kfilter_byname_user(name);
	if (kfilter == NULL) /* not found */
		return (ENOENT);

	if (kfilter->name[0] != '\0') {
		/* XXX Cast away const (but we know it's safe. */
		free((void*) kfilter->name, M_KEVENT);
		kfilter->name = ""; /* mark as `not implemented' */
	}
	if (kfilter->filtops != NULL) {
		/* XXX Cast away const (but we know it's safe. */
		free((void*) kfilter->filtops, M_KEVENT);
		kfilter->filtops = NULL; /* mark as `not implemented' */
	}
	return (0);
}

int
filt_fileattach(kn)
	struct knote *kn;
{
	struct file *fp = kn->kn_fp;

	return fp->f_ops->fo_kqfilter(fp, kn);
}

/*
 * Filter detach method for EVFILT_READ on kqueue descriptor.
 */
static void
filt_kqdetach(kn)
	struct knote *kn;
{
	struct kqueue *kq;

	kq = (struct kqueue *)kn->kn_fp->f_data;
	SIMPLEQ_REMOVE(&kq->kq_sel.si_klist, kn, knote, kn_selnext);
}

/*
 * Filter event method for EVFILT_READ on kqueue descriptor.
 */
/*ARGSUSED*/
static int
filt_kqueue(kn, hint)
	struct knote *kn;
	long hint;
{
	struct kqueue *kq;

	kq = (struct kqueue *)kn->kn_fp->f_data;
	kn->kn_data = kq->kq_count;
	return (kn->kn_data > 0);
}

/*
 * Filter attach method for EVFILT_PROC.
 */
static int
filt_procattach(kn)
	struct knote *kn;
{
	struct proc *p;

	p = pfind(kn->kn_id);
	if (p == NULL)
		return (ESRCH);

	/*
	 * Fail if it's not owned by you, or the last exec gave us
	 * setuid/setgid privs (unless you're root).
	 */
	if ((p->p_cred->p_ruid != curproc->p_cred->p_ruid || (p->p_flag & P_SUGID))
			&& suser1(curproc->p_ucred, &curproc->p_acflag) != 0)
		return (EACCES);

	kn->kn_ptr.p_proc = p;
	kn->kn_flags |= EV_CLEAR; /* automatically set */

	/*
	 * internal flag indicating registration done by kernel
	 */
	if (kn->kn_flags & EV_FLAG1) {
		kn->kn_data = kn->kn_sdata; /* ppid */
		kn->kn_fflags = NOTE_CHILD;
		kn->kn_flags &= ~EV_FLAG1;
	}

	/* XXXSMP lock the process? */
	SIMPLEQ_INSERT_HEAD(&p->p_klist, kn, kn_selnext);

	return (0);
}

/*
 * Filter detach method for EVFILT_PROC.
 *
 * The knote may be attached to a different process, which may exit,
 * leaving nothing for the knote to be attached to.  So when the process
 * exits, the knote is marked as DETACHED and also flagged as ONESHOT so
 * it will be deleted when read out.  However, as part of the knote deletion,
 * this routine is called, so a check is needed to avoid actually performing
 * a detach, because the original process might not exist any more.
 */
static void
filt_procdetach(kn)
	struct knote *kn;
{
	struct proc *p;

	if (kn->kn_status & KN_DETACHED)
		return;

	p = kn->kn_ptr.p_proc;
	KASSERT(p->p_stat == SZOMB || pfind(kn->kn_id) == p);

	/* XXXSMP lock the process? */
	SIMPLEQ_REMOVE(&p->p_klist, kn, knote, kn_selnext);
}

/*
 * Filter event method for EVFILT_PROC.
 */
static int
filt_proc(kn, hint)
	struct knote *kn;
	long hint;
{
	u_int event;

	/*
	 * mask off extra data
	 */
	event = (u_int) hint & NOTE_PCTRLMASK;

	/*
	 * if the user is interested in this event, record it.
	 */
	if (kn->kn_sfflags & event)
		kn->kn_fflags |= event;

	/*
	 * process is gone, so flag the event as finished.
	 */
	if (event == NOTE_EXIT) {
		/*
		 * Detach the knote from watched process and mark
		 * it as such. We can't leave this to kqueue_scan(),
		 * since the process might not exist by then. And we
		 * have to do this now, since psignal KNOTE() is called
		 * also for zombies and we might end up reading freed
		 * memory if the kevent would already be picked up
		 * and knote g/c'ed.
		 */
		kn->kn_fop->f_detach(kn);
		kn->kn_status |= KN_DETACHED;

		/* Mark as ONESHOT, so that the knote it g/c'ed when read */
		kn->kn_flags |= (EV_EOF | EV_ONESHOT);
		return (1);
	}

	/*
	 * process forked, and user wants to track the new process,
	 * so attach a new knote to it, and immediately report an
	 * event with the parent's pid.
	 */
	if ((event == NOTE_FORK) && (kn->kn_sfflags & NOTE_TRACK)) {
		struct kevent kev;
		int error;

		/*
		 * register knote with new process.
		 */
		kev.ident = hint & NOTE_PDATAMASK; /* pid */
		kev.filter = kn->kn_filter;
		kev.flags = kn->kn_flags | EV_ADD | EV_ENABLE | EV_FLAG1;
		kev.fflags = kn->kn_sfflags;
		kev.data = kn->kn_id; /* parent */
		kev.udata = kn->kn_kevent.udata; /* preserve udata */
		error = kqueue_register(kn->kn_kq, &kev, NULL);
		if (error)
			kn->kn_fflags |= NOTE_TRACKERR;
	}

	return (kn->kn_fflags != 0);
}

static void
filt_timerexpire(knx)
	void *knx;
{
	struct knote *kn = knx;
	int tticks;

	kn->kn_data++;
	KNOTE_ACTIVATE(kn);

	if ((kn->kn_flags & EV_ONESHOT) == 0) {
		tticks = mstohz(kn->kn_sdata);
		callout_schedule((struct callout*) kn->kn_hook, tticks);
	}
}

/*
 * data contains amount of time to sleep, in milliseconds
 */
static int
filt_timerattach(kn)
	struct knote *kn;
{
	struct callout *calloutp;
	int tticks;

	if (kq_ncallouts >= kq_calloutmax)
		return (ENOMEM);
	kq_ncallouts++;

	tticks = mstohz(kn->kn_sdata);

	/* if the supplied value is under our resolution, use 1 tick */
	if (tticks == 0) {
		if (kn->kn_sdata == 0)
			return (EINVAL);
		tticks = 1;
	}

	kn->kn_flags |= EV_CLEAR; /* automatically set */
	MALLOC(calloutp, struct callout*, sizeof(*calloutp), M_KEVENT, 0);
	callout_init(calloutp);
	callout_reset(calloutp, tticks, filt_timerexpire, kn);
	kn->kn_hook = calloutp;

	return (0);
}

static void
filt_timerdetach(kn)
	struct knote *kn;
{
	struct callout *calloutp;

	calloutp = (struct callout *)kn->kn_hook;
	callout_stop(calloutp);
	FREE(calloutp, M_KEVENT);
	kq_ncallouts--;
}

static int
filt_timer(kn, hint)
	struct knote *kn;
	long hint;
{
	return (kn->kn_data != 0);
}

/*
 * filt_seltrue:
 *
 *	This filter "event" routine simulates seltrue().
 */
int
filt_seltrue(kn, hint)
	struct knote *kn;
	long hint;
{

	/*
	 * We don't know how much data can be read/written,
	 * but we know that it *can* be.  This is about as
	 * good as select/poll does as well.
	 */
	kn->kn_data = 0;
	return (1);
}

/*
 * This provides full kqfilter entry for device switch tables, which
 * has same effect as filter using filt_seltrue() as filter method.
 */
static void
filt_seltruedetach(kn)
	struct knote *kn;
{
	/* Nothing to do */
}

static const struct filterops seltrue_filtops = {
		.f_isfd = 1,
		.f_attach = NULL,
		.f_detach = filt_seltruedetach,
		.f_event = filt_seltrue,
};

int
seltrue_kqfilter(dev, kn)
	dev_t dev;
	struct knote *kn;
{
	switch (kn->kn_filter) {
	case EVFILT_READ:
	case EVFILT_WRITE:
		kn->kn_fop = &seltrue_filtops;
		break;
	default:
		return (1);
	}

	/* Nothing more to do */
	return (0);
}

/*
 * kqueue(2) system call.
 */
int
kqueue()
{
	struct proc 	*p;
	struct filedesc	*fdp;
	struct kqueue	*kq;
	struct file		*fp;
	int				fd, error;

	error = 0;
	p = u.u_procp;
	fdp = u.u_fd;
	fp = falloc(); /* setup a new file descriptor */
	if (fp == NULL) {
		error = ENFILE;
		return (error);
	}
	fp->f_flag = FREAD | FWRITE;
	fp->f_type = DTYPE_KQUEUE;
	fp->f_ops = &kqueueops;
	kq = &kern_kqueue;
	memset((char*) kq, 0, sizeof(struct kqueue));
	simple_lock_init(&kq->kq_lock, "kqueue lock");
	TAILQ_INIT(&kq->kq_head);
	fp->f_data = (caddr_t) kq; 	/* store the kqueue with the fp */
	u.u_r.r_val1 = fd;
	if (fdp->fd_knlistsize < 0)
		fdp->fd_knlistsize = 0; 		/* this process has a kq */
	kq->kq_fdp = fdp;
	fp->f_flag &= ~FIF_LARVAL;
	FILE_UNUSE(fp, p); 					/* falloc() does FILE_USE() */
	return (error);
}

/*
 * kevent(2) system call.
 */
int
kevent()
{
	struct kevent_args {
		syscallarg(int) 					fd;
		syscallarg(const struct kevent *) 	changelist;
		syscallarg(size_t) 					nchanges;
		syscallarg(struct kevent *) 		eventlist;
		syscallarg(size_t) 					nevents;
		syscallarg(const struct timespec *) timeout;
	} *uap = (struct kevent_args *)u.u_ap;

	register struct proc *p;
	register register_t *retval;
	struct kevent	*kevp;
	struct kqueue	*kq;
	struct file		*fp;
	struct timespec	ts;
	size_t			i, n;
	int				nerrors, error;

	p = u.u_procp;
	/* check that we're dealing with a kq */
	fp = fd_getfile(SCARG(uap, fd));
	if (fp == NULL)
		return (EBADF);

	if (fp->f_type != DTYPE_KQUEUE) {
		simple_unlock(&fp->f_slock);
		return (EBADF);
	}

	FILE_USE(fp);
	if (SCARG(uap, timeout) != NULL) {
		error = copyin(SCARG(uap, timeout), &ts, sizeof(ts));
		if (error)
			goto done;
		SCARG(uap, timeout) = &ts;
	}

	kq = (struct kqueue*) fp->f_data;
	nerrors = 0;

	/* traverse list of events to register */
	while (SCARG(uap, nchanges) > 0) {
		/* copyin a maximum of KQ_EVENTS at each pass */
		n = MIN(SCARG(uap, nchanges), KQ_NEVENTS);
		error = copyin(SCARG(uap, changelist), kq->kq_kev,
				n * sizeof(struct kevent));
		if (error)
			goto done;
		for (i = 0; i < n; i++) {
			kevp = &kq->kq_kev[i];
			kevp->flags &= ~EV_SYSFLAGS;
			/* register each knote */
			error = kqueue_register(kq, kevp, p);
			if (error) {
				if (SCARG(uap, nevents) != 0) {
					kevp->flags = EV_ERROR;
					kevp->data = error;
					error = copyout((caddr_t) kevp,
							(caddr_t) SCARG(uap, eventlist), sizeof(*kevp));
					if (error)
						goto done;
					SCARG(uap, eventlist)++;
					SCARG(uap, nevents)--;
					nerrors++;
				} else {
					goto done;
				}
			}
		}
		SCARG(uap, nchanges) -= n; /* update the results */
		SCARG(uap, changelist) += n;
	}
	if (nerrors) {
		*retval = nerrors;
		error = 0;
		goto done;
	}

	/* actually scan through the events */
	error = kqueue_scan(fp, SCARG(uap, nevents), SCARG(uap, eventlist),
			SCARG(uap, timeout), p, retval);
done:
	FILE_UNUSE(fp, p);
	return (error);
}

/*
 * Register a given kevent kev onto the kqueue
 */
int
kqueue_register(kq, kev, p)
	struct kqueue *kq;
	struct kevent *kev;
	struct proc *p;
{
	const struct kfilter *kfilter;
	struct filedesc	*fdp;
	struct file	*fp;
	struct knote	*kn;
	int		s, error;

	fdp = kq->kq_fdp;
	fp = NULL;
	kn = NULL;
	error = 0;
	kfilter = kfilter_byfilter(kev->filter);
	if (kfilter == NULL || kfilter->filtops == NULL) {
		/* filter not found nor implemented */
		return (EINVAL);
	}

	/* search if knote already exists */
	if (kfilter->filtops->f_isfd) {
		/* monitoring a file descriptor */
		if ((fp = fd_getfile(kev->ident)) == NULL)
			return (EBADF); /* validate descriptor */
		FILE_USE(fp);

		if (kev->ident < fdp->fd_knlistsize) {
			SIMPLEQ_FOREACH(kn, &fdp->fd_knlist[kev->ident], kn_link)
				if (kq == kn->kn_kq && kev->filter == kn->kn_filter)
					break;
		}
	} else {
		/*
		 * not monitoring a file descriptor, so
		 * lookup knotes in internal hash table
		 */
		if (fdp->fd_knhashmask != 0) {
			struct klist *list;

			list = &fdp->fd_knhash[KN_HASH((u_long )kev->ident,
					fdp->fd_knhashmask)];
			SIMPLEQ_FOREACH(kn, list, kn_link)
				if (kev->ident == kn->kn_id && kq == kn->kn_kq&&
				kev->filter == kn->kn_filter)
					break;
		}
	}

	if (kn == NULL && ((kev->flags & EV_ADD) == 0)) {
		error = ENOENT; /* filter not found */
		goto done;
	}

	/*
	 * kn now contains the matching knote, or NULL if no match
	 */
	if (kev->flags & EV_ADD) {
		/* add knote */

		if (kn == NULL) {
			/* create new knote */
			kn = &kern_knote;
			if (kn == NULL) {
				error = ENOMEM;
				goto done;
			}
			kn->kn_fp = fp;
			kn->kn_kq = kq;
			kn->kn_fop = kfilter->filtops;

			/*
			 * apply reference count to knote structure, and
			 * do not release it at the end of this routine.
			 */
			fp = NULL;

			kn->kn_sfflags = kev->fflags;
			kn->kn_sdata = kev->data;
			kev->fflags = 0;
			kev->data = 0;
			kn->kn_kevent = *kev;

			knote_attach(kn, fdp);
			if ((error = kfilter->filtops->f_attach(kn)) != 0) {
				knote_drop(kn, p, fdp);
				goto done;
			}
		} else {
			/* modify existing knote */

			/*
			 * The user may change some filter values after the
			 * initial EV_ADD, but doing so will not reset any
			 * filter which have already been triggered.
			 */
			kn->kn_sfflags = kev->fflags;
			kn->kn_sdata = kev->data;
			kn->kn_kevent.udata = kev->udata;
		}

		s = splhigh();
		if (kn->kn_fop->f_event(kn, 0))
			KNOTE_ACTIVATE(kn);
		splx(s);

	} else if (kev->flags & EV_DELETE) {	/* delete knote */
		kn->kn_fop->f_detach(kn);
		knote_drop(kn, p, fdp);
		goto done;
	}

	/* disable knote */
	if ((kev->flags & EV_DISABLE) &&
	    ((kn->kn_status & KN_DISABLED) == 0)) {
		s = splhigh();
		kn->kn_status |= KN_DISABLED;
		splx(s);
	}

	/* enable knote */
	if ((kev->flags & EV_ENABLE) && (kn->kn_status & KN_DISABLED)) {
		s = splhigh();
		kn->kn_status &= ~KN_DISABLED;
		if ((kn->kn_status & KN_ACTIVE) &&
		    ((kn->kn_status & KN_QUEUED) == 0))
			knote_enqueue(kn);
		splx(s);
	}

 done:
	if (fp != NULL)
		FILE_UNUSE(fp, p);
	return (error);
}

/*
 * Scan through the list of events on fp (for a maximum of maxevents),
 * returning the results in to ulistp. Timeout is determined by tsp; if
 * NULL, wait indefinitely, if 0 valued, perform a poll, otherwise wait
 * as appropriate.
 */
static int
kqueue_scan(fp, maxevents, ulistp, tsp, p, retval)
	struct file *fp;
	size_t maxevents;
	struct kevent *ulistp;
	const struct timespec *tsp;
	struct proc *p;
	register_t *retval;
{
	struct kqueue	*kq;
	struct kevent	*kevp;
	struct timeval	atv;
	struct knote	*kn, *marker=NULL;
	size_t			count, nkev;
	int				s, timeout, error;

	kq = (struct kqueue*) fp->f_data;
	count = maxevents;
	nkev = error = 0;
	if (count == 0)
		goto done;

	if (tsp) { /* timeout supplied */
		TIMESPEC_TO_TIMEVAL(&atv, tsp);
		if (itimerfix(&atv)) {
			error = EINVAL;
			goto done;
		}
		s = splclock();
		timeradd(&atv, &time, &atv); /* calc. time to wait until */
		splx(s);
		timeout = hzto(&atv);
		if (timeout <= 0)
			timeout = -1; /* do poll */
	} else {
		/* no timeout, wait forever */
		timeout = 0;
	}

	MALLOC(marker, struct knote*, sizeof(*marker), M_KEVENT, M_WAITOK);
	memset(marker, 0, sizeof(*marker));

	goto start;

 retry:
	if (tsp) {
		/*
		 * We have to recalculate the timeout on every retry.
		 */
		timeout = hzto(&atv);
		if (timeout <= 0)
			goto done;
	}

 start:
	kevp = kq->kq_kev;
	s = splhigh();
	simple_lock(&kq->kq_lock);
	if (kq->kq_count == 0) {
		if (timeout < 0) {
			error = EWOULDBLOCK;
			simple_unlock(&kq->kq_lock);
		} else {
			kq->kq_state |= KQ_SLEEP;
			error = ltsleep(kq, PSOCK | PCATCH, "kqread", timeout, &kq->kq_lock);
		}
		splx(s);
		if (error == 0)
			goto retry;
		/* don't restart after signals... */
		if (error == ERESTART)
			error = EINTR;
		else if (error == EWOULDBLOCK)
			error = 0;
		goto done;
	}

	/* mark end of knote list */
	TAILQ_INSERT_TAIL(&kq->kq_head, marker, kn_tqe);
	simple_unlock(&kq->kq_lock);

	while (count) { /* while user wants data ... */
		simple_lock(&kq->kq_lock);
		kn = TAILQ_FIRST(&kq->kq_head); /* get next knote */
		TAILQ_REMOVE(&kq->kq_head, kn, kn_tqe);
		if (kn == marker) { /* if it's our marker, stop */
			/* What if it's some else's marker? */
			simple_unlock(&kq->kq_lock);
			splx(s);
			if (count == maxevents)
				goto retry;
			goto done;
		}
		kq->kq_count--;
		simple_unlock(&kq->kq_lock);

		if (kn->kn_status & KN_DISABLED) {
			/* don't want disabled events */
			kn->kn_status &= ~KN_QUEUED;
			continue;
		}
		if ((kn->kn_flags & EV_ONESHOT) == 0
				&& kn->kn_fop->f_event(kn, 0) == 0) {
			/*
			 * non-ONESHOT event that hasn't
			 * triggered again, so de-queue.
			 */
			kn->kn_status &= ~(KN_QUEUED | KN_ACTIVE);
			continue;
		}
		*kevp = kn->kn_kevent;
		kevp++;
		nkev++;
		if (kn->kn_flags & EV_ONESHOT) {
			/* delete ONESHOT events after retrieval */
			kn->kn_status &= ~KN_QUEUED;
			splx(s);
			kn->kn_fop->f_detach(kn);
			knote_drop(kn, p, p->p_fd);
			s = splhigh();
		} else if (kn->kn_flags & EV_CLEAR) {
			/* clear state after retrieval */
			kn->kn_data = 0;
			kn->kn_fflags = 0;
			kn->kn_status &= ~(KN_QUEUED | KN_ACTIVE);
		} else {
			/* add event back on list */
			simple_lock(&kq->kq_lock);
			TAILQ_INSERT_TAIL(&kq->kq_head, kn, kn_tqe);
			kq->kq_count++;
			simple_unlock(&kq->kq_lock);
		}
		count--;
		if (nkev == KQ_NEVENTS) {
			/* do copyouts in KQ_NEVENTS chunks */
			splx(s);
			error = copyout((caddr_t) &kq->kq_kev, (caddr_t) ulistp,
					sizeof(struct kevent) * nkev);
			ulistp += nkev;
			nkev = 0;
			kevp = kq->kq_kev;
			s = splhigh();
			if (error)
				break;
		}
	}

	/* remove marker */
	simple_lock(&kq->kq_lock);
	TAILQ_REMOVE(&kq->kq_head, marker, kn_tqe);
	simple_unlock(&kq->kq_lock);
	splx(s);
 done:
	if (marker)
		FREE(marker, M_KEVENT);

	if (nkev != 0) {
		/* copyout remaining events */
		error = copyout((caddr_t) &kq->kq_kev, (caddr_t) ulistp, sizeof(struct kevent) * nkev);
	}
	*retval = maxevents - count;

	return (error);
}

int
kqueue_rw(fp, uio, cred)
	struct file *fp;
	struct uio *uio;
	struct ucred *cred;
{
	enum uio_rw rw = uio->uio_rw;
	int error;

	if (rw == UIO_READ) {
		error = kqueue_read(fp, uio, cred);
	} else {
		error = kqueue_write(fp, uio, cred);
	}
	return (error);
}

/*
 * struct fileops read method for a kqueue descriptor.
 * Not implemented.
 * XXX: This could be expanded to call kqueue_scan, if desired.
 */
/*ARGSUSED*/
int
kqueue_read(fp, uio, cred)
	struct file *fp;
	struct uio *uio;
	struct ucred *cred;
{

	return (ENXIO);
}

/*
 * struct fileops write method for a kqueue descriptor.
 * Not implemented.
 */
/*ARGSUSED*/
int
kqueue_write(fp, uio, cred)
	struct file *fp;
	struct uio *uio;
	struct ucred *cred;
{

	return (ENXIO);
}

/*
 * struct fileops ioctl method for a kqueue descriptor.
 *
 * Two ioctls are currently supported. They both use struct kfilter_mapping:
 *	KFILTER_BYNAME		find name for filter, and return result in
 *				name, which is of size len.
 *	KFILTER_BYFILTER	find filter for name. len is ignored.
 */
/*ARGSUSED*/
int
kqueue_ioctl(fp, com, data, p)
	struct file *fp;
	u_long com;
	void *data;
	struct proc *p;
{
	struct kfilter_mapping	*km;
	const struct kfilter	*kfilter;
	char			*name;
	int			error;

	km = (struct kfilter_mapping*) data;
	error = 0;

	switch (com) {
	case KFILTER_BYFILTER: /* convert filter -> name */
		kfilter = kfilter_byfilter(km->filter);
		if (kfilter != NULL)
			error = copyoutstr(kfilter->name, km->name, km->len,
			NULL);
		else
			error = ENOENT;
		break;

	case KFILTER_BYNAME: /* convert name -> filter */
		MALLOC(name, char*, KFILTER_MAXNAME, M_KEVENT, M_WAITOK);
		error = copyinstr(km->name, name, KFILTER_MAXNAME, NULL);
		if (error) {
			FREE(name, M_KEVENT);
			break;
		}
		kfilter = kfilter_byname(name);
		if (kfilter != NULL)
			km->filter = kfilter->filter;
		else
			error = ENOENT;
		FREE(name, M_KEVENT);
		break;

	default:
		error = ENOTTY;

	}
	return (error);
}

/*
 * struct fileops poll method for a kqueue descriptor.
 * Determine if kqueue has events pending.
 */
int
kqueue_poll(fp, events, p)
    struct file *fp;
    int events;
    struct proc *p;
{
	struct kqueue *kq;
	int revents;

	kq = (struct kqueue*) fp->f_data;
	revents = 0;
	if (events & (POLLIN | POLLRDNORM)) {
		if (kq->kq_count) {
			revents |= events & (POLLIN | POLLRDNORM);
		} else {
			selrecord(p, &kq->kq_sel);
		}
	}
	return (revents);
}

int
kqueue_stat(fp, ub, p)
	struct file *fp;
	struct stat *ub;
	struct proc *p;
{
	struct kqueue	*kq;
	kq = (struct kqueue *)fp->f_data;
	bzero((void *)ub, sizeof(*ub));
	ub->st_size = kq->kq_count;
	ub->st_blksize = sizeof(struct kevent);
	ub->st_mode = S_IFIFO;
	return (0);
}

/*
 * struct fileops close method for a kqueue descriptor.
 * Cleans up kqueue.
 */
int
kqueue_close(fp, p)
    struct file *fp;
    struct proc *p;
{
	struct kqueue	*kq;
	struct filedesc	*fdp;
	struct knote	**knp, *kn, *kn0;
	int		i;

	kq = (struct kqueue *)fp->f_data;
	fdp = p->p_fd;
	for (i = 0; i < fdp->fd_knlistsize; i++) {
		knp = &SIMPLEQ_FIRST(&fdp->fd_knlist[i]);
		kn = *knp;
		while (kn != NULL) {
			kn0 = SIMPLEQ_NEXT(kn, kn_link);
			if (kq == kn->kn_kq) {
				kn->kn_fop->f_detach(kn);
				FILE_UNUSE(kn->kn_fp, p);
				kn = &kern_knote;
				*knp = kn0;
			} else {
				knp = &SIMPLEQ_NEXT(kn, kn_link);
			}
			kn = kn0;
		}
	}
	if (fdp->fd_knhashmask != 0) {
		for (i = 0; i < fdp->fd_knhashmask + 1; i++) {
			knp = &SIMPLEQ_FIRST(&fdp->fd_knhash[i]);
			kn = *knp;
			while (kn != NULL) {
				kn0 = SIMPLEQ_NEXT(kn, kn_link);
				if (kq == kn->kn_kq) {
					kn->kn_fop->f_detach(kn);
					/* XXX non-fd release of kn->kn_ptr */
					kn = &kern_knote;
					*knp = kn0;
				} else {
					knp = &SIMPLEQ_NEXT(kn, kn_link);
				}
				kn = kn0;
			}
		}
	}
	kq = &kern_kqueue;
	fp->f_data = NULL;

	return (0);
}

/*
 * wakeup a kqueue
 */
static void
kqueue_wakeup(kq)
	struct kqueue *kq;
{
	int s;

	s = splhigh();
	simple_lock(&kq->kq_lock);
	if (kq->kq_state & KQ_SLEEP) { /* if currently sleeping ...  */
		kq->kq_state &= ~KQ_SLEEP;
		wakeup(kq); /* ... wakeup */
	}

	/* Notify select/poll and kevent. */
	selnotify(&kq->kq_sel, 0);
	simple_unlock(&kq->kq_lock);
	splx(s);
}

/*
 * struct fileops kqfilter method for a kqueue descriptor.
 * Event triggered when monitored kqueue changes.
 */
/*ARGSUSED*/
int
kqueue_kqfilter(fp, kn)
	struct file *fp;
	struct knote *kn;
{
	struct kqueue *kq;

	KASSERT(fp == kn->kn_fp);
	kq = (struct kqueue *)kn->kn_fp->f_data;
	if (kn->kn_filter != EVFILT_READ)
		return (1);
	kn->kn_fop = &kqread_filtops;
	SIMPLEQ_INSERT_HEAD(&kq->kq_sel.si_klist, kn, kn_selnext);
	return (0);
}

/*
 * Walk down a list of knotes, activating them if their event has triggered.
 */
void
knote(list, hint)
	struct klist *list;
	long hint;
{
	struct knote *kn;

	SIMPLEQ_FOREACH(kn, list, kn_selnext)
		if (kn->kn_fop->f_event(kn, hint))
			KNOTE_ACTIVATE(kn);
}

/*
 * Remove all knotes from a specified klist
 */
void
knote_remove(p, list)
	struct proc *p;
	struct klist *list;
{
	struct knote *kn;

	while ((kn = SIMPLEQ_FIRST(list)) != NULL) {
		kn->kn_fop->f_detach(kn);
		knote_drop(kn, p, p->p_fd);
	}
}

/*
 * Remove all knotes referencing a specified fd
 */
void
knote_fdclose(p, fd)
	struct proc *p;
	int fd;
{
	struct filedesc	*fdp;
	struct klist	*list;

	fdp = p->p_fd;
	list = &fdp->fd_knlist[fd];
	knote_remove(p, list);
}

/*
 * Attach a new knote to a file descriptor
 */
static void
knote_attach(kn, fdp)
	struct knote *kn;
	struct filedesc *fdp;
{
	struct klist	*list;
	int				size;

	if (!kn->kn_fop->f_isfd) {
		/* if knote is not on an fd, store on internal hash table */
		if (fdp->fd_knhashmask == 0)
			fdp->fd_knhash = hashinit(KN_HASHSIZE, M_KEVENT, &fdp->fd_knhashmask);
		list = &fdp->fd_knhash[KN_HASH(kn->kn_id, fdp->fd_knhashmask)];
		goto done;
	}

	/*
	 * otherwise, knote is on an fd.
	 * knotes are stored in fd_knlist indexed by kn->kn_id.
	 */
	if (fdp->fd_knlistsize <= kn->kn_id) {
		/* expand list, it's too small */
		size = fdp->fd_knlistsize;
		while (size <= kn->kn_id) {
			/* grow in KQ_EXTENT chunks */
			size += KQ_EXTENT;
		}
		list = malloc(size * sizeof(struct klist*), M_KEVENT, M_WAITOK);
		if (fdp->fd_knlist) {
			/* copy existing knlist */
			memcpy((caddr_t) list, (caddr_t) fdp->fd_knlist, fdp->fd_knlistsize * sizeof(struct klist*));
		}
		/*
		 * Zero new memory. Stylistically, SIMPLEQ_INIT() should be
		 * used here, but that does same thing as the memset() anyway.
		 */
		memset(&list[fdp->fd_knlistsize], 0, (size - fdp->fd_knlistsize) * sizeof(struct klist*));

		/* switch to new knlist */
		if (fdp->fd_knlist != NULL)
			free(fdp->fd_knlist, M_KEVENT);
		fdp->fd_knlistsize = size;
		fdp->fd_knlist = list;
	}

	/* get list head for this fd */
	list = &fdp->fd_knlist[kn->kn_id];
 done:
	/* add new knote */
 	SIMPLEQ_INSERT_HEAD(list, kn, kn_link);
	kn->kn_status = 0;
}

/*
 * Drop knote.
 * Should be called at spl == 0, since we don't want to hold spl
 * while calling FILE_UNUSE and free.
 */
static void
knote_drop(kn, p, fdp)
	struct knote *kn;
	struct proc *p;
	struct filedesc *fdp;
{
	struct klist	*list;

	if (kn->kn_fop->f_isfd)
		list = &fdp->fd_knlist[kn->kn_id];
	else
		list = &fdp->fd_knhash[KN_HASH(kn->kn_id, fdp->fd_knhashmask)];

	SIMPLEQ_REMOVE(list, kn, knote, kn_link);
	if (kn->kn_status & KN_QUEUED)
		knote_dequeue(kn);
	if (kn->kn_fop->f_isfd)
		FILE_UNUSE(kn->kn_fp, p);
	kern_knote = *kn;
}

/*
 * Queue new event for knote.
 */
static void
knote_enqueue(kn)
	struct knote *kn;
{
	struct kqueue	*kq;
	int		s;

	kq = kn->kn_kq;
	KASSERT((kn->kn_status & KN_QUEUED) == 0);

	s = splhigh();
	simple_lock(&kq->kq_lock);
	TAILQ_INSERT_TAIL(&kq->kq_head, kn, kn_tqe);
	kn->kn_status |= KN_QUEUED;
	kq->kq_count++;
	simple_unlock(&kq->kq_lock);
	splx(s);
	kqueue_wakeup(kq);
}

/*
 * Dequeue event for knote.
 */
static void
knote_dequeue(kn)
	struct knote *kn;
{
	struct kqueue	*kq;
	int		s;

	KASSERT(kn->kn_status & KN_QUEUED);
	kq = kn->kn_kq;

	s = splhigh();
	simple_lock(&kq->kq_lock);
	TAILQ_REMOVE(&kq->kq_head, kn, kn_tqe);
	kn->kn_status &= ~KN_QUEUED;
	kq->kq_count--;
	simple_unlock(&kq->kq_lock);
	splx(s);
}
