#	$NetBSD: Makefile,v 1.4 2002/12/08 20:20:01 thorpej Exp $

HOSTPROGNAME=	${_TOOL_PREFIX}lex
HOST_SRCDIR=	contrib/flex/bin

.include "${.CURDIR}/../Makefile.host"
HOST_CPPFLAGS+=	-DM4=\"${TOOL_M4}\"

.y.c .l.c .y.h:
	@true


__BUILD: .USE
	(echo '#include "nbtool_config.h"'; \
	echo "#include <init${.TARGET}>") > ${.TARGET}

scan.c: __BUILD
parse.c: __BUILD
parse.h: __BUILD

scan.c: parse.h
parse.h: Makefile

CLEANFILES+=scan.c parse.c parse.h
