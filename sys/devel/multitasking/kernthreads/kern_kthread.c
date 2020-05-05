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
#include <sys/malloc.h>
#include <mutex.h>
#include <kthread.h>

extern struct kthread kthread0;
struct kthread *curkthread = &kthread0;

mutex_t kthread_mtx;

void
startkthread(kt)
	register struct kthread *kt;
{
	/* initialize current kthread(0) */
    kt = &kthread0;
    curkthread = kt;

    /* Set thread to idle & waiting */
    kt->kt_stat |= TSIDL | TSWAIT | TSREADY;

    /* setup kthread mutex manager */
    kthread_mutex_init(kthread_mtx, kt);

    /* Initialize Thread Table  */
    threadinit();
}

int
kthread_create(p)
	struct proc *p;
{
	register struct kthread *kt;
	int error;

	kt = &kthread0;
	curkthread = kt;

	if(!proc0.p_stat) {
		panic("kthread_create called too soon");
	}
	if(kt == NULL) {
		startkthread(kt);

	}
}

int
kthread_join(kthread_t kt)
{

}

int
kthread_cancel(kthread_t kt)
{

}

int
kthread_exit(kthread_t kt)
{

}

int
kthread_detach(kthread_t kt)
{

}

int
kthread_equal(kthread_t kt1, kthread_t kt2)
{
	if(kt1 > kt2) {
		return (1);
	} else if(kt1 < kt2) {
		return (-1);
	}
	return (0);
}

int
kthread_kill(kthread_t kt)
{

}

/* Initialize a Mutex on a kthread
 * Setup up Error flags */
int
kthread_mutex_init(mtx, kt)
    mutex_t mtx;
    kthread_t kt;
{
    int error = 0;
    mutex_init(mtx, mtx->mtx_prio, mtx->mtx_wmesg, mtx->mtx_timo, mtx->mtx_flags);
    kt->kt_mutex = mtx;
    mtx->mtx_ktlockholder = kt;
    return (error);
}

int
kthread_mutexmgr(mtx, flags, kt)
    mutex_t mtx;
    unsigned int flags;
    kthread_t kt;
{
    int error = 0;
    tid_t tid;
    if (kt) {
        tid = kt->kt_tid;
    } else {
        tid = MTX_THREAD;
    }
    return mutexmgr(mtx, flags, tid);
}
