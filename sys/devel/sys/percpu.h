/*	$NetBSD: percpu.h,v 1.5 2020/02/07 11:55:22 thorpej Exp $	*/

/*-
 * Copyright (c)2007,2008 YAMAMOTO Takashi,
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

#ifndef _SYS_PERCPU_H_
#define	_SYS_PERCPU_H_

#include <sys/types.h>

struct cpu_info;
typedef struct percpu percpu_t;

typedef struct percpu_cpu {
	size_t 		pcc_size;
	void 		*pcc_data;
} percpu_cpu_t;

void		percpu_init(void);
void		percpu_init_cpu(struct cpu_info *);
percpu_t 	*percpu_alloc(size_t);
void		percpu_free(percpu_t *, size_t);
void		*percpu_getref(percpu_t *);
void		percpu_putref(percpu_t *);

typedef void (*percpu_callback_t)(void *, void *, struct cpu_info *);
void		percpu_foreach(percpu_t *, percpu_callback_t, void *);
void		percpu_foreach_xcall(percpu_t *, u_int, percpu_callback_t, void *);

percpu_t *percpu_create(size_t, percpu_callback_t, percpu_callback_t, void *);

/* low-level api; don't use unless necessary */
void		percpu_traverse_enter(void);
void		percpu_traverse_exit(void);
void		*percpu_getptr_remote(percpu_t *, struct cpu_info *);

#endif /* _SYS_PERCPU_H_ */
