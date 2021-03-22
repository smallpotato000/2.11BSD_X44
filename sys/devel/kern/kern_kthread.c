/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/user.h>
#include <sys/malloc.h>
#include <sys/lock.h>
#include <sys/rwlock.h>
#include <sys/wait.h>
#include <sys/queue.h>
#include "devel/sys/kthread.h"

#include <vm/include/vm_param.h>

extern struct kthread 		 kthread0;
struct kthread *curkthread = &kthread0;
int	kthread_create_now;

void
kthread_init(p, kt)
	register struct proc  	*p;
	register struct kthread *kt;
{
	register_t 	rval[2];
	int 		error;

	/* initialize current kthread0 from proc overseer */
	&kthread0 = &proc0->p_kthreado;

	/* initialize current kthread from kthread0 */
    kt = &kthread0;
    curkthread = kt;

	/* Initialize kthread and kthread group structures. */
    threadinit();

	/* set up kernel thread */
    allkthread = (struct kthread *) kt;
    kt->kt_prev = (struct kthread **)&allkthread;
	tgrphash[0] = &pgrp0;
	p->p_kthreado = kt;

	/* give the kthread the same creds as the initial thread */
	kt->kt_ucred = p->p_ucred;
	crhold(kt->kt_ucred);

    /* setup kthread locks */
    kthread_lock_init(kthread_lkp, kt);
    kthread_rwlock_init(kthread_rwl, kt);

    /* initialize kthreadpools */
    kthreadpools_init();
}

/*
 * Fork a kernel thread.  Any process can request this to be done.
 * The VM space and limits, etc. will be shared with proc0.
 */
int
kthread_create(func, arg, newpp, name)
	void (*func)(void *);
	void *arg;
	struct proc **newpp;
	const char *name;
{
	struct proc *p;
	register_t 	rval[2];
	int 		error;

	/* First, create the new process. */
	error = newproc(0);
	if(__predict_false(error != 0)) {
		panic("kthread_create");
		return (error);
	}

	if(rval[1]) {
		p->p_flag |= P_SYSTEM | P_NOCLDWAIT;
		p->p_kthreado->kt_flag |= KT_INMEM | KT_SYSTEM;
	}

	/* Name it as specified. */
	bcopy(p->p_kthreado->kt_comm, name, MAXCOMLEN);

	if (newpp != NULL) {
		*newpp = p;
	}
	return (0);
}

/*
 * Cause a kernel thread to exit.  Assumes the exiting thread is the
 * current context.
 */
void
kthread_exit(ecode)
	int ecode;
{
	/*
	 * XXX What do we do with the exit code?  Should we even bother
	 * XXX with it?  The parent (proc0) isn't going to do much with
	 * XXX it.
	 */
	if (ecode != 0) {
		printf("WARNING: thread `%s' (%d) exits with status %d\n", curproc->p_comm, curproc->p_pid, ecode);
	}
	exit(W_EXITCODE(ecode, 0));

	for (;;);
}

struct kthread_queue {
	SIMPLEQ_ENTRY(kthread_queue) 	kq_q;
	void 							(*kq_func)(void *);
	void 							*kq_arg;
};

SIMPLEQ_HEAD(, kthread_queue) kthread_queue = SIMPLEQ_HEAD_INITIALIZER(kthread_queue);

void
kthread_create_deferred(void (*func)(void *), void *arg)
{
	struct kthread_queue *kq;
	if (kthread_create_now) {
		(*func)(arg);
		return;
	}

	kq = malloc(sizeof *kq, M_TEMP, M_NOWAIT | M_ZERO);
	if (kq == NULL) {
		panic("unable to allocate kthread_queue");
	}

	kq->kq_func = func;
	kq->kq_arg = arg;

	SIMPLEQ_INSERT_TAIL(&kthread_queue, kq, kq_q);
}

void
kthread_run_deferred_queue(void)
{
	struct kthread_queue *kq;

	/* No longer need to defer kthread creation. */
	kthread_create_now = 1;

	while ((kq = SIMPLEQ_FIRST(&kthread_queue)) != NULL) {
		SIMPLEQ_REMOVE_HEAD(&kthread_queue, kq_q);
		(*kq->kq_func)(kq->kq_arg);
		free(kq, M_TEMP, sizeof(*kq));
	}
}

/* Threadpool's FIFO Queue (IPC) */
void
kthreadpool_itc_send(itpc, ktpool)
	struct threadpool_itpc *itpc;
    struct kthreadpool *ktpool;
{
    /* command / action */
    itpc->itc_ktpool = ktpool;
    itpc->itc_jobs = ktpool->ktp_jobs;  /* add/ get current job */
	/* send flagged jobs */
	ktpool->ktp_issender = TRUE;
	ktpool->ktp_isreciever = FALSE;

	/* update job pool */
}

void
kthreadpool_itc_recieve(itpc, ktpool)
	struct threadpool_itpc *itpc;
    struct kthreadpool *ktpool;
{
    /* command / action */
	itpc->itc_ktpool = ktpool;
	itpc->itc_jobs = ktpool->ktp_jobs; /* add/ get current job */
	ktpool->ktp_issender = FALSE;
	ktpool->ktp_isreciever = TRUE;

	/* update job pool */
}

/* Initialize a lock on a kthread */
int
kthread_lock_init(lkp, kt)
    struct lock *lkp;
    kthread_t kt;
{
    int error = 0;
    lockinit(lkp, lkp->lk_prio, lkp->lk_wmesg, lkp->lk_timo, lkp->lk_flags);
    set_kthread_lockholder(lkp->lk_lockholder, kt);
    if(get_kthread_lockholder(lkp->lk_lockholder, kt->kt_tid) == NULL) {
    	panic("kthread lock unavailable");
    	error = EBUSY;
    }
    return (error);
}

int
kthread_lockmgr(lkp, flags, kt)
	struct lock *lkp;
	u_int flags;
    kthread_t kt;
{
    pid_t pid;
    if (kt) {
        pid = kt->kt_tid;
    } else {
        pid = LK_KERNPROC;
    }
    return lockmgr(lkp, flags, lkp->lk_lnterlock, pid);
}

/* Initialize a rwlock on a kthread */
int
kthread_rwlock_init(rwl, kt)
	rwlock_t rwl;
	kthread_t kt;
{
	int error = 0;
	rwlock_init(rwl, rwl->rwl_prio, rwl->rwl_wmesg, rwl->rwl_timo, rwl->rwl_flags);
	set_kthread_lockholder(rwl->rwl_lockholder, kt);
    if(get_kthread_lockholder(rwl->rwl_lockholder, kt->kt_tid) == NULL) {
    	panic("kthread rwlock unavailable");
    	error = EBUSY;
    }
	return (error);
}

int
kthread_rwlockmgr(rwl, flags, kt)
	rwlock_t rwl;
	u_int flags;
	kthread_t kt;
{
	pid_t pid;
	if (kt) {
		pid = kt->kt_tid;
	} else {
		pid = LK_KERNPROC;
	}
	return rwlockmgr(rwl, flags, pid);
}

/*
 * Locate a kthread by number
 */
struct kthread *
ktfind(tid)
	register int tid;
{
	register struct kthread *kt;
	for (kt = TIDHASH(tid); kt != 0; kt = LIST_NEXT(kt, kt_hash))
		if(kt->kt_tid == tid)
			return (kt);
	return (NULL);
}

/*
 * remove kthread from thread group
 */
int
leavektgrp(kt)
	register struct kthread *kt;
{
	register struct kthread **ktt = &kt->kt_pgrp->pg_mem;

	for (; *ktt; ktt = &(*ktt)->kt_pgrpnxt) {
		if (*ktt == kt) {
			*ktt = kt->kt_pgrpnxt;
			break;
		}
	}
#ifdef DIAGNOSTIC
	if (ktt == NULL)
		panic("leavepgrp: can't find p in pgrp");
#endif
	if (!kt->kt_pgrp->pg_mem)
		tgdelete(kt->kt_pgrp);
	kt->kt_pgrp = 0;
	return (0);
}
