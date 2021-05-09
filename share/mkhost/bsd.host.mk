#	$NetBSD: bsd.host.mk,v 1.5 2020/08/09 21:13:38 christos Exp $

.if !defined(_BSD_HOST_MK_)
_BSD_HOST_MK_=1

.if ${MKTOOLSDEBUG:Uno} == "yes"
HOST_DBG?= -g
.else
HOST_DBG?= -O
.endif

.if ${MKDTRACE:Uno} != "no"
# disable compiler options that interfere with dtrace
HOST_DTRACE_OPTS?=	-fno-omit-frame-pointer -fno-optimize-sibling-calls -fno-ipa-sra
.endif

# Helpers for cross-compiling
HOST_CC?=			cc
HOST_CFLAGS?=		${HOST_DBG}
HOST_COMPILE.c?=	${HOST_CC} ${HOST_CFLAGS} ${HOST_DTRACE_OPTS} ${HOST_CPPFLAGS} -c
HOST_COMPILE.cc?=   ${HOST_CXX} ${HOST_CXXFLAGS} ${HOST_DTRACE_OPTS} ${HOST_CPPFLAGS} -c
HOST_LINK.cc?=  	${HOST_CXX} ${HOST_CXXFLAGS} ${HOST_CPPFLAGS} ${HOST_LDFLAGS}
.if defined(HOSTPROG_CXX)
HOST_LINK.c?=   	${HOST_LINK.cc}
.else
HOST_LINK.c?=		${HOST_CC} ${HOST_CFLAGS} ${HOST_CPPFLAGS} ${HOST_LDFLAGS}
.endif

HOST_CXX?=			c++
HOST_CXXFLAGS?=		${HOST_DBG}

HOST_CPP?=			cpp
HOST_CPPFLAGS?=

HOST_LD?=			ld
HOST_LDFLAGS?=

HOST_AR?=			ar
HOST_RANLIB?=		ranlib

HOST_LN?=			ln

# HOST_SH must be an absolute path
HOST_SH?=			/bin/sh

.if !defined(HOST_OSTYPE)
_HOST_OSNAME!=	uname -s
_HOST_OSREL!=	uname -r
# For _HOST_ARCH, if uname -p fails, or prints "unknown", or prints
# something that does not look like an identifier, then use uname -m.
_HOST_ARCH!=	uname -p 2>/dev/null
_HOST_ARCH:=	${HOST_ARCH:tW:C/.*[^-_A-Za-z0-9].*//:S/unknown//}
.if empty(_HOST_ARCH)
_HOST_ARCH!=	uname -m
.endif
HOST_OSTYPE:=	${_HOST_OSNAME}-${_HOST_OSREL:C/\([^\)]*\)//g:[*]:C/ /_/g}-${_HOST_ARCH:C/\([^\)]*\)//g:[*]:C/ /_/g}
.MAKEOVERRIDES+= HOST_OSTYPE
.endif # !defined(HOST_OSTYPE)

.if ${USETOOLS} == "yes"
HOST_MKDEP?=	${TOOLDIR}/bin/${_TOOL_PREFIX}host-mkdep
HOST_MKDEPCXX?=	${TOOLDIR}/bin/${_TOOL_PREFIX}host-mkdep
.else
HOST_MKDEP?=	CC=${HOST_CC:Q} mkdep
HOST_MKDEPCXX?=	CC=${HOST_CXX:Q} mkdep
.endif

.if ${HOST_OSTYPE:MLinux*}
HOST_CPPFLAGS+=-D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
.endif

.if ${NEED_OWN_INSTALL_TARGET} != "no"
HOST_INSTALL_FILE?=	${INSTALL} ${COPY} ${PRESERVE} ${RENAME}
HOST_INSTALL_DIR?=	${INSTALL} -d
HOST_INSTALL_SYMLINK?=	${INSTALL} ${SYMLINK} ${RENAME}
.endif

.endif