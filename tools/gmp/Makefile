#	$NetBSD: Makefile,v 1.4 2020/09/26 20:56:53 mrg Exp $
#

GNUHOSTDIST=   ${.CURDIR}/../../contrib/gnu/gmp/dist

CONFIGURE_ARGS+=--disable-shared

.include "${.CURDIR}/../Makefile.gnuhost"

# Don't override CFLAGS and CXXFLAGS, it breaks ABI detection.
# PATH needs special treating since "make" tries to expand $PATH
# when variable is used.
CONFIGURE_ENV:=	${CONFIGURE_ENV:NPATH=*:NCFLAGS=*:NCXXFLAGS=*} \
		PATH="${TOOLDIR:Q}/bin:$$PATH"

# Force avoiding possibly non-executable install-sh.
CONFIGURE_ENV+=	ac_cv_path_mkdir="${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-install -d"

# mknative-gmp support
#
# XXX early version; does not use mknative.common yet.

native-gmp: .native/.configure_done
	@echo 'Extracting GNU GMP configury for a native toolchain.'
	(cd ${.OBJDIR}/.native && \
	 ${MAKE} -f ${.CURDIR}/../../contrib/gnu/gmp/Makefile.netbsd-gmp copy-files)
	@echo 'HA HA, only joking. Manual fixes maybe needed now.'

.native/.configure_done: ${_GNU_CFGSRC} ${.CURDIR}/Makefile
	mkdir .native 2>/dev/null || true
	(cd ${.OBJDIR}/.native && \
	 ${MAKE} -f ${.CURDIR}/../../contrib/gnu/gmp/Makefile.netbsd-gmp all)
