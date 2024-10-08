/*-
 * Copyright (C) 1989, 1990 W. Jolitz
 * Copyright (c) 1992, 1993
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
 *	@(#)icu.s	8.1 (Berkeley) 6/11/93
 */

#include "assym.h"

#include <machine/asm.h>
#include <machine/intr.h>
#include <machine/psl.h>
#include <machine/segments.h>
#include <machine/trap.h>

#include "opt_inet.h"
#include "opt_ns.h"

#include <net/netisr.h>

		.data
		.globl	_C_LABEL(imen),_C_LABEL(cpl),_C_LABEL(ipending),_C_LABEL(idepth)
		.globl	_C_LABEL(netisr)
_C_LABEL(cpl):
		.long	0xffff			# current priority level (all off)
_C_LABEL(imen):
		.long	0xffff			# interrupt mask enable (all off)
_C_LABEL(idepth):
		.long	0xffff
_C_LABEL(ipending):
		.long	0xffff
		
		.text
			
#if defined(PROF) || defined(GPROF)
		.globl	_C_LABEL(splhigh), _C_LABEL(splx)

		ALIGN_TEXT
_C_LABEL(splhigh):
		movl	$-1,%eax
		xchgl	%eax,_C_LABEL(cpl)
		ret

		ALIGN_TEXT
_C_LABEL(splx):
		movl	4(%esp),%eax
		movl	%eax,_C_LABEL(cpl)
		testl	%eax,%eax
		jnz		_C_LABEL(Xspllower)
		ret
#endif /* PROF || GPROF */

/*
 * Process pending interrupts.
 *
 * Important registers:
 *   ebx - cpl
 *   esi - address to resume loop at
 *   edi - scratch for Xsoftnet
 */
IDTVEC(spllower)
		pushl	%ebx
		pushl	%esi
		pushl	%edi
		movl	_C_LABEL(cpl),%ebx		# save priority
		movl	$1f,%esi				# address to resume loop at
1:		movl	%ebx,%eax
		notl	%eax
		cli
		andl	_C_LABEL(ipending),%eax
		jz	2f
		sti
		bsfl	%eax,%eax
		btrl	%eax,_C_LABEL(ipending)
		jnc	1b
		jmp		*APIC_RECURSE(,%eax,4)
2:		movl	%ebx,_C_LABEL(cpl)
		sti
		popl	%edi
		popl	%esi
		popl	%ebx
		ret

/*
 * Handle return from interrupt after device handler finishes.
 *
 * Important registers:
 *   ebx - cpl to restore
 *   esi - address to resume loop at
 *   edi - scratch for Xsoftnet
 */
IDTVEC(doreti)
		popl	%ebx					# get previous priority
		movl	$1f,%esi				# address to resume loop at
1:		movl	%ebx,%eax
		notl	%eax
		cli
		andl	_C_LABEL(ipending),%eax
		jz		2f
		sti
		bsfl    %eax,%eax               # slow, but not worth optimizing
		btrl    %eax,_C_LABEL(ipending)
		jnc     1b						# some intr cleared the in-memory bit
		jmp		*APIC_RESUME(,%eax,4)
2:		/* Check for ASTs on exit to user mode. */
		movl	%ebx,_C_LABEL(cpl)
5:
		testb   $SEL_UPL,TF_CS(%esp)
		jnz		doreti_checkast
		testl	$PSL_VM,TF_EFLAGS(%esp)
		jz		3f

doreti_checkast:
		CHECK_ASTPENDING(%eax)
		jz		3f
4:		CLEAR_ASTPENDING(%eax)
		sti
		movl	$T_ASTFLT,TF_TRAPNO(%esp)	/* XXX undo later.. */
		/* Pushed T_ASTFLT into tf_trapno on entry. */
		pushl	%esp
		call	_C_LABEL(trap)
		addl	$4,%esp
		cli
		jmp		5b
3:
		INTRFASTEXIT

/*
 * Soft interrupt handlers
 */
IDTVEC(softclock)
		movl	_C_LABEL(imask) + IPL_SOFTCLOCK * 4,%eax
		movl	%eax,_C_LABEL(cpl)
		call	_C_LABEL(softclock)
		movl	%ebx,_C_LABEL(cpl)
		jmp		*%esi

IDTVEC(softserial)
		movl	_C_LABEL(imask) + IPL_SOFTSERIAL * 4,%eax
		movl	%eax,_C_LABEL(cpl)

#include "com.h"
#if NCOM > 0
		call	_C_LABEL(comsoft)
#endif

		movl	%ebx,_C_LABEL(cpl)
		jmp		*%esi

IDTVEC(softnet)
		movl	_C_LABEL(imask) + IPL_SOFTNET * 4,%eax
		movl	%eax,_C_LABEL(cpl)
		xorl	%edi,%edi
		xchgl	_C_LABEL(netisr),%edi

#define DONET(s, c) 			\
		.globl   _C_LABEL(c)	;\
		testl	$(1 << s),%edi	;\
		jz		1f				;\
		call	_C_LABEL(c)		;\
1:

#include <net/netisr_dispatch.h>
#undef DONET

		movl	%ebx,_C_LABEL(cpl)
		jmp		*%esi

