/*
 * Copyright (c) 1980,1983,1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 *	@(#)netdb.h	5.9.1 (2.11BSD GTE) 12/31/93
 */

#ifndef _NETDB_H_
#define	_NETDB_H_

#include <sys/cdefs.h>
#include <machine/endian.h>
#include <sys/ansi.h>
#include <inttypes.h>

/*
 * Data types
 */

#ifdef  _BSD_SIZE_T_
typedef _BSD_SIZE_T_	size_t;
#undef  _BSD_SIZE_T_
#endif

#define	_PATH_HEQUIV	"/etc/hosts.equiv"
#define	_PATH_HOSTS		"/etc/hosts"
#define	_PATH_NETWORKS	"/etc/networks"
#define	_PATH_PROTOCOLS	"/etc/protocols"
#define	_PATH_SERVICES	"/etc/services"

/*
 * Structures returned by network
 * data base library.  All addresses
 * are supplied in host order, and
 * returned in network order (suitable
 * for use in system calls).
 */
struct	hostent {
	char		*h_name;			/* official name of host */
	char		**h_aliases;		/* alias list */
	int			h_addrtype;			/* host address type */
	int			h_length;			/* length of address */
	char		**h_addr_list;		/* list of addresses from name server */
#define	h_addr	h_addr_list[0]		/* address, for backward compatiblity */
};

/*
 * Assumption here is that a network number
 * fits in 32 bits -- probably a poor one.
 */
struct	netent {
	char			*n_name;		/* official name of net */
	char			**n_aliases;	/* alias list */
	int				n_addrtype;		/* net address type */
	unsigned long	n_net;			/* network # */
};

struct	servent {
	char		*s_name;		/* official service name */
	char		**s_aliases;	/* alias list */
	int			s_port;			/* port # */
	char		*s_proto;		/* protocol to use */
};

struct	protoent {
	char		*p_name;		/* official protocol name */
	char		**p_aliases;	/* alias list */
	int			p_proto;		/* protocol # */
};

/*
 * Error return codes from gethostbyname() and gethostbyaddr()
 * (left in extern int h_errno).
 */

#define	HOST_NOT_FOUND	1 /* Authoritative Answer Host not found */
#define	TRY_AGAIN		2 /* Non-Authoritive Host not found, or SERVERFAIL */
#define	NO_RECOVERY		3 /* Non recoverable errors, FORMERR, REFUSED, NOTIMP */
#define	NO_DATA			4 /* Valid name, no data record of requested type */
#define	NO_ADDRESS		NO_DATA		/* no address, look for MX record */

unsigned long	gethostid();

__BEGIN_DECLS
void			endhostent(void);
void			endnetent(void);
void			endprotoent(void);
void			endservent(void);
struct hostent	*gethostbyaddr(const char *, int, int);
struct hostent	*gethostbyname(const char *);
struct hostent	*gethostent(void);
struct netent	*getnetbyaddr(long, int); /* u_long? */
struct netent	*getnetbyname(const char *);
struct netent	*getnetent(void);
struct servent	*getservbyname(const char *, const char *);
struct servent	*getservbyport(int, const char *);
struct servent	*getservent(void);
struct protoent	*getprotobyname(const char *);
struct protoent	*getprotobynumber(int);
struct protoent	*getprotoent(void);
void			herror(const char *);
void			sethostent(int);
void			setnetent(int);
void			setprotoent(int);
void			setservent(int);
__END_DECLS

#endif /* !_NETDB_H_ */
