# This file is automatically generated.  DO NOT EDIT!
# Generated from: NetBSD: mknative-gcc,v 1.113 2021/04/11 01:44:14 mrg Exp 
# Generated from: NetBSD: mknative.common,v 1.16 2018/04/15 15:13:37 christos Exp 
#
G_INCLUDES=-I. -I. -I../.././gcc -I${GNUHOSTDIST}/libgcc -I${GNUHOSTDIST}/libgcc/. -I${GNUHOSTDIST}/libgcc/../gcc -I${GNUHOSTDIST}/libgcc/../include 
G_INTERNAL_CFLAGS=-g -O2 -O2  -DIN_GCC    -W -Wall -Wno-narrowing -Wwrite-strings -Wcast-qual -Wno-error=format-diag -Wstrict-prototypes -Wmissing-prototypes -Wno-error=format-diag -Wold-style-definition  -isystem ./include   -fPIC -g -DIN_LIBGCC2 -fbuilding-libgcc -fno-stack-protector   -fPIC -I. -I. -I../.././gcc -I${GNUHOSTDIST}/libgcc -I${GNUHOSTDIST}/libgcc/. -I${GNUHOSTDIST}/libgcc/../gcc -I${GNUHOSTDIST}/libgcc/../include  -DHAVE_CC_TLS 
G_LIB2ADD= ${GNUHOSTDIST}/libgcc/soft-fp/addtf3.c ${GNUHOSTDIST}/libgcc/soft-fp/divtf3.c ${GNUHOSTDIST}/libgcc/soft-fp/eqtf2.c ${GNUHOSTDIST}/libgcc/soft-fp/getf2.c ${GNUHOSTDIST}/libgcc/soft-fp/letf2.c ${GNUHOSTDIST}/libgcc/soft-fp/multf3.c ${GNUHOSTDIST}/libgcc/soft-fp/negtf2.c ${GNUHOSTDIST}/libgcc/soft-fp/subtf3.c ${GNUHOSTDIST}/libgcc/soft-fp/unordtf2.c ${GNUHOSTDIST}/libgcc/soft-fp/fixtfsi.c ${GNUHOSTDIST}/libgcc/soft-fp/fixunstfsi.c ${GNUHOSTDIST}/libgcc/soft-fp/floatsitf.c ${GNUHOSTDIST}/libgcc/soft-fp/floatunsitf.c ${GNUHOSTDIST}/libgcc/soft-fp/extendsftf2.c ${GNUHOSTDIST}/libgcc/soft-fp/extenddftf2.c ${GNUHOSTDIST}/libgcc/soft-fp/trunctfsf2.c ${GNUHOSTDIST}/libgcc/soft-fp/trunctfdf2.c ${GNUHOSTDIST}/libgcc/soft-fp/fixtfdi.c ${GNUHOSTDIST}/libgcc/soft-fp/fixunstfdi.c ${GNUHOSTDIST}/libgcc/soft-fp/floatditf.c ${GNUHOSTDIST}/libgcc/soft-fp/floatunditf.c enable-execute-stack.c
G_LIB2ADDEH=${GNUHOSTDIST}/libgcc/unwind-dw2.c ${GNUHOSTDIST}/libgcc/unwind-dw2-fde-dip.c ${GNUHOSTDIST}/libgcc/unwind-sjlj.c ${GNUHOSTDIST}/libgcc/unwind-c.c ${GNUHOSTDIST}/libgcc/emutls.c
G_LIB2ADD_ST=
G_LIB1ASMFUNCS=
G_LIB1ASMSRC=
G_LIB2_DIVMOD_FUNCS=_divdi3 _moddi3 _divmoddi4 _udivdi3 _umoddi3 _udivmoddi4 _udiv_w_sdiv
G_LIB2FUNCS_ST=_eprintf __gcc_bcmp
G_LIB2FUNCS_EXTRA=
G_LIBGCC2_CFLAGS=-O2  -DIN_GCC    -W -Wall -Wno-narrowing -Wwrite-strings -Wcast-qual -Wno-error=format-diag -Wstrict-prototypes -Wmissing-prototypes -Wno-error=format-diag -Wold-style-definition  -isystem ./include   -fPIC -g -DIN_LIBGCC2 -fbuilding-libgcc -fno-stack-protector 
G_SHLIB_MKMAP=${GNUHOSTDIST}/libgcc/mkmap-symver.awk
G_SHLIB_MKMAP_OPTS=
G_SHLIB_MAPFILES=libgcc-std.ver
G_SHLIB_NM_FLAGS=-pg
G_NOEXCEPTION_FLAGS=-fno-exceptions -fno-rtti -fasynchronous-unwind-tables
G_EXTRA_HEADERS=${GNUHOSTDIST}/gcc/ginclude/tgmath.h
