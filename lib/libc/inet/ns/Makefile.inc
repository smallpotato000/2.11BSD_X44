#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	6.1 (Berkeley) 1/29/86
#
.PATH: ${ARCHDIR}/inet/ns ${.CURDIR}/inet/ns

SRCS+=	ns_addr.c ns_ntoa.c

MAN+= 	ns.3
