#
#	@(#)Makefile	1.2 (2.11BSD GTE) 1996/3/26
#	$NetBSD: Makefile,v 1.7 2016/01/23 21:22:46 christos Exp $
# This is the Makefile for 'libkern.a'.  These are the specially compiled
# versions of libc.a routines which the kernel and network use.
#
# There are no source files in this directory.  Links are made to the files.
#
# libkern.a needs to be installed only once (usually when the 
# C library is installed) rather than compiling the sources each time 
# a kernel is compiled..


.include <bsd.obj.mk>		# Pull in OBJDIR name rules.
.include <bsd.own.mk>
#.include <bsd.klinks.mk>

LIBISPRIVATE?=	yes
LIB=			kern
DBG=			-Os

S= ${NETBSDSRCDIR}/sys
CPPFLAGS+= -nostdinc -I${.OBJDIR} -I${S} -I${S}/arch
CPPFLAGS+= -D_STANDALONE

COPTS+=	-ffreestanding
COPTS+=	-fno-stack-protector
COPTS+=	-fno-unwind-tables
CWARNFLAGS+= -Werror
CWARNFLAGS+= -Wall -Wmissing-prototypes -Wstrict-prototypes -Wpointer-arith
CPPFLAGS+= -D_KERNTYPES

### find out what to use for libkern
KERN_AS=	library
KERNDIR=	${S}/lib/libkern

.include "${KERNDIR}/Makefile.libkern"
.ifndef ARCHSUBDIR
.BEGIN:
		@echo no ARCHSUBDIR for ${MACHINE_ARCH} nor ${MACHINE_CPU}
		@false
.endif
.include <bsd.lib.mk>
