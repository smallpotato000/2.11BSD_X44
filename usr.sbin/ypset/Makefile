#	$NetBSD: Makefile,v 1.10 1997/10/18 06:43:14 lukem Exp $
#	from: @(#)Makefile	5.8 (Berkeley) 7/28/90

USE_FORT?= yes	# network client

PROG=	ypset
MAN=	ypset.8

COPTS.ypset.c+=	${CC_WNO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
