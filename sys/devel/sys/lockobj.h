/*	$OpenBSD: _lock.h,v 1.4 2019/04/23 13:35:12 visa Exp $	*/

/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Berkeley Software Design Inc's name may not be used to endorse or
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BERKELEY SOFTWARE DESIGN INC ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL BERKELEY SOFTWARE DESIGN INC BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: head/sys/sys/_lock.h 179025 2008-05-15 20:10:06Z attilio $
 */

#ifndef SYS_LOCKOBJ_H_
#define SYS_LOCKOBJ_H_

#define	LC_SLEEPLOCK	0x00000001		/* Sleep lock. */
#define	LC_SPINLOCK		0x00000002		/* Spin lock. */
#define	LC_SLEEPABLE	0x00000004		/* Sleeping allowed with this lock. */
#define	LC_RECURSABLE	0x00000008		/* Locks of this type may recurse. */
#define	LC_UPGRADABLE	0x00000010		/* Upgrades and downgrades permitted. */

#define	LO_CLASSFLAGS	0x0000ffff		/* Class specific flags. */
#define	LO_INITIALIZED	0x00010000		/* Lock has been initialized. */
#define	LO_WITNESS		0x00020000		/* Should witness monitor this lock. */
#define	LO_QUIET		0x00040000		/* Don't log locking operations. */
#define	LO_RECURSABLE	0x00080000		/* Lock may recurse. */
#define	LO_SLEEPABLE	0x00100000		/* Lock may be held while sleeping. */
#define	LO_UPGRADABLE	0x00200000		/* Lock may be upgraded/downgraded. */
#define	LO_DUPOK		0x00400000		/* Don't check for duplicate acquires */
#define	LO_IS_VNODE		0x00800000		/* Tell WITNESS about a VNODE lock */
#define	LO_CLASSMASK	0x0f000000		/* Class index bitmask. */
#define	LO_NOPROFILE	0x10000000		/* Don't profile this lock */
#define	LO_NEW			0x20000000		/* Don't check for double-init */

#define	LO_CLASSSHIFT	24

enum lock_class_index {
	LO_CLASS_ABQL,
	LO_CLASS_RWLOCK,
	LO_CLASS_LOCK
};

/*
 * Lock classes.  Each lock has a class which describes characteristics
 * common to all types of locks of a given class.
 *
 * Spin locks in general must always protect against preemption, as it is
 * an error to perform any type of context switch while holding a spin lock.
 * Also, for an individual lock to be recursable, its class must allow
 * recursion and the lock itself must explicitly allow recursion.
 */

struct lock_class {
    const	char 	 			*lc_name;
	u_int			 			lc_flags;
    void		     			(*lc_lock)(struct lock_object *, const char *, u_int);
    int		         			(*lc_unlock)(struct lock_object *, const char *, u_int);
};

/* Should replace simplelock: as common to all locks */
struct lock_object {
	const struct lock_type		*lo_type;
	const char 					*lo_name;		/* Individual lock name. */
	u_int						lo_flags;
	u_int						lo_data;		/* General class specific data. */
	struct witness 				*lo_witness;	/* Data for witness. */
};

struct lock_type {
	const char					*lt_name;
};

struct lock_holder {
	struct proc 				*lh_proc;
	struct kthread 				*lh_kthread;
	struct uthread 				*lh_uthread;
	pid_t						lh_pid;
};

#define PROC_LOCKHOLDER(h)		((h)->lh_proc)
#define KTHREAD_LOCKHOLDER(h)	((h)->lh_kthread)
#define UTHREAD_LOCKHOLDER(h)	((h)->lh_uthread)

static int						_isitmyx(struct witness *w1, struct witness *w2, int rmask, const char *fname);
static void						adopt(struct witness *parent, struct witness *child);
static struct witness			*enroll(const struct lock_type *, const char *, struct lock_class *);
static struct lock_instance		*find_instance(struct lock_list_entry *list, const struct lock_object *lock);
static int						isitmychild(struct witness *parent, struct witness *child);
static int						isitmydescendant(struct witness *parent, struct witness *child);
static void						itismychild(struct witness *parent, struct witness *child);

static int						witness_alloc_stacks(void);
static void						witness_debugger(int dump);
static void						witness_free(struct witness *m);
static struct witness			*witness_get(void);
static uint32_t					witness_hash_djb2(const uint8_t *key, uint32_t size);
static struct witness			*witness_hash_get(const struct lock_type *, const char *);
static void						witness_hash_put(struct witness *w);
static void						witness_init_hash_tables(void);
static void						witness_increment_graph_generation(void);
static int						witness_list_locks(struct lock_list_entry **, int (*)(const char *, ...));
static void						witness_lock_list_free(struct lock_list_entry *lle);
static struct lock_list_entry	*witness_lock_list_get(void);
static void						witness_lock_stack_free(union lock_stack *stack);
static union lock_stack			*witness_lock_stack_get(void);
static int						witness_lock_order_add(struct witness *parent, struct witness *child);
static int						witness_lock_order_check(struct witness *parent, struct witness *child);
static struct witness_lock_order_data	*witness_lock_order_get(struct witness *parent, struct witness *child);
static void						witness_list_lock(struct lock_instance *instance, int (*prnt)(const char *fmt, ...));
static void						witness_setflag(struct lock_object *lock, int flag, int set);

#endif /* SYS_LOCKOBJ_H_ */
