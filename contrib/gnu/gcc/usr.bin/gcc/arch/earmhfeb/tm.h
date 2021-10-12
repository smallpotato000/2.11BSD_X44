/* This file is automatically generated.  DO NOT EDIT! */
/* Generated from: NetBSD: mknative-gcc,v 1.114 2021/04/11 07:35:45 mrg Exp  */
/* Generated from: NetBSD: mknative.common,v 1.16 2018/04/15 15:13:37 christos Exp  */

#ifndef GCC_TM_H
#define GCC_TM_H
#define TARGET_CPU_DEFAULT ("arm10e")
#ifndef LIBC_GLIBC
# define LIBC_GLIBC 1
#endif
#ifndef LIBC_UCLIBC
# define LIBC_UCLIBC 2
#endif
#ifndef LIBC_BIONIC
# define LIBC_BIONIC 3
#endif
#ifndef LIBC_MUSL
# define LIBC_MUSL 4
#endif
#ifndef TARGET_BIG_ENDIAN_DEFAULT
# define TARGET_BIG_ENDIAN_DEFAULT 1
#endif
#ifndef TARGET_DEFAULT_FLOAT_ABI
# define TARGET_DEFAULT_FLOAT_ABI ARM_FLOAT_ABI_HARD
#endif
#ifdef IN_GCC
# include "options.h"
# include "insn-constants.h"
# include "config/vxworks-dummy.h"
# include "config/dbxelf.h"
# include "config/elfos.h"
# include "config/netbsd.h"
# include "config/netbsd-stdint.h"
# include "config/netbsd-elf.h"
# include "config/arm/elf.h"
# include "config/arm/bpabi.h"
# include "config/arm/netbsd-elf.h"
# include "config/arm/netbsd-eabi.h"
# include "config/arm/aout.h"
# include "config/arm/arm.h"
# include "config/initfini-array.h"
#endif
#if defined IN_GCC && !defined GENERATOR_FILE && !defined USED_FOR_TARGET
# include "insn-flags.h"
#endif
#if defined IN_GCC && !defined GENERATOR_FILE
# include "insn-modes.h"
#endif
# include "defaults.h"
#endif /* GCC_TM_H */
