#	$NetBSD: Makefile.boot,v 1.23 2020/10/25 13:25:19 rillig Exp $
#
# A very simple makefile...
#
# You only want to use this if you aren't running NetBSD.
#
# Modify MACHINE and MACHINE_ARCH as appropriate for your target architecture.
# See config.h and the various #ifdef directives for further configuration.
#

CC=gcc -O -g

.c.o:
		${CC} ${CFLAGS} -c $< -o $@

MACHINE=i386
MACHINE_ARCH=i386
CPPFLAGS= 	-DTARGET_MACHINE=\"${MACHINE}\" \
			-DTARGET_MACHINE_ARCH=\"${MACHINE_ARCH}\" \
			-DMAKE_MACHINE=\"${MACHINE}\"

OBJ=	arch.o buf.o compat.o cond.o dir.o for.o hash.o job.o main.o make.o \
    	parse.o str.o suff.o targ.o var.o util.o

LIBOBJ= lst.lib/lstAppend.o lst.lib/lstAtEnd.o lst.lib/lstAtFront.o \
		lst.lib/lstClose.o lst.lib/lstConcat.o lst.lib/lstDatum.o \
		lst.lib/lstDeQueue.o lst.lib/lstDestroy.o lst.lib/lstDupl.o \
		lst.lib/lstEnQueue.o lst.lib/lstFind.o lst.lib/lstFindFrom.o \
		lst.lib/lstFirst.o lst.lib/lstForEach.o lst.lib/lstForEachFrom.o \
		lst.lib/lstInit.o lst.lib/lstInsert.o lst.lib/lstIsAtEnd.o \
		lst.lib/lstIsEmpty.o lst.lib/lstLast.o lst.lib/lstMember.o \
		lst.lib/lstNext.o lst.lib/lstOpen.o lst.lib/lstRemove.o \
		lst.lib/lstReplace.o lst.lib/lstSucc.o

bmake: ${OBJ} ${LIBOBJ}
#	@echo 'make of make and make.0 started.'
	${CC} ${CFLAGS} ${OBJ} ${LIBOBJ} -o bmake ${LIBS}
	@ls -l $@
#	nroff -h -man make.1 > make.0
#	@echo 'make of make and make.0 completed.'

clean:
	rm -f ${OBJ} ${LIBOBJ} ${PORTOBJ} bmake