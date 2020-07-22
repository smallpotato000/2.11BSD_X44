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
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/map.h>
#include <sys/user.h>
#include <sys/threadpool.h>
#include <sys/kthread.h>

struct kthreadpool_thread 			ktpool_thread;
mutex_t 							kthreadpools_lock;

struct kthreadpool_unbound {
	struct kthreadpool				ktpu_pool;

	/* protected by threadpools_lock */
	LIST_ENTRY(threadpool_unbound)	ktpu_link;
	uint64_t						ktpu_refcnt;
};
static LIST_HEAD(, kthreadpool_unbound) unbound_kthreadpools;

static struct kthreadpool_unbound *
kthreadpool_lookup_unbound(u_char pri)
{
	struct kthreadpool_unbound *ktpu;
	LIST_FOREACH(ktpu, &unbound_kthreadpools, ktpu_link) {
		if (ktpu->ktpu_pool.ktp_pri == pri)
			return ktpu;
	}
	return NULL;
}

static void
kthreadpool_insert_unbound(struct kthreadpool_unbound *ktpu)
{
	KASSERT(kthreadpool_lookup_unbound(ktpu->ktpu_pool.ktp_pri) == NULL);
	LIST_INSERT_HEAD(&unbound_kthreadpools, ktpu, ktpu_link);
}

static void
kthreadpool_remove_unbound(struct kthreadpool_unbound *ktpu)
{
	KASSERT(kthreadpool_lookup_unbound(ktpu->ktpu_pool.ktp_pri) == ktpu);
	LIST_REMOVE(ktpu, ktpu_link);
}

void
kthreadpools_init(void)
{
	MALLOC(&ktpool_thread, struct kthreadpool_thread *, sizeof(struct kthreadpool_thread *), M_KTPOOLTHREAD, NULL);
	LIST_INIT(&unbound_kthreadpools);
//	LIST_INIT(&percpu_threadpools);
//	kthread_mutex_init(&kthreadpools_lock, &ktpool_thread->ktpt_pool->);
	itpc_threadpool_init();
}

/* Thread pool creation */

static int
kthreadpool_create(struct kthreadpool *ktpool, u_char pri)
{
	struct proc *p;
	int ktflags;
	int error;

//	kthread_mutex_init(&ktpool->ktp_lock, )
	/* XXX overseer */
	TAILQ_INIT(&ktpool->ktp_jobs);
	TAILQ_INIT(&ktpool->ktp_idle_threads);

	ktpool->ktp_refcnt = 1;
	ktpool->ktp_flags = 0;
	ktpool->ktp_pri = pri;

	ktpool->ktp_overseer.ktpt_kthread = NULL;
	ktpool->ktp_overseer.ktpt_pool = ktpool;
	ktpool->ktp_overseer.ktpt_job = NULL;

	ktflags = 0;
	if(pri) {
		error = kthread_create(p);
	}
	if(error) {
		goto fail;
	}

	ktpool->ktp_overseer.ktpt_kthread = p->p_kthreado;

fail:
	KASSERT(error);
	KASSERT(ktpool->ktp_overseer.ktpt_job == NULL);
	KASSERT(ktpool->ktp_overseer.ktpt_pool == pool);
	KASSERT(ktpool->ktp_flags == 0);
	KASSERT(ktpool->ktp_refcnt == 0);
	KASSERT(TAILQ_EMPTY(&ktpool->ktp_idle_threads));
	KASSERT(TAILQ_EMPTY(&ktpool->ktp_jobs));
	//mutex_destroy(&ktpool->ktp_lock);
	return (error);
}

/* Thread pool destruction */
static void
kthreadpool_destroy(struct kthreadpool *ktpool)
{
	struct kthreadpool_thread *kthread;

}

static void
kthreadpool_hold(ktpool)
	struct kthreadpool *ktpool;
{
	ktpool->ktp_refcnt++;
	KASSERT(ktpool->ktp_refcnt != 0);
}

static void
kthreadpool_rele(ktpool)
	struct kthreadpool *ktpool;
{
	KASSERT(0 < ktpool->ktp_refcnt);
}

/* Unbound thread pools */

int
kthreadpool_get(struct kthreadpool **ktpoolp, u_char pri)
{
	struct kthreadpool_unbound *ktpu, *tmp = NULL;
	int error;

	//mutex_enter(&kthreadpools_lock);
	ktpu = kthreadpool_lookup_unbound(pri);
	if (ktpu == NULL) {
		error = kthreadpool_create(&tmp->ktpu_pool, pri);
		if (error) {
			FREE(tmp, M_KTPOOLTHREAD);
			return error;
		}
		//mutex_enter(&kthreadpools_lock);
		ktpu = kthreadpool_lookup_unbound(pri);
		if (ktpu == NULL) {
			ktpu = tmp;
			tmp = NULL;
			kthreadpool_insert_unbound(ktpu);
		}
	}
	KASSERT(ktpu != NULL);
	ktpu->ktpu_refcnt++;
	KASSERT(ktpu->ktpu_refcnt != 0);
	//mutex_exit(&kthreadpools_lock);

	if (tmp != NULL) {
		threadpool_destroy(&tmp->ktpu_pool);
		FREE(tmp, M_KTPOOLTHREAD);
	}

	KASSERT(ktpu != NULL);
	*ktpoolp = &ktpu->ktpu_pool;
	return (0);
}

void
kthreadpool_put(struct kthreadpool *ktpool, u_char pri)
{
	struct kthreadpool_unbound *ktpu;

	//mutex_enter(&kthreadpools_lock);
	KASSERT(ktpu == kthreadpool_lookup_unbound(pri));
	KASSERT(0 < ktpu->ktpu_refcnt);
	if (ktpu->ktpu_refcnt-- == 0) {
		kthreadpool_remove_unbound(ktpu);
	} else {
		ktpu = NULL;
	}
	//mutex_exit(&kthreadpools_lock);

	if (ktpu) {
		kthreadpool_destroy(&ktpu->ktpu_pool);
		FREE(ktpu, M_KTPOOLTHREAD);
	}
}

static void
kthreadpool_overseer_thread(void *arg)
{
	struct kthreadpool_thread *const overseer = arg;
	struct kthreadpool *const ktpool = overseer->ktpt_pool;
	struct proc *p = NULL;
	int ktflags;
	int error;


	KASSERT((ktpool->ktp_cpu == NULL) || (ktpool->ktp_cpu == curcpu()));
	KASSERT((ktpool->ktp_cpu == NULL) || (curproc->p_flag & LP_BOUND));

	/* Wait until we're initialized.  */
	//mutex_spin_enter(&ktpool->ktp_lock);

	for (;;) {
		/* Wait until there's a job.  */
		while (TAILQ_EMPTY(&ktpool->ktp_jobs)) {

		}
		if (__predict_false(TAILQ_EMPTY(&ktpool->ktp_jobs)))
			break;

		/* If there are no threads, we'll have to try to start one.  */
		if (TAILQ_EMPTY(&ktpool->ktp_idle_threads)) {
			kthreadpool_hold(ktpool);
			mutex_spin_exit(&ktpool->ktp_lock);

			struct kthreadpool_thread *const kthread = (struct kthreadpool_thread *) malloc(sizeof(struct kthreadpool_thread *), M_KTPOOLTHREAD, M_WAITOK);
			kthread->ktpt_proc = NULL;
			kthread->ktpt_pool = pool;
			kthread->ktpt_job = NULL;
			//cv_init(&thread->tpt_cv, "poolthrd");

			ktflags = 0;
			ktflags |= KTHREAD_MPSAFE;
			if (ktpool->ktp_pri < PRI_KERNEL)
				ktflags |= KTHREAD_TS;
			error = kthread_create(ktpool->ktp_pri, ktflags, ktpool->ktp_cpu, &kthreadpool_thread, kthread, &p, "poolthread/%d@%d", (ktpool->ktp_cpu ? cpu_index(ktpool->ktp_cpu) : -1), (int)ktpool->ktp_pri);

			//mutex_spin_enter(&pool->tp_lock);
			if (error) {
			//	pool_cache_put(kthreadpool_thread_pc, kthread);
				kthreadpool_rele(ktpool);
				/* XXX What to do to wait for memory?  */
				//(void)kpause("thrdplcr", FALSE, hz, &ktpool->ktp_lock);
				continue;
			}
			/*
			 * New kthread now owns the reference to the pool
			 * taken above.
			 */
			KASSERT(p != NULL);
			TAILQ_INSERT_TAIL(&ktpool->ktp_idle_threads, kthread, ktpt_entry);
			kthread->ktpt_proc = p;
			p = NULL;
			//cv_broadcast(&thread->tpt_cv);
			continue;
		}

		/* There are idle threads, so try giving one a job.  */
		struct threadpool_job *const job = TAILQ_FIRST(&ktpool->ktp_jobs);
		TAILQ_REMOVE(&ktpool->ktp_jobs, job, job_entry);
		/*
		 * Take an extra reference on the job temporarily so that
		 * it won't disappear on us while we have both locks dropped.
		 */
		threadpool_job_hold(job);
		//mutex_spin_exit(&pool->tp_lock);

		//mutex_enter(job->job_lock);
		/* If the job was cancelled, we'll no longer be its thread.  */
		if (__predict_true(job->job_ktp_thread == overseer)) {
			//mutex_spin_enter(&pool->tp_lock);
			if (__predict_false(TAILQ_EMPTY(&ktpool->ktp_idle_threads))) {
				/*
				 * Someone else snagged the thread
				 * first.  We'll have to try again.
				 */
				TAILQ_INSERT_HEAD(&ktpool->ktp_jobs, job, job_entry);
			} else {
				/*
				 * Assign the job to the thread and
				 * wake the thread so it starts work.
				 */
				struct kthreadpool_thread *const thread = TAILQ_FIRST(&ktpool->ktp_idle_threads);

				KASSERT(thread->ktpt_job == NULL);
				TAILQ_REMOVE(&ktpool->ktp_idle_threads, thread, ktpt_entry);
				thread->ktpt_job = job;
				job->job_ktp_thread = thread;
				//cv_broadcast(&thread->tpt_cv);
			}
			//mutex_spin_exit(&pool->tp_lock);
		}
		threadpool_job_rele(job);
		//mutex_exit(job->job_lock);

		//mutex_spin_enter(&pool->tp_lock);
	}
	kthreadpool_rele(ktpool);
	//mutex_spin_exit(&pool->tp_lock);

	kthread_exit(0);
}

static void
kthreadpool_thread(void *arg)
{
	struct kthreadpool_thread *const kthread = arg;
	struct kthreadpool *const ktpool = kthread->ktpt_pool;

	/* Wait until we're initialized and on the queue.  */
	//mutex_spin_enter(&ktpool->ktp_lock);

	KASSERT(kthread->ktpt_proc == curproc);
	for (;;) {
		/* Wait until we are assigned a job.  */
		while (kthread->ktpt_job == NULL) {

		}
		if (__predict_false(kthread->ktpt_job == NULL)) {
			TAILQ_REMOVE(&ktpool->ktp_idle_threads, kthread, ktpt_entry);
			break;
		}
		struct threadpool_job *const job = kthread->ktpt_job;
		KASSERT(job != NULL);


		/* Set our lwp name to reflect what job we're doing.  */
		proc_lock(curproc);
		char *const proc_name = curproc->p_name;
		kthread->ktpt_kthread_savedname = curproc->p_name;
		curproc->p_name = job->job_name;
		proc_unlock(curproc);

		//mutex_spin_exit(&ktpool->ktp_lock);


		/* Run the job.  */
		(*job->job_fn)(job);

		/* lwp name restored in threadpool_job_done(). */
		KASSERTMSG((curproc->p_name == proc_name), "someone forgot to call threadpool_job_done()!");

		/*
		 * We can compare pointers, but we can no longer deference
		 * job after this because threadpool_job_done() drops the
		 * last reference on the job while the job is locked.
		 */

		//mutex_spin_enter(&ktpool->ktp_lock);
		KASSERT(kthread->ktpt_job == job);
		kthread->ktpt_job = NULL;
		TAILQ_INSERT_TAIL(&ktpool->ktp_idle_threads, kthread, ktpt_entry);
	}
	kthreadpool_rele(ktpool);
	//mutex_spin_exit(&ktpool->ktp_lock);

	kthread_exit(0);
}
