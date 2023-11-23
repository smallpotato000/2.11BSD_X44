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

#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mutex.h>

#include <devel/sys/thread.h>

#include <vm/include/vm_param.h>

#define M_THREAD 101

extern struct thread  thread0;
struct thread *curthread = &thread0;

struct tidhashhead *tidhashtbl;
u_long tidhash;

struct lock_holder 	thread_loholder;

void
thread_init(p, td)
	register struct proc  	*p;
	register struct thread *td;
{
	/* initialize current thread & proc overseer from thread0 */
	p->p_threado = &thread0;
	td = p->p_threado;
	curthread = td;

	/* Initialize thread structures. */
	threadinit(p, td);

	/* set up kernel thread */
    LIST_INSERT_HEAD(&p->p_allthread, td, td_list);
    td->td_pgrp = &pgrp0;

	/* give the thread the same creds as the initial thread */
	td->td_ucred = p->p_ucred;
	crhold(td->td_ucred);
}

void
threadinit(p, td)
	struct proc *p;
	struct thread *td;
{
	tdqinit(p, td);
	tidhashtbl = hashinit(maxthread / 4, M_THREAD, &tidhash);

	/* setup thread mutex */
	mtx_init(td->td_mtx, &thread_loholder, "thread_mutex", (struct thread *)td, td->td_tid);
}

void
tdqinit(p, td)
	register struct proc *p;
	register struct thread *td;
{
	LIST_INIT(&p->p_allthread);

	LIST_INSERT_HEAD(&p->p_allthread, td, td_list);
}

void
thread_add(p, td)
	struct proc *p;
	struct thread *td;
{
    if (td->td_procp == p) {
        LIST_INSERT_HEAD(&p->p_allthread, td, td_sibling);
    }
    LIST_INSERT_HEAD(TIDHASH(td->td_tid), td , td_hash);
    LIST_INSERT_HEAD(&p->p_allthread, td, td_list);
    p->p_nthreads++;
}

void
thread_remove(p, td)
	struct proc *p;
	struct thread *td;
{
	if (td->td_procp == p) {
		LIST_REMOVE(td, td_sibling);
	}
	LIST_REMOVE(td, td_hash);
	LIST_REMOVE(td, td_list);
	p->p_nthreads--;
}

pid_t
tidmask(p)
	register struct proc *p;
{
	pid_t tid = p->p_pid + p->p_nthreads + TID_MIN;
	if (tid >= TID_MAX) {
		printf("tidmask: tid max reached");
		tid = NO_TID;
	}
	return (tid);
}

struct thread *
tdfind(p, tid)
	register struct proc *p;
	register pid_t tid;
{
	register struct thread *td;

	tid = tidmask(p);
	LIST_FOREACH(td, TIDHASH(tid), td_hash) {
		if (td->td_tid == tid) {
			return (td);
		}
	}
	return (NULL);
}

struct thread *
thread_alloc(p, stack)
	struct proc *p;
	size_t stack;
{
    struct thread *td;

    td = (struct thread *)malloc(sizeof(struct thread), M_THREAD, M_NOWAIT);
    td->td_procp = p;
    td->td_stack = stack;
    td->td_stat = SIDL;
    td->td_flag = 0;
    td->td_tid = tidmask(p);
    td->td_ptid = p->p_pid;
    td->td_pgrp = p->p_pgrp;

    if (!LIST_EMPTY(&p->p_allthread)) {
        p->p_threado = LIST_FIRST(&p->p_allthread);
    } else {
        p->p_threado = td;
    }
    thread_add(p, td);
    return (td);
}

void
thread_free(struct proc *p, struct thread *td)
{
	if (td != NULL) {
		thread_remove(p, td);
		free(td, M_THREAD);
	}
}
