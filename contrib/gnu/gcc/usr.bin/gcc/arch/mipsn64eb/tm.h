/* This file is automatically generated.  DO NOT EDIT! */
/* Generated from: NetBSD: mknative-gcc,v 1.114 2021/04/11 07:35:45 mrg Exp  */
/* Generated from: NetBSD: mknative.common,v 1.16 2018/04/15 15:13:37 christos Exp  */

#ifndef GCC_TM_H
#define GCC_TM_H
#define TARGET_CPU_DEFAULT (((MASK_ABICALLS|MASK_FLOAT64|MASK_SOFT_FLOAT_ABI|MASK_SPLIT_ADDRESSES))|MASK_EXPLICIT_RELOCS)
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
#ifndef MIPS_ABI_DEFAULT
# define MIPS_ABI_DEFAULT ABI_64
#endif
#ifdef IN_GCC
# include "options.h"
# include "insn-constants.h"
# include "config/vxworks-dummy.h"
# include "config/elfos.h"
# include "config/mips/mips.h"
# include "config/mips/elf.h"
# include "config/netbsd.h"
# include "config/netbsd-stdint.h"
# include "config/netbsd-elf.h"
# include "config/mips/netbsd.h"
# include "config/mips/netbsd64.h"
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
