/*	$NetBSD: event.h,v 1.40 2020/10/31 14:55:52 christos Exp $	*/

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
 *	$FreeBSD: src/sys/sys/event.h,v 1.12 2001/02/24 01:44:03 jlemon Exp $
 */

#ifndef _SYS_EVENT_H_
#define	_SYS_EVENT_H_

#include <sys/types.h>			/* for size_t */
#include <sys/stdint.h>			/* for uintptr_t */
#include <sys/null.h>			/* for NULL */

#define	EVFILT_READ			0U
#define	EVFILT_WRITE		1U
#define	EVFILT_AIO			2U	/* attached to aio requests */
#define	EVFILT_VNODE		3U	/* attached to vnodes */
#define	EVFILT_PROC			4U	/* attached to struct proc */
#define	EVFILT_SIGNAL		5U	/* attached to struct proc */
#define	EVFILT_TIMER		6U	/* arbitrary timer (in ms) */
#define	EVFILT_FS			7U	/* filesystem events */
#define	EVFILT_USER			8U	/* user events */
#define	EVFILT_SYSCOUNT		9U	/* number of filters */

struct kevent {
	__uintptr_t	ident;		/* identifier for this event */
	uint32_t	filter;		/* filter for event */
	uint32_t	flags;		/* action flags for kqueue */
	uint32_t	fflags;		/* filter flag value */
	int64_t		data;		/* filter data value */
	void		*udata;		/* opaque user data identifier */
};

static __inline void
_EV_SET(struct kevent *_kevp, __uintptr_t _ident, uint32_t _filter,
    uint32_t _flags, uint32_t _fflags, int64_t _data, void *_udata)
{
	_kevp->ident = _ident;
	_kevp->filter = _filter;
	_kevp->flags = _flags;
	_kevp->fflags = _fflags;
	_kevp->data = _data;
	_kevp->udata = _udata;
}

#define EV_SET(kevp, ident, filter, flags, fflags, data, udata)			\
    _EV_SET((kevp), __CAST(__uintptr_t, (ident)), (filter), (flags), 	\
    (fflags), (data), __CAST(void *, (udata)))

/* actions */
#define	EV_ADD			0x0001U		/* add event to kq (implies ENABLE) */
#define	EV_DELETE		0x0002U		/* delete event from kq */
#define	EV_ENABLE		0x0004U		/* enable event */
#define	EV_DISABLE		0x0008U		/* disable event (not reported) */

/* flags */
#define	EV_ONESHOT		0x0010U		/* only report one occurrence */
#define	EV_CLEAR		0x0020U		/* clear event state after reporting */
#define EV_RECEIPT		0x0040U		/* force EV_ERROR on success, data=0 */
#define EV_DISPATCH		0x0080U		/* disable event after reporting */

#define	EV_SYSFLAGS		0xF000U		/* reserved by system */
#define	EV_FLAG1		0x2000U		/* filter-specific flag */

/* returned values */
#define	EV_EOF			0x8000U		/* EOF detected */
#define	EV_ERROR		0x4000U		/* error, data contains errno */

/*
 * data/hint flags/masks for EVFILT_USER, shared with userspace
 *
 * On input, the top two bits of fflags specifies how the lower twenty four
 * bits should be applied to the stored value of fflags.
 *
 * On output, the top two bits will always be set to NOTE_FFNOP and the
 * remaining twenty four bits will contain the stored fflags value.
 */
#define	NOTE_FFNOP		0x00000000U		/* ignore input fflags */
#define	NOTE_FFAND		0x40000000U		/* AND fflags */
#define	NOTE_FFOR		0x80000000U		/* OR fflags */
#define	NOTE_FFCOPY		0xc0000000U		/* copy fflags */

#define	NOTE_FFCTRLMASK	0xc0000000U		/* masks for operations */
#define	NOTE_FFLAGSMASK	0x00ffffffU

#define	NOTE_TRIGGER	0x01000000U		/* Cause the event to be triggered for output. */
/*
 * hint flag for in-kernel use - must not equal any existing note
 */
#ifdef _KERNEL
#define NOTE_SUBMIT		0x01000000U		/* initial knote submission */
#endif
/*
 * data/hint flags for EVFILT_{READ|WRITE}, shared with userspace
 */
#define	NOTE_LOWAT		0x0001U			/* low water mark */
#define NOTE_FILE_POLL	0x0002U			/* behave like poll() */

/*
 * data/hint flags for EVFILT_VNODE, shared with userspace
 */
#define	NOTE_DELETE		0x0001U			/* vnode was removed */
#define	NOTE_WRITE		0x0002U			/* data contents changed */
#define	NOTE_EXTEND		0x0004U			/* size increased */
#define	NOTE_ATTRIB		0x0008U			/* attributes changed */
#define	NOTE_LINK		0x0010U			/* link count changed */
#define	NOTE_RENAME		0x0020U			/* vnode was renamed */
#define	NOTE_REVOKE		0x0040U			/* vnode access was revoked */

/*
 * data/hint flags for EVFILT_PROC, shared with userspace
 */
#define	NOTE_EXIT		0x80000000U		/* process exited */
#define	NOTE_FORK		0x40000000U		/* process forked */
#define	NOTE_EXEC		0x20000000U		/* process exec'd */
#define	NOTE_PCTRLMASK	0xf0000000U		/* mask for hint bits */
#define	NOTE_PDATAMASK	0x000fffffU		/* mask for pid */

/* additional flags for EVFILT_PROC */
#define	NOTE_TRACK		0x00000001U		/* follow across forks */
#define	NOTE_TRACKERR	0x00000002U		/* could not track child */
#define	NOTE_CHILD		0x00000004U		/* am a child process */

/*
 * This is currently visible to userland to work around broken
 * programs which pull in <sys/proc.h> or <sys/select.h>.
 */
#include <sys/queue.h>

struct knote;
SIMPLEQ_HEAD(klist, knote);

/*
 * ioctl(2)s supported on kqueue descriptors.
 */
#include <sys/ioctl.h>

struct kfilter_mapping {
	char			*name;		/* name to lookup or return */
	uint32_t		len;		/* length of name */
	uint32_t		filter;		/* filter to lookup or return */
};

/* map filter to name (max size len) */
#define KFILTER_BYFILTER	_IOWR('k', 0, struct kfilter_mapping)
/* map name to filter (len ignored) */
#define KFILTER_BYNAME		_IOWR('k', 1, struct kfilter_mapping)

#ifdef _KERNEL

#define	KNOTE(list, hint)	if (!SIMPLEQ_EMPTY(list)) knote(list, hint)

/*
 * Flag indicating hint is a signal.  Used by EVFILT_SIGNAL, and also
 * shared by EVFILT_PROC  (all knotes attached to p->p_klist)
 */
#define	NOTE_SIGNAL			0x08000000U

/*
 * Hint values for the optional f_touch event filter.  If f_touch is not set
 * to NULL and f_isfd is zero the f_touch filter will be called with the type
 * argument set to EVENT_REGISTER during a kevent() system call.  It is also
 * called under the same conditions with the type argument set to EVENT_PROCESS
 * when the event has been triggered.
 */
#define	EVENT_REGISTER		1
#define	EVENT_PROCESS		2

/*
 * Callback methods for each filter type.
 */
struct filterops {
	int		f_isfd;								/* true if ident == filedescriptor */
	int		(*f_attach)	(struct knote *);		/* called when knote is ADDed */
	void	(*f_detach)	(struct knote *);		/* called when knote is DELETEd */
	int		(*f_event)	(struct knote *, long); /* called when event is triggered */
};

/*
 * Field locking:
 *
 * f	kn_kq->kq_fdp->fd_lock
 * q	kn_kq->kq_lock
 * o	object mutex (e.g. device driver or vnode interlock)
 */
struct kfilter;

struct knote {
	SIMPLEQ_ENTRY(knote)	kn_link;	/* f: for fd */
	SIMPLEQ_ENTRY(knote)	kn_selnext;	/* o: for struct selinfo */
	TAILQ_ENTRY(knote)		kn_tqe;		/* q: for struct kqueue */
	struct kqueue			*kn_kq;		/* q: which queue we are on */
	struct kevent			kn_kevent;
	uint32_t				kn_status;	/* q: flags below */
	uint32_t				kn_sfflags;	/*    saved filter flags */
	__uintptr_t				kn_sdata;	/*    saved data field */
	union {
			struct file 	*p_fp;		/* file data pointer */
			struct proc 	*p_proc;	/* proc pointer */
			void			*p_v;		/* generic other pointer */
	} kn_ptr;
	void					*kn_obj;	/*    monitored obj */
	const struct filterops	*kn_fop;
	struct kfilter			*kn_kfilter;
	void 					*kn_hook;
	int						kn_hookid;


#define KN_ACTIVE			0x01		/* event has been triggered */
#define KN_QUEUED			0x02		/* event is on queue */
#define KN_DISABLED			0x04		/* event is disabled */
#define KN_DETACHED			0x08		/* knote is detached */
#define KN_MARKER			0x20		/* ignore this knote */
#define KN_KQUEUE			0x40		/* this knote belongs to a kq */
#define	KN_SCAN				0x100		/* flux set in kqueue_scan() */

#define	kn_id				kn_kevent.ident
#define	kn_filter			kn_kevent.filter
#define	kn_flags			kn_kevent.flags
#define	kn_fflags			kn_kevent.fflags
#define	kn_data				kn_kevent.data
#define kn_fp				kn_ptr.p_fp
};

struct proc;
struct timespec;

void	kqueue_init(void);

void	knote(struct klist *, long);
void	knote_remove(struct proc *, struct klist *);
void	knote_fdclose(struct proc *, int);
int 	kqueue_register(struct kqueue *, struct kevent *, struct proc *);

int		kfilter_register(const char *, const struct filterops *, int *);
int		kfilter_unregister(const char *);

int		filt_seltrue(struct knote *, long);
int		seltrue_kqfilter(dev_t, struct knote *);

#else 	/* !_KERNEL */

#include <sys/cdefs.h>

struct timespec;

__BEGIN_DECLS
int     kqueue(void);
int     kevent(int, const struct kevent *, int, struct kevent *, int, const struct timespec *);
__END_DECLS

#endif /* !_KERNEL */

#endif /* !_SYS_EVENT_H_ */
