/*	$NetBSD: subr_workqueue.c,v 1.7 2006/11/01 10:17:59 yamt Exp $	*/

/*-
 * Copyright (c)2002, 2005 YAMAMOTO Takashi,
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
 */

#include <sys/cdefs.h>
/* __KERNEL_RCSID(0, "$NetBSD: subr_workqueue.c,v 1.7 2006/11/01 10:17:59 yamt Exp $"); */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kthread.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/user.h>

#include <devel/sys/malloctypes.h>
#include <devel/sys/workqueue.h>

SIMPLEQ_HEAD(workqhead, work);
struct workqueue_queue {
	struct lock_object 		*q_lock;
	int 					q_savedipl;
	struct workqhead 		q_queue;
	struct proc 			*q_worker;
};

struct workqueue {
	struct workqueue_queue 	wq_queue; /* todo: make this per-cpu */

	void 					(*wq_func)(struct work *, void *);
	void 					*wq_arg;
	int 					wq_flags;

	const char 				*wq_name;
	int 					wq_prio;
	int 					wq_ipl;
};

struct workqueue_exitargs {
	struct work 			wqe_wk;
	struct workqueue_queue 	*wqe_q;
};

#define	POISON				0xaabbccdd

static void
workqueue_lock(wq, q)
	struct workqueue *wq;
	struct workqueue_queue *q;
{
	int s;

	s = splhigh(); /* XXX */
	simple_lock(&q->q_lock);
	q->q_savedipl = s;
}

static void
workqueue_unlock(wq, q)
	struct workqueue *wq;
	struct workqueue_queue *q;
{
	int s;

	s = q->q_savedipl;
	simple_unlock(&q->q_lock);
	splx(s);
}

static void
workqueue_runlist(wq, list)
	struct workqueue *wq;
	struct workqhead *list;
{
	struct work *wk;
	struct work *next;

	for (wk = SIMPLEQ_FIRST(list); wk != NULL; wk = next) {
		next = SIMPLEQ_NEXT(wk, wk_entry);
		(*wq->wq_func)(wk, wq->wq_arg);
	}
}

static void
workqueue_run(wq)
	struct workqueue *wq;
{
	struct workqueue_queue *q;

	q = &wq->wq_queue;

	for (;;) {
		struct workqhead tmp;
		int error;

		SIMPLEQ_INIT(&tmp);


		/*
		 * we violate abstraction of SIMPLEQ.
		 */
#if defined(DIAGNOSTIC)
		tmp.sqh_last = (void *)POISON;
#endif /* defined(DIAGNOSTIC) */

		workqueue_lock(wq, q);
		while (SIMPLEQ_EMPTY(&q->q_queue)) {
			error = ltsleep(q, wq->wq_prio, wq->wq_name, 0, &q->q_lock);
			if (error) {
				panic("%s: %s error=%d", __func__, wq->wq_name, error);
			}
		}
		SIMPLEQ_FIRST(tmp) = SIMPLEQ_FIRST(q->q_queue); /* XXX */
		SIMPLEQ_INIT(&q->q_queue);
		workqueue_unlock(wq, q);

		workqueue_runlist(wq, &tmp);
	}
}

static void
workqueue_worker(arg)
	void *arg;
{
	struct workqueue *wq = arg;

	workqueue_run(wq);
}

static void
workqueue_init(wq, name, callback_func, callback_arg, prio, ipl)
	struct workqueue *wq;
	const char *name;
	void (*callback_func)(struct work *, void *);
	void *callback_arg;
	int prio, ipl;
{
	wq->wq_ipl = ipl;
	wq->wq_prio = prio;
	wq->wq_name = name;
	wq->wq_func = callback_func;
	wq->wq_arg = callback_arg;
}

static int
workqueue_initqueue(wq)
	struct workqueue *wq;
{
	struct workqueue_queue *q;
	int error;

	q = &wq->wq_queue;
	simple_lock_init(&q->q_lock);
	SIMPLEQ_INIT(&q->q_queue);
	error = kthread_create(workqueue_worker, wq, &q->q_worker, wq->wq_name);

	return (error);
}

static void
workqueue_exit(wk, arg)
	struct work *wk;
	void *arg;
{
	struct workqueue_exitargs *wqe = (void *)wk;
	struct workqueue_queue *q = wqe->wqe_q;

	/*
	 * no need to raise ipl because only competition at this point
	 * is workqueue_finiqueue.
	 */

	KASSERT(q->q_worker == curproc());
	simple_lock(&q->q_lock);
	q->q_worker = NULL;
	simple_unlock(&q->q_lock);
	wakeup(q);
	kthread_exit(0);
}

static void
workqueue_finiqueue(wq)
	struct workqueue *wq;
{
	struct workqueue_queue *q;
	struct workqueue_exitargs wqe;

	q = &wq->wq_queue;
	wq->wq_func = workqueue_exit;

	wqe.wqe_q = q;
	KASSERT(SIMPLEQ_EMPTY(&q->q_queue));
	KASSERT(q->q_worker != NULL);
	workqueue_lock(wq, q);
	SIMPLEQ_INSERT_TAIL(&q->q_queue, &wqe.wqe_wk, wk_entry);
	wakeup(q);
	while (q->q_worker != NULL) {
		int error;

		error = ltsleep(q, wq->wq_prio, "wqfini", 0, &q->q_lock);
		if (error) {
			panic("%s: %s error=%d", __func__, wq->wq_name, error);
		}
	}
	workqueue_unlock(wq, q);
}

/* --- */
int
workqueue_create(wqp, name, callback_func, callback_arg, prio, ipl, flags)
	struct workqueue **wqp;
	const char *name;
	void (*callback_func)(struct work *, void *);
	void *callback_arg;
	int prio, ipl, flags;
{
	struct workqueue *wq;
	int error;

	wq = (struct workqueue *)malloc(wq, sizeof(struct workqueue *), M_WORKQUEUE, M_WAITOK);
	if (wq == NULL) {
		return ENOMEM;
	}

	workqueue_init(wq, name, callback_func, callback_arg, prio, ipl);

	error = workqueue_initqueue(wq);
	if (error) {
		free(wq, M_WORKQUEUE);
		return error;
	}

	*wqp = wq;
	return 0;
}

void
workqueue_destroy(wq)
	struct workqueue *wq;
{
	workqueue_finiqueue(wq);
	free(wq, M_WORKQUEUE);
}

void
workqueue_enqueue(wq, wk)
	struct workqueue *wq;
	struct work *wk;
{
	struct workqueue_queue *q = &wq->wq_queue;
	bool_t wasempty;

	workqueue_lock(wq, q);
	wasempty = SIMPLEQ_EMPTY(&q->q_queue);
	SIMPLEQ_INSERT_TAIL(&q->q_queue, wk, wk_entry);
	workqueue_unlock(wq, q);

	if (wasempty) {
		wakeup(q);
	}
}
