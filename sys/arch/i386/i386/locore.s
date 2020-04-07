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
 *	from: @(#)locore.s	7.3 (Berkeley) 5/13/91
 *	from NetBSD: Id: locore.s,v 1.12 1993/05/27 16:44:13 cgd Exp
 *
 *      @(#)locore.s	8.3 (Berkeley) 9/23/93
 */


/*
 * locore.s:	4BSD machine support for the Intel 386
 *		Preliminary version
 *		Written by William F. Jolitz, 386BSD Project
 */

#include "assym.h"

#include <sys/errno.h>
#include <machine/asm.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/trap.h>
#include <machine/specialreg.h>

#ifdef cgd_notdef
#include <machine/cputypes.h>
#endif

/**********************************************************************/
/* Some Handy Macros */

#define	KDSEL		0x10

#define	NOP	inb $0x84, %al ; inb $0x84, %al

#define	fillkpt														\
	1:	movl	%eax,0(%ebx)	; 									\
		addl	$ NBPG,%eax		; /* increment physical address */ 	\
		addl	$4,%ebx			; /* next pte */ 					\
		loop	1b				;

#define	LCALL(x,y)		.byte 0x9a ; .long y; .word x

#define	IDTVEC(name)	.align 4; .globl _X/**/name; _X/**/name:

#define	PANIC(msg)				  \
		xorl 	%eax,%eax		; \
		movl 	%eax,_waittime	; \
		pushl 	1f				; \
		call 	_panic; 1: .asciz 	msg

#define	PRINTF(n,msg)			  \
		pushal 					; \
		nop 					; \
		pushl 	1f				; \
		call 	_printf			; \
		MSG(msg) 				; \
		popl %eax 				; \
		popal

#define	MSG(msg)		.data; 1: .asciz msg; .text

#define	TRAP(a)			pushl $(a) ; jmp alltraps
#ifdef KGDB
#define	BPTTRAP(a)		pushl $(a) ; jmp bpttraps
#else
#define	BPTTRAP(a)		TRAP(a)
#endif

/**********************************************************************/

/*
 * Note: This version greatly munged to avoid various assembler errors
 * that may be fixed in newer versions of gas. Perhaps newer versions
 * will have more pleasant appearance.
 */

		.set	IDXSHIFT,10
		.set	SYSTEM,0xFE000000						# virtual address of system start
/*note: gas copys sign bit (e.g. arithmetic >>), can't do SYSTEM>>22! */
		.set	SYSPDROFF,0x3F8							# Page dir index of System Base

/*
 * PTmap is recursive pagemap at top of virtual address space.
 * Within PTmap, the page directory can be found (third indirection).
 */
		.set	PDRPDROFF,0x3F7							# Page dir index of Page dir
		.globl	_PTmap, _PTD, _PTDpde, _Sysmap
		.set	_PTmap,0xFDC00000
		.set	_PTD,0xFDFF7000
		.set	_Sysmap,0xFDFF8000
		.set	_PTDpde,0xFDFF7000+4*PDRPDROFF

/*
 * APTmap, APTD is the alternate recursive pagemap.
 * It's used when modifying another process's page tables.
 */
		.set	APDRPDROFF,0x3FE						# Page dir index of Page dir
		.globl	_APTmap, _APTD, _APTDpde
		.set	_APTmap,0xFF800000
		.set	_APTD,0xFFBFE000
		.set	_APTDpde,0xFDFF7000+4*APDRPDROFF

/*
 * Access to each processes kernel stack is via a region of
 * per-process address space (at the beginning), immediatly above
 * the user process stack.
 */
		.set	_kstack, USRSTACK
		.globl	_kstack
		.set	PPDROFF,0x3F6
		.set	PPTEOFF,0x400-UPAGES					# 0x3FE

/*
 * Note: This version greatly munged to avoid various assembler errors
 * that may be fixed in newer versions of gas. Perhaps newer versions
 * will have more pleasant appearance.
 */

		.set	IDXSHIFT,10
		.set	SYSTEM,0xFE000000						# virtual address of system start
/*note: gas copys sign bit (e.g. arithmetic >>), can't do SYSTEM>>22! */
		.set	SYSPDROFF,0x3F8							# Page dir index of System Base


/*
 * Compiled KERNBASE location
 */
		.globl	kernbase
		.set	kernbase,KERNBASE

/*
 * Globals
 */
 		.data
		.globl	_cpu,_cold,_boothowto,_bootdev,_cyloffset,_atdevbase,_atdevphys
_cpu:			.long	0								# are we 386, 386sx, or 486
_cold:			.long	1								# cold till we are not
_atdevbase:		.long	0								# location of start of iomem in virtual
_atdevphys:		.long	0								# location of device mapping ptes (phys)

		.globl	_IdlePTD, _KPTphys
_IdlePTD:		.long	0
_KPTphys:		.long	0

		.globl	_curpcb
_curpcb:		.long	0
		.globl	_proc0paddr
_proc0paddr: 	.long	0								/* address of proc 0 address space */

/**********************************************************************/
/* Initialization */

		.globl	tmpstk
		.space	0x2000									/* space for tmpstk - temporary stack */
tmpstk:
		.text
		.globl	start
start:	movw	$0x1234,0x472							# warm boot
		jmp		1f
		.space	0x500									# skip over warm boot shit

		/*
		 * pass parameters on stack (howto, bootdev, unit, cyloffset)
		 * note: 0(%esp) is return address of boot
		 * ( if we want to hold onto /boot, it's physical %esp up to _end)
		 */

 1:		movl	4(%esp),%eax
		movl	%eax,_boothowto-SYSTEM
		movl	8(%esp),%eax
		movl	%eax,_bootdev-SYSTEM
		movl	12(%esp),%eax
		movl	%eax, _cyloffset-SYSTEM

/* Set up a real frame in case the double return in newboot is executed. */
		//xorl	%ebp,%ebp
		//pushl	%ebp
		//movl	%esp, %ebp

/* Don't trust what the BIOS gives for eflags. */
		//pushl	$PSL_MBO
		//popfl

/*
 * Don't trust what the BIOS gives for %fs and %gs.  Trust the bootstrap
 * to set %cs, %ds, %es and %ss.
 */
		//mov		%ds, %ax
		//mov		%ax, %fs
		//mov		%ax, %gs

/**********************************************************************/
/* Identify CPU's */

#ifdef cgd_notdef
		/* find out our CPU type. */
        pushfl
        popl    %eax
        movl    %eax, %ecx
        xorl    $0x40000, %eax
        pushl   %eax
        popfl
        pushfl
        popl    %eax
        xorl    %ecx, %eax
        shrl    $18, %eax
        andl    $1, %eax
        push    %ecx
        popfl

        cmpl    $0, %eax
        jne     1f
        movl    $CPU_386, _cpu-SYSTEM
		jmp		2f
1:      movl    $CPU_486, _cpu-SYSTEM
2:
#endif

/**********************************************************************/
/* Garbage: Clean Up */

#ifdef garbage
		/* count up memory */
		xorl	%eax,%eax							# start with base memory at 0x0
		#movl	$ 0xA0000/NBPG,%ecx	# look every 4K up to 640K
		movl	$ 0xA0,%ecx							# look every 4K up to 640K
1:		movl	0(%eax),%ebx						# save location to check
		movl	$0xa55a5aa5,0(%eax)					# write test pattern
/* flush stupid cache here! (with bcopy (0,0,512*1024) ) */
		cmpl	$0xa55a5aa5,0(%eax)					# does not check yet for rollover
		jne		2f
		movl	%ebx,0(%eax)						# restore memory
		addl	$ NBPG,%eax
		loop	1b
2:		shrl	$12,%eax
		movl	%eax,_Maxmem-SYSTEM

		movl	$0x100000,%eax						# next, talley remaining memory
		#movl	$((0xFFF000-0x100000)/NBPG),%ecx
		movl	$(0xFFF-0x100),%ecx
1:		movl	0(%eax),%ebx						# save location to check
		movl	$0xa55a5aa5,0(%eax)					# write test pattern
		cmpl	$0xa55a5aa5,0(%eax)					# does not check yet for rollover
		jne		2f
		movl	%ebx,0(%eax)						# restore memory
		addl	$ NBPG,%eax
		loop	1b
2:		shrl	$12,%eax
		movl	%eax,_Maxmem-SYSTEM
#endif

/**********************************************************************/
/* Create Page Tables */

/* find end of kernel image */
		movl	$_end-SYSTEM,%ecx
		addl	$ NBPG-1,%ecx
		andl	$~(NBPG-1),%ecx
		movl	%ecx,%esi

/* clear bss and memory for bootstrap pagetables. */
		movl	$_edata-SYSTEM,%edi
		subl	%edi,%ecx
		addl	$(UPAGES+5)*NBPG,%ecx
/*
 * Virtual address space of kernel:
 *
 *	text | data | bss | page dir | proc0 kernel stack | usr stk map | Sysmap
 *			     0               1       2       3             4
 */
		xorl	%eax,%eax							# pattern
		cld
		rep
		stosb

		movl	%esi,_IdlePTD-SYSTEM 				/*physical address of Idle Address space */
		movl	$ tmpstk-SYSTEM,%esp				# bootstrap stack end location

/*
 * Map Kernel
 * N.B. don't bother with making kernel text RO, as 386
 * ignores R/W AND U/S bits on kernel access (only v works) !
 *
 * First step - build page tables
 */
		movl	%esi,%ecx							# this much memory,
		shrl	$ PGSHIFT,%ecx						# for this many pte s
		addl	$ UPAGES+4,%ecx						# including our early context
		movl	$ PG_V,%eax							# having these bits set,
		lea		(4*NBPG)(%esi),%ebx					# physical address of KPT in proc 0,
		movl	%ebx,_KPTphys-SYSTEM				# in the kernel page table,
		fillkpt

/* map I/O memory map */

		movl	$0x100-0xa0,%ecx					# for this many pte s,
		movl	$(0xa0000|PG_V|PG_UW),%eax 			# having these bits set,(perhaps URW?) XXX 06 Aug 92
		movl	%ebx,_atdevphys-SYSTEM				# remember phys addr of ptes
		fillkpt

 /* map proc 0's kernel stack into user page table page */

		movl	$ UPAGES,%ecx						# for this many pte s,
		lea		(1*NBPG)(%esi),%eax					# physical address in proc 0
		lea		(SYSTEM)(%eax),%edx
		movl	%edx,_proc0paddr-SYSTEM  			# remember VA for 0th process init
		orl		$ PG_V|PG_URKW,%eax					# having these bits set,
		lea		(3*NBPG)(%esi),%ebx					# physical address of stack pt in proc 0
		addl	$(PPTEOFF*4),%ebx
		fillkpt

/*
 * Construct a page table directory
 * (of page directory elements - pde's)
 */

/* install a pde for temporary double map of bottom of VA */
		lea		(4*NBPG)(%esi),%eax					# physical address of kernel page table
		orl     $ PG_V|PG_UW,%eax					# pde entry is valid XXX 06 Aug 92
		movl	%eax,(%esi)							# which is where temp maps!

/* kernel pde's */
		movl	$ 3,%ecx							# for this many pde s,
		lea		(SYSPDROFF*4)(%esi), %ebx			# offset of pde for kernel
		fillkpt

/* install a pde recursively mapping page directory as a page table! */
		movl	%esi,%eax							# phys address of ptd in proc 0
		orl		$ PG_V|PG_UW,%eax					# pde entry is valid XXX 06 Aug 92
		movl	%eax, PDRPDROFF*4(%esi)				# which is where PTmap maps!

/* install a pde to map kernel stack for proc 0 */
		lea		(3*NBPG)(%esi),%eax					# physical address of pt in proc 0
		orl		$ PG_V,%eax							# pde entry is valid
		movl	%eax,PPDROFF*4(%esi)				# which is where kernel stack maps!

/* load base of page directory, and enable mapping */
		movl	%esi,%eax							# phys address of ptd in proc 0
 		orl		$ I386_CR3PAT,%eax
		movl	%eax,%cr3							# load ptd addr into mmu
		movl	%cr0,%eax							# get control word
		orl		$0x80000001,%eax					# and let s page!
		movl	%eax,%cr0							# NOW!

		pushl	$begin								# jump to high mem!
		ret

begin: /* now running relocated at SYSTEM where the system is linked to run */

		.globl 	_Crtat
		movl	_Crtat,%eax
		subl	$0xfe0a0000,%eax
		movl	_atdevphys,%edx						# get pte PA
		subl	_KPTphys,%edx						# remove base of ptes, now have phys offset
		shll	$ PGSHIFT-2,%edx  					# corresponding to virt offset
		addl	$ SYSTEM,%edx						# add virtual base
		movl	%edx, _atdevbase
		addl	%eax,%edx
		movl	%edx,_Crtat

		/* set up bootstrap stack */
		movl	_proc0paddr, %eax
		movl	$ _kstack+UPAGES*NBPG-4*12,%esp		# bootstrap stack end location
		xorl	%eax,%eax							# mark end of frames
		movl	%eax,%ebp
		movl	IdlePTD,%esi
		movl	%esi, PCB_CR3(%eax)

		lea		7*NBPG(%esi),%esi					# skip past stack.
		pushl	%esi
		call	init386								# wire 386 chip for unix operation

		movl	$0,_PTD
		call 	main								/* autoconfiguration, mountroot etc */
		popl	%esi

		.globl	__ucodesel,__udatasel
		movzwl	__ucodesel,%eax
		movzwl	__udatasel,%ecx

		# build outer stack frame

		pushl	%ecx								# user ss
		pushl	$ USRSTACK							# user esp
		pushl	%eax								# user cs
		pushl	$0									# user ip
		movw	%cx,%ds
		movw	%cx,%es
		movw	%ax,%fs								# double map cs to fs
		movw	%cx,%gs								# and ds to gs
		lret										# goto user!

		pushl	$lretmsg1							/* "should never get here!" */
		call	_panic
lretmsg1:
		.asciz	"lret: toinit\n"

/**********************************************************************/
/*
 * Signal trampoline, copied to top of user stack
 */
 		.set	exec,59
 		.set	exit,1
	 	.globl	icode
		.globl	szicode
/*
 * Icode is copied out to process 1 to exec /etc/init.
 * If the exec fails, process 1 exits.
 */
icode:
		# pushl	$argv-_icode	# gas fucks up again
		movl	$argv,%eax
		subl	$icode,%eax
		pushl	%eax

		# pushl	$init-_icode
		movl	$init,%eax
		subl	$icode,%eax
		pushl	%eax
		pushl	%eax								# dummy out rta

		movl	%esp,%ebp
		movl	$exec,%eax
		LCALL(0x7,0x0)
		pushl	%eax
		movl	$exit,%eax
		pushl	%eax								# dummy out rta
		LCALL(0x7,0x0)

init:
		.asciz	"/sbin/init"
		.align	2
argv:
		.long	init+6-icode						# argv[0] = "init" ("/sbin/init" + 6)
		.long	esigcode-icode						# argv[1] follows icode after copyout
		.long	0
esigcode:

szicode:
		.long	szicode-icode

		.globl	sigcode,szsigcode
sigcode:
		movl	12(%esp),%eax						# unsure if call will dec stack 1st
		call	%eax
		xorl	%eax,%eax							# smaller movl $103,%eax
		movb	$103,%al							# sigreturn()
		LCALL(0x7,0)								# enter kernel with args on stack
		hlt											# never gets here

		.globl	szsigcode
szsigcode:
		.long	szsigcode-sigcode

/**********************************************************************/
/* Scheduling */
/*
 * The following primitives manipulate the run queues.  _whichqs tells which
 * of the 32 queues _qs have processes in them.  Setrunqueue puts processes
 * into queues, Remrq removes them from queues.  The running process is on
 * no queue, other processes are on a queue related to p->p_priority, divided
 * by 4 actually to shrink the 0-127 range of priorities into the 32 available
 * queues.
 */
		.globl	_whichqs,_qs,_cnt,_panic
		.comm	_noproc,4
		.comm	_runrun,4

/*
 * Setrq(p)
 *
 * Call should be made at spl6(), and p->p_stat should be SRUN
 */
ENTRY(setrunqueue)
		movl	4(%esp),%eax
		cmpl	$0,P_BACK(%eax)						# should not be on q already
		je		set1
		pushl	$set2
		call	_panic
set1:
		movzbl	P_PRIORITY(%eax),%edx
		shrl	$2,%edx
		btsl	%edx,_whichqs						# set q full bit
		shll	$3,%edx
		addl	$_qs,%edx							# locate q hdr
		movl	%edx,P_FORW(%eax)					# link process on tail of q
		movl	P_BACK(%edx),%ecx
		movl	%ecx,P_BACK(%eax)
		movl	%eax,P_BACK(%edx)
		movl	%eax,P_FORW(%ecx)
		ret

set2:	.asciz	"setrunqueue"

/*
 * Remrq(p)
 *
 * Call should be made at spl6().
 */
ENTRY(remrq)
		movl	4(%esp),%eax
		movzbl	P_PRIORITY(%eax),%edx
		shrl	$2,%edx
		btrl	%edx,_whichqs						# clear full bit, panic if clear already
		jb		rem1
		pushl	$rem3
		call	_panic
rem1:
		pushl	%edx
		movl	P_FORW(%eax),%ecx					# unlink process
		movl	P_BACK(%eax),%edx
		movl	%edx,P_BACK(%ecx)
		movl	P_BACK(%eax),%ecx
		movl	P_FORW(%eax),%edx
		movl	%edx,P_FORW(%ecx)
		popl	%edx
		movl	$_qs,%ecx
		shll	$3,%edx
		addl	%edx,%ecx
		cmpl	P_FORW(%ecx),%ecx					# q still has something?
		je		rem2
		shrl	$3,%edx								# yes, set bit as still full
		btsl	%edx,_whichqs
rem2:
		movl	$0,P_BACK(%eax)						# zap reverse link to indicate off list
		ret

rem3:	.asciz	"remrq"
sw0:	.asciz	"Xswitch"

/*
 * When no processes are on the runq, Swtch branches to idle
 * to wait for something to come ready.
 */
ENTRy(Idle)
		call	_spl0
		cmpl	$0,_whichqs
		jne		sw1
		hlt		# wait for interrupt
		jmp		idle

		.align 	4 									/* ..so that profiling doesn't lump Idle with Xswitch().. */
badsw:
		pushl	$sw0
		call	_panic
		/*NOTREACHED*/

/*
 * Swtch()
 */
ENTRY(Xswitch)

		incl	_cnt+V_SWTCH

/* switch to new process. first, save context as needed */

		movl	_curproc,%ecx

/* if no process to save, don't bother */
		cmpl	$0,%ecx
		je		sw1

		movl	P_ADDR(%ecx),%ecx


		movl	(%esp),%eax							# Hardware registers
		movl	%eax, PCB_EIP(%ecx)
		movl	%ebx, PCB_EBX(%ecx)
		movl	%esp, PCB_ESP(%ecx)
		movl	%ebp, PCB_EBP(%ecx)
		movl	%esi, PCB_ESI(%ecx)
		movl	%edi, PCB_EDI(%ecx)

#ifdef NPX
/* have we used fp, and need a save? */
		mov		_curproc,%eax
		cmp		%eax,_npxproc
		jne		1f
		pushl	%ecx								/* h/w bugs make saving complicated */
		leal	PCB_SAVEFPU(%ecx),%eax
		pushl	%eax
		call	_npxsave							/* do it in a big C function */
		popl	%eax
		popl	%ecx
1:
#endif

		movl	_CMAP2,%eax							# save temporary map PTE
		movl	%eax,PCB_CMAP2(%ecx)				# in our context
		movl	$0,_curproc							# out of process

		# movw	_cpl, %ax
		# movw	%ax, PCB_IML(%ecx)	# save ipl

/* save is done, now choose a new process or idle */
sw1:
		movl	_whichqs,%edi
2:
		cli
		bsfl	%edi,%eax							# find a full q
		jz		idle								# if none, idle
# XX update whichqs?
swfnd:
		btrl	%eax,%edi							# clear q full status
		jnb		2b									# if it was clear, look for another
		movl	%eax,%ebx							# save which one we are using

		shll	$3,%eax
		addl	$_qs,%eax							# select q
		movl	%eax,%esi

#ifdef	DIAGNOSTIC
		cmpl	P_FORW(%eax),%eax 					# linked to self? (e.g. not on list)
		je		badsw								# not possible
#endif

		movl	P_FORW(%eax),%ecx					# unlink from front of process q
		movl	P_FORW(%ecx),%edx
		movl	%edx,P_FORW(%eax)
		movl	P_BACK(%ecx),%eax
		movl	%eax,P_BACK(%edx)

		cmpl	P_FORW(%ecx),%esi					# q empty
		je		3f
		btsl	%ebx,%edi							# nope, set to indicate full
3:
		movl	%edi,_whichqs						# update q status

		movl	$0,%eax
		movl	%eax,_want_resched

#ifdef	DIAGNOSTIC
		cmpl	%eax,P_WCHAN(%ecx)
		jne		badsw
		cmpb	$ SRUN,P_STAT(%ecx)
		jne		badsw
#endif

		movl	%eax,P_BACK(%ecx)					/* isolate process to run */
		movl	P_ADDR(%ecx),%edx
		movl	PCB_CR3(%edx),%ebx

		/* switch address space */
		movl	%ebx,%cr3

		/* restore context */
		movl	PCB_EBX(%edx), %ebx
		movl	PCB_ESP(%edx), %esp
		movl	PCB_EBP(%edx), %ebp
		movl	PCB_ESI(%edx), %esi
		movl	PCB_EDI(%edx), %edi
		movl	PCB_EIP(%edx), %eax
		movl	%eax, (%esp)

		movl	PCB_CMAP2(%edx),%eax				# get temporary map
		movl	%eax,_CMAP2							# reload temporary map PTE

		movl	%ecx,_curproc						# into next process
		movl	%edx,_curpcb

		/* pushl	PCB_IML(%edx)
		call	_splx
		popl	%eax*/

		movl	%edx,%eax							# return (1);
		ret

ENTRY(mvesp)
		movl	%esp,%eax
		ret

/*
 * struct proc *switch_to_inactive(p) ; struct proc *p;
 *
 * At exit of a process, move off the address space of the
 * process and onto a "safe" one. Then, on a temporary stack
 * return and run code that disposes of the old state.
 * Since this code requires a parameter from the "old" stack,
 * pass it back as a return value.
 */

ENTRY(switch_to_inactive)
		popl	%edx								# old pc
		popl	%eax								# arg, our return value
		movl	_IdlePTD,%ecx
		movl	%ecx,%cr3							# good bye address space
 #write buffer?
		movl	$tmpstk-4,%esp						# temporary stack, compensated for call
		jmp		%edx								# return, execute remainder of cleanup

/*
 * savectx(pcb, altreturn)
 * Update pcb, saving current processor state and arranging
 * for alternate return ala longjmp in Xswitch if altreturn is true.
 */
ENTRY(savectx)
		movl	4(%esp), %ecx
		movw	_cpl, %ax
		movw	%ax,  PCB_IML(%ecx)
		movl	(%esp), %eax
		movl	%eax, PCB_EIP(%ecx)
		movl	%ebx, PCB_EBX(%ecx)
		movl	%esp, PCB_ESP(%ecx)
		movl	%ebp, PCB_EBP(%ecx)
		movl	%esi, PCB_ESI(%ecx)
		movl	%edi, PCB_EDI(%ecx)

#ifdef NPX
/*
 * If npxproc == NULL, then the npx h/w state is irrelevant and the
 * state had better already be in the pcb.  This is true for forks
 * but not for dumps (the old book-keeping with FP flags in the pcb
 * always lost for dumps because the dump pcb has 0 flags).
 *
 * If npxproc != NULL, then we have to save the npx h/w state to
 * npxproc's pcb and copy it to the requested pcb, or save to the
 * requested pcb and reload.  Copying is easier because we would
 * have to handle h/w bugs for reloading.  We used to lose the
 * parent's npx state for forks by forgetting to reload.
 */
		mov	_npxproc,%eax
		testl	%eax,%eax
  		je	1f

		pushl	%ecx
		movl	P_ADDR(%eax),%eax
		leal	PCB_SAVEFPU(%eax),%eax
		pushl	%eax
		pushl	%eax
		call	_npxsave
		popl	%eax
		popl	%eax
		popl	%ecx

		pushl	%ecx
		pushl	$108+8*2						/* XXX h/w state size + padding */
		leal	PCB_SAVEFPU(%ecx),%ecx
		pushl	%ecx
		pushl	%eax
		call	_bcopy
		addl	$12,%esp
		popl	%ecx
1:
#endif

		movl	_CMAP2, %edx					# save temporary map PTE
		movl	%edx, PCB_CMAP2(%ecx)			# in our context

		cmpl	$0, 8(%esp)
		je	1f
		movl	%esp, %edx						# relocate current sp relative to pcb
		subl	$_kstack, %edx					# (sp is relative to kstack):
		addl	%edx, %ecx						# pcb += sp - kstack;
		movl	%eax, (%ecx)					# write return pc at (relocated) sp@
		# this mess deals with replicating register state gcc hides
		movl	12(%esp),%eax
		movl	%eax,12(%ecx)
		movl	16(%esp),%eax
		movl	%eax,16(%ecx)
		movl	20(%esp),%eax
		movl	%eax,20(%ecx)
		movl	24(%esp),%eax
		movl	%eax,24(%ecx)
1:
		xorl	%eax, %eax						# return 0
		ret

/*
 * addupc(int pc, struct uprof *up, int ticks):
 * update profiling information for the user process.
 */

ENTRY(addupc)
		pushl 	%ebp
		movl 	%esp,%ebp
		movl 	12(%ebp),%edx					/* up */
		movl 	8(%ebp),%eax					/* pc */

		subl 	PR_OFF(%edx),%eax				/* pc -= up->pr_off */
		jl 		L1								/* if (pc < 0) return */

		shrl 	$1,%eax							/* praddr = pc >> 1 */
		imull	PR_SCALE(%edx),%eax				/* praddr *= up->pr_scale */
		shrl 	$15,%eax						/* praddr = praddr << 15 */
		andl 	$-2,%eax						/* praddr &= ~1 */

		cmpl 	PR_SIZE(%edx),%eax				/* if (praddr > up->pr_size) return */
		ja 		L1

/*		addl 	%eax,%eax						/* praddr -> word offset */
		addl 	PR_BASE(%edx),%eax				/* praddr += up-> pr_base */
		movl 	16(%ebp),%ecx					/* ticks */

		movl 	_curpcb,%edx
		movl 	$proffault,PCB_ONFAULT(%edx)
		addl 	%ecx,(%eax)						/* storage location += ticks */
		movl 	$0,PCB_ONFAULT(%edx)
L1:
		leave
		ret

proffault:
/* if we get a fault, then kill profiling all together */
		movl 	$0,PCB_ONFAULT(%edx)			/* squish the fault handler */
 		movl 	12(%ebp),%ecx
		movl 	$0,PR_SCALE(%ecx)				/* up->pr_scale = 0 */
		leave
		ret

LF:		.asciz "Xswitch %x"

.text
 # To be done:
		.globl _astoff
_astoff:
		ret



/**********************************************************************/
/*
 * Trap and fault vector routines
 */

IDTVEC(div)
		pushl 	$0; TRAP(T_DIVIDE)
IDTVEC(dbg)
		pushl 	$0; BPTTRAP(T_TRCTRAP)
IDTVEC(nmi)
		pushl 	$0; TRAP(T_NMI)
IDTVEC(bpt)
		pushl 	$0; BPTTRAP(T_BPTFLT)
IDTVEC(ofl)
		pushl 	$0; TRAP(T_OFLOW)
IDTVEC(bnd)
		pushl 	$0; TRAP(T_BOUND)
IDTVEC(ill)
		pushl 	$0; TRAP(T_PRIVINFLT)
IDTVEC(dna)
		pushl 	$0; TRAP(T_DNA)
IDTVEC(dble)
		TRAP(T_DOUBLEFLT)
		/*PANIC("Double Fault");*/
IDTVEC(fpusegm)
		pushl 	$0; TRAP(T_FPOPFLT)
IDTVEC(tss)
		TRAP(T_TSSFLT)
		/*PANIC("TSS not valid");*/
IDTVEC(missing)
		TRAP(T_SEGNPFLT)
IDTVEC(stk)
		TRAP(T_STKFLT)
IDTVEC(prot)
		TRAP(T_PROTFLT)
IDTVEC(page)
		TRAP(T_PAGEFLT)
IDTVEC(rsvd)
		pushl 	$0; TRAP(T_RESERVED)
IDTVEC(fpu)
#ifdef NPX
/*
 * Handle like an interrupt so that we can call npxintr to clear the
 * error.  It would be better to handle npx interrupts as traps but
 * this is difficult for nested interrupts.
 */
		pushl	$0						/* dummy error code */
		pushl	$T_ASTFLT
		pushal
		nop								/* silly, the bug is for popal and it only
				 		 				 * bites when the next instruction has a
				 		 				 * complicated address mode */
		pushl	%ds
		pushl	%es						/* now the stack frame is a trap frame */
		movl	$KDSEL,%eax
		movl	%ax,%ds
		movl	%ax,%es
		pushl	_cpl
		pushl	$0						/* dummy unit to finish building intr frame */
		incl	_cnt+V_TRAP
		call	_npxintr
		jmp		doreti
#else
		pushl 	$0; TRAP(T_ARITHTRAP)
#endif
/* 17 - 31 reserved for future exp */
IDTVEC(rsvd0)
		pushl $0; TRAP(17)
IDTVEC(rsvd1)
		pushl $0; TRAP(18)
IDTVEC(rsvd2)
		pushl $0; TRAP(19)
IDTVEC(rsvd3)
		pushl $0; TRAP(20)
IDTVEC(rsvd4)
		pushl $0; TRAP(21)
IDTVEC(rsvd5)
		pushl $0; TRAP(22)
IDTVEC(rsvd6)
		pushl $0; TRAP(23)
IDTVEC(rsvd7)
		pushl $0; TRAP(24)
IDTVEC(rsvd8)
		pushl $0; TRAP(25)
IDTVEC(rsvd9)
		pushl $0; TRAP(26)
IDTVEC(rsvd10)
		pushl $0; TRAP(27)
IDTVEC(rsvd11)
		pushl $0; TRAP(28)
IDTVEC(rsvd12)
		pushl $0; TRAP(29)
IDTVEC(rsvd13)
		pushl $0; TRAP(30)
IDTVEC(rsvd14)
		pushl $0; TRAP(31)

alltraps:
		pushal
		nop
		push 	%ds
		push 	%es
		# movw	$KDSEL,%ax
		movw	$0x10,%ax
		movw	%ax,%ds
		movw	%ax,%es
calltrap:
		incl	_cnt+V_TRAP
		call	_trap
/*
 * Return through doreti to handle ASTs.  Have to change trap frame
 * to interrupt frame.
 */
		movl	$T_ASTFLT,4+4+32(%esp)	/* new trap type (err code not used) */
		pushl	_cpl
		pushl	$0						/* dummy unit */
		jmp		doreti

#ifdef KGDB
/*
 * This code checks for a kgdb trap, then falls through
 * to the regular trap code.
 */
bpttraps:
		pushal
		nop
		push	%es
		push	%ds
		# movw	$KDSEL,%ax
		movw	$0x10,%ax
		movw	%ax,%ds
		movw	%ax,%es
		movzwl	52(%esp),%eax
		test	$3,%eax
		jne		calltrap
		call	_kgdb_trap_glue
		jmp		calltrap
#endif

/*
 * Call gate entry for syscall
 */

IDTVEC(syscall)
		pushfl						# only for stupid carry bit and more stupid wait3 cc kludge
		pushal						# only need eax,ecx,edx - trap resaves others
		nop
		movl	$KDSEL,%eax			# switch to kernel segments
		movl	%ax,%ds
		movl	%ax,%es
		incl	_cnt+V_SYSCALL  	# kml 3/25/93
		call	_syscall
/*
 * Return through doreti to handle ASTs.  Have to change syscall frame
 * to interrupt frame.
 *
 * XXX - we should have set up the frame earlier to avoid the
 * following popal/pushal (not much can be done to avoid shuffling
 * the flags).  Consistent frames would simplify things all over.
 */
		movl	32+0(%esp),%eax		/* old flags, shuffle to above cs:eip */
		movl	32+4(%esp),%ebx		/* `int' frame should have been ef, eip, cs */
		movl	32+8(%esp),%ecx
		movl	%ebx,32+0(%esp)
		movl	%ecx,32+4(%esp)
		movl	%eax,32+8(%esp)
		popal
		nop
		pushl	$0					/* dummy error code */
		pushl	$T_ASTFLT
		pushal
		nop
		movl	__udatasel,%eax		/* switch back to user segments */
		push	%eax				/* XXX - better to preserve originals? */
		push	%eax
		pushl	_cpl
		pushl	$0
		jmp		doreti
/**********************************************************************/

#include <vector.s>
#include <i386/isa/icu.s>
