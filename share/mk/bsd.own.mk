#	$OpenBSD: bsd.own.mk,v 1.204 2020/07/20 08:14:53 kettenis Exp $
#	$NetBSD: bsd.own.mk,v 1.24 1996/04/13 02:08:09 thorpej Exp $

.if defined(MAKECONF) && exists(${MAKECONF})
.include "${MAKECONF}"
.elif exists(/etc/mk.conf)
.include "/etc/mk.conf"
.endif

# Set `WARNINGS' to `yes' to add appropriate warnings to each compilation
WARNINGS?=		no
# Defining `SKEY' causes support for S/key authentication to be compiled in.
SKEY=			yes

CLANG_ARCH=		i386
LLD_ARCH=		i386 

PIE_ARCH=		i386 
STATICPIE_ARCH=	i386

.for _arch in ${MACHINE_ARCH}
.if !empty(GCC3_ARCH:M${_arch})
COMPILER_VERSION?=gcc3
.elif !empty(GCC4_ARCH:M${_arch})
COMPILER_VERSION?=gcc4
.elif !empty(CLANG_ARCH:M${_arch})
COMPILER_VERSION?=clang
.endif

.if !empty(GCC3_ARCH:M${_arch})
BUILD_GCC3?=yes
.else
BUILD_GCC3?=no
.endif
.if !empty(GCC4_ARCH:M${_arch}) || ${MACHINE_ARCH} == "amd64" || \
    ${MACHINE_ARCH} == "mips64" || ${MACHINE_ARCH} == "powerpc"
BUILD_GCC4?=yes
.else
BUILD_GCC4?=no
.endif
.if !empty(CLANG_ARCH:M${_arch})
BUILD_CLANG?=yes
.else
BUILD_CLANG?=no
.endif

.if !empty(LLD_ARCH:M${_arch})
LINKER_VERSION?=lld
.else
LINKER_VERSION?=bfd
.endif

.if !empty(STATICPIE_ARCH:M${_arch})
STATICPIE?=-pie
.endif

.if !empty(PIE_ARCH:M${_arch})
NOPIE_FLAGS?=-fno-pie
NOPIE_LDFLAGS?=-nopie
PIE_DEFAULT?=${DEFAULT_PIE_DEF}
.else
NOPIE_FLAGS?=
PIE_DEFAULT?=
.endif
.endfor

# where the system object and source trees are kept; can be configurable
# by the user in case they want them in ~/foosrc and ~/fooobj, for example
BSDSRCDIR?=		/usr/src
BSDOBJDIR?=		/usr/obj

BINGRP?=		bin
BINOWN?=		root
BINMODE?=		555
NONBINMODE?=	444
DIRMODE?=		755

SHAREDIR?=		/usr/share
SHAREGRP?=		bin
SHAREOWN?=		root
SHAREMODE?=		${NONBINMODE}

MANDIR?=		/usr/share/man
MANGRP?=		bin
MANOWN?=		root
MANMODE?=		${NONBINMODE}
MANINSTALL?=	maninstall catinstall

LIBDIR?=		/usr/lib
LINTLIBDIR?=	/usr/libdata/lint
LIBGRP?=		${BINGRP}
LIBOWN?=		${BINOWN}
LIBMODE?=		${NONBINMODE}

DOCDIR?=    	/usr/share/doc
DOCGRP?=		bin
DOCOWN?=		root
DOCMODE?=   	${NONBINMODE}

NLSDIR?=		/usr/share/nls
NLSGRP?=		bin
NLSOWN?=		root
NLSMODE?=		${NONBINMODE}

LOCALEDIR?=		/usr/share/locale
LOCALEGRP?=		wheel
LOCALEOWN?=		root
LOCALEMODE?=	${NONBINMODE}

.if !defined(CDIAGFLAGS)
CDIAGFLAGS=		-Wall -Wpointer-arith -Wuninitialized -Wstrict-prototypes
CDIAGFLAGS+=	-Wmissing-prototypes -Wunused -Wsign-compare
CDIAGFLAGS+=	-Wshadow
.  if ${COMPILER_VERSION} == "gcc4"
CDIAGFLAGS+=	-Wdeclaration-after-statement
.  endif
.endif

# Shared files for system gnu configure, not used yet
GNUSYSTEM_AUX_DIR?=${BSDSRCDIR}/share/gnu

INSTALL_COPY?=	-c
.ifndef DEBUG
INSTALL_STRIP?=	-s
.endif

STATIC?=	-static ${STATICPIE}

# Define SYS_INCLUDE to indicate whether you want symbolic links to the system
# source (``symlinks''), or a separate copy (``copies''); (latter useful
# in environments where it's not possible to keep /sys publicly readable)
#SYS_INCLUDE= 	symlinks

# pic relocation flags.
.if ${MACHINE_ARCH} == "alpha" || ${MACHINE_ARCH} == "powerpc" || \
    ${MACHINE_ARCH} == "sparc64"
PICFLAG?=-fPIC
.else
PICFLAG?=-fpic
.endif

.if ${MACHINE_ARCH} == "alpha" || ${MACHINE_ARCH} == "powerpc" || \
    ${MACHINE_ARCH} == "sparc64"
# big PIE
DEFAULT_PIE_DEF=-DPIE_DEFAULT=2
.else
# small pie
DEFAULT_PIE_DEF=-DPIE_DEFAULT=1
.endif

# don't try to generate PROFILED versions of libraries on machines
# which don't support profiling.
.if 0
NOPROFILE=
.endif

BUILDUSER?= build
WOBJGROUP?= wobj
WOBJUMASK?= 007

BSD_OWN_MK=Done

.PHONY: spell clean cleandir obj manpages print all \
		depend beforedepend afterdepend cleandepend subdirdepend \
		all cleanman includes \
		beforeinstall realinstall maninstall afterinstall install
