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

#include <sys/queue.h>

/* Tasks: taken from job_pool */
struct task {
	LIST_ENTRY(task) 			tk_entry;
	void 						(*tk_func)(void *);
	void 						*tk_arg;

	int 						tk_state;

	lock_t 						tk_lock;
	rwlock_t					tk_rwlock;

	int 						tk_prio;
	char						*tk_wmesg;
	int 						tk_timo;
	int 						tk_flags;
};
LIST_HEAD(taskhead, task);

/* Workqueue */
struct wqueue {
	struct taskhead 			wq_running;
	struct taskhead 			wq_pending;
	struct proc					*wq_worker;
	int                 		wq_nthreads;
};

#define	POISON					0xaabbccdd

/* task states  */
#define TQ_PENDING 				0x01	/* task is waiting for time to run */
#define TQ_RUNNING 				0x02	/* task is running */
#define TQ_IDLE 				0x03	/* task not finished but set to idle */

void		task_set(struct task *, int, char *, int, int, int);
void		task_add(struct wqueue *, struct task *);
void		task_remove(struct wqueue *, struct task *);
struct task *task_lookup(struct wqueue *, struct task *, int);

#endif /* SYS_WORKQUEUE_H_ */
