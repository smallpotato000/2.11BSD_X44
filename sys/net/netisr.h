/* $NetBSD: netisr.h,v 1.32 2003/08/07 16:32:55 agc Exp $ */

/*
 * Copyright (c) 1980, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)netisr.h	8.1 (Berkeley) 6/10/93
 */

#ifndef _NET_NETISR_H_
#define _NET_NETISR_H_

/*
 * The networking code runs off software interrupts.
 *
 * You can switch into the network by doing splsoftnet() and return by splx().
 * The software interrupt level for the network is higher than the software
 * level for the clock (so you can enter the network in routines called
 * at timeout time).
 *
 * The routine to request a network software interrupt, setsoftnet(),
 * is defined in the machine-specific include files.
 */

#if defined(_KERNEL)
#if !defined(_LOCORE)

/* XXX struct sockaddr defn for for if.h, if_arp.h */
#include <sys/socket.h>

/*
 * XXX IFNAMSIZE for if_ppp.h, natm.h; struct ifnet decl for in6.h, in.h;
 * XXX struct mbuf decl for in6.h, in.h, route.h (via in_var.h).
 */
#include <net/if.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/ip_var.h>
#if NARP > 0
#include <netinet/if_inarp.h>
#endif
#endif
#ifdef INET6
# ifndef INET
#  include <netinet/in.h>
# endif
#include <netinet/ip6.h> 
#include <netinet6/ip6_var.h>
#endif
#ifdef NS
#include <netns/ns_var.h>
#endif
#ifndef __HAVE_GENERIC_SOFT_INTERRUPTS		/* XXX XXX XXX */
#if NSL > 0
extern void slnetisr(void);
#endif
#if NSTRIP > 0
extern void stripnetisr(void);
#endif
#if NPPP > 0
extern void pppnetisr(void);
#endif
#endif /* __HAVE_GENERIC_SOFT_INTERRUPTS */

#endif /* !defined(_LOCORE) */
#endif /* defined(_KERNEL) */


/*
 * Each ``pup-level-1'' input queue has a bit in a ``netisr'' status
 * word which is used to de-multiplex a single software
 * interrupt used for scheduling the network code to calls
 * on the lowest level routine of each protocol.
 */
#define	NETISR_RAW		0		/* same as AF_UNSPEC */
#define	NETISR_IP		2		/* same as AF_INET */
#define	NETISR_NS		6		/* same as AF_NS */
#define	NETISR_CLOCK	15		/* protocol timeout */
#define	NETISR_IPV6		24		/* same as AF_INET6 */
#define	NETISR_ARP		28		/* same as AF_ARP */
#ifndef __HAVE_GENERIC_SOFT_INTERRUPTS
#define	NETISR_SLIP		29		/* for SLIP processing */
#define	NETISR_STRIP	30		/* for STRIP processing */
#define	NETISR_PPP		31		/* for PPP processing */
#endif

#if defined(_KERNEL) && !defined(_LOCORE)

#ifndef schednetisr
#define	schednetisr(anisr)	{ netisr |= 1<<(anisr); setsoftnet(); }
#endif

extern	int netisr;			/* scheduling bits for network */
#endif /* defined(_KERNEL) && !defined(_LOCORE) */

#ifdef INET
#if NARP > 0
	DONET(NETISR_ARP, arpintr)
#endif
	DONET(NETISR_IP, ipintr)
#endif
#ifdef IMP
	DONET(NETISR_IMP, impintr)
#endif
#ifdef NS
	DONET(NETISR_NS, nsintr)
#endif
#if NSL > 0
	DONET(NETISR_SLIP, slintr);
#endif
#if NSTRIP > 0
	DONET(NETISR_STRIP, stripintr);
#endif
#if NPPP > 0
	DONET(NETISR_PPP, pppintr)
#endif

#endif /* _NET_NETISR_H_ */
