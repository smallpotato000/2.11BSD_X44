# This file is automatically generated.  DO NOT EDIT!
# Generated from: NetBSD: mknative-gdb,v 1.12 2020/09/17 16:54:31 christos Exp 
# Generated from: NetBSD: mknative.common,v 1.16 2018/04/15 15:13:37 christos Exp 
#
G_libbfd_la_DEPENDENCIES=elf32-sh.lo elf-vxworks.lo elf32.lo elf.lo elflink.lo elf-attrs.lo elf-strtab.lo elf-eh-frame.lo dwarf1.lo dwarf2.lo coff-sh.lo cofflink.lo coffgen.lo elf32-gen.lo plugin.lo cpu-sh.lo netbsd-core.lo  ofiles
G_libbfd_la_OBJECTS=archive.lo archures.lo bfd.lo bfdio.lo bfdwin.lo cache.lo coff-bfd.lo compress.lo corefile.lo elf-properties.lo format.lo hash.lo init.lo libbfd.lo linker.lo merge.lo opncls.lo reloc.lo section.lo simple.lo stab-syms.lo stabs.lo syms.lo targets.lo binary.lo ihex.lo srec.lo tekhex.lo verilog.lo
G_DEFS=-DHAVE_CONFIG_H
G_INCLUDES=
G_TDEFAULTS=-DDEFAULT_VECTOR=sh_elf32_nbsd_vec -DSELECT_VECS='&sh_elf32_nbsd_vec,&sh_elf32_nbsd_le_vec,&sh_coff_vec,&sh_coff_le_vec,&elf32_le_vec,&elf32_be_vec' -DSELECT_ARCHITECTURES='&bfd_sh_arch'
