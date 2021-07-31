/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)cpu.h	8.5 (Berkeley) 5/17/95
 */

#ifndef _I386_CPU_H_
#define _I386_CPU_H_

/*
 * Definitions unique to i386 cpu support.
 */
#include <machine/param.h>
#include <machine/psl.h>
#include <machine/frame.h>
#include <machine/segments.h>

struct pmap;
/*
 * definitions of cpu-dependent requirements
 * referenced in generic code
 */
#undef	COPY_SIGCODE				/* don't copy sigcode above user stack in exec */

#define	cpu_exec(p)					/* nothing */
#define	cpu_swapin(p)				/* nothing */
#define cpu_setstack(p, ap)			(p)->p_md.md_regs[SP] = ap
#define cpu_set_init_frame(p, fp)	(p)->p_md.md_regs = fp

#define cpu_number					NCPUS	/* number of cpus available */

/*
 * Arguments to hardclock, softclock and gatherstats
 * encapsulate the previous machine state in an opaque
 * clockframe; for now, use generic intrframe.
 */
struct clockframe {
	struct intrframe				cf_if;
};

#define	CLKF_USERMODE(framep)		(ISPL((framep)->cf_if.if_cs) == SEL_UPL)
#define	CLKF_BASEPRI(framep)		((framep)->cf_if.if_ppl == 0)
#define	CLKF_PC(framep)				((framep)->cf_if.if_eip)

/*
 * Preempt the current process if in interrupt from user mode,
 * or after the current trap/syscall if in system mode.
 */
extern void	need_resched(struct proc *p);

/*
 * Give a profiling tick to the current process from the softclock
 * interrupt.  On tahoe, request an ast to send us through trap(),
 * marking the proc as needing a profiling tick.
 */
extern void	cpu_need_proftick(struct proc *p);

/*
 * Notify the current process (p) that it has a signal pending,
 * process as soon as possible.
 */
#define cpu_signotify(p)			(aston(p))

#define aston(p) 					((p)->p_md.md_astpending = 1)

#define	want_resched(p)				((p)->p_md.md_want_resched = 1) /* resched() was called */

#ifdef _KERNEL
extern char		btext[];
extern char		etext[];

/* machdep.c */
void	cpu_halt(void);

/* locore.s */
struct 	pcb;
void	savectx (struct pcb *);

/* clock.c */
extern u_int tsc_freq;

void	startrtclock(void);
int		clockintr(void *);
int		gettick();
void	delay(int);
void	sysbeepstop(void);
int		sysbeep(int, int);
void	findcpuspeed(void);
void	cpu_initclocks(void);
void	rtcinit(void);
int		rtcget(mc_todregs *);
void	rtcput(mc_todregs *);
int 	yeartoday (int);
int		bcdtobin(int);
int		bintobcd(int);
void 	inittodr(time_t);
void	resettodr(void);
void	setstatclockrate(int);

/* autoconf.c */
void	configure (void);

/* vm_machdep.c */
int		cpu_fork (struct proc *, struct proc *);

#ifdef USER_LDT
/* sys_machdep.h */
int		i386_get_ldt (struct proc *, char *, register_t *);
int		i386_set_ldt (struct proc *, char *, register_t *);
#endif

/* isa_machdep.c */
void	isa_defaultirq (void);
int		isa_nmi (void);

/* bus_machdep.c */
void 	i386_bus_space_init	(void);
void 	i386_bus_space_mallocok	(void);
void	i386_bus_space_check (vm_offset_t, int, int);

#endif /* _KERNEL */

/*
 * CTL_MACHDEP definitions.
 */
#define	CPU_CONSDEV		1	/* dev_t: console terminal device */
#define	CPU_BIOSBASEMEM	2	/* int: bios-reported base mem (K) */
#define	CPU_BIOSEXTMEM	3	/* int: bios-reported ext. mem (K) */

#define	CPU_MAXID		4	/* number of valid machdep ids */

#define CTL_MACHDEP_NAMES { 				\
	{ 0, 0 }, 								\
	{ "console_device", CTLTYPE_STRUCT }, 	\
	{ "biosbasemem", CTLTYPE_INT }, 		\
	{ "biosextmem", CTLTYPE_INT }, 			\
}
#endif /* !_I386_CPU_H_ */
