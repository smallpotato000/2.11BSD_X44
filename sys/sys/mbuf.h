/*
 * Copyright (c) 1982, 1986, 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 *	@(#)mbuf.h	7.8.2 (2.11BSD GTE) 12/31/93
 */

#ifndef	_SYS_MBUF_H_
#define	_SYS_MBUF_H_

/*
 * The default values for NMBUFS and NMBCLUSTERS (160 and 12 respectively)
 * result in approximately 32K bytes of buffer memory being allocated to
 * the network.  Taking into account the other data used by the network,
 * this leaves approximately 8K bytes free, so the formula is roughly:
 *
 * (NMBUFS / 8) + NMBCLUSTERS < 40
 */
#define	NMBUFS		170			/* number of mbufs */
#define	MSIZE		128			/* size of an mbuf */

#if CLBYTES > 1024
#define	MCLBYTES	1024
#define	MCLSHIFT	10
#define	MCLOFSET	(MCLBYTES - 1)
#else
#define	MCLBYTES	CLBYTES
#define	MCLSHIFT	CLSHIFT
#define	MCLOFSET	CLOFSET
#endif

#define	MMINOFF		8								/* mbuf header length */
#define	MTAIL		2
#define	MMAXOFF		(MSIZE-MTAIL)					/* offset where data ends */
#define	MLEN		(MSIZE-MMINOFF-MTAIL)			/* mbuf data length */
#define	MHLEN		(MLEN - sizeof(struct pkthdr))	/* data len w/pkthdr */
#define	NMBCLUSTERS	12
#define	NMBPCL		(CLBYTES/MSIZE)					/* # mbufs per cluster */
#define	MINCLSIZE	(MHLEN + MLEN)					/* smallest amount to put in cluster */
/*
 * Macros for type conversion
 */

/* network cluster number to virtual address, and back */
#define	cltom(x) ((struct mbuf *)((int)mbutl + ((x) << MCLSHIFT)))
#define	mtocl(x) (((int)x - (int)mbutl) >> MCLSHIFT)

/* address in mbuf to mbuf head */
#define	dtom(x)		((struct mbuf *)((int)x & ~(MSIZE-1)))

/* mbuf head, to typed data */
#define	mtod(x,t)	((t)((int)(x) + (x)->m_off))

struct mbuf {
	struct	mbuf 		*m_next;		/* next buffer in chain */
	struct	mbuf 		*m_nextpkt;		/* next chain in queue/record */
	struct	mbuf_cont 	*m_cont;		/* next chain in queue/record */
	u_short				m_off;			/* offset of data */
	int					m_len;			/* amount of data in this mbuf */
	short				m_type;			/* mbuf type (0 == free) */
	caddr_t				m_dat;			/* data storage */
	short				m_flags;		/* flags; see below */
	struct	mbuf 		*m_act;			/* link in higher-level mbuf list */
};

/* record/packet header in first mbuf of chain; valid if M_PKTHDR set */
struct	pkthdr {
	struct	ifnet *rcvif;		/* rcv interface */
	int	len;					/* total packet length */
};

/* description of external storage mapped into mbuf, valid if M_EXT set */
struct m_ext {
	caddr_t	ext_buf;			/* start of buffer */
	void	(*ext_free)();		/* free routine if not the usual */
	u_int	ext_size;			/* size of buffer, for ext_free */
};

/* Contents of mbuf: */
struct mbuf_cont {
	struct	mbuf mbuf;
	union {
		struct {
			struct pkthdr MH_pkthdr;	/* M_PKTHDR set */
			union {
				struct	m_ext MH_ext;	/* M_EXT set */
				char	MH_databuf[MHLEN];
			} MH_dat;
		} MH;
		char	M_databuf[MLEN];		/* !M_PKTHDR, !M_EXT */
	} M_dat;
};

#define	mc_next		mbuf.m_next
#define	mc_len		mbuf.m_len
#define	mc_data		mbuf.m_dat
#define	mc_off		mbuf.m_off
#define	mc_type		mbuf.m_type
#define	mc_flags	mbuf.m_flags
#define	mc_nextpkt	mbuf.m_nextpkt
#define	mc_act		mbuf.m_act
#define	mc_pkthdr	M_dat.MH.MH_pkthdr
#define	mc_ext		M_dat.MH.MH_dat.MH_ext
#define	mc_pktdat	M_dat.MH.MH_dat.MH_databuf
#define	mc_dat		M_dat.M_databuf

/* mbuf flags */
#define	M_EXT		0x0001	/* has associated external storage */
#define	M_PKTHDR	0x0002	/* start of record */
#define	M_EOR		0x0004	/* end of record */

/* mbuf pkthdr flags, also in m_flags */
#define	M_BCAST		0x0100	/* send/received as link-level broadcast */
#define	M_MCAST		0x0200	/* send/received as link-level multicast */

/* flags copied when copying m_pkthdr */
#define	M_COPYFLAGS	(M_PKTHDR|M_EOR|M_BCAST|M_MCAST)

/* mbuf types */
#define	MT_FREE		0	/* should be on free list */
#define	MT_DATA		1	/* dynamic (data) allocation */
#define	MT_HEADER	2	/* packet header */
#define	MT_SOCKET	3	/* socket structure */
#define	MT_PCB		4	/* protocol control block */
#define	MT_RTABLE	5	/* routing tables */
#define	MT_HTABLE	6	/* IMP host tables */
#define	MT_ATABLE	7	/* address resolution tables */
#define	MT_SONAME	8	/* socket name */
#define	MT_ZOMBIE	9	/* zombie proc status */
#define	MT_SOOPTS	10	/* socket options */
#define	MT_FTABLE	11	/* fragment reassembly header */
#define	MT_RIGHTS	12	/* access rights */
#define	MT_IFADDR	13	/* interface address */
#define MT_CONTROL	14	/* extra-data protocol message */
#define MT_OOBDATA	15	/* expedited data  */
#define	NMBTYPES	16

/* flags to m_get */
#define	M_DONTWAIT		M_NOWAIT
#define	M_WAIT			M_WAITOK
#define	M_DONTWAITLONG	2

/* flags to m_pgalloc */
#define	MPG_MBUFS		0		/* put new mbufs on free list */
#define	MPG_CLUSTERS	1		/* put new clusters on free list */
#define	MPG_SPACE		2		/* don't free; caller wants space */

/* length to m_copy to copy all */
#define	M_COPYALL	077776

/*
 * mbuf utility macros:
 *
 *	MBUFLOCK(code)
 * prevents a section of code from from being interrupted by network
 * drivers.
 */
#define	MBUFLOCK(code) \
	{ int ms = splimp(); \
	  { code } \
	  splx(ms); \
	}

/*
 * m_pullup will pull up additional length if convenient;
 * should be enough to hold headers of second-level and higher protocols. 
 */
#define	MPULL_EXTRA	32

#define	MGET(m, i, t) \
	{ int ms = splimp(); \
	  if ((m)=mfree) \
		{ if ((m)->m_type != MT_FREE) panic("mget"); (m)->m_type = t; \
		  mbstat.m_mtypes[MT_FREE]--; mbstat.m_mtypes[t]++; \
		  mfree = (m)->m_next; (m)->m_next = 0; \
		  (m)->m_off = MMINOFF; } \
	  else \
		(m) = m_more((((ms&0340) <= 0100) && (i==M_DONTWAIT)) ? M_DONTWAITLONG : i, t); \
	  splx(ms); }
/*
 * Mbuf page cluster macros.
 * MCLALLOC allocates mbuf page clusters.
 * Note that it works only with a count of 1 at the moment.
 * MCLGET adds such clusters to a normal mbuf.
 * m->m_len is set to MCLBYTES upon success, and to MLEN on failure.
 * MCLFREE frees clusters allocated by MCLALLOC.
 */
#ifndef	pdp11
#define	MCLALLOC(m, i) {													\
	  int ms = splimp(); 													\
	  if (mclfree == 0) 													\
		(void)m_clalloc((i), MPG_CLUSTERS, M_DONTWAIT); 					\
	  if ((m)=mclfree) 														\
	     {++mclrefcnt[mtocl(m)];mbstat.m_clfree--;mclfree = (m)->m_next;} 	\
	  splx(ms); 															\
}
#else
#define	MCLALLOC(m, i) {													\
	  int ms = splimp(); 													\
	  if ((m)=mclfree) 														\
	     {++mclrefcnt[mtocl(m)];mbstat.m_clfree--;mclfree = (m)->m_next;} 	\
	  splx(ms);																\
}
#endif
#define	M_HASCL(m)	((m)->m_off >= MSIZE)
#define	MTOCL(m)	((struct mbuf *)(mtod((m), int) &~ MCLOFSET))

#define	MCLGET(m) { 												\
	  struct mbuf *p; 												\
	  MCLALLOC(p, 1); 												\
	  if (p) { 														\
		(m)->m_off = (int)p - (int)(m); 							\
		(m)->m_len = MCLBYTES; 										\
	  } else 														\
		(m)->m_len = MLEN; 											\
}
#define	MCLFREE(m) { 												\
	if (--mclrefcnt[mtocl(m)] == 0) 								\
	    { (m)->m_next = mclfree;mclfree = (m);mbstat.m_clfree++;} 	\
}
#define	MFREE(m, n) {												\
	  int ms = splimp(); 											\
	  if ((m)->m_type == MT_FREE) panic("mfree"); 					\
	  mbstat.m_mtypes[(m)->m_type]--; mbstat.m_mtypes[MT_FREE]++; 	\
	  (m)->m_type = MT_FREE; 										\
	  if (M_HASCL(m)) { 											\
		(n) = MTOCL(m); 											\
		MCLFREE(n); 												\
	  } 															\
	  (n) = (m)->m_next; (m)->m_next = mfree; 						\
	  (m)->m_off = 0; (m)->m_act = 0; mfree = (m); 					\
	  splx(ms); 													\
	  if (m_want) { 												\
		  m_want = 0; 												\
		  WAKEUP((caddr_t)&mfree); 									\
	  } 															\
}

/*
 * Mbuf statistics.
 */
struct mbstat {
	u_short	m_mbufs;			/* mbufs obtained from page pool */
	u_short	m_clusters;			/* clusters obtained from page pool */
	u_short	m_space;			/* interface pages obtained from page pool */
	u_short	m_clfree;			/* free clusters */
	u_short	m_drops;			/* times failed to find space */
	u_short m_wait;				/* times waited for space */
	u_short m_drain;			/* times drained protocols for space */
	u_short	m_mtypes[NMBTYPES];	/* type specific mbuf allocations */
};

extern	struct	mbuf *mbutl;		/* virtual address of net free mem */
struct	mbstat mbstat;
int		nmbclusters;
struct	mbuf *mfree, *mclfree;
char	mclrefcnt[NMBCLUSTERS + 1];
int		m_want;
//struct	mbuf *m_get(),*m_getclr(),*m_free(),*m_more(),*m_copy(),*m_pullup();
#ifndef	pdp11
caddr_t	m_clalloc();
#endif

#endif /* _SYS_MBUF_H_ */
