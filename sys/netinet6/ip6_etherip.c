/*      $NetBSD: ip6_etherip.c,v 1.15 2011/07/17 20:54:53 joerg Exp $        */

/*
 *  Copyright (c) 2006, Hans Rosenfeld <rosenfeld@grumpf.hope-2000.org>
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of Hans Rosenfeld nor the names of his
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 *  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 *  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 *  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 *  SUCH DAMAGE.
 *
 *
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ip6_etherip.c,v 1.15 2011/07/17 20:54:53 joerg Exp $");

#include "opt_inet.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/mbuf.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <sys/protosw.h>
#include <sys/kernel.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#ifdef INET
#include <netinet/ip.h>
#endif
#ifdef INET6
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>

#include <netinet6/in6_var.h>
#include <netinet6/ip6_etherip.h>
#endif

#include <net/if_ether.h>
#include <net/if_media.h>
#include <net/if_etherip.h>
#include <net/bpf.h>

int
ip6_etherip_output(ifp, m)
	struct ifnet *ifp;
	struct mbuf *m;
{
	struct etherip_softc *sc = (struct etherip_softc*)ifp;
	struct sockaddr_in6 *dst = (struct sockaddr_in6 *)&sc->sc_ro6.ro_dst;
	struct sockaddr_in6 *sin6_src = (struct sockaddr_in6 *)sc->sc_src;
	struct sockaddr_in6 *sin6_dst = (struct sockaddr_in6 *)sc->sc_dst;
	struct ip6_hdr *ip6;    /* capsule IP header, host byte ordered */
	struct etherip_header eiphdr;
	int proto, error;

	if (sin6_src == NULL || sin6_dst == NULL ||
	    sin6_src->sin6_family != AF_INET6 ||
	    sin6_dst->sin6_family != AF_INET6) {
		m_freem(m);
		return EAFNOSUPPORT;
	}

	/* reset broadcast/multicast flags */
	m->m_flags &= ~(M_BCAST | M_MCAST);

	m->m_flags |= M_PKTHDR;
	proto = IPPROTO_ETHERIP;

	/* fill and prepend Ethernet-in-IP header */
	eiphdr.eip_ver = ETHERIP_VERSION & ETHERIP_VER_VERS_MASK;
	eiphdr.eip_pad = 0;
	M_PREPEND(m, sizeof(struct etherip_header), M_DONTWAIT);
	if (m && m->m_len < sizeof(struct etherip_header))
		m = m_pullup(m, sizeof(struct etherip_header));
	if (m == NULL)
		return ENOBUFS;
	bcopy(&eiphdr, mtod(m, struct etherip_header *), sizeof(struct etherip_header));

	/* prepend new IP header */
	M_PREPEND(m, sizeof(struct ip6_hdr), M_DONTWAIT);
	if (m && m->m_len < sizeof(struct ip6_hdr))
		m = m_pullup(m, sizeof(struct ip6_hdr));
	if (m == NULL)
		return ENOBUFS;

	ip6 = mtod(m, struct ip6_hdr*);
	ip6->ip6_flow = 0;
	ip6->ip6_vfc &= ~IPV6_VERSION_MASK;
	ip6->ip6_vfc |= IPV6_VERSION;
	ip6->ip6_nxt  = proto;
	ip6->ip6_hlim = ETHERIP_TTL;
	ip6->ip6_src  = sin6_src->sin6_addr;

	/* bidirectional configured tunnel mode */
	if (!IN6_IS_ADDR_UNSPECIFIED(&sin6_dst->sin6_addr))
		ip6->ip6_dst = sin6_dst->sin6_addr;
	else  {
		m_freem(m);
		return ENETUNREACH;
	}

	if (dst->sin6_family != sin6_dst->sin6_family ||
	     !IN6_ARE_ADDR_EQUAL(&dst->sin6_addr, &sin6_dst->sin6_addr)) {
		/* cache route doesn't match */
		bzero(dst, sizeof(*dst));
		dst->sin6_family = sin6_dst->sin6_family;
		dst->sin6_len = sizeof(struct sockaddr_in6);
		dst->sin6_addr = sin6_dst->sin6_addr;
		if (sc->sc_ro6.ro_rt) {
			RTFREE(sc->sc_ro6.ro_rt);
			sc->sc_ro6.ro_rt = NULL;
		}
	}

	if (sc->sc_ro6.ro_rt == NULL) {
		rtalloc((struct route *)&sc->sc_ro6);
		if (sc->sc_ro6.ro_rt == NULL) {
			m_freem(m);
			return ENETUNREACH;
		}

		/* if it constitutes infinite encapsulation, punt. */
		if (sc->sc_ro.ro_rt->rt_ifp == ifp) {
			m_freem(m);
			return ENETUNREACH;	/* XXX */
		}
	}
#ifdef IPV6_MINMTU
	error = ip6_output(m, 0, &sc->sc_ro6, IPV6_MINMTU, NULL, NULL, NULL);
#else
	error = ip6_output(m, 0, &sc->sc_ro6, 0, NULL, NULL, NULL);
#endif
	return error;
}

int
ip6_etherip_input(mp, offp, proto)
	struct mbuf **mp;
	int *offp, proto;
{
	struct mbuf *m = *mp;
	int off = *offp;
	struct etherip_softc *sc;
	const struct ip6_hdr *ip6;
	struct sockaddr_in6 *src6, *dst6;
	struct ifnet *ifp = NULL;
	int s;

	if (proto != IPPROTO_ETHERIP) {
		m_freem(m);
		ip6stat.ip6s_nogif++;
		return IPPROTO_DONE;
	}

	ip6 = mtod(m, const struct ip6_hdr*);

	/* find device configured for this packets src and dst */
	LIST_FOREACH(sc, &etherip_softc_list, etherip_list)	{
		if (!sc->sc_src || !sc->sc_dst)
			continue;
		if (sc->sc_src->sa_family != AF_INET6
				|| sc->sc_dst->sa_family != AF_INET6)
			continue;

		src6 = (struct sockaddr_in6*) sc->sc_src;
		dst6 = (struct sockaddr_in6*) sc->sc_dst;

		if (!IN6_ARE_ADDR_EQUAL(&src6->sin6_addr, &ip6->ip6_dst) ||
		!IN6_ARE_ADDR_EQUAL(&dst6->sin6_addr, &ip6->ip6_src))
			continue;

		ifp = &sc->sc_ec.ec_if;
		break;
	}

	/* no matching device found */
	if (!ifp) {
		m_freem(m);
		ip6stat.ip6s_odropped++;
		return IPPROTO_DONE;
	}

	m_adj(m, off);

	/*
	 * Section 4 of RFC 3378 requires that the EtherIP header of incoming
	 * packets is verified to contain the correct values in the version and
	 * reserved fields, and packets with wrong values be dropped.
	 *
	 * There is some discussion about what exactly the header should look
	 * like, the RFC is not very clear there. To be compatible with broken
	 * implementations, we don't check the header on incoming packets,
	 * relying on the ethernet code to filter out garbage.
	 *
	 * The header we use for sending is compatible with the original
	 * implementation in OpenBSD, which was used in former NetBSD versions
	 * and is used in FreeBSD. One Linux implementation is known to use the
	 * same value.
	 */
	m_adj(m, sizeof(struct etherip_header));
	m = m_pullup(m, sizeof(struct ether_header));
	if (m == NULL) {
		ifp->if_ierrors++;
		return IPPROTO_DONE;
	}

	m->m_pkthdr.rcvif = ifp;
	m->m_flags &= ~(M_BCAST | M_MCAST);

	bpf_mtap(ifp->if_bpf, m);

	ifp->if_ipackets++;

	s = splnet();
	(ifp->if_input)(ifp, m);
	splx(s);

	return IPPROTO_DONE;
}
