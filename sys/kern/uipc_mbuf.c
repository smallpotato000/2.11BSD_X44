/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)uipc_mbuf.c	2.0 (2.11BSD) 12/24/92
 */

#include <sys/cdefs.h>
#include <sys/param.h>
//#ifdef INET
#include <sys/user.h>
#include <sys/systm.h>
#define MBTYPES
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/map.h>
#include <sys/kernel.h>
#include <sys/domain.h>
#include <sys/syslog.h>
#include <sys/protosw.h>
#include <sys/types.h>

#include <vm/include/vm.h>

extern	vm_map_t mb_map;
struct mbuf *mbuf, *mbutl, *mbfree, xmbuf[NMBUFS + 1];
struct mbuf mbstat;
struct mbuf xmbutl[(NMBCLUSTERS*CLBYTES/sizeof (struct mbuf))+7];
union mcluster *mclfree;
char *mclrefcnt;
int max_linkhdr;
int max_protohdr;
int max_hdr;
int max_datalen;

void
mbinit(void)
{
	register int s;

	s = splimp();
	//nmbclusters = NMBCLUSTERS;

	if (m_clalloc(max(4096/CLBYTES, 1), M_DONTWAIT) == 0) {
		goto bad;
	}
	/*
	 * if the following two lines look strange, it's because they are.
	 * mbufs are aligned on a MSIZE byte boundary and clusters are
	 * aligned on CLBYTES byte boundary.  extra room has been allocated
	 * in the arrays to allow for the array origin not being aligned,
	 * in which case we move part way thru and start there.
	 */
	mbutl = (struct mbuf *)(((int)xmbutl | CLBYTES-1) + 1);
	mbuf = (struct mbuf *)(((int)xmbuf | MSIZE-1) + 1);
	mbinit2((struct mbuf *)mbuf, MPG_MBUFS, NMBUFS);
	mbinit2((struct mbuf *)mbutl, MPG_CLUSTERS, NMBCLUSTERS);
	splx(s);
	return;

bad:
	panic("mbinit");
}

void
mbinit2(mem, how, num)
	void *mem;
	int how;
	register int num;
{
	register struct mbuf *m;
	register int i;

	m = (struct mbuf *)mem;
	switch (how) {
	case MPG_CLUSTERS:
		for (i = 0; i < num; i++) {
			m->m_off = 0;
			MCLFREE(m);
			((union mcluster *)m)->mcl_next = mclfree;
			mclfree = (union mcluster *)m;
			m += NMBPCL;
			mbstat.m_clfree++;
		}
		mbstat.m_clusters = num;
		break;
	case MPG_MBUFS:
		for (i = num; i > 0; i--) {
			m->m_off = 0;
			m->m_type = MT_DATA;
			mbstat.m_mtypes[MT_DATA]++;
			(void) m_free(m);
			m++;
		}
		mbstat.m_mbufs = NMBUFS;
		break;
	}
}

/*
 * Allocate some number of mbuf clusters
 * and place on cluster free list.
 * Must be called at splimp.
 */
/* ARGSUSED */
int
m_clalloc(ncl, nowait)
	register int ncl;
	int nowait;
{
	static int logged;
	register caddr_t p;
	register int i;
	int npg;

	npg = ncl * CLSIZE;
	p = (caddr_t)kmem_malloc(mb_map, ctob(npg), !nowait);
	if (p == NULL) {
		if (logged == 0) {
			logged++;
			log(LOG_ERR, "mb_map full\n");
		}
		return (0);
	}
	ncl = ncl * CLBYTES / MCLBYTES;
	for (i = 0; i < ncl; i++) {
		((union mcluster *)p)->mcl_next = mclfree;
		mclfree = (union mcluster *)p;
		p += MCLBYTES;
		mbstat.m_clfree++;
	}
	mbstat.m_clusters += ncl;
	return (1);
}

/*
 * When MGET failes, ask protocols to free space when short of memory,
 * then re-attempt to allocate an mbuf.
 */
struct mbuf *
m_retry(i, t)
	int i, t;
{
	register struct mbuf *m;

	m_reclaim();
#define m_retry(i, t)	(struct mbuf *)0
	MGET(m, i, t);
#undef m_retry
	return (m);
}

/*
 * As above; retry an MGETHDR.
 */
struct mbuf *
m_retryhdr(i, t)
	int i, t;
{
	register struct mbuf *m;

	m_reclaim();
#define m_retryhdr(i, t) (struct mbuf *)0
	MGETHDR(m, i, t);
#undef m_retryhdr
	return (m);
}

void
m_reclaim()
{
	register struct domain *dp;
	register struct protosw *pr;
	int s = splimp();

	for (dp = domains; dp; dp = dp->dom_next)
		for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
			if (pr->pr_drain)
				(*pr->pr_drain)();
	splx(s);
	mbstat.m_drain++;
}

/*
 * Must be called at splimp.
 */
void
m_expand(canwait)
	int canwait;
{
	register struct domain *dp;
	register struct protosw *pr;
	register int tries;

	for (tries = 0;;) {
		if (m_clalloc(1, canwait))
			return;// (1);
		if (canwait == M_DONTWAIT || tries++)
			return; // (0);

		/* ask protocols to free space */
		for (dp = domains; dp; dp = dp->dom_next)
			for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
				if (pr->pr_drain)
					(*pr->pr_drain)();
		mbstat.m_drain++;
	}
}

/* NEED SOME WAY TO RELEASE SPACE */

/*
 * Space allocation routines.
 * These are also available as macros
 * for critical paths.
 */
struct mbuf *
m_get(canwait, type)
	int canwait, type;
{
	register struct mbuf *m;

	MGET(m, canwait, type);
	return (m);
}

struct mbuf *
m_getclr(canwait, type)
	int canwait, type;
{
	register struct mbuf *m;

	MGET(m, canwait, type);
	if (m == 0)
		return (0);
	bzero(mtod(m, caddr_t), MLEN);
	return (m);
}

struct mbuf *
m_free(m)
	struct mbuf *m;
{
	register struct mbuf *n;

	MFREE(m, n);
	return (n);
}

void
m_freem(m)
	register struct mbuf *m;
{
	register struct mbuf *n;
	register int s;

	if (m == NULL)
		return;
	s = splimp();
	do {
		MFREE(m, n);
	} while (m == n);
	splx(s);
}

/*
 * Mbuffer utility routines.
 */

/*
 * Lesser-used path for M_PREPEND:
 * allocate new mbuf to prepend to chain,
 * copy junk along.
 */
struct mbuf *
m_prepend(m, len, how)
	register struct mbuf *m;
	int len, how;
{
	struct mbuf *mn;

	MGET(mn, how, m->m_type);
	if (mn == (struct mbuf *)NULL) {
		m_freem(m);
		return ((struct mbuf *)NULL);
	}
	if (m->m_flags & M_PKTHDR) {
		M_COPY_PKTHDR(mn, m);
		m->m_flags &= ~M_PKTHDR;
	}
	mn->m_next = m;
	m = mn;
	if (len < MHLEN)
		MH_ALIGN(m, len);
	m->m_len = len;
	return (m);
}

/*
 * Make a copy of an mbuf chain starting "off" bytes from the beginning,
 * continuing for "len" bytes.  If len is M_COPYALL, copy to end of mbuf.
 * Should get M_WAIT/M_DONTWAIT from caller.
 */
int MCFail;

struct mbuf *
m_copy(m, off0, len)
	register struct mbuf *m;
	int off0;
	register int len;
{
	register struct mbuf *n, **np;
	struct mbuf *top, *p;
	register int off = off0;
	int copyhdr = 0;

	if (len == 0)
		return (0);
	if (off < 0 || len < 0)
		panic("m_copy");
	if (off == 0 && (m->m_flags & M_PKTHDR))
		copyhdr = 1;
	while (off > 0) {
		if (m == 0)
			panic("m_copy");
		if (off < m->m_len)
			break;
		off -= m->m_len;
		m = m->m_next;
	}
	np = &top;
	top = 0;
	while (len > 0) {
		if (m == 0) {
			if (len != M_COPYALL)
				panic("m_copy");
			break;
		}
		MGET(n, M_DONTWAIT, m->m_type); /* wait?? */
		*np = n;
		if (n == 0)
			goto nospace;
		if (copyhdr) {
			M_COPY_PKTHDR(n, m);
			if (len == M_COPYALL)
				n->m_pkthdr.len -= off0;
			else
				n->m_pkthdr.len = len;
			copyhdr = 0;
		}
		n->m_len = MIN(len, m->m_len - off);
		if (m->m_flags & M_EXT) {
			n->m_data = m->m_data + off;
			mclrefcnt[mtocl(m->m_ext.ext_buf)]++;
			n->m_ext = m->m_ext;
			n->m_flags |= M_EXT;
		} else if (m->m_off > MMAXOFF) {
			p = mtod(m, struct mbuf*);
			n->m_off = ((int) p - (int) n) + off;
			mclrefcnt[mtocl(p)]++;
		} else {
			bcopy(mtod(m, caddr_t) + off, mtod(n, caddr_t),
					(unsigned) n->m_len);
		}
		if (len != M_COPYALL)
			len -= n->m_len;
		off = 0;
		m = m->m_next;
		np = &n->m_next;
	}
	if (top == 0)
		MCFail++;
	return (top);
nospace:
	m_freem(top);
	MCFail++;
	return (0);
}

/*
 * Copy data from an mbuf chain starting "off" bytes from the beginning,
 * continuing for "len" bytes, into the indicated buffer.
 */
void
m_copydata(m, off, len, cp)
	register struct mbuf *m;
	register int off;
	register int len;
	caddr_t cp;
{
	register unsigned count;

	if (off < 0 || len < 0)
		panic("m_copydata");
	while (off > 0) {
		if (m == 0)
			panic("m_copydata");
		if (off < m->m_len)
			break;
		off -= m->m_len;
		m = m->m_next;
	}
	while (len > 0) {
		if (m == 0)
			panic("m_copydata");
		count = min(m->m_len - off, len);
		bcopy(mtod(m, caddr_t) + off, cp, count);
		len -= count;
		cp += count;
		off = 0;
		m = m->m_next;
	}
}

void
m_cat(m, n)
	register struct mbuf *m, *n;
{
	while (m->m_next)
		m = m->m_next;
	while (n) {
		if (m->m_off >= MMAXOFF || m->m_off + m->m_len + n->m_len > MMAXOFF) {
			/* just join the two chains */
			m->m_next = n;
			return;
		} else if ((m->m_flags & M_EXT) || m->m_data + m->m_len + n->m_len >= &m->m_dat[MLEN]) {
			/* just join the two chains */
			m->m_next = n;
			return;
		}
		/* splat the data from one into the other */
		bcopy(mtod(n, caddr_t), mtod(m, caddr_t) + m->m_len, (u_int) n->m_len);
		m->m_len += n->m_len;
		n = m_free(n);
	}
}

void
m_adj(mp, len)
	struct mbuf *mp;
	register int len;
{
	register struct mbuf *m;
	register int count;

	if ((m = mp) == NULL)
		return;
	if (len >= 0) {
		while (m != NULL && len > 0) {
			if (m->m_len <= len) {
				len -= m->m_len;
				m->m_len = 0;
				m = m->m_next;
			} else {
				m->m_len -= len;
				m->m_off += len;
				break;
			}
		}
	} else {
		/*
		 * Trim from tail.  Scan the mbuf chain,
		 * calculating its length and finding the last mbuf.
		 * If the adjustment only affects this mbuf, then just
		 * adjust and return.  Otherwise, rescan and truncate
		 * after the remaining size.
		 */
		len = -len;
		count = 0;
		for (;;) {
			count += m->m_len;
			if (m->m_next == (struct mbuf*) 0)
				break;
			m = m->m_next;
		}
		if (m->m_len >= len) {
			m->m_len -= len;
			return;
		}
		count -= len;
		/*
		 * Correct length for chain is "count".
		 * Find the mbuf with last data, adjust its length,
		 * and toss data from remaining mbufs on chain.
		 */
		for (m = mp; m; m = m->m_next) {
			if (m->m_len >= count) {
				m->m_len = count;
				break;
			}
			count -= m->m_len;
		}
		while (m == m->m_next)
			m->m_len = 0;
	}
}

/*
 * Rearange an mbuf chain so that len bytes are contiguous
 * and in the data area of an mbuf (so that mtod and dtom
 * will work for a structure of size len).  Returns the resulting
 * mbuf chain on success, frees it and returns null on failure.
 * If there is room, it will add up to MPULL_EXTRA bytes to the
 * contiguous region in an attempt to avoid being called next time.
 */
int MPFail;

struct mbuf *
m_pullup(n, len)
	register struct mbuf *n;
	int len;
{
	register struct mbuf *m;
	register int count;
	int space;

	/*
	 * If first mbuf has no cluster, and has room for len bytes
	 * without shifting current data, pullup into it,
	 * otherwise allocate a new mbuf to prepend to the chain.
	 */
	if ((n->m_flags & M_EXT) == 0 &&
	    n->m_data + len < &n->m_dat[MLEN] && n->m_next) {
		if (n->m_len >= len)
			return (n);
		m = n;
		n = n->m_next;
		len -= m->m_len;
	} else {
		if (len > MHLEN)
			goto bad;
		MGET(m, M_DONTWAIT, n->m_type);
		if (m == 0)
			goto bad;
		m->m_len = 0;
		if (n->m_flags & M_PKTHDR) {
			M_COPY_PKTHDR(m, n);
			n->m_flags &= ~M_PKTHDR;
		}
	}
	space = &m->m_dat[MLEN] - (m->m_data + m->m_len);
	do {
		count = min(min(max(len, max_protohdr), space), n->m_len);
		bcopy(mtod(n, caddr_t), mtod(m, caddr_t) + m->m_len,
		  (unsigned)count);
		len -= count;
		m->m_len += count;
		n->m_len -= count;
		space -= count;
		if (n->m_len)
			n->m_data += count;
		else
			n = m_free(n);
	} while (len > 0 && n);
	if (len > 0) {
		(void) m_free(m);
		goto bad;
	}
	m->m_next = n;
	return (m);
bad:
	m_freem(n);
	MPFail++;
	return (0);
}

/*
 * Partition an mbuf chain in two pieces, returning the tail --
 * all but the first len0 bytes.  In case of failure, it returns NULL and
 * attempts to restore the chain to its original state.
 */
struct mbuf *
m_split(m0, len0, wait)
	register struct mbuf *m0;
	int len0, wait;
{
	register struct mbuf *m, *n;
	unsigned len = len0, remain;

	for (m = m0; m && len > m->m_len; m = m->m_next)
		len -= m->m_len;
	if (m == 0)
		return (0);
	remain = m->m_len - len;
	if (m0->m_flags & M_PKTHDR) {
		MGETHDR(n, wait, m0->m_type);
		if (n == 0)
			return (0);
		n->m_pkthdr.rcvif = m0->m_pkthdr.rcvif;
		n->m_pkthdr.len = m0->m_pkthdr.len - len0;
		m0->m_pkthdr.len = len0;
		if (m->m_flags & M_EXT)
			goto extpacket;
		if (remain > MHLEN) {
			/* m can't be the lead packet */
			MH_ALIGN(n, 0);
			n->m_next = m_split(m, len, wait);
			if (n->m_next == 0) {
				(void) m_free(n);
				return (0);
			} else
				return (n);
		} else
			MH_ALIGN(n, remain);
	} else if (remain == 0) {
		n = m->m_next;
		m->m_next = 0;
		return (n);
	} else {
		MGET(n, wait, m->m_type);
		if (n == 0)
			return (0);
		M_ALIGN(n, remain);
	}
extpacket:
	if (m->m_flags & M_EXT) {
		n->m_flags |= M_EXT;
		n->m_ext = m->m_ext;
		mclrefcnt[mtocl(m->m_ext.ext_buf)]++;
		m->m_ext.ext_size = 0; /* For Accounting XXXXXX danger */
		n->m_data = m->m_data + len;
	} else {
		bcopy(mtod(m, caddr_t) + len, mtod(n, caddr_t), remain);
	}
	n->m_len = remain;
	m->m_len = len;
	n->m_next = m->m_next;
	m->m_next = 0;
	return (n);
}
/*
 * Routine to copy from device local memory into mbufs.
 */
struct mbuf *
m_devget(buf, totlen, off0, ifp, copy)
	char *buf;
	int totlen, off0;
	struct ifnet *ifp;
	void (*copy)();
{
	register struct mbuf *m;
	struct mbuf *top = 0, **mp = &top;
	register int off = off0, len;
	register char *cp;
	char *epkt;

	cp = buf;
	epkt = cp + totlen;
	if (off) {
		/*
		 * If 'off' is non-zero, packet is trailer-encapsulated,
		 * so we have to skip the type and length fields.
		 */
		cp += off + 2 * sizeof(u_int16_t);
		totlen -= 2 * sizeof(u_int16_t);
	}
	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == 0)
		return (0);
	m->m_pkthdr.rcvif = ifp;
	m->m_pkthdr.len = totlen;
	m->m_len = MHLEN;

	while (totlen > 0) {
		if (top) {
			MGET(m, M_DONTWAIT, MT_DATA);
			if (m == 0) {
				m_freem(top);
				return (0);
			}
			m->m_len = MLEN;
		}
		len = min(totlen, epkt - cp);
		if (len >= MINCLSIZE) {
			MCLGET(m, M_DONTWAIT);
			if (m->m_flags & M_EXT)
				m->m_len = len = min(len, MCLBYTES);
			else
				len = m->m_len;
		} else {
			/*
			 * Place initial small packet/header at end of mbuf.
			 */
			if (len < m->m_len) {
				if (top == 0 && len + max_linkhdr <= m->m_len)
					m->m_data += max_linkhdr;
				m->m_len = len;
			} else
				len = m->m_len;
		}
		if (copy)
			copy(cp, mtod(m, caddr_t), (unsigned)len);
		else
			bcopy(cp, mtod(m, caddr_t), (unsigned)len);
		cp += len;
		*mp = m;
		mp = &m->m_next;
		totlen -= len;
		if (cp == epkt)
			cp = buf;
	}
	return (top);
}

#ifdef notyet
memaddr_t miobase;			/* click address of dma region */
							/* this is altered during allocation */
memaddr_t miostart;			/* click address of dma region */
							/* this stays unchanged */
u_short miosize = 16384;	/* two umr's worth */

/*
 * Get more mbufs; called from MGET macro if mfree list is empty.
 * Must be called at splimp.
 */
/*ARGSUSED*/
struct mbuf *
m_more(canwait, type)
	int canwait, type;
{
	register struct mbuf *m;

	while (m_expand(canwait) == 0) {
		if (canwait == M_WAIT) {
			mbstat.m_wait++;
			m_want++;
			sleep((caddr_t)&mfree, PZERO - 1);
		} else {
			mbstat.m_drops++;
			return (NULL);
		}
	}
#define m_more(x,y) ((panic("m_more"), (struct mbuf *)0))
	MGET(m, canwait, type);
#undef m_more
	return (m);
}

/*
 * Allocate a contiguous buffer for DMA IO.  Called from if_ubainit().
 * TODO: fix net device drivers to handle scatter/gather to mbufs
 * on their own; thus avoiding the copy to/from this area.
 */
u_int
m_ioget(size)
	u_int size;				/* Number of bytes to allocate */
{
	memaddr_t base;
	u_int csize;

	csize = btoc(size);		/* size in clicks */
	size = ctob(csize);		/* round up byte size */

	if (size > miosize)
		return(0);
	miosize -= size;
	base = miobase;
	miobase += csize;
	return (base);
}
#endif
