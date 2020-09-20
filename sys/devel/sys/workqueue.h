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

#ifndef SYS_WORKQUEUE_H_
#define SYS_WORKQUEUE_H_
/* job_head:
 * - kthreadpool
 *
 * threadpool_job:
 * - kthreadpool_thread
 */
#include <sys/queue.h>

#define	POISON					0xaabbccdd

/* Tasks */
typedef void task_fn_t(void *);
struct task {
	TAILQ_ENTRY(task) 			tk_entry;
	task_fn_t 					*tk_func;
	void 						*tk_arg;

	int 						tk_state;
	int 						tk_flags;

	lock_t 						tk_lock;
	rwlock_t					tk_rwlock;
};
TAILQ_HEAD(taskhead, task);

/* Workqueue */
struct wqueue {
	struct taskhead 			wq_head;
	struct proc					*wq_worker;	/* kthread?? */
	const char 					*wq_name;
	int                 		wq_nthreads;
	int 						wq_state;
	int 						wq_flags;
};

/* workqueue states */
#define WQ_CREATED				0x01
#define WQ_DESTROYED			0x02

void			task_set(struct task *, void *, void *);
void			task_add(struct wqueue *, struct task *);
void			task_remove(struct wqueue *, struct task *);
struct task 	*task_lookup(struct wqueue *, struct task *);
bool			task_check(struct wqueue *, struct task *);

void			wqueue_alloc(struct wqueue *);
void			wqueue_free(struct wqueue *);
struct wqueue 	*wqueue_create(struct wqueue *, const char *, int, int);
void			wqueue_destroy(struct wqueue *, const char *);

#endif /* SYS_WORKQUEUE_H_ */
