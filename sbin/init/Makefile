#
# Public Domain.  1997/1/10 - Steven Schultz
#
#	@(#)Makefile	1.0 (2.11BSD) 1997/1/10
#

PROG=	init
SRCS=	init.c
MAN=	init.8

DPADD=	${LIBUTIL}
LDADD=	-lutil

COPTS.init.c+=	${GCC_NO_FORMAT_TRUNCATION} ${GCC_NO_STRINGOP_TRUNCATION} ${GCC_NO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.
