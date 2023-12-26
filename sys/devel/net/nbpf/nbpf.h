/*	$NetBSD: npf_ncode.h,v 1.5.6.5 2013/02/11 21:49:49 riz Exp $	*/

/*-
 * Copyright (c) 2009-2010 The NetBSD Foundation, Inc.
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

/*
 * NBPF: A BPF Extension (Based of the NPF ncode from NetBSD 6.1)
 * Goals:
 * - Use most of the existing BPF framework
 * - Works with BPF not instead of.
 * - Most Important: Extend upon filter capabilities
 */

#ifndef _NET_NBPF_H_
#define _NET_NBPF_H_

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stdbool.h>

#include <sys/ioctl.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>

/*
 * Packet information cache.
 */
#include <net/if.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp6.h>

#include <net/bpf.h>
#include <net/bpfdesc.h>

typedef struct nbpf_cache 	nbpf_cache_t;

/* Storage of address (both for IPv4 and IPv6) and netmask */
typedef struct in6_addr		nbpf_addr_t;
typedef uint8_t				nbpf_netmask_t;

#define	NBPF_MAX_NETMASK	(128)
#define	NBPF_NO_NETMASK		((nbpf_netmask_t)~0)

#define	NBPC_IP4			0x01	/* Indicates fetched IPv4 header. */
#define	NBPC_IP6			0x02	/* Indicates IPv6 header. */
#define	NBPC_IPFRAG			0x04	/* IPv4/IPv6 fragment. */
#define	NBPC_LAYER4			0x08	/* Layer 4 has been fetched. */

#define	NBPC_TCP			0x10	/* TCP header. */
#define	NBPC_UDP			0x20	/* UDP header. */
#define	NBPC_ICMP			0x40	/* ICMP header. */
#define	NBPC_ICMP_ID		0x80	/* ICMP with query ID. */

#define	NBPC_ALG_EXEC		0x100	/* ALG execution. */

#define	NBPC_IP46			(NBPC_IP4|NBPC_IP6)

struct bpf_filter {
	struct bpf_insn 		*bf_insn;
	struct nbpf_insn 		*bf_ncode;
};

struct nbpf_cache {
    /* Information flags. */
	uint32_t				bpc_info;
	/* Pointers to the IP v4/v6 addresses. */
	nbpf_addr_t 			*bpc_srcip;
	nbpf_addr_t 			*bpc_dstip;
	/* Size (v4 or v6) of IP addresses. */
	uint8_t					bpc_alen;
	uint8_t					bpc_hlen;
	uint16_t				bpc_proto;
	/* IPv4, IPv6. */
	union {
		struct ip 			*v4;
		struct ip6_hdr 		*v6;
	} bpc_ip;
	/* TCP, UDP, ICMP. */
	union {
		struct tcphdr 		*tcp;
		struct udphdr 		*udp;
		struct icmp 		*icmp;
		struct icmp6_hdr 	*icmp6;
		void 				*hdr;
	} bpc_l4;
};

int nbpf_match_ether(struct mbuf *, int, uint16_t, uint32_t *);
int nbpf_match_proto(const nbpf_cache_t *, uint32_t);
int nbpf_match_ipmask(const nbpf_cache_t *, int, const nbpf_addr_t *, nbpf_netmask_t);
int nbpf_match_tcp_ports(const nbpf_cache_t *, int, uint32_t);
int nbpf_match_udp_ports(const nbpf_cache_t *, int, uint32_t);
int nbpf_match_icmp4(const nbpf_cache_t *, uint32_t);
int nbpf_match_icmp6(const nbpf_cache_t *, uint32_t);
int nbpf_match_tcpfl(const nbpf_cache_t *, uint32_t);

static inline bool_t
nbpf_iscached(const nbpf_cache_t *bpc, const int inf)
{
	return __predict_true((bpc->bpc_info & inf) != 0);
}

size_t		nbpf_offset(struct mbuf *);
uint32_t 	nbpf_ensure_contig(struct mbuf *, size_t);
uint32_t	nbpf_advance(struct mbuf *, size_t);
bool 		nbpf_cksum_barrier(struct mbuf *, int);
int			nbpf_add_tag(struct mbuf *, uint32_t, uint32_t);
int 		nbpf_find_tag(struct mbuf *, int, uint32_t, void **);
int 		nbpf_addr_cmp(const nbpf_addr_t *, const nbpf_netmask_t, const nbpf_addr_t *, const nbpf_netmask_t, const int);
void		nbpf_addr_mask(const nbpf_addr_t *, const nbpf_netmask_t, const int, nbpf_addr_t *);
nbpf_cache_t *nbpf_cache_init(struct mbuf *);
/*
 * NBPF n-code interface.
 */
struct nbpf_ncode {
	const void 			*iptr; 	/* N-code instruction pointer. */
	uint32_t			d;		/* Local, state variables. */
	uint32_t			i;
	uint32_t			n;
};

struct nbpf_insn {
	struct nbpf_ncode  	*ncode;
	const void 			*nc;
	size_t 				sz;
};

int nbpf_filter(struct nbpf_insn *, struct mbuf *, int);
int nbpf_validate(struct nbpf_insn *, size_t);

/* Error codes. */
#define	NBPF_ERR_OPCODE		-1	/* Invalid instruction. */
#define	NBPF_ERR_JUMP		-2	/* Invalid jump (e.g. out of range). */
#define	NBPF_ERR_REG		-3	/* Invalid register. */
#define	NBPF_ERR_INVAL		-4	/* Invalid argument value. */
#define	NBPF_ERR_RANGE		-5	/* Processing out of range. */

/* Number of registers: [0..N] */
#define	NBPF_NREGS			4

/* Maximum loop count. */
#define	NBPF_LOOP_LIMIT		100

/* Shift to check if CISC-like instruction. */
#define	NBPF_CISC_SHIFT		7
#define	NBPF_CISC_OPCODE(insn)	(insn >> NBPF_CISC_SHIFT)

/*
 * RISC-like n-code instructions.
 */

/* Return, advance, jump, tag and invalidate instructions. */

#define	NBPF_OPCODE_RET			0x00
#define	NBPF_OPCODE_ADVR		0x01
#define	NBPF_OPCODE_J			0x02
#define	NBPF_OPCODE_INVL		0x03
#define	NBPF_OPCODE_TAG			0x04

/* Set and load instructions. */
#define	NBPF_OPCODE_MOVE		0x10
#define	NBPF_OPCODE_LW			0x11

/* Compare and jump instructions. */
#define	NBPF_OPCODE_CMP			0x21
#define	NBPF_OPCODE_CMPR		0x22
#define	NBPF_OPCODE_BEQ			0x23
#define	NBPF_OPCODE_BNE			0x24
#define	NBPF_OPCODE_BGT			0x25
#define	NBPF_OPCODE_BLT			0x26

/* Arithmetic instructions. */
#define	NBPF_OPCODE_ADD			0x30
#define	NBPF_OPCODE_SUB			0x31
#define	NBPF_OPCODE_MULT		0x32
#define	NBPF_OPCODE_DIV			0x33

/* Bitwise instructions. */
#define	NBPF_OPCODE_NOT			0x40
#define	NBPF_OPCODE_AND			0x41
#define	NBPF_OPCODE_OR			0x42
#define	NBPF_OPCODE_XOR			0x43
#define	NBPF_OPCODE_SLL			0x44
#define	NBPF_OPCODE_SRL			0x45

/*
 * CISC-like n-code instructions.
 */

#define	NBPF_OPCODE_ETHER		0x80
#define	NBPF_OPCODE_PROTO		0x81

#define	NBPF_OPCODE_IP4MASK		0x90
#define	NBPF_OPCODE_TABLE		0x91
#define	NBPF_OPCODE_ICMP4		0x92
#define	NBPF_OPCODE_IP6MASK		0x93
#define	NBPF_OPCODE_ICMP6		0x94

#define	NBPF_OPCODE_TCP_PORTS	0xa0
#define	NBPF_OPCODE_UDP_PORTS	0xa1
#define	NBPF_OPCODE_TCP_FLAGS	0xa2

#endif /* _NET_NBPF_H_ */
