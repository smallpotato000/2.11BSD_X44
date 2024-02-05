/*	$NetBSD: npf_mbuf.c,v 1.6.14.3 2013/02/08 19:18:10 riz Exp $	*/

/*-
 * Copyright (c) 2009-2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This material is based upon work partially supported by The
 * NetBSD Foundation under a contract with Mindaugas Rasiukevicius.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_mbuf.c,v 1.6.14.3 2013/02/08 19:18:10 riz Exp $");

#include <sys/param.h>
#include <sys/mbuf.h>

#include "nbpf_impl.h"

static int
nbpf_check_cache(nbpf_state_t *state, nbpf_buf_t *nbuf, void *nptr)
{
	if (!nbpf_iscached(state, NBPC_IP46) &&
			!nbpf_fetch_ipv4(state, &state->nbs_ip4, nbuf, nptr) &&
			!nbpf_fetch_ipv6(state, &state->nbs_ip6, nbuf, nptr)) {
		return (state->nbs_info);
	}
	if (nbpf_iscached(state, NBPC_IPFRAG)) {
		return (state->nbs_info);
	}
	switch (nbpf_cache_ipproto(state)) {
	case IPPROTO_TCP:
		(void)nbpf_fetch_tcp(state, &state->nbs_proto, nbuf, nptr);
		break;
	case IPPROTO_UDP:
		(void)nbpf_fetch_udp(state, &state->nbs_proto, nbuf, nptr);
		break;
	case IPPROTO_ICMP:
	case IPPROTO_ICMPV6:
		(void)nbpf_fetch_icmp(state, &state->nbs_icmp, nbuf, nptr);
		break;
	}
	return (state->nbs_info);
}

/*
 * npf_cache_all: general routine to cache all relevant IP (v4 or v6)
 * and TCP, UDP or ICMP headers.
 */
int
nbpf_cache_all(nbpf_state_t *state, nbpf_buf_t *nbuf)
{
	void *nptr;

	nptr = nbpf_dataptr(nbuf);
	return (nbpf_check_cache(state, nbuf, nptr));
}

void
nbpf_recache(nbpf_state_t *state, nbpf_buf_t *nbuf)
{
	const int mflags;
	int flags;

	mflags = state->nbs_info & (NBPC_IP46 | NBPC_LAYER4);
	state->nbs_info = 0;
	flags = nbpf_cache_all(state, nbuf);
	KASSERT((flags & mflags) == mflags);
}

int
nbpf_reassembly(nbpf_state_t *state, nbpf_buf_t *nbuf, struct mbuf **mp)
{
	int error;

	if (nbpf_iscached(state, NBPC_IP4)) {
		struct ip *ip = nbpf_dataptr(nbuf);
		error = ip_reass_packet(mp, ip);
	} else if (nbpf_iscached(state, NBPC_IP6)) {
		error = ip6_reass_packet(mp, state->nbs_hlen);
		if (error && *mp == NULL) {
			memset(nbuf, 0, sizeof(nbpf_buf_t));
		}
	}
	if (error) {
		return (error);
	}
	if (*mp == NULL) {
		return (0);
	}
	state->nbs_info = 0;
	if (nbpf_cache_all(state, nbuf) & NBPC_IPFRAG) {
		return (EINVAL);
	}
	return (0);
}
