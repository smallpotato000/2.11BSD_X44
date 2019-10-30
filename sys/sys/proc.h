/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)proc.h	1.5 (2.11BSD) 1999/9/5
 *
 *	Parts of proc.h are borrowed from FreeBSD 2.0 proc.h.
 */

#ifndef	_SYS_PROC_H_
#define	_SYS_PROC_H_

#include <machine/proc.h>		/* Machine-dependent proc substruct. */
#include <sys/rtprio.h>			/* For struct rtprio. */
#include <sys/select.h>			/* For struct selinfo. */
#include <sys/time.h>			/* For structs itimerval, timeval. */

/*
 * One structure allocated per active
 * process. It contains all data needed
 * about the process while the
 * process may be swapped out.
 * Other per process data (user.h)
 * is swapped with the process.
 */
struct	proc {
    struct	proc 		*p_nxt;			/* linked list of allocated proc slots */
	struct	proc 		**p_prev;		/* also zombies, and free proc's */

    struct	proc 		*p_forw;		/* Doubly-linked run/sleep queue. */
	struct	proc 		*p_back;

    int	    p_flag;						/* P_* flags. */
	char	p_stat;						/* S* process status. */
	char	p_lock;						/* Process lock count. */
	char	p_pad1[2];
	char	p_dummy;					/* room for one more, here */

    short	p_uid;						/* user id, used to direct tty signals */
	short	p_pid;						/* unique process id */
	short	p_ppid;						/* process id of parent */

    /* Substructures: */
	struct	pcred 	 	*p_cred;		/* Process owner's identity. */
	struct	filedesc 	*p_fd;			/* Ptr to open files structure. */
	struct	pstats 	 	*p_stats;		/* Accounting/statistics (PROC ONLY). */
	struct	plimit 	 	*p_limit;		/* Process limits. */
	struct	vmspace  	*p_vmspace;		/* Address space. */
	struct 	sigacts 	*p_sig;			/* Signal actions, state (PROC ONLY). */

#define	p_ucred		p_cred->pc_ucred
#define	p_rlimit	p_limit->pl_rlimit

	struct	proc    	*p_hash;        /* hashed based on p_pid for kill+exit+... */
    struct	proc    	*p_pgrpnxt;	    /* Pointer to next process in process group. */
    struct	proc        *p_pptr;		/* pointer to process structure of parent */
    struct	proc 		*p_osptr;	 	/* Pointer to older sibling processes. */


/* The following fields are all zeroed upon creation in fork. */
#define	p_startzero	p_ysptr
	struct	proc 		*p_ysptr;	 	/* Pointer to younger siblings. */
	struct	proc 		*p_cptr;	 	/* Pointer to youngest living child. */
    pid_t	p_oppid;	                /* Save parent pid during ptrace. XXX */

    caddr_t	            p_wchan;		/* event process is awaiting */
	caddr_t	            p_wmesg;	 	/* Reason for sleep. */
    char	            p_slptime;		/* Time since last blocked. */

    struct	itimerval   p_realtimer;	/* Alarm timer. */
    struct	timeval     p_rtime;	    /* Real time. */
    char	p_ptracesig;			    /* used between parent & traced child */
    int	    p_traceflag;		        /* Kernel trace points. */
    struct	vnode 	    *p_tracep;		/* Trace to vnode. */
    struct	vnode 	    *p_textvp;		/* Vnode of executable. */

    caddr_t	p_wchan;			        /* event process is awaiting */
    caddr_t	p_wmesg;	 		        /* Reason for sleep. */

/* End area that is zeroed on creation. */
#define	p_endzero	    p_startcopy

/* The following fields are all copied upon creation in fork. */
#define	p_startcopy	    p_sigmask

    sigset_t p_sigmask;			        /* Current signal mask. */
    sigset_t p_sigignore;			    /* signals being ignored */
    sigset_t p_sigcatch;			    /* signals being caught by user */

    u_char	p_pri;					    /* Process  priority, negative is high */
    u_char	p_cpu;					    /* cpu usage for scheduling */
    char	p_nice;					    /* nice for cpu usage */
    char	p_comm[MAXCOMLEN+1];

    struct  pgrp 	    *p_pgrp;        /* Pointer to process group. */
    struct  sysentvec   *p_sysent; 	    /* System call dispatch information. */
    struct	rtprio 	    p_rtprio;		/* Realtime priority. */

    struct	proc 	    *p_link;		/* linked list of running processes */
    struct	user 		*p_addr;        /* address of u. area */
    struct	user  		*p_daddr;		/* address of data area */
    struct	user  		*p_saddr;		/* address of stack area */
	size_t	p_dsize;				    /* size of data area (clicks) */
	size_t	p_ssize;				    /* size of stack segment (clicks) */
    struct	k_itimerval p_krealtimer;   /* Alarm Timer?? in 2.11BSD */
    u_short p_acflag;	                /* Accounting flags. */

    short	p_xstat;				    /* exit status for wait */
	struct  k_rusage    p_ru;			/* exit information */
};

struct	session {
	int		s_count;					/* Ref cnt; pgrps in session. */
	struct	proc *s_leader;				/* Session leader. */
	struct	vnode *s_ttyvp;				/* inode of controlling terminal. */
	struct	tty *s_ttyp;				/* Controlling terminal. */
	char	s_login[MAXLOGNAME];		/* Setlogin() name. */
};

struct	pgrp {
	struct	pgrp *pg_hforw;				/* Forward link in hash bucket. */
	struct	proc *pg_mem;				/* Pointer to pgrp members. */
	struct	session *pg_session;		/* Pointer to session. */
	pid_t	pg_id;						/* Pgrp id. */
	int		pg_jobc;					/* # procs qualifying pgrp for job control */
};

struct pcred {
	struct	ucred *pc_ucred;			/* Current credentials. */
	uid_t	p_ruid;						/* Real user id. */
	uid_t	p_svuid;					/* Saved effective user id. */
	gid_t	p_rgid;						/* Real group id. */
	gid_t	p_svgid;					/* Saved effective group id. */
	int		p_refcnt;					/* Number of references. */
};

#define	p_session	p_pgrp->pg_session
#define	p_pgid		p_pgrp->pg_id

/* stat codes */
#define	SSLEEP	1		/* awaiting an event */
#define	SWAIT	2		/* (abandoned state) */
#define	SRUN	3		/* running */
#define	SIDL	4		/* intermediate state in process creation */
#define	SZOMB	5		/* intermediate state in process termination */
#define	SSTOP	6		/* process being traced */

/* flag codes */
#define	P_CONTROLT	0x00002	/* Has a controlling terminal. */

#define	SLOAD		0x0001	/* in core */
#define	SSYS		0x0002	/* swapper or pager process */
#define	SLOCK		0x0004	/* process being swapped out */
#define	SSWAP		0x0008	/* save area flag */
#define	P_TRACED	0x0010	/* process is being traced */
#define	P_WAITED	0x0020	/* another tracing flag */
#define	SULOCK		0x0040	/* user settable lock in core */
#define	P_SINTR		0x0080	/* sleeping interruptibly */
#define	SVFORK		0x0100	/* process resulted from vfork() */
#define	SVFPRNT		0x0200	/* parent in vfork, waiting for child */
#define	SVFDONE		0x0400	/* parent has released child in vfork */
			/*		0x0800	/* unused */
#define	P_TIMEOUT	0x1000	/* tsleep timeout expired */
#define	P_NOCLDSTOP	0x2000	/* no SIGCHLD signal to parent */
#define	P_SELECT	0x4000	/* selecting; wakeup/waiting danger */
			/*		0x8000	/* unused */


#define	P_PPWAIT	0x00010	/* Parent is waiting for child to exec/exit. */
#define	P_SUGID		0x00100	/* Had set id privileges since last exec. */
#define	P_TRACED	0x00800	/* Debugged process being traced. */
#define P_EXEC		0x04000	/* Process called exec. */

#define	S_DATA	0			/* specified segment */
#define	S_STACK	1

#ifdef KERNEL
#define	PID_MAX		30000
#define	NO_PID		30001

#define	PIDHSZ		16
#define	PIDHASH(pid)	((pid) & (PIDHSZ - 1))

#define SESS_LEADER(p)	((p)->p_session->s_leader == (p))


extern struct proc *pidhash[];			/* In param.c. */
extern struct pgrp *pgrphash[];			/* In param.c. */
extern int pidhashmask;					/* In param.c. */

extern struct proc *pfind();
extern struct proc proc[], *procNPROC;	/* the proc table itself */
extern struct proc *freeproc;

extern struct proc *zombproc;			/* List of zombie procs. */
extern volatile struct proc *allproc;	/* List of active procs. */
extern struct proc proc0;				/* Process slot for swapper. */
int	nproc, maxproc;						/* Current and max number of procs. */

#define	NQS	32							/* 32 run queues. */
extern struct prochd qs[];				/* queue schedule */
extern struct prochd rtqs[];
extern struct prochd idqs[];
extern int	whichqs;					/* Bit mask summary of non-empty Q's. */
struct	prochd {
	struct	proc *ph_link;				/* Linked list of running processes. */
	struct	proc *ph_rlink;
};

int		chgproccnt __P((uid_t, int));
struct proc *pfind __P((pid_t));		/* Find process by id. */
struct pgrp *pgfind __P((pid_t));		/* Find process group by id. */

int		setpri __P((struct proc *));
void	setrun __P((struct proc *));
void	setrq __P((struct proc *));
void	remrq __P((struct proc *));
void	swtch __P();
void	sleep __P((void *chan, int pri));
int		tsleep __P((void *chan, int pri, char *wmesg, int timo));
void	unsleep __P((struct proc *));
void	wakeup __P((void *chan));

int	inferior __P((struct proc *));
#endif

#endif	/* !_SYS_PROC_H_ */
