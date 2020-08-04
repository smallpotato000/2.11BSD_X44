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

/* License header should be NetBSD */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/map.h>
#include <sys/user.h>
#include <sys/threadpool.h>
#include <sys/uthread.h>

struct uthreadpool_thread 			utpool_thread;
lock_t	 							uthreadpools_lock;

struct uthreadpool_unbound {
	struct uthreadpool				utpu_pool;

	/* protected by threadpools_lock */
	LIST_ENTRY(uthreadpool_unbound)	utpu_link;
	uint64_t						utpu_refcnt;
};
static LIST_HEAD(, uthreadpool_unbound) unbound_uthreadpools;

static struct uthreadpool_unbound *
uthreadpool_lookup_unbound(u_char pri)
{
	struct uthreadpool_unbound *utpu;
	LIST_FOREACH(utpu, &unbound_uthreadpools, utpu_link) {
		if (utpu->utpu_pool.utp_pri == pri)
			return utpu;
	}
	return NULL;
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

void
uthreadpools_init(void)
{
	MALLOC(&utpool_thread, struct uthreadpool_thread *, sizeof(struct uthreadpool_thread *), M_UTPOOLTHREAD, NULL);
	LIST_INIT(&unbound_uthreadpools);
//	LIST_INIT(&percpu_threadpools);
//	uthread_mutex_init(&uthreadpools_lock, &utpool_thread->utpt_pool->);
	itpc_threadpool_init();
}

/* Thread pool creation */

static int
uthreadpool_create(struct uthreadpool *utpool, u_char pri)
{
	struct kthread *kt;
	int utflags;
	int error;

//	uthread_mutex_init(&utpool->utp_lock, )
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
		error = uthread_create(kt);
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
	//mutex_destroy(&utpool->utp_lock);
	return (error);
}

/* Thread pool destruction */
static void
uthreadpool_destroy(struct uthreadpool *utpool)
{
	struct uthreadpool_thread *uthread;

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

	//mutex_enter(&kthreadpools_lock);
	utpu = uthreadpool_lookup_unbound(pri);
	if (utpu == NULL) {
		error = uthreadpool_create(&tmp->utpu_pool, pri);
		if (error) {
			FREE(tmp, M_UTPOOLTHREAD);
			return error;
		}
		//mutex_enter(&kthreadpools_lock);
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
	//mutex_exit(&kthreadpools_lock);

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

	//mutex_lock(&uthreadpools_lock);
	KASSERT(utpu == uthreadpool_lookup_unbound(pri));
	KASSERT(0 < utpu->utpu_refcnt);
	if (utpu->utpu_refcnt-- == 0) {
		uthreadpool_remove_unbound(utpu);
	} else {
		utpu = NULL;
	}
	//mutex_exit(&uthreadpools_lock);

	if (utpu) {
		kthreadpool_destroy(&utpu->utpu_pool);
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
	KASSERT((utpool->utp_cpu == NULL) || (curkthread->kt_flag & LP_BOUND));

	/* Wait until we're initialized.  */
	//mutex_spin_enter(&ktpool->ktp_lock);

	for (;;) {
		/* Wait until there's a job.  */
		while (TAILQ_EMPTY(&utpool->utp_jobs)) {

		}
		if (__predict_false(TAILQ_EMPTY(&utpool->utp_jobs)))
			break;

		/* If there are no threads, we'll have to try to start one.  */
		if (TAILQ_EMPTY(&utpool->utp_idle_threads)) {
			uthreadpool_hold(utpool);
			mutex_spin_exit(&utpool->utp_lock);

			struct uthreadpool_thread *const uthread = (struct uthreadpool_thread *) malloc(sizeof(struct uthreadpool_thread *), M_UTPOOLTHREAD, M_WAITOK);
			uthread->utpt_kthread = NULL;
			uthread->utpt_pool = pool;
			uthread->utpt_job = NULL;
			//cv_init(&thread->tpt_cv, "poolthrd");

			utflags = 0;
			utflags |= UTHREAD_MPSAFE;
			if (utpool->utp_pri < PRI_KERNEL)
				utflags |= UTHREAD_TS;
			error = uthread_create(utpool->utp_pri, utflags, utpool->utp_cpu, &uthreadpool_thread, uthread, &p, "poolthread/%d@%d", (utpool->utp_cpu ? cpu_index(utpool->utp_cpu) : -1), (int)utpool->utp_pri);

			//mutex_spin_enter(&pool->tp_lock);
			if (error) {
			//	pool_cache_put(kthreadpool_thread_pc, kthread);
				uthreadpool_rele(utpool);
				/* XXX What to do to wait for memory?  */
				//(void)kpause("thrdplcr", FALSE, hz, &ktpool->ktp_lock);
				continue;
			}
			/*
			 * New kthread now owns the reference to the pool
			 * taken above.
			 */
			KASSERT(p != NULL);
			TAILQ_INSERT_TAIL(&utpool->utp_idle_threads, uthread, utpt_entry);
			uthread->utpt_kthread = kt;
			p = NULL;
			//cv_broadcast(&thread->tpt_cv);
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
		//mutex_spin_exit(&pool->tp_lock);

		//mutex_enter(job->job_lock);
		/* If the job was cancelled, we'll no longer be its thread.  */
		if (__predict_true(job->job_utp_thread == overseer)) {
			//mutex_spin_enter(&pool->tp_lock);
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
				job->job_ktp_thread = thread;
				//cv_broadcast(&thread->tpt_cv);
			}
			//mutex_spin_exit(&pool->tp_lock);
		}
		threadpool_job_rele(job);
		//mutex_exit(job->job_lock);

		//mutex_spin_enter(&pool->tp_lock);
	}
	kthreadpool_rele(utpool);
	//mutex_spin_exit(&pool->tp_lock);

	uthread_exit(0);
}

static void
uthreadpool_thread(void *arg)
{
	struct uthreadpool_thread *const uthread = arg;
	struct uthreadpool *const utpool = uthread->utpt_pool;

	/* Wait until we're initialized and on the queue.  */
	//mutex_spin_enter(&ktpool->ktp_lock);

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
		kthread_lock(curkthread);
		char *const kthread_name = curkthread->kt_name;
		uthread->utpt_uthread_savedname = curkthread->kt_name;
		curkthread->kt_name = job->job_name;
		kthread_unlock(curkthread);

		//mutex_spin_exit(&ktpool->ktp_lock);


		/* Run the job.  */
		(*job->job_fn)(job);

		/* lwp name restored in threadpool_job_done(). */
		KASSERTMSG((curkthread->kt_name == kthread_name), "someone forgot to call threadpool_job_done()!");

		/*
		 * We can compare pointers, but we can no longer deference
		 * job after this because threadpool_job_done() drops the
		 * last reference on the job while the job is locked.
		 */

		//mutex_spin_enter(&ktpool->ktp_lock);
		KASSERT(uthread->utpt_job == job);
		uthread->utpt_job = NULL;
		TAILQ_INSERT_TAIL(&utpool->utp_idle_threads, uthread, utpt_entry);
	}
	uthreadpool_rele(utpool);
	//mutex_spin_exit(&ktpool->ktp_lock);

	uthread_exit(0);
}
