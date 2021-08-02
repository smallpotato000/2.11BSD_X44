/*	$NetBSD: x86_softintr.c,v 1.3 2020/05/08 21:43:54 ad Exp $	*/

/*
 * Copyright (c) 2007, 2008, 2009, 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran, and by Jason R. Thorpe.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright 2002 (c) Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Frank van der Linden for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
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
 *
 *	@(#)isa.c	7.2 (Berkeley) 5/13/91
 */

/*-
 * Copyright (c) 1993, 1994 Charles Hannum.
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
 *	@(#)isa.c	7.2 (Berkeley) 5/13/91
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/user.h>

#include <arch/i386/include/apic/lapicvar.h>

#include <arch/i386/isa/icu.h>
#include <arch/i386/include/intr.h>
#include <arch/i386/include/pic.h>

#include <dev/core/isa/isareg.h>
#include <dev/core/isa/isavar.h>

struct intrsource 	*intrsrc[MAX_INTR_SOURCES];
struct intrhand 	*intrhand[MAX_INTR_SOURCES];

int 				intrtype[MAX_INTR_SOURCES];
int 				intrmask[MAX_INTR_SOURCES];
int 				intrlevel[MAX_INTR_SOURCES];

void
intr_default_setup()
{
	/* icu & apic vectors */
	intr_legacy_vectors();
	intr_apic_vectors();
	intr_x2apic_vectors();
}

void
intr_apic_vectors()
{
	int idx;
	for(idx = 0; idx < MAX_INTR_SOURCES; idx++) {
		setidt(idx, &IDTVEC(apic_intr), 0, SDT_SYS386IGT, SEL_KPL);
	}
}

void
intr_x2apic_vectors()
{
	int idx;
	for(idx = 0; idx < MAX_INTR_SOURCES; idx++) {
		setidt(idx, &IDTVEC(x2apic_intr), 0, SDT_SYS386IGT, SEL_KPL);
	}
}

void
intr_legacy_vectors()
{
	int i;
	for(i = 0; i < NUM_LEGACY_IRQS; i++) {
		int idx = ICU_OFFSET + i;
		setidt(idx, &IDTVEC(legacy_intr), 0, SDT_SYS386IGT, SEL_KPL);
	}
	i8259_default_setup();
}

void
intr_calculatemasks()
{
	int irq, level, unusedirqs;
	struct intrhand **p, *q;

	/* First, figure out which levels each IRQ uses. */
	unusedirqs = 0xffffffff;
	for (irq = 0; irq < MAX_INTR_SOURCES; irq++) {
		int levels = 0;
		if(intrsrc[irq] == NULL) {
			intrlevel[irq] = 0;
			continue;
		}

		for (p = intrhand[irq]; (q = *p) != NULL; p = &q->ih_next) {
			levels |= 1U << q->ih_level;
		}
		intrlevel[irq] = levels;
		if (levels) {
			unusedirqs &= ~(1U << irq);
		}
	}

	/* Then figure out which IRQs use each level. */
	for (level = 0; level < NIPL; level++) {
		int irqs = 0;
		for (irq = 0; irq < MAX_INTR_SOURCES; irq++) {
			if (intrlevel[irq] & (1U << level)) {
				irqs |= 1U << irq;
			}
		}
		imask[level] = irqs | unusedirqs;
	}

	/* Initialize soft interrupt masks */
	init_intrmask();

	/*
	 * Enforce a hierarchy that gives slow devices a better chance at not
	 * dropping data.
	 */
	for (level = 0; level < (NIPL-1); level++) {
		imask[level+1] |= imask[level];
	}

	/* And eventually calculate the complete masks. */
	for (irq = 0; irq < MAX_INTR_SOURCES; irq++) {
		int irqs = 1 << irq;
		int maxlevel = IPL_NONE;
		int minlevel = IPL_HIGH;

		if(intrsrc[irq] != NULL) {
			continue;
		};

		for (q = intrsrc[irq]->is_handlers; q; q = q->ih_next) {
			irqs |= imask[q->ih_level];
			if (q->ih_level < minlevel) {
				minlevel = q->ih_level;
			}
			if (q->ih_level > maxlevel) {
				maxlevel = q->ih_level;
			}
		}
		intrmask[irq] = irqs;
		intrsrc[irq]->is_maxlevel =  maxlevel;
		intrsrc[irq]->is_minlevel = minlevel;
	}

	/* Lastly, determine which IRQs are actually in use. */
	{
		int irqs = 0;
		for (irq = 0; irq < ICU_LEN; irq++) {
			if (intrhand[irq]) {
				irqs |= 1 << irq;
			}
		}
		if (irqs >= 0x100) { /* any IRQs >= 8 in use */
			irqs |= 1 << IRQ_SLAVE;
		}
		imen = ~irqs;
		outb(IO_ICU1 + 1, imen);
		outb(IO_ICU2 + 1, imen >> 8);
	}

	for (level = 0; level < NIPL; level++) {
		iunmask[level] = ~imask[level];
	}
}

/*
 * soft interrupt masks
 */
void
init_intrmask()
{
	/*
	 * Initialize soft interrupt masks to block themselves.
	 */
	SOFTINTR_MASK(imask[IPL_SOFTCLOCK], SIR_CLOCK);
	SOFTINTR_MASK(imask[IPL_SOFTNET], SIR_NET);
	SOFTINTR_MASK(imask[IPL_SOFTSERIAL], SIR_SERIAL);

	/*
	 * IPL_NONE is used for hardware interrupts that are never blocked,
	 * and do not block anything else.
	 */
	imask[IPL_NONE] = 0;

	/*
	 * Enforce a hierarchy that gives slow devices a better chance at not
	 * dropping data.
	 */
	imask[IPL_SOFTCLOCK] |= imask[IPL_NONE];
	imask[IPL_SOFTNET] |= imask[IPL_SOFTCLOCK];
	imask[IPL_BIO] |= imask[IPL_SOFTNET];
	imask[IPL_NET] |= imask[IPL_BIO];
	imask[IPL_SOFTSERIAL] |= imask[IPL_NET];
	imask[IPL_TTY] |= imask[IPL_SOFTSERIAL];

	/*
	 * There are tty, network and disk drivers that use free() at interrupt
	 * time, so imp > (tty | net | bio).
	 */
	imask[IPL_IMP] |= imask[IPL_TTY];

	imask[IPL_AUDIO] |= imask[IPL_IMP];

	/*
	 * Since run queues may be manipulated by both the statclock and tty,
	 * network, and disk drivers, clock > imp.
	 */
	imask[IPL_CLOCK] |= imask[IPL_AUDIO];

	/*
	 * IPL_HIGH must block everything that can manipulate a run queue.
	 */
	imask[IPL_HIGH] |= imask[IPL_CLOCK];

	/*
	 * We need serial drivers to run at the absolute highest priority to
	 * avoid overruns, so serial > high.
	 */
	imask[IPL_SERIAL] |= imask[IPL_HIGH];
}

int
fakeintr(arg)
	void *arg;
{
	intr_calculatemasks();
	return (0);
}

void *
intr_establish(isapic, pictemplate, irq, type, level, ih_fun, ih_arg)
	boolean_t isapic;
	int irq, type, level, pictemplate;
	int (*ih_fun)(void *);
	void *ih_arg;
{
	register struct softpic *spic;
	register struct intrhand *ih;

	spic = softpic_intr_handler(&intrspic, irq, type, isapic, pictemplate);

	ih = spic->sp_inthnd;
	if(isapic) {
		ih->ih_irq = spic->sp_irq;
	}
	return (ih);
}

void
intr_disestablish(ih)
	struct intrhand *ih;
{
	int irq = ih->ih_irq;
	struct intrhand **p, *q;

	if (!LEGAL_IRQ(irq)) {
		panic("intr_disestablish: bogus irq");
	}
	/*
	 * Remove the handler from the chain.
	 * This is O(n^2), too.
	 */
	for (p = &intrhand[irq]; (q = *p) != NULL && q != ih; p = &q->ih_next) {
		;
	}

	if (q) {
		*p = q->ih_next;
	} else {
		panic("intr_disestablish: handler not registered");
	}

	free(ih, M_DEVBUF);

	intr_calculatemasks();

	if (intrhand[irq] == NULL) {
		intrtype[irq] = IST_NONE;
	}
}
