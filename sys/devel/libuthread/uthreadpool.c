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
/*	$NetBSD: kern_threadpool.c,v 1.18 2020/04/25 17:43:23 thorpej Exp $	*/
/*-
 * Copyright (c) 2014, 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell and Jason R. Thorpe.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/malloc.h>

#include <sys/map.h>
#include <sys/user.h>

#include <devel/sys/threadpool.h>
#include <devel/sys/malloctypes.h>
#include <devel/libuthread/uthread.h>

struct uthreadpool_thread 				utpool_thread;
struct lock_object	 					uthreadpools_lock;

struct uthreadpool_unbound {
	struct uthreadpool					utpu_pool;

	/* protected by uthreadpools_lock */
	LIST_ENTRY(uthreadpool_unbound)		utpu_link;
	uint64_t							utpu_refcnt;
};
static LIST_HEAD(, uthreadpool_unbound) unbound_uthreadpools;

struct uthreadpool_percpu {
	struct percpu						*utpp_percpu;
	u_char								utpp_pri;
	/* protected by threadpools_lock */
	LIST_ENTRY(uthreadpool_percpu)		utpp_link;
	uint64_t							utpp_refcnt;
};
static LIST_HEAD(, uthreadpool_percpu) 	percpu_uthreadpools;

static struct uthreadpool_unbound *
uthreadpool_lookup_unbound(u_char pri)
{
	struct uthreadpool_unbound *utpu;
	LIST_FOREACH(utpu, &unbound_uthreadpools, utpu_link) {
		if (utpu->utpu_pool.utp_pri == pri)
			return (utpu);
	}
	return (NULL);
}

static void
uthreadpool_insert_unbound(struct uthreadpool_unbound *utpu)
{
	KASSERT(uthreadpool_lookup_unbound(utpu->utpu_pool.utp_pri) == NULL);
	LIST_INSERT_HEAD(&unbound_uthreadpools, utpu, utpu_link);
}

static void
uthreadpool_remove_unbound(struct uthreadpool_unbound *utpu)
{
	KASSERT(uthreadpool_lookup_unbound(utpu->utpu_pool.utp_pri) == utpu);
	LIST_REMOVE(utpu, utpu_link);
}

static struct uthreadpool_percpu *
uthreadpool_lookup_percpu(u_char pri)
{
	struct uthreadpool_percpu *utpp;

	LIST_FOREACH(utpp, &percpu_uthreadpools, utpp_link) {
		if (utpp->utpp_pri == pri)
			return (utpp);
	}
	return (NULL);
}

static void
uthreadpool_insert_percpu(struct uthreadpool_percpu *utpp)
{
	KASSERT(uthreadpool_lookup_percpu(utpp->utpp_pri) == NULL);
	LIST_INSERT_HEAD(&percpu_uthreadpools, utpp, utpp_link);
}

static void
uthreadpool_remove_percpu(struct uthreadpool_percpu *utpp)
{
	KASSERT(uthreadpool_lookup_percpu(utpp->utpp_pri) == utpp);
	LIST_REMOVE(utpp, utpp_link);
}

void
uthreadpool_init(void)
{
	MALLOC(&utpool_thread, struct uthreadpool_thread *, sizeof(struct uthreadpool_thread *), M_UTPOOLTHREAD, NULL);
	LIST_INIT(&unbound_uthreadpools);
	LIST_INIT(&percpu_uthreadpools);
	simple_lock_init(&uthreadpools_lock, "uthreadpools_lock");
}

/* Thread pool creation */

static int
uthreadpool_create(struct uthreadpool *utpool, u_char pri)
{
	struct kthread *kt;
	int utflags;
	int error;

	simple_lock(&utpool->utp_lock);
	/* XXX overseer */
	TAILQ_INIT(&utpool->utp_jobs);
	TAILQ_INIT(&utpool->utp_idle_threads);

	utpool->utp_refcnt = 1;
	utpool->utp_flags = 0;
	utpool->utp_pri = pri;

	utpool->utp_overseer.utpt_kthread = NULL;
	utpool->utp_overseer.utpt_pool = utpool;
	utpool->utp_overseer.utpt_job = NULL;

	utflags = 0;
	if(pri) {
		error = uthread_create(&uthreadpool_overseer_thread, &utpool->utp_overseer, &kt, "uthread pooloverseer/%d@%d");
	}
	if(error) {
		goto fail;
	}

	utpool->utp_overseer.utpt_kthread = kt->kt_uthreado;

fail:
	KASSERT(error);
	KASSERT(utpool->utp_overseer.utpt_job == NULL);
	KASSERT(utpool->utp_overseer.utpt_pool == utpool);
	KASSERT(utpool->utp_flags == 0);
	KASSERT(utpool->utp_refcnt == 0);
	KASSERT(TAILQ_EMPTY(&utpool->utp_idle_threads));
	KASSERT(TAILQ_EMPTY(&utpool->utp_jobs));
	simple_unlock(&utpool->utp_lock);

	return (error);
}

/* Thread pool destruction */
static void
uthreadpool_destroy(struct uthreadpool *utpool)
{
	struct uthreadpool_thread *uthread;

	simple_lock(&utpool->utp_lock);
	KASSERT(TAILQ_EMPTY(&utpool->utp_jobs));
	utpool->utp_flags |= UTHREADPOOL_DYING;

	KASSERT(utpool->utp_overseer.utpt_job == NULL);
	KASSERT(utpool->utp_overseer.utpt_pool == utpool);
	KASSERT(utpool->utp_flags == UTHREADPOOL_DYING);
	KASSERT(utpool->utp_refcnt == 0);
	KASSERT(TAILQ_EMPTY(&utpool->utp_idle_threads));
	KASSERT(TAILQ_EMPTY(&utpool->utp_jobs));

	simple_unlock(&utpool->utp_lock);
}

static void
uthreadpool_hold(utpool)
	struct uthreadpool *utpool;
{
	utpool->utp_refcnt++;
	KASSERT(utpool->utp_refcnt != 0);
}

static void
uthreadpool_rele(utpool)
	struct uthreadpool *utpool;
{
	KASSERT(0 < utpool->utp_refcnt);
}

/* Unbound thread pools */

int
uthreadpool_get(struct uthreadpool **utpoolp, u_char pri)
{
	struct uthreadpool_unbound *utpu, *tmp = NULL;
	int error;

	simple_lock(&uthreadpools_lock);
	utpu = uthreadpool_lookup_unbound(pri);
	if (utpu == NULL) {
		error = uthreadpool_create(&tmp->utpu_pool, pri);
		if (error) {
			FREE(tmp, M_UTPOOLTHREAD);
			return error;
		}
		simple_lock(&uthreadpools_lock);
		utpu = uthreadpool_lookup_unbound(pri);
		if (utpu == NULL) {
			utpu = tmp;
			tmp = NULL;
			uthreadpool_insert_unbound(utpu);
		}
	}
	KASSERT(utpu != NULL);
	utpu->utpu_refcnt++;
	KASSERT(utpu->utpu_refcnt != 0);
	simple_unlock(&uthreadpools_lock);

	if (tmp != NULL) {
		uthreadpool_destroy(&tmp->utpu_pool);
		FREE(tmp, M_UTPOOLTHREAD);
	}

	KASSERT(utpu != NULL);
	*utpoolp = &utpu->utpu_pool;
	return (0);
}

void
uthreadpool_put(struct uthreadpool *utpool, u_char pri)
{
	struct uthreadpool_unbound *utpu;

	simple_lock(&uthreadpools_lock);
	KASSERT(utpu == uthreadpool_lookup_unbound(pri));
	KASSERT(0 < utpu->utpu_refcnt);
	if (utpu->utpu_refcnt-- == 0) {
		uthreadpool_remove_unbound(utpu);
	} else {
		utpu = NULL;
	}
	simple_unlock(&uthreadpools_lock);

	if (utpu) {
		uthreadpool_destroy(&utpu->utpu_pool);
		FREE(utpu, M_UTPOOLTHREAD);
	}
}

static void
uthreadpool_overseer_thread(void *arg)
{
	struct uthreadpool_thread *const overseer = arg;
	struct uthreadpool *const utpool = overseer->utpt_pool;
	struct kthread *kt = NULL;
	int utflags;
	int error;


	KASSERT((utpool->utp_cpu == NULL) || (utpool->utp_cpu == curcpu()));
	KASSERT((utpool->utp_cpu == NULL) || (curkthread->kt_flag & KT_BOUND));

	/* Wait until we're initialized.  */
	simple_lock(&utpool->utp_lock);

	for (;;) {
		/* Wait until there's a job.  */
		while (TAILQ_EMPTY(&utpool->utp_jobs)) {

		}
		if (__predict_false(TAILQ_EMPTY(&utpool->utp_jobs)))
			break;

		/* If there are no threads, we'll have to try to start one.  */
		if (TAILQ_EMPTY(&utpool->utp_idle_threads)) {
			uthreadpool_hold(utpool);
			simple_unlock(&utpool->utp_lock);

			struct uthreadpool_thread *const uthread = (struct uthreadpool_thread *) malloc(sizeof(struct uthreadpool_thread *), M_UTPOOLTHREAD, M_WAITOK);
			uthread->utpt_kthread = NULL;
			uthread->utpt_pool = utpool;
			uthread->utpt_job = NULL;

			utflags = 0;
			utflags |= UTHREAD_MPSAFE;
			if (utpool->utp_pri < PUSER)
				utflags |= UTHREAD_TS;
			error = uthread_create(&uthreadpool_thread, uthread, &kt, "uthread poolthread/%d@%d");

			simple_lock(&utpool->utp_lock);
			if (error) {
				uthreadpool_rele(utpool);
				continue;
			}
			/*
			 * New kthread now owns the reference to the pool
			 * taken above.
			 */
			KASSERT(kt != NULL);
			TAILQ_INSERT_TAIL(&utpool->utp_idle_threads, uthread, utpt_entry);
			uthread->utpt_kthread = kt;
			kt = NULL;
			continue;
		}

		/* There are idle threads, so try giving one a job.  */
		struct threadpool_job *const job = TAILQ_FIRST(&utpool->utp_jobs);
		TAILQ_REMOVE(&utpool->utp_jobs, job, job_entry);
		/*
		 * Take an extra reference on the job temporarily so that
		 * it won't disappear on us while we have both locks dropped.
		 */
		threadpool_job_hold(job);
		simple_unlock(&utpool->utp_lock);

		simple_lock(job->job_lock);
		/* If the job was cancelled, we'll no longer be its thread.  */
		if (__predict_true(job->job_uthread == overseer)) {
			simple_lock(&utpool->utp_lock);
			if (__predict_false(TAILQ_EMPTY(&utpool->utp_idle_threads))) {
				/*
				 * Someone else snagged the thread
				 * first.  We'll have to try again.
				 */
				TAILQ_INSERT_HEAD(&utpool->utp_jobs, job, job_entry);
			} else {
				/*
				 * Assign the job to the thread and
				 * wake the thread so it starts work.
				 */
				struct uthreadpool_thread *const thread = TAILQ_FIRST(&utpool->utp_idle_threads);

				KASSERT(thread->utpt_job == NULL);
				TAILQ_REMOVE(&utpool->utp_idle_threads, thread, utpt_entry);
				thread->utpt_job = job;
				job->job_kthread = thread;
			}
			simple_unlock(&utpool->utp_lock);
		}
		threadpool_job_rele(job);
		simple_unlock(job->job_lock);

		simple_lock(&utpool->utp_lock);
	}
	uthreadpool_rele(utpool);
	simple_unlock(&utpool->utp_lock);

	uthread_exit(0);
}

static void
uthreadpool_thread(void *arg)
{
	struct uthreadpool_thread *const uthread = arg;
	struct uthreadpool *const utpool = uthread->utpt_pool;

	/* Wait until we're initialized and on the queue.  */
	simple_lock(&utpool->utp_lock);

	KASSERT(uthread->utpt_kthread == curkthread);
	for (;;) {
		/* Wait until we are assigned a job.  */
		while (uthread->utpt_job == NULL) {

		}
		if (__predict_false(uthread->utpt_job == NULL)) {
			TAILQ_REMOVE(&utpool->utp_idle_threads, uthread, utpt_entry);
			break;
		}
		struct threadpool_job *const job = uthread->utpt_job;
		KASSERT(job != NULL);

		/* Set our lwp name to reflect what job we're doing.  */
		KTHREAD_LOCK(curkthread);
		char *const kthread_name = curkthread->kt_name;
		uthread->utpt_uthread_savedname = curkthread->kt_name;
		curkthread->kt_name = job->job_name;
		KTHREAD_UNLOCK(curkthread);

		simple_unlock(&utpool->utp_lock);


		/* Run the job.  */
		(*job->job_func)(job);

		/* lwp name restored in threadpool_job_done(). */
		KASSERTMSG((curkthread->kt_name == kthread_name), "someone forgot to call threadpool_job_done()!");

		/*
		 * We can compare pointers, but we can no longer deference
		 * job after this because threadpool_job_done() drops the
		 * last reference on the job while the job is locked.
		 */

		simple_lock(&utpool->utp_lock);
		KASSERT(uthread->utpt_job == job);
		uthread->utpt_job = NULL;
		TAILQ_INSERT_TAIL(&utpool->utp_idle_threads, uthread, utpt_entry);
	}
	uthreadpool_rele(utpool);
	simple_unlock(&utpool->utp_lock);

	uthread_exit(0);
}

/* Threadpool Jobs */
void
uthreadpool_job_init(struct threadpool_job *job, threadpool_job_fn_t func, lock_t lock, char *name, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	(void) snprintf(job->job_name, sizeof(job->job_name), fmt, ap);
	va_end(ap);

	job->job_lock = lock;
	job->job_name = name;
	job->job_refcnt = 0;
	job->job_itpc->itpc_ktpool = NULL;
	job->job_itpc->itpc_utpool = NULL;
	job->job_func = func;
}

void
uthreadpool_job_dead(struct threadpool_job *job)
{
	panic("threadpool job %p ran after destruction", job);
}

void
uthreadpool_job_destroy(struct threadpool_job *job)
{
	KASSERTMSG((job->job_uthread == NULL), "job %p still running", job);
	job->job_lock = NULL;
	KASSERT(job->job_uthread == NULL);
	KASSERT(job->job_refcnt == 0);
	job->job_func = uthreadpool_job_dead;
	(void) strlcpy(job->job_name, "deadjob", sizeof(job->job_name));
}

void
uthreadpool_job_hold(struct threadpool_job *job)
{
	unsigned int refcnt;

	do {
		refcnt = job->job_refcnt;
		KASSERT(refcnt != UINT_MAX);
	} while (atomic_cas_uint(&job->job_refcnt, refcnt, (refcnt + 1)) != refcnt);
}

void
uthreadpool_job_rele(struct threadpool_job *job)
{
	unsigned int refcnt;

	do {
		refcnt = job->job_refcnt;
		KASSERT(0 < refcnt);
		if (refcnt == 1) {
			refcnt = atomic_dec_int_nv(&job->job_refcnt);
			KASSERT(refcnt != UINT_MAX);
			return;
		}
	} while (atomic_cas_uint(&job->job_refcnt, refcnt, (refcnt - 1)) != refcnt);
}

void
uthreadpool_job_done(struct threadpool_job *job)
{
	KASSERT(job->job_uthread != NULL);
	KASSERT(job->job_uthread->utpt_kthread == curkthread);

	/*
	 * We can safely read this field; it's only modified right before
	 * we call the job work function, and we are only preserving it
	 * to use here; no one cares if it contains junk afterward.
	 */
	KTHREAD_LOCK(curkthread);
	curkthread->kt_name = job->job_uthread->utpt_uthread_savedname;
	KTHREAD_UNLOCK(curkthread);

	/*
	 * Inline the work of threadpool_job_rele(); the job is already
	 * locked, the most likely scenario (XXXJRT only scenario?) is
	 * that we're dropping the last reference (the one taken in
	 * threadpool_schedule_job()), and we always do the cv_broadcast()
	 * anyway.
	 */
	KASSERT(0 < job->job_refcnt);
	unsigned int refcnt __diagused = atomic_dec_int_nv(&job->job_refcnt);
	KASSERT(refcnt != UINT_MAX);
	job->job_uthread = NULL;
}

void
uthreadpool_schedule_job(struct uthreadpool *utpool, struct threadpool_job *job)
{
	if (__predict_true(job->job_uthread != NULL)) {
		return;
	}

	threadpool_job_hold(job);

	simple_lock(&utpool->utp_lock);
	if (__predict_false(TAILQ_EMPTY(&utpool->utp_idle_threads))) {
		job->job_uthread = &utpool->utp_overseer;
		TAILQ_INSERT_TAIL(&utpool->utp_jobs, job, job_entry);
	} else {
		/* Assign it to the first idle thread.  */
		job->job_uthread = TAILQ_FIRST(&utpool->utp_idle_threads);
		job->job_uthread->utpt_job = job;
	}

	/* Notify whomever we gave it to, overseer or idle thread.  */
	KASSERT(job->job_uthread != NULL);
	simple_unlock(&utpool->utp_lock);
}

bool
uthreadpool_cancel_job_async(struct uthreadpool *utpool, struct threadpool_job *job)
{
	/*
	 * XXXJRT This fails (albeit safely) when all of the following
	 * are true:
	 *
	 *	=> "pool" is something other than what the job was
	 *	   scheduled on.  This can legitimately occur if,
	 *	   for example, a job is percpu-scheduled on CPU0
	 *	   and then CPU1 attempts to cancel it without taking
	 *	   a remote pool reference.  (this might happen by
	 *	   "luck of the draw").
	 *
	 *	=> "job" is not yet running, but is assigned to the
	 *	   overseer.
	 *
	 * When this happens, this code makes the determination that
	 * the job is already running.  The failure mode is that the
	 * caller is told the job is running, and thus has to wait.
	 * The overseer will eventually get to it and the job will
	 * proceed as if it had been already running.
	 */

	if (job->job_uthread == NULL) {
		/* Nothing to do.  Guaranteed not running.  */
		return (TRUE);
	} else if (job->job_uthread == &utpool->utp_overseer) {
		/* Take it off the list to guarantee it won't run.  */
		job->job_uthread = NULL;
		simple_lock(&utpool->utp_lock);

		TAILQ_REMOVE(&utpool->utp_jobs, job, job_entry);
		simple_unlock(&utpool->utp_lock);
		threadpool_job_rele(job);
		return (TRUE);
	} else {
		/* Too late -- already running.  */
		return (FALSE);
	}
}

void
uthreadpool_cancel_job(struct uthreadpool *utpool, struct threadpool_job *job)
{
	/*
	 * We may sleep here, but we can't ASSERT_SLEEPABLE() because
	 * the job lock (used to interlock the cv_wait()) may in fact
	 * legitimately be a spin lock, so the assertion would fire
	 * as a false-positive.
	 */

	if (uthreadpool_cancel_job_async(utpool, job))
		return;
}

/* Per-CPU thread pools */
int
uthreadpool_percpu_get(struct uthreadpool_percpu **utpool_percpup, u_char pri)
{
	struct uthreadpool_percpu *utpool_percpu, *tmp = NULL;
	int error;
	utpool_percpu = uthreadpool_lookup_percpu(pri);
	simple_lock(&uthreadpools_lock);
	if(utpool_percpu == NULL) {
		simple_unlock(&uthreadpools_lock);
		error = uthreadpool_percpu_create(&tmp, pri);
		if (error) {
			return (error);
		}
		KASSERT(tmp != NULL);
		simple_lock(&uthreadpools_lock);
		utpool_percpu = uthreadpool_lookup_percpu(pri);
		if (utpool_percpu == NULL) {
			utpool_percpu = tmp;
			tmp = NULL;
			uthreadpool_insert_percpu(utpool_percpu);
		}
	}
	KASSERT(utpool_percpu != NULL);
	utpool_percpu->utpp_refcnt++;
	KASSERT(utpool_percpu->utpp_refcnt != 0);
	simple_unlock(&uthreadpools_lock);

	if (tmp != NULL) {
		uthreadpool_percpu_destroy(tmp);
	}
	KASSERT(utpool_percpu != NULL);
	*utpool_percpup = utpool_percpu;
	return (0);
}

void
uthreadpool_percpu_put(struct uthreadpool_percpu *utpool_percpu, u_char pri)
{
	//KASSERT(uthreadpool_pri_is_valid(pri));

	simple_lock(&uthreadpools_lock);
	KASSERT(utpool_percpu == uthreadpool_lookup_percpu(pri));
	KASSERT(0 < utpool_percpu->utpp_refcnt);
	if (--utpool_percpu->utpp_refcnt == 0) {
		uthreadpool_remove_percpu(utpool_percpu);
	} else {
		utpool_percpu = NULL;
	}
	simple_unlock(&uthreadpools_lock);

	if (utpool_percpu) {
		uthreadpool_percpu_destroy(utpool_percpu);
	}
}

static int
uthreadpool_percpu_create(struct uthreadpool_percpu **utpool_percpup, u_char pri)
{
	struct uthreadpool_percpu *utpool_percpu;
	int error, cpu;

	for (cpu = 0; cpu < cpu_number(); cpu++) {
		error = uthreadpool_percpu_start(utpool_percpu, pri, cpu);
	}

	*utpool_percpup = (struct uthreadpool_percpu *)utpool_percpu;
	return (error);
}

int
uthreadpool_percpu_start(struct uthreadpool_percpu *utpool_percpu, u_char pri, int ncpus)
{
	if (count <= 0) {
		return (EINVAL);
	}

	utpool_percpu = (struct uthreadpool_percpu*) malloc(
			sizeof(struct uthreadpool_percpu*), M_UTPOOLTHREAD, M_WAITOK);
	utpool_percpu->utpp_pri = pri;
	utpool_percpu->utpp_percpu = percpu_create(&cpu_info, sizeof(struct uthreadpool*), 1, ncpus);

	if (utpool_percpu->utpp_percpu) {
		/* Success!  */
		uthreadpool_percpu_init(pri);
		return (0);
	}

	uthreadpool_percpu_fini();
	percpu_extent_free(utpool_percpu->utpp_percpu,
			utpool_percpu->utpp_percpu->pc_start,
			utpool_percpu->utpp_percpu->pc_end);
	free(utpool_percpu, M_UTPOOLTHREAD);
	return (ENOMEM);
}

static void
uthreadpool_percpu_destroy(struct uthreadpool_percpu *utpool_percpu)
{
	percpu_free(utpool_percpu->utpp_percpu, sizeof(struct uthreadpool *));
	free(utpool_percpu, M_KTPOOLTHREAD);
}

static void
uthreadpool_percpu_init(pri)
	u_char pri;
{
	struct uthreadpool **const utpoolp;
	int error;

	*utpoolp = (struct uthreadpool *) malloc(sizeof(struct uthreadpool *), M_UTPOOLTHREAD, M_WAITOK);
	error = uthreadpool_create(*utpoolp, pri);
	if (error) {
		KASSERT(error == ENOMEM);
		free(*utpoolp, M_UTPOOLTHREAD);
		*utpoolp = NULL;
	}
}

static void
uthreadpool_percpu_fini()
{
	struct uthreadpool **const utpoolp;

	if (*utpoolp == NULL) {	/* initialization failed */
		return;
	}
	uthreadpool_destroy(*utpoolp);
	free(*utpoolp, M_UTPOOLTHREAD);
}

/* UThreadpool ITPC */

/* Add a thread to the itc queue */
void
itpc_add_uthreadpool(itpc, utpool)
	struct threadpool_itpc *itpc;
	struct uthreadpool *utpool;
{
	struct uthread *ut;

	/* check user threadpool is not null & has a job/task entry to send */
	if(utpool != NULL) {
		itpc->itpc_utpool = utpool;
		ut = utpool->utp_overseer.utpt_uthread;
		if(ut != NULL) {
			itpc->itpc_utinfo->itpc_uthread = ut;
			itpc->itpc_utinfo->itpc_utid = ut->ut_tid;
			itpc->itpc_utinfo->itpc_utgrp = ut->ut_pgrp;
			itpc->itpc_utinfo->itpc_utjob = utpool->utp_overseer.utpt_job;
			itpc->itpc_refcnt++;
			utpool->utp_initcq = TRUE;
			TAILQ_INSERT_HEAD(itpc->itpc_header, itpc, itpc_entry);
		} else {
			printf("no uthread found in uthreadpool");
		}
	} else {
		printf("no uthreadpool found");
	}
}

/*
 * Remove a thread from the itc queue:
 * If threadpool entry is not null, search queue for entry & remove
 */
void
itpc_remove_uthreadpool(itpc, utpool)
	struct threadpool_itpc *itpc;
	struct uthreadpool *utpool;
{
	struct uthread *ut;

	ut = itpc->itpc_utinfo->itpc_uthread;
	if(utpool != NULL) {
		TAILQ_FOREACH(itpc, itpc->itpc_header, itpc_entry) {
			if(TAILQ_NEXT(itpc, itpc_entry)->itpc_utpool == utpool && utpool->utp_overseer.utpt_uthread == ut) {
				if(ut != NULL) {
					itpc->itpc_utinfo->itpc_uthread = NULL;
					itpc->itpc_utinfo->itpc_utjob = NULL;
					itpc->itpc_refcnt--;
					utpool->utp_initcq = FALSE;
					TAILQ_REMOVE(itpc->itpc_header, itpc, itpc_entry);
				} else {
					printf("cannot remove uthread. does not exist");
				}
			}
		}
	} else {
		printf("no uthread to remove");
	}
}

/* Sender checks request from receiver: providing info */
void
itpc_check_uthreadpool(itpc, utpool)
	struct threadpool_itpc *itpc;
	struct uthreadpool 	*utpool;
{
	register struct uthread *ut;

	ut = utpool->utp_overseer.utpt_uthread;
	if(utpool->utp_issender) {
		printf("user threadpool to send");
		if(itpc->itpc_utpool == utpool) {
			if(itpc->itpc_utinfo->itpc_uthread == ut) {
				if(itpc->itpc_utinfo->itpc_utid == ut->ut_tid) {
					printf("uthread tid found");
				} else {
					printf("uthread tid not found");
					goto retry;
				}
			} else {
				printf("uthread not found");
				goto retry;
			}
		}
		panic("no uthreadpool");
	}

retry:
	if (utpool->utp_retcnt <= 5) { /* retry up to 5 times */
		if (utpool->utp_initcq) {
			/* exit and re-enter queue increasing retry count */
			itpc_remove_uthreadpool(itpc, utpool);
			utpool->utp_retcnt++;
			itpc_add_uthreadpool(itpc, utpool);
		} else {
			/* exit queue, reset count to 0 and panic */
			itpc_remove_uthreadpool(itpc, utpool);
			utpool->utp_retcnt = 0;
			panic("uthreadpool x exited itpc queue after 5 failed attempts");
		}
	}
}

/* Receiver verifies request to sender: providing info */
void
itpc_verify_uthreadpool(itpc, utpool)
	struct threadpool_itpc *itpc;
	struct uthreadpool *utpool;
{
	register struct uthread *ut;

	ut = utpool->utp_overseer.utpt_uthread;
	if(utpool->utp_isreciever) {
		printf("user threadpool to recieve");
		if(itpc->itpc_utpool == utpool) {
			if(itpc->itpc_utinfo->itpc_uthread == ut) {
				if(itpc->itpc_utinfo->itpc_utid == ut->ut_tid) {
					printf("uthread tid found");
				} else {
					printf("uthread tid not found");
				}
			} else {
				printf("uthread not found");
			}
		}
		panic("no uthreadpool");
	}
}
