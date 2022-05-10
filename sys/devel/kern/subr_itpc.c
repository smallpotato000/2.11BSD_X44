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
#include <sys/itpc.h>
#include <sys/threadpool.h>
#include <sys/kthread.h>
#include <sys/percpu.h>

#include <vm/include/vm_param.h>

#include <devel/sys/malloctypes.h>

struct itpc_list itpc_header;

/* Threadpool ITPC */
void
itpc_threadpool_init(void)
{
	itpc_setup(&itpc);
}

static void
itpc_setup(itpc)
	struct threadpool_itpc *itpc;
{
	MALLOC(itpc, struct threadpool_itpc *, sizeof(struct threadpool_itpc *), M_ITPC, NULL);
	TAILQ_INIT(itpc->itpc_header);
	itpc->itpc_refcnt = 0;
	itpc->itpc_jobs = NULL;
	itpc->itpc_job_name = NULL;
}

struct itpc *
itpc_create(void)
{
	struct itpc *itpc;
	itpc = (struct itpc *)malloc(sizeof(struct itpc *), M_ITPC, M_WAITOK);
	return (itpc);
}

void
itpc_free(struct itpc *itpc)
{
	free(itpc, M_ITPC);
}

void
itpc_add(struct itpc *itpc)
{
	TAILQ_INSERT_HEAD(&itpc_header, itpc, itpc_entry);
}

void
itpc_remove(struct itpc *itpc)
{
	TAILQ_REMOVE(&itpc_header, itpc, itpc_entry);
}

void
itpc_job_init(itpc, job, fmt, ap)
	struct threadpool_itpc *itpc;
	struct threadpool_job *job;
	const char *fmt;
	va_list ap;
{
	if (job == itpc->itpc_jobs) {
		job->job_itpc = itpc;
		threadpool_job_init(job, job->job_func, job->job_lock, job->job_name, fmt, ap);
	}
}

void
itpc_job_dead(itpc, job)
	struct threadpool_itpc *itpc;
	struct threadpool_job *job;
{
	if (job == itpc->itpc_jobs) {
		threadpool_job_dead(job);
	}
}

void
itpc_job_destroy(itpc, job)
	struct threadpool_itpc *itpc;
	struct threadpool_job *job;
{
	if (job == itpc->itpc_jobs) {
		threadpool_job_destroy(job);
	}
}

void
itpc_job_hold(itpc, job)
	struct threadpool_itpc *itpc;
	struct threadpool_job *job;
{
	if (job == itpc->itpc_jobs) {
		threadpool_job_hold(job);
	}
}

void
itpc_job_rele(itpc, job)
	struct threadpool_itpc *itpc;
	struct threadpool_job *job;
{
	if (job == itpc->itpc_jobs) {
		threadpool_job_rele(job);
	}
}

void
itpc_job_done(itpc, job)
	struct threadpool_itpc *itpc;
	struct threadpool_job *job;
{
	if (job == itpc->itpc_jobs) {
		threadpool_job_done(job);
	}
}

struct kthreadpool
itpc_kthreadpool(itpc, ktpool)
	struct threadpool_itpc 	*itpc;
	struct kthreadpool 		*ktpool;
{
	if (itpc->itpc_ktpool == ktpool) {
		return (itpc->itpc_ktpool);
	}
	return (NULL);
}

struct uthreadpool
itpc_uthreadpool(itpc, utpool)
	struct threadpool_itpc 	*itpc;
	struct uthreadpool 		*utpool;
{
	if (itpc->itpc_utpool == utpool) {
		return (itpc->itpc_utpool);
	}
	return (NULL);
}

/* Threadpool Jobs */
void
threadpool_job_init(void *thread, struct threadpool_job *job, threadpool_job_fn_t func, struct lock *lock, char *name, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	(void) snprintf(job->job_name, sizeof(job->job_name), fmt, ap);
	va_end(ap);

	job->job_lock = lock;
	job->job_name = name;
	job->job_refcnt = 0;
	job->job_thread = thread;
	job->job_func = func;
}

void
threadpool_job_dead(struct threadpool_job *job)
{
	panic("threadpool job %p ran after destruction", job);
}

void
threadpool_job_destroy(struct threadpool_job *job)
{
	KASSERTMSG((job->job_thread == NULL), "job %p still running", job);
	job->job_lock = NULL;
	KASSERT(job->job_thread == NULL);
	KASSERT(job->job_refcnt == 0);
	job->job_func = threadpool_job_dead;
	(void) strlcpy(job->job_name, "deadjob", sizeof(job->job_name));
}

void
threadpool_job_hold(struct threadpool_job *job)
{
	unsigned int refcnt;

	do {
		refcnt = job->job_refcnt;
		KASSERT(refcnt != UINT_MAX);
	} while (atomic_cas_uint(&job->job_refcnt, refcnt, (refcnt + 1)) != refcnt);
}

void
threadpool_job_rele(struct threadpool_job *job)
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
threadpool_job_done(struct threadpool_job *job)
{
	KASSERT(0 < job->job_refcnt);
	unsigned int refcnt __diagused = atomic_dec_int_nv(&job->job_refcnt);
	KASSERT(refcnt != UINT_MAX);
	job->job_thread = NULL;
}

/* kthread */
/* Add kthread to itpc queue */
void
itpc_add_kthreadpool(itpc, ktpool)
    struct threadpool_itpc *itpc;
    struct kthreadpool *ktpool;
{
    struct kthread *kt = NULL;

    if(ktpool != NULL) {
        itpc->itpc_ktpool = ktpool;
        kt = ktpool->ktp_overseer.ktpt_kthread;
        if(kt != NULL) {
        	itpc->itpc_ktinfo->itpc_kthread = kt;
        	itpc->itpc_ktinfo->itpc_ktid = kt->kt_tid;
        	itpc->itpc_ktinfo->itpc_ktgrp = kt->kt_pgrp;
        	itpc->itpc_ktinfo->itpc_ktjob =  ktpool->ktp_overseer.ktpt_job;
            itpc->itpc_refcnt++;
            ktpool->ktp_initcq = TRUE;
            TAILQ_INSERT_HEAD(itpc->itpc_header, itpc, itpc_entry);
        } else {
        	printf("no kthread found in kthreadpool");
        }
    } else {
    	printf("no kthreadpool found");
    }
}

/* Remove kthread from itpc queue */
void
itpc_remove_kthreadpool(itpc, ktpool)
	struct threadpool_itpc *itpc;
	struct kthreadpool *ktpool;
{
	register struct kthread *kt;

	kt = itpc->itpc_ktinfo->itpc_kthread;
	if (ktpool != NULL) {
		TAILQ_FOREACH(itpc, itpc->itpc_header, itpc_entry) {
			if (TAILQ_NEXT(itpc, itpc_entry)->itpc_ktpool == ktpool &&  ktpool->ktp_overseer.ktpt_kthread == kt) {
				if(kt != NULL) {
		        	itpc->itpc_ktinfo->itpc_kthread = NULL;
		        	itpc->itpc_ktinfo->itpc_ktjob = NULL;
					itpc->itpc_refcnt--;
					ktpool->ktp_initcq = FALSE;
					TAILQ_REMOVE(itpc->itpc_header, itpc, itpc_entry);
				} else {
					printf("cannot remove kthread. does not exist");
				}
			}
		}
	} else {
		printf("no kthread to remove");
	}
}

/* Sender checks request from receiver: providing info */
void
itpc_check_kthreadpool(itpc, ktpool)
	struct threadpool_itpc *itpc;
	struct kthreadpool *ktpool;
{
	register struct kthread *kt;

	kt = ktpool->ktp_overseer.ktpt_kthread;
	if(ktpool->ktp_issender) {
		printf("kernel threadpool to send");
		if(itpc->itpc_ktpool == ktpool) {
			if(itpc->itpc_ktinfo->itpc_kthread == kt) {
				if(itpc->itpc_ktinfo->itpc_ktid == kt->kt_tid) {
					printf("kthread tid found");
				} else {
					printf("kthread tid not found");
					goto retry;
				}
			} else {
				printf("kthread not found");
				goto retry;
			}
		}
		panic("no kthreadpool");
	}

retry:
	if(ktpool->ktp_retcnt <= 5) { /* retry up to 5 times */
		if(ktpool->ktp_initcq) {
			/* exit and re-enter queue increasing retry count */
			itpc_remove_kthreadpool(itpc, ktpool);
			ktpool->ktp_retcnt++;
			itpc_add_kthreadpool(itpc, ktpool);
		} else {
			/* exit queue, reset count to 0 and panic */
			itpc_remove_kthreadpool(itpc, ktpool);
			ktpool->ktp_retcnt = 0;
			panic("kthreadpool x exited itpc queue after 5 failed attempts");
		}
	}
}

/* Receiver verifies request to sender: providing info */
void
itpc_verify_kthreadpool(itpc, ktpool)
	struct threadpool_itpc *itpc;
	struct kthreadpool *ktpool;
{
	register struct kthread *kt;

	kt = ktpool->ktp_overseer.ktpt_kthread;
	if(ktpool->ktp_isreciever) {
		printf("kernel threadpool to recieve");
		if(itpc->itpc_ktpool == ktpool) {
			if(itpc->itpc_ktinfo->itpc_kthread == kt) {
				if(itpc->itpc_ktinfo->itpc_ktid == kt->kt_tid) {
					printf("kthread tid found");
				} else {
					printf("kthread tid not found");
				}
			} else {
				printf("kthread not found");
			}
		}
		panic("no kthreadpool");
	}
}

/* Threadpool's FIFO Queue (IPC) */
void
kthreadpool_itpc_send(itpc, ktpool, pid, cmd)
	struct threadpool_itpc *itpc;
    struct kthreadpool *ktpool;
    pid_t pid;
    int cmd;
{
     /* sync itpc to threadpool */
    itpc->itpc_ktpool = ktpool;
    itpc->itpc_jobs = ktpool->ktp_jobs;  /* add/ get current job */
	/* send flagged jobs */
	ktpool->ktp_issender = TRUE;
	ktpool->ktp_isreciever = FALSE;

	/* command / action */
	switch(cmd) {
	case ITPC_INIT:
		itpc_job_init(ktpool->ktp_itpc, ktpool->ktp_jobs);
		break;

	case ITPC_DEAD:
		itpc_job_dead(ktpool->ktp_itpc, ktpool->ktp_jobs);
		break;

	case ITPC_DESTROY:
		itpc_job_destroy(ktpool->ktp_itpc, ktpool->ktp_jobs);
		break;

	case ITPC_HOLD:
		itpc_job_hold(ktpool->ktp_itpc, ktpool->ktp_jobs);
		break;

	case ITPC_RELE:
		itpc_job_rele(ktpool->ktp_itpc, ktpool->ktp_jobs);
		break;

	case ITPC_SCHEDULE:
		kthreadpool_schedule_job(ktpool, ktpool->ktp_jobs);
		break;

	case ITPC_DONE:
		kthreadpool_job_done(ktpool->ktp_jobs);
		break;

	case ITPC_CANCEL:
		kthreadpool_cancel_job(ktpool, ktpool->ktp_jobs);
		break;

	case ITPC_CANCEL_ASYNC:
		kthreadpool_cancel_job_async(ktpool, ktpool->ktp_jobs);
		break;
	}

	/* update job pool */
	itpc_check_kthreadpool(itpc, pid);
}

void
kthreadpool_itpc_recieve(itpc, ktpool, pid, cmd)
	struct threadpool_itpc *itpc;
    struct kthreadpool *ktpool;
    pid_t pid;
    int cmd;
{
    /* sync itpc to threadpool */
	itpc->itpc_ktpool = ktpool;
	itpc->itpc_jobs = ktpool->ktp_jobs; /* add/ get current job */
	ktpool->ktp_issender = FALSE;
	ktpool->ktp_isreciever = TRUE;

	/* command / action */
	switch(cmd) {
	case ITPC_INIT:
		itpc_job_init(ktpool->ktp_itpc, ktpool->ktp_jobs);
		break;

	case ITPC_DEAD:
		itpc_job_dead(ktpool->ktp_itpc, ktpool->ktp_jobs);
		break;

	case ITPC_DESTROY:
		itpc_job_destroy(ktpool->ktp_itpc, ktpool->ktp_jobs);
		break;

	case ITPC_HOLD:
		itpc_job_hold(ktpool->ktp_itpc, ktpool->ktp_jobs);
		break;

	case ITPC_RELE:
		itpc_job_rele(ktpool->ktp_itpc, ktpool->ktp_jobs);
		break;

	case ITPC_SCHEDULE:
		kthreadpool_schedule_job(ktpool, ktpool->ktp_jobs);
		break;

	case ITPC_DONE:
		kthreadpool_job_done(ktpool->ktp_jobs);
		break;

	case ITPC_CANCEL:
		kthreadpool_cancel_job(ktpool, ktpool->ktp_jobs);
		break;

	case ITPC_CANCEL_ASYNC:
		kthreadpool_cancel_job_async(ktpool, ktpool->ktp_jobs);
		break;
	}

	/* update job pool */
	itpc_verify_kthreadpool(itpc, pid);
}
