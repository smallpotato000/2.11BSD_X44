#	$NetBSD: bsd.own.mk,v 1.1247 2021/05/06 13:23:36 rin Exp $
#  
#	Notes: bsd.own.mk only defines parameters related to the toolchain,
#	machine archictechure and compiler.
#

# This needs to be before bsd.init.mk
.if defined(BSD_MK_COMPAT_FILE)
.include <${BSD_MK_COMPAT_FILE}>
.endif

.if !defined(_BSD_OWN_MK_)
_BSD_OWN_MK_=1

MAKECONF?=	/etc/mk.conf
.-include "${MAKECONF}"


#
# Subdirectory or path component used for the following paths:
#   distrib/${RELEASEMACHINE}
#   distrib/notes/${RELEASEMACHINE}
#   etc/etc.${RELEASEMACHINE}
# Used when building a release.
#
RELEASEMACHINE?=	${MACHINE}

#
# NEED_OWN_INSTALL_TARGET is set to "no" by pkgsrc/mk/bsd.pkg.mk to
# ensure that things defined by <bsd.own.mk> (default targets,
# INSTALL_FILE, etc.) are not conflicting with bsd.pkg.mk.
#
NEED_OWN_INSTALL_TARGET?=	yes

#
# This lists the platforms which do not have working in-tree toolchains.  For
# the in-tree gcc toolchain, this list is empty.
#
# If some future port is not supported by the in-tree toolchain, this should
# be set to "yes" for that port only.
#
# .if ${MACHINE} == "playstation2"
# TOOLCHAIN_MISSING?=	yes
# .endif

TOOLCHAIN_MISSING?=	no

#
# GCC Using platforms.
#
.if ${MKGCC:Uyes} != "no"

#
# What GCC is used?
#
.if ${MACHINE_ARCH} == "x86_64"
HAVE_GCC?=	10
.else
HAVE_GCC?=	9
.endif

#
# What binutils is used?
#
HAVE_BINUTILS?=	234
.if ${HAVE_BINUTILS} == 234
EXTERNAL_BINUTILS_SUBDIR=	binutils
.else
EXTERNAL_BINUTILS_SUBDIR=	/does/not/exist
.endif

#
# What GDB is used?
#
HAVE_GDB?=	1100
.if ${HAVE_GDB} == 1100
EXTERNAL_GDB_SUBDIR=		gdb
.else
EXTERNAL_GDB_SUBDIR=		/does/not/exist
.endif

.if empty(.MAKEFLAGS:tW:M*-V .OBJDIR*)
.if defined(MAKEOBJDIRPREFIX) || defined(MAKEOBJDIR)
PRINTOBJDIR=	${MAKE} -r -V .OBJDIR -f /dev/null xxx
.else
PRINTOBJDIR=	${MAKE} -V .OBJDIR
.endif
.else
PRINTOBJDIR=	echo /error/bsd.own.mk/PRINTOBJDIR # avoid infinite recursion
.endif

#
# Determine if running in the NetBSD source tree by checking for the
# existence of build.sh and tools/ in the current or a parent directory,
# and setting _SRC_TOP_ to the result.
#
.if !defined(_SRC_TOP_)			# {
_SRC_TOP_!= cd "${.CURDIR}"; while :; do \
		here=`pwd`; \
		[ -f build.sh  ] && [ -d tools ] && { echo $$here; break; }; \
		case $$here in /) echo ""; break;; esac; \
		cd ..; done

.MAKEOVERRIDES+=	_SRC_TOP_

.endif					# }

#
# If _SRC_TOP_ != "", we're within the NetBSD source tree.
# * Set defaults for NETBSDSRCDIR and _SRC_TOP_OBJ_.
# * Define _NETBSD_VERSION_DEPENDS.  Targets that depend on the
#   NetBSD version, or on variables defined at build time, can
#   declare a dependency on ${_NETBSD_VERSION_DEPENDS}.
#
.if (${_SRC_TOP_} != "")		# {

NETBSDSRCDIR?=	${_SRC_TOP_}

.if !defined(_SRC_TOP_OBJ_)
_SRC_TOP_OBJ_!=		cd "${_SRC_TOP_}" && ${PRINTOBJDIR}
.MAKEOVERRIDES+=	_SRC_TOP_OBJ_
.endif

_NETBSD_VERSION_DEPENDS=	${_SRC_TOP_OBJ_}/params
_NETBSD_VERSION_DEPENDS+=	${NETBSDSRCDIR}/sys/sys/param.h
_NETBSD_VERSION_DEPENDS+=	${NETBSDSRCDIR}/sys/conf/newvers.sh
_NETBSD_VERSION_DEPENDS+=	${NETBSDSRCDIR}/sys/conf/osrelease.sh
${_SRC_TOP_OBJ_}/params: .NOTMAIN .OPTIONAL # created by top level "make build"

.endif	# _SRC_TOP_ != ""		# }

.if (${_SRC_TOP_} != "") && \
    (${TOOLCHAIN_MISSING} == "no" || defined(EXTERNAL_TOOLCHAIN))
USETOOLS?=	yes
.endif
USETOOLS?=	no

#
# Host platform information; may be overridden
#
.include <bsd.host.mk>

.if ${USETOOLS} == "yes"						# {

#
# Provide a default for TOOLDIR.
#
.if !defined(TOOLDIR)
TOOLDIR:=	${_SRC_TOP_OBJ_}/tooldir.${HOST_OSTYPE}
.MAKEOVERRIDES+= TOOLDIR
.endif

#
# This is the prefix used for the NetBSD-sourced tools.
#
_TOOL_PREFIX?=	nb

#
# If an external toolchain base is specified, use it.
#
.if defined(EXTERNAL_TOOLCHAIN)						# {
AR=					${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-ar
AS=					${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-as
LD=					${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-ld
NM=					${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-nm
OBJCOPY=			${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-objcopy
OBJDUMP=			${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-objdump
RANLIB=				${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-ranlib
READELF=			${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-readelf
SIZE=				${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-size
STRINGS=			${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-strings
STRIP=				${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-strip

TOOL_CC.gcc=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-gcc
TOOL_CPP.gcc=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-cpp
TOOL_CXX.gcc=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-c++
TOOL_FC.gcc=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-gfortran
TOOL_OBJC.gcc=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-gcc

TOOL_CC.clang=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-clang
TOOL_CPP.clang=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-clang-cpp
TOOL_CXX.clang=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-clang++
TOOL_OBJC.clang=	${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-clang
.else									# } {
# Define default locations for common tools.
.if ${USETOOLS_BINUTILS:Uyes} == "yes"					#  {
AR=					${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-ar
AS=					${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-as
LD=					${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-ld
NM=					${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-nm
OBJCOPY=			${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-objcopy
OBJDUMP=			${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-objdump
RANLIB=				${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-ranlib
READELF=			${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-readelf
SIZE=				${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-size
STRINGS=			${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-strings
STRIP=				${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-strip

# GCC supports C, C++, Fortran and Objective C
TOOL_CC.gcc=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-gcc
TOOL_CPP.gcc=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-cpp
TOOL_CXX.gcc=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-c++
TOOL_FC.gcc=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-gfortran
TOOL_OBJC.gcc=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-gcc
.endif									#  }

# Clang supports C, C++ and Objective C
TOOL_CC.clang=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-clang
TOOL_CPP.clang=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-clang-cpp
TOOL_CXX.clang=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-clang++
TOOL_OBJC.clang=	${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-clang

# PCC supports C and Fortran
TOOL_CC.pcc=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-pcc
TOOL_CPP.pcc=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-pcpp
TOOL_CXX.pcc=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-p++
.endif	# EXTERNAL_TOOLCHAIN		

#
# Make sure DESTDIR is set, so that builds with these tools always
# get appropriate -nostdinc, -nostdlib, etc. handling.  The default is
# <empty string>, meaning start from /, the root directory.
#
DESTDIR?=

# Don't append another copy of sysroot (coming from COMPATCPPFLAGS etc.)
# because it confuses Coverity. Still we need to cov-configure specially
# for each specific sysroot argument.
# Also don't add a sysroot at all if a rumpkernel build.
.if !defined(HOSTPROG) && !defined(HOSTLIB) && !defined(RUMPRUN)
.  if ${DESTDIR} != ""
.	if empty(CPPFLAGS:M*--sysroot=*)
CPPFLAGS+=	--sysroot=${DESTDIR}
.	endif
LDFLAGS+=	--sysroot=${DESTDIR}
.  else
.	if empty(CPPFLAGS:M*--sysroot=*)
CPPFLAGS+=	--sysroot=/
.	endif
LDFLAGS+=	--sysroot=/
.  endif
.endif

DBSYM=				${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-dbsym
INSTALL=			${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-install
LEX=				${TOOLDIR}/bin/${_TOOL_PREFIX}lex
LINT=				CC=${CC:Q} ${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-lint
LORDER=				NM=${NM:Q} MKTEMP=${TOOL_MKTEMP:Q} ${TOOLDIR}/bin/${_TOOL_PREFIX}lorder
MKDEP=				CC=${CC:Q} ${TOOLDIR}/bin/${_TOOL_PREFIX}mkdep
MKDEPCXX=			CC=${CXX:Q} ${TOOLDIR}/bin/${_TOOL_PREFIX}mkdep
#PAXCTL=			${TOOLDIR}/bin/${_TOOL_PREFIX}paxctl
TSORT=				${TOOLDIR}/bin/${_TOOL_PREFIX}tsort -q
YACC=				${TOOLDIR}/bin/${_TOOL_PREFIX}yacc

TOOL_AWK=			${TOOLDIR}/bin/${_TOOL_PREFIX}awk
TOOL_CAP_MKDB=		${TOOLDIR}/bin/${_TOOL_PREFIX}cap_mkdb
TOOL_CAT=			${TOOLDIR}/bin/${_TOOL_PREFIX}cat
TOOL_CKSUM=			${TOOLDIR}/bin/${_TOOL_PREFIX}cksum
TOOL_CLANG_TBLGEN=	${TOOLDIR}/bin/${_TOOL_PREFIX}clang-tblgen
TOOL_COMPILE_ET=	${TOOLDIR}/bin/${_TOOL_PREFIX}compile_et
TOOL_CONFIG=		${TOOLDIR}/bin/${_TOOL_PREFIX}config
TOOL_CRUNCHGEN=		MAKE=${.MAKE:Q} ${TOOLDIR}/bin/${_TOOL_PREFIX}crunchgen
TOOL_CTAGS=			${TOOLDIR}/bin/${_TOOL_PREFIX}ctags
TOOL_CTFCONVERT=	${TOOLDIR}/bin/${_TOOL_PREFIX}ctfconvert
TOOL_CTFMERGE=		${TOOLDIR}/bin/${_TOOL_PREFIX}ctfmerge
TOOL_CVSLATEST=		${TOOLDIR}/bin/${_TOOL_PREFIX}cvslatest
TOOL_DB=			${TOOLDIR}/bin/${_TOOL_PREFIX}db
TOOL_DISKLABEL=		${TOOLDIR}/bin/${_TOOL_PREFIX}disklabel
TOOL_DTC=			${TOOLDIR}/bin/${_TOOL_PREFIX}dtc
TOOL_EQN=			${TOOLDIR}/bin/${_TOOL_PREFIX}eqn
TOOL_FDISK=			${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-fdisk
TOOL_FGEN=			${TOOLDIR}/bin/${_TOOL_PREFIX}fgen
TOOL_GENASSYM=		${TOOLDIR}/bin/${_TOOL_PREFIX}genassym
TOOL_GENCAT=		${TOOLDIR}/bin/${_TOOL_PREFIX}gencat
TOOL_GMAKE=			${TOOLDIR}/bin/${_TOOL_PREFIX}gmake
TOOL_GPT=			${TOOLDIR}/bin/${_TOOL_PREFIX}gpt
TOOL_GREP=			${TOOLDIR}/bin/${_TOOL_PREFIX}grep
GROFF_SHARE_PATH=	${TOOLDIR}/share/groff
TOOL_GROFF_ENV= 															\
    GROFF_ENCODING= 														\
    GROFF_BIN_PATH=${TOOLDIR}/lib/groff 									\
    GROFF_FONT_PATH=${GROFF_SHARE_PATH}/site-font:${GROFF_SHARE_PATH}/font 	\
    GROFF_TMAC_PATH=${GROFF_SHARE_PATH}/site-tmac:${GROFF_SHARE_PATH}/tmac
TOOL_GROFF=			${TOOL_GROFF_ENV} ${TOOLDIR}/bin/${_TOOL_PREFIX}groff ${GROFF_FLAGS}

TOOL_HEXDUMP=		${TOOLDIR}/bin/${_TOOL_PREFIX}hexdump
TOOL_INDXBIB=		${TOOLDIR}/bin/${_TOOL_PREFIX}indxbib
TOOL_INSTALLBOOT=	${TOOLDIR}/bin/${_TOOL_PREFIX}installboot
TOOL_INSTALL_INFO=	${TOOLDIR}/bin/${_TOOL_PREFIX}install-info
TOOL_JOIN=			${TOOLDIR}/bin/${_TOOL_PREFIX}join
TOOL_LLVM_TBLGEN=	${TOOLDIR}/bin/${_TOOL_PREFIX}llvm-tblgen
TOOL_M4=			${TOOLDIR}/bin/${_TOOL_PREFIX}m4
TOOL_MACPPCFIXCOFF=	${TOOLDIR}/bin/${_TOOL_PREFIX}macppc-fixcoff
TOOL_MAKEFS=		${TOOLDIR}/bin/${_TOOL_PREFIX}makefs
TOOL_MAKEINFO=		${TOOLDIR}/bin/${_TOOL_PREFIX}makeinfo
TOOL_MAKEKEYS=		${TOOLDIR}/bin/${_TOOL_PREFIX}makekeys
TOOL_MAKESTRS=		${TOOLDIR}/bin/${_TOOL_PREFIX}makestrs
TOOL_MAKEWHATIS=	${TOOLDIR}/bin/${_TOOL_PREFIX}makewhatis
TOOL_MANDOC_ASCII=	${TOOLDIR}/bin/${_TOOL_PREFIX}mandoc -Tascii
TOOL_MANDOC_HTML=	${TOOLDIR}/bin/${_TOOL_PREFIX}mandoc -Thtml
TOOL_MANDOC_LINT=	${TOOLDIR}/bin/${_TOOL_PREFIX}mandoc -Tlint
TOOL_MDSETIMAGE=	${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-mdsetimage
TOOL_MENUC=			MENUDEF=${TOOLDIR}/share/misc ${TOOLDIR}/bin/${_TOOL_PREFIX}menuc
TOOL_MKCSMAPPER=	${TOOLDIR}/bin/${_TOOL_PREFIX}mkcsmapper
TOOL_MKESDB=		${TOOLDIR}/bin/${_TOOL_PREFIX}mkesdb
TOOL_MKLOCALE=		${TOOLDIR}/bin/${_TOOL_PREFIX}mklocale
TOOL_MKMAGIC=		${TOOLDIR}/bin/${_TOOL_PREFIX}file
TOOL_MKNOD=			${TOOLDIR}/bin/${_TOOL_PREFIX}mknod
TOOL_MKTEMP=		${TOOLDIR}/bin/${_TOOL_PREFIX}mktemp
TOOL_MKUBOOTIMAGE=	${TOOLDIR}/bin/${_TOOL_PREFIX}mkubootimage
TOOL_ELFTOSB=		${TOOLDIR}/bin/${_TOOL_PREFIX}elftosb
TOOL_MSGC=			MSGDEF=${TOOLDIR}/share/misc ${TOOLDIR}/bin/${_TOOL_PREFIX}msgc
TOOL_MTREE=			${TOOLDIR}/bin/${_TOOL_PREFIX}mtree
TOOL_NBPERF=		${TOOLDIR}/bin/${_TOOL_PREFIX}perf
TOOL_NCDCS=			${TOOLDIR}/bin/${_TOOL_PREFIX}ibmnws-ncdcs
TOOL_PAX=			${TOOLDIR}/bin/${_TOOL_PREFIX}pax
TOOL_PIC=			${TOOLDIR}/bin/${_TOOL_PREFIX}pic
TOOL_PIGZ=			${TOOLDIR}/bin/${_TOOL_PREFIX}pigz
TOOL_XZ=			${TOOLDIR}/bin/${_TOOL_PREFIX}xz
TOOL_PKG_CREATE=	${TOOLDIR}/bin/${_TOOL_PREFIX}pkg_create
TOOL_PWD_MKDB=		${TOOLDIR}/bin/${_TOOL_PREFIX}pwd_mkdb
TOOL_REFER=			${TOOLDIR}/bin/${_TOOL_PREFIX}refer
TOOL_ROFF_ASCII=	${TOOL_GROFF_ENV} ${TOOLDIR}/bin/${_TOOL_PREFIX}nroff
TOOL_ROFF_DOCASCII=	${TOOL_GROFF} -Tascii
TOOL_ROFF_DOCHTML=	${TOOL_GROFF} -Thtml
TOOL_ROFF_DVI=		${TOOL_GROFF} -Tdvi ${ROFF_PAGESIZE}
TOOL_ROFF_HTML=		${TOOL_GROFF} -Tlatin1 -mdoc2html
TOOL_ROFF_PS=		${TOOL_GROFF} -Tps ${ROFF_PAGESIZE}
TOOL_ROFF_RAW=		${TOOL_GROFF} -Z
TOOL_RPCGEN=		RPCGEN_CPP=${CPP:Q} ${TOOLDIR}/bin/${_TOOL_PREFIX}rpcgen
TOOL_SED=			${TOOLDIR}/bin/${_TOOL_PREFIX}sed
TOOL_SLC=			${TOOLDIR}/bin/${_TOOL_PREFIX}slc
TOOL_SOELIM=		${TOOLDIR}/bin/${_TOOL_PREFIX}soelim
TOOL_SORTINFO=		${TOOLDIR}/bin/${_TOOL_PREFIX}sortinfo
TOOL_SPARKCRC=		${TOOLDIR}/bin/${_TOOL_PREFIX}sparkcrc
TOOL_STAT=			${TOOLDIR}/bin/${_TOOL_PREFIX}stat
TOOL_STRFILE=		${TOOLDIR}/bin/${_TOOL_PREFIX}strfile
TOOL_SUNLABEL=		${TOOLDIR}/bin/${_TOOL_PREFIX}sunlabel
TOOL_TBL=			${TOOLDIR}/bin/${_TOOL_PREFIX}tbl
TOOL_TIC=			${TOOLDIR}/bin/${_TOOL_PREFIX}tic
TOOL_UUDECODE=		${TOOLDIR}/bin/${_TOOL_PREFIX}uudecode
TOOL_VGRIND=		${TOOLDIR}/bin/${_TOOL_PREFIX}vgrind -f
TOOL_VFONTEDPR=		${TOOLDIR}/libexec/${_TOOL_PREFIX}vfontedpr
TOOL_ZIC=			${TOOLDIR}/bin/${_TOOL_PREFIX}zic

.else	# USETOOLS != yes						# } {

# Clang supports C, C++ and Objective C
TOOL_CC.clang=		clang
TOOL_CPP.clang=		clang-cpp
TOOL_CXX.clang=		clang++
TOOL_OBJC.clang=	clang

# GCC supports C, C++, Fortran and Objective C
TOOL_CC.gcc=		gcc
TOOL_CPP.gcc=		cpp
TOOL_CXX.gcc=		c++
TOOL_FC.gcc=		gfortran
TOOL_OBJC.gcc=		gcc

# PCC supports C and Fortran
TOOL_CC.pcc=		pcc
TOOL_CPP.pcc=		pcpp
TOOL_CXX.pcc=		p++

# Missing below TOOLDIR from NetBSD's bsd.own.mk

.endif	# USETOOLS != yes						# }

# Standalone code should not be compiled with PIE or CTF
# Should create a better test
.if defined(BINDIR) && ${BINDIR} == "/usr/mdec"
NOPIE=			# defined
NOCTF=			# defined
.elif ${MACHINE} == "sun2"
NOPIE=			# we don't have PIC, so no PIE
.endif

# Fallback to ensure that all variables are defined to something
TOOL_CC.false=		false
TOOL_CPP.false=		false
TOOL_CXX.false=		false
TOOL_FC.false=		false
TOOL_OBJC.false=	false

AVAILABLE_COMPILER?=	${HAVE_PCC:Dpcc} ${HAVE_LLVM:Dclang} ${HAVE_GCC:Dgcc} false

.for _t in CC CPP CXX FC OBJC
ACTIVE_${_t}=	${AVAILABLE_COMPILER:@.c.@ ${ !defined(UNSUPPORTED_COMPILER.${.c.}) && defined(TOOL_${_t}.${.c.}) :? ${.c.} : }@:[1]}
SUPPORTED_${_t}=${AVAILABLE_COMPILER:Nfalse:@.c.@ ${ !defined(UNSUPPORTED_COMPILER.${.c.}) && defined(TOOL_${_t}.${.c.}) :? ${.c.} : }@}
.endfor
# make bugs prevent moving this into the .for loop
CC=		${TOOL_CC.${ACTIVE_CC}}
CPP=	${TOOL_CPP.${ACTIVE_CPP}}
CXX=	${TOOL_CXX.${ACTIVE_CXX}}
FC=		${TOOL_FC.${ACTIVE_FC}}
OBJC=	${TOOL_OBJC.${ACTIVE_OBJC}}

# For each ${MACHINE_CPU}, list the ports that use it.
MACHINES.i386=		i386
MACHINES.x86_64=	amd64

#
# Targets to check if DESTDIR or RELEASEDIR is provided
#
.if !target(check_DESTDIR)
check_DESTDIR: .PHONY .NOTMAIN
.if !defined(DESTDIR)
	@echo "setenv DESTDIR before doing that!"
	@false
.else
	@true
.endif
.endif

.if !target(check_RELEASEDIR)
check_RELEASEDIR: .PHONY .NOTMAIN
.if !defined(RELEASEDIR)
	@echo "setenv RELEASEDIR before doing that!"
	@false
.else
	@true
.endif
.endif


.if ${USETOOLS} == "yes"						# {
#
# Make sure DESTDIR is set, so that builds with these tools always
# get appropriate -nostdinc, -nostdlib, etc. handling.  The default is
# <empty string>, meaning start from /, the root directory.
#
DESTDIR?=
.endif									# }

#
# Build a dynamically linked /bin and /sbin, with the necessary shared
# libraries moved from /usr/lib to /lib and the shared linker moved
# from /usr/libexec to /lib
#
# Note that if the BINDIR is not /bin or /sbin, then we always use the
# non-DYNAMICROOT behavior (i.e. it is only enabled for programs in /bin
# and /sbin).  See <bsd.shlib.mk>.
#
MKDYNAMICROOT?=	yes

# where the system object and source trees are kept; can be configurable
# by the user in case they want them in ~/foosrc and ~/fooobj, for example
BSDSRCDIR?=			/usr/src
BSDOBJDIR?=			/usr/obj
NETBSDSRCDIR?=		${BSDSRCDIR}

BINGRP?=			bin
BINOWN?=			root
BINMODE?=			555
NONBINMODE?=		444
DIRMODE?=			755

SHAREDIR?=			/usr/share
SHAREGRP?=			bin
SHAREOWN?=			root
SHAREMODE?=			${NONBINMODE}

MANDIR?=			/usr/share/man
MANGRP?=			bin
MANOWN?=			root
MANMODE?=			${NONBINMODE}
MANINSTALL?=		maninstall catinstall

LIBDIR?=			/usr/lib
LINTLIBDIR?=		/usr/libdata/lint
LIBGRP?=			${BINGRP}
LIBOWN?=			${BINOWN}
LIBMODE?=			${NONBINMODE}

DOCDIR?=    		/usr/share/doc
DOCGRP?=			bin
DOCOWN?=			root
DOCMODE?=   		${NONBINMODE}

NLSDIR?=			/usr/share/nls
NLSGRP?=			bin
NLSOWN?=			root
NLSMODE?=			${NONBINMODE}

LOCALEDIR?=			/usr/share/locale
LOCALEGRP?=			wheel
LOCALEOWN?=			root
LOCALEMODE?=		${NONBINMODE}

# Data-driven table using make variables to control how 
# toolchain-dependent targets and shared libraries are built
# for different platforms and object formats.
# OBJECT_FMT:		currently either "ELF" or "a.out".
#

OBJECT_FMT=	ELF

#
# If this platform's toolchain is missing, we obviously cannot build it.
#
.if ${TOOLCHAIN_MISSING} != "no"
MKBFD:= no
MKGDB:= no
MKGCC:= no
.endif

#
# If we are using an external toolchain, we can still build the target's
# BFD stuff, but we cannot build GCC's support libraries, since those are
# tightly-coupled to the version of GCC being used.
#
.if defined(EXTERNAL_TOOLCHAIN)
MKGCC:= no
.endif

#
# Location of the file that contains the major and minor numbers of the
# version of a shared library.  If this file exists a shared library
# will be built by <bsd.lib.mk>.
#
SHLIB_VERSION_FILE?= ${.CURDIR}/shlib_version

#
# GNU sources and packages sometimes see architecture names differently.
#
GNU_ARCH.i386=i486
GCC_CONFIG_ARCH.i386=i486
GCC_CONFIG_TUNE.i386=nocona
GCC_CONFIG_TUNE.x86_64=nocona
MACHINE_GNU_ARCH=${GNU_ARCH.${MACHINE_ARCH}:U${MACHINE_ARCH}}

#
# In order to identify NetBSD to GNU packages, we sometimes need
# an "elf" tag for historically a.out platforms.
#
.if ${MACHINE_ARCH} == "i386"
MACHINE_GNU_PLATFORM?=${MACHINE_GNU_ARCH}--netbsdelf
.else
MACHINE_GNU_PLATFORM?=${MACHINE_GNU_ARCH}--netbsd
.endif

TARGETS+=	all clean cleandir depend dependall includes \
			install lint obj regress tags
.PHONY:		all clean cleandir depend dependall distclean includes \
			install lint obj regress tags beforedepend afterdepend \
			beforeinstall afterinstall realinstall realdepend realall \
			subdir-all subdir-install subdir-depend
			
.if ${NEED_OWN_INSTALL_TARGET} != "no"
.if !target(install)
install:		.NOTMAIN beforeinstall subdir-install realinstall afterinstall
beforeinstall:	.NOTMAIN
subdir-install:	.NOTMAIN beforeinstall
realinstall:	.NOTMAIN beforeinstall
afterinstall:	.NOTMAIN subdir-install realinstall
.endif
all:			.NOTMAIN realall subdir-all
subdir-all:		.NOTMAIN
realall:		.NOTMAIN
depend:			.NOTMAIN realdepend subdir-depend
subdir-depend:	.NOTMAIN
realdepend:		.NOTMAIN
distclean:		.NOTMAIN cleandir
cleandir:		.NOTMAIN clean

dependall:		.NOTMAIN realdepend .MAKE
	@cd ${.CURDIR}; ${MAKE} realall
.endif

# Define MKxxx variables (which are either yes or no) for users
# to set in /etc/mk.conf and override on the make commandline.
# These should be tested with `== "no"' or `!= "no"'.
# The NOxxx variables should only be used by Makefiles.
#

MKCATPAGES?=yes

#
# Make the bootloader on supported arches
#
.if ${MACHINE_ARCH} == "i386"
MKBOOT= 	yes
.endif

.if defined(NODOC)
MKDOC=		no
#.elif !defined(MKDOC)
#MKDOC=yes
.else
MKDOC?=		yes
.endif

MKINFO?=	yes

.if defined(NOLINKLIB)
MKLINKLIB=	no
.else
MKLINKLIB?=	yes
.endif
.if ${MKLINKLIB} == "no"
MKPICINSTALL=	no
MKPROFILE=	no
.endif

.if defined(NOLINT)
MKLINT=		no
.else
MKLINT?=	yes
.endif

.if defined(NOMAN)
MKMAN=		no
.else
MKMAN?=		yes
.endif
.if ${MKMAN} == "no"
MKCATPAGES=	no
.endif

.if defined(NONLS)
MKNLS=		no
.else
MKNLS?=		yes
.endif

.if defined(NOOBJ)
MKOBJ=		no
.else
MKOBJ?=		yes
.endif

.if defined(NOPIC)
MKPIC=		no
.else
MKPIC?=		yes
.endif

.if defined(NOPICINSTALL)
MKPICINSTALL=	no
.else
MKPICINSTALL?=	yes
.endif

.if defined(NOPROFILE)
MKPROFILE=	no
.else
MKPROFILE?=	yes
.endif

.if defined(NOSHARE)
MKSHARE=	no
.else
MKSHARE?=	yes
.endif
.if ${MKSHARE} == "no"
MKCATPAGES=	no
MKDOC=		no
MKINFO=		no
MKMAN=		no
MKNLS=		no
.endif

#
# install(1) parameters.
#
COPY?=		-c
.if ${MKUPDATE} == "no"
PRESERVE?=	
.else
PRESERVE?=	-p
.endif
RENAME?=	-r
HRDLINK?=	-l h
SYMLINK?=	-l s

METALOG?=	${DESTDIR}/METALOG
METALOG.add?=	${TOOL_CAT} -l >> ${METALOG}
.if (${_SRC_TOP_} != "")	# only set INSTPRIV if inside ${NETBSDSRCDIR}
.if ${MKUNPRIVED} != "no"
INSTPRIV.unpriv=-U -M ${METALOG} -D ${DESTDIR} -h sha1
.else
INSTPRIV.unpriv=
.endif
INSTPRIV?=		${INSTPRIV.unpriv} -N ${NETBSDSRCDIR}/etc
.endif
SYSPKGTAG?=		${SYSPKG:D-T ${SYSPKG}_pkg}
SYSPKGDOCTAG?=	${SYSPKG:D-T ${SYSPKG}-doc_pkg}
STRIPFLAG?=	

.if ${NEED_OWN_INSTALL_TARGET} != "no"
INSTALL_DIR?=		${INSTALL} ${INSTPRIV} -d
INSTALL_FILE?=		${INSTALL} ${INSTPRIV} ${COPY} ${PRESERVE} ${RENAME}
INSTALL_LINK?=		${INSTALL} ${INSTPRIV} ${HRDLINK} ${RENAME}
INSTALL_SYMLINK?=	${INSTALL} ${INSTPRIV} ${SYMLINK} ${RENAME}
HOST_INSTALL_FILE?=	${INSTALL} ${COPY} ${PRESERVE} ${RENAME}
.endif

#
# MAKEDIRTARGET dir target [extra make(1) params]
#	run "cd $${dir} && ${MAKEDIRTARGETENV} ${MAKE} [params] $${target}", with a pretty message
#
MAKEDIRTARGETENV?=
MAKEDIRTARGET=\
	@_makedirtarget() { \
		dir="$$1"; shift; \
		target="$$1"; shift; \
		case "$${dir}" in \
		/*)	this="$${dir}/"; \
			real="$${dir}" ;; \
		.)	this="${_THISDIR_}"; \
			real="${.CURDIR}" ;; \
		*)	this="${_THISDIR_}$${dir}/"; \
			real="${.CURDIR}/$${dir}" ;; \
		esac; \
		show=$${this:-.}; \
		echo "$${target} ===> $${show%/}$${1:+	(with: $$@)}"; \
		cd "$${real}" \
		&& ${MAKEDIRTARGETENV} ${MAKE} _THISDIR_="$${this}" "$$@" $${target}; \
	}; \
	_makedirtarget
	
#
# MAKEVERBOSE support.  Levels are:
#	0	Minimal output ("quiet")
#	1	Describe what is occurring
#	2	Describe what is occurring and echo the actual command
#	3	Ignore the effect of the "@" prefix in make commands
#	4	Trace shell commands using the shell's -x flag
#		
MAKEVERBOSE?=		2

.if ${MAKEVERBOSE} == 0
_MKMSG?=	@\#
_MKSHMSG?=	: echo
_MKSHECHO?=	: echo
.SILENT:
.elif ${MAKEVERBOSE} == 1
_MKMSG?=	@echo '   '
_MKSHMSG?=	echo '   '
_MKSHECHO?=	: echo
.SILENT:
.else	# MAKEVERBOSE >= 2
_MKMSG?=	@echo '\#  '
_MKSHMSG?=	echo '\#  '
_MKSHECHO?=	echo
.SILENT: __makeverbose_dummy_target__
.endif	# MAKEVERBOSE >= 2
.if ${MAKEVERBOSE} >= 3
.MAKEFLAGS:	-dl
.endif	# ${MAKEVERBOSE} >= 3
.if ${MAKEVERBOSE} >= 4
.MAKEFLAGS:	-dx
.endif	# ${MAKEVERBOSE} >= 4

_MKMSG_BUILD?=		${_MKMSG} "  build "
_MKMSG_CREATE?=		${_MKMSG} " create "
_MKMSG_COMPILE?=	${_MKMSG} "compile "
_MKMSG_FORMAT?=		${_MKMSG} " format "
_MKMSG_INSTALL?=	${_MKMSG} "install "
_MKMSG_LINK?=		${_MKMSG} "   link "
_MKMSG_LEX?=		${_MKMSG} "    lex "
_MKMSG_REMOVE?=		${_MKMSG} " remove "
_MKMSG_YACC?=		${_MKMSG} "   yacc "

_MKSHMSG_CREATE?=	${_MKSHMSG} " create "
_MKSHMSG_INSTALL?=	${_MKSHMSG} "install "

_MKTARGET_BUILD?=	${_MKMSG_BUILD} ${.CURDIR:T}/${.TARGET}
_MKTARGET_CREATE?=	${_MKMSG_CREATE} ${.CURDIR:T}/${.TARGET}
_MKTARGET_COMPILE?=	${_MKMSG_COMPILE} ${.CURDIR:T}/${.TARGET}
_MKTARGET_FORMAT?=	${_MKMSG_FORMAT} ${.CURDIR:T}/${.TARGET}
_MKTARGET_INSTALL?=	${_MKMSG_INSTALL} ${.TARGET}
_MKTARGET_LINK?=	${_MKMSG_LINK} ${.CURDIR:T}/${.TARGET}
_MKTARGET_LEX?=		${_MKMSG_LEX} ${.CURDIR:T}/${.TARGET}
_MKTARGET_REMOVE?=	${_MKMSG_REMOVE} ${.TARGET}
_MKTARGET_YACC?=	${_MKMSG_YACC} ${.CURDIR:T}/${.TARGET}

.endif	# !defined(_BSD_OWN_MK_)