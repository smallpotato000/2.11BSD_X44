/*	$NetBSD: memcpy.S,v 1.4 2003/08/07 16:32:13 agc Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from locore.s.
 * Optimised by David Laight 2003
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 */

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: memcpy.S,v 1.4 2003/08/07 16:32:13 agc Exp $")
#endif

	/*
	 * (ov)bcopy (src,dst,cnt)
	 *  ws@tools.de     (Wolfgang Solfrank, TooLs GmbH) +49-228-985800
	 */

#ifdef BCOPY
ENTRY(bcopy)
#else
#ifdef MEMMOVE
ENTRY(memmove)
#else
#define NO_OVERLAP
ENTRY(memcpy)
#endif
#endif
	push	%esi
	mov		%edi,%edx
#if defined(BCOPY)
	movl	8(%esp),%esi
	movl	12(%esp),%edi
#else
	movl	8(%esp),%edi
	movl	12(%esp),%esi
#endif
	movl	16(%esp),%ecx
#if defined(NO_OVERLAP)
	movl	%ecx,%eax
#else
	movl	%edi,%eax
	subl	%esi,%eax
	cmpl	%ecx,%eax	/* overlapping? */
	movl	%ecx,%eax
	jb		backwards
#endif
	cld					/* nope, copy forwards. */
	shrl	$2,%ecx		/* copy by words */
	rep
	movsl
	and		$3,%eax		/* any bytes left? */
	jnz		trailing
done:
#if defined(MEMCPY) || defined(MEMMOVE)
	movl	8(%esp),%eax
#endif
	mov		%edx,%edi
	pop		%esi
	ret

trailing:
	cmp		$2,%eax
	jb		1f
	movw	(%esi),%ax
	movw	%ax,(%edi)
	je		done
	movb	2(%esi),%al
	movb	%al,2(%edi)
	jmp		done
1:	movb	(%esi),%al
	movb	%al,(%edi)
	jmp		done

#if !defined(NO_OVERLAP)
backwards:
	addl	%ecx,%edi	/* copy backwards. */
	addl	%ecx,%esi
	and		$3,%eax		/* any fractional bytes? */
	jnz		back_align
back_aligned:
	shrl	$2,%ecx
	subl	$4,%esi
	subl	$4,%edi
	std
	rep
	movsl
	cld
	jmp		done

back_align:
	sub		%eax,%esi
	sub		%eax,%edi
	cmp		$2,%eax
	jb		1f
	je		2f
	movb	2(%esi),%al
	movb	%al,2(%edi)
2:	movw	(%esi),%ax
	movw	%ax,(%edi)
	jmp		back_aligned
1:	movb	(%esi),%al
	movb	%al,(%edi)
	jmp		back_aligned
#endif
