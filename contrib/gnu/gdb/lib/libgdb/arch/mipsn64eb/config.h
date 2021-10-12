/* This file is automatically generated.  DO NOT EDIT! */
/* Generated from: NetBSD: mknative-gdb,v 1.15 2020/12/05 21:27:31 christos Exp  */
/* Generated from: NetBSD: mknative.common,v 1.16 2018/04/15 15:13:37 christos Exp  */

/* config.h.  Generated from config.in by configure.  */
/* config.in.  Generated from configure.ac by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Directories from which to load auto-loaded scripts. */
#define AUTO_LOAD_DIR "$debugdir:$datadir/auto-load"

/* Directories safe to hold auto-loaded files. */
#define AUTO_LOAD_SAFE_PATH "$debugdir:$datadir/auto-load"

/* Directory of programs. */
#define BINDIR "/usr/bin"

/* Define to one of `_getb67', `GETB67', `getb67' for Cray-2 and Cray-YMP
   systems. This function is required for `alloca.c' support on those systems.
   */
/* #undef CRAY_STACKSEG_END */

/* Define to 1 if std::thread works. */
#define CXX_STD_THREAD 1

/* Define to 1 if using `alloca.c'. */
/* #undef C_ALLOCA */

/* look for global separate debug info in this path [LIBDIR/debug] */
#define DEBUGDIR "/usr/libdata/debug"

/* Define if the separate-debug-dir directory should be relocated when GDB is
   moved. */
#define DEBUGDIR_RELOCATABLE 1

/* Define to BFD's default architecture. */
#define DEFAULT_BFD_ARCH bfd_mips_arch

/* Define to BFD's default target vector. */
#define DEFAULT_BFD_VEC mips_elf32_trad_be_vec

/* Handle .ctf type-info sections */
#define ENABLE_LIBCTF 1

/* Define to 1 if translation of program messages to the user's native
   language is requested. */
#define ENABLE_NLS 1

/* The .gdbinit filename. */
#define GDBINIT ".gdbinit"

/* look for global separate data files in this path [DATADIR/gdb] */
#define GDB_DATADIR "/usr/share/gdb"

/* Define if the gdb-datadir directory should be relocated when GDB is moved.
   */
#define GDB_DATADIR_RELOCATABLE 1

/* Define to be a string naming the default host character set. */
#define GDB_DEFAULT_HOST_CHARSET "UTF-8"

/* Host double floatformat */
#define GDB_HOST_DOUBLE_FORMAT 0

/* Host float floatformat */
#define GDB_HOST_FLOAT_FORMAT 0

/* Host long double floatformat */
#define GDB_HOST_LONG_DOUBLE_FORMAT 0

/* nativefile */
/* #undef GDB_NM_FILE */

/* Define to the default OS ABI for this configuration. */
/* #undef GDB_OSABI_DEFAULT */

/* Define if self-testing features should be enabled */
#define GDB_SELF_TEST 1

/* Define to 1 if you have `alloca', as a function or macro. */
#define HAVE_ALLOCA 1

/* Define to 1 if you have <alloca.h> and it should be used (not on Ultrix).
   */
/* #undef HAVE_ALLOCA_H */

/* Define to 1 if you have the `btowc' function. */
#define HAVE_BTOWC 1

/* Define to 1 if you have the <cursesX.h> header file. */
/* #undef HAVE_CURSESX_H */

/* Define to 1 if you have the <curses.h> header file. */
#define HAVE_CURSES_H 1

/* define if the compiler supports basic C++11 syntax */
#define HAVE_CXX11 1

/* Define to 1 if you have the declaration of `ADDR_NO_RANDOMIZE', and to 0 if
   you don't. */
#define HAVE_DECL_ADDR_NO_RANDOMIZE 0

/* Define to 1 if you have the declaration of `asprintf', and to 0 if you
   don't. */
#define HAVE_DECL_ASPRINTF 1

/* Define to 1 if you have the declaration of `basename(char *)', and to 0 if
   you don't. */
#define HAVE_DECL_BASENAME 0

/* Define to 1 if you have the declaration of `ffs', and to 0 if you don't. */
#define HAVE_DECL_FFS 1

/* Define to 1 if you have the declaration of `getthrds', and to 0 if you
   don't. */
/* #undef HAVE_DECL_GETTHRDS */

/* Define to 1 if you have the declaration of `snprintf', and to 0 if you
   don't. */
#define HAVE_DECL_SNPRINTF 1

/* Define to 1 if you have the declaration of `strstr', and to 0 if you don't.
   */
#define HAVE_DECL_STRSTR 1

/* Define to 1 if you have the declaration of `strtol', and to 0 if you don't.
   */
#define HAVE_DECL_STRTOL 1

/* Define to 1 if you have the declaration of `strtoll', and to 0 if you
   don't. */
#define HAVE_DECL_STRTOLL 1

/* Define to 1 if you have the declaration of `strtoul', and to 0 if you
   don't. */
#define HAVE_DECL_STRTOUL 1

/* Define to 1 if you have the declaration of `strtoull', and to 0 if you
   don't. */
#define HAVE_DECL_STRTOULL 1

/* Define to 1 if you have the declaration of `strverscmp', and to 0 if you
   don't. */
#define HAVE_DECL_STRVERSCMP 0

/* Define to 1 if you have the declaration of `vasprintf', and to 0 if you
   don't. */
#define HAVE_DECL_VASPRINTF 1

/* Define to 1 if you have the declaration of `vsnprintf', and to 0 if you
   don't. */
#define HAVE_DECL_VSNPRINTF 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define if ELF support should be included. */
#define HAVE_ELF 1

/* Define if <sys/procfs.h> has elf_fpregset_t. */
/* #undef HAVE_ELF_FPREGSET_T */

/* Define to 1 if you have the <elf_hp.h> header file. */
/* #undef HAVE_ELF_HP_H */

/* Define to 1 if your system has the etext variable. */
#define HAVE_ETEXT 1

/* Define to 1 if you have the `fdwalk' function. */
/* #undef HAVE_FDWALK */

/* Define to 1 if you have the `fork' function. */
#define HAVE_FORK 1

/* Define if <sys/procfs.h> has fpregset_t. */
/* #undef HAVE_FPREGSET_T */

/* Define to 1 if you have the `getauxval' function. */
/* #undef HAVE_GETAUXVAL */

/* Define to 1 if you have the `getgid' function. */
#define HAVE_GETGID 1

/* Define to 1 if you have the `getpagesize' function. */
#define HAVE_GETPAGESIZE 1

/* Define to 1 if you have the `getpgid' function. */
#define HAVE_GETPGID 1

/* Define to 1 if you have the `getrlimit' function. */
#define HAVE_GETRLIMIT 1

/* Define to 1 if you have the `getrusage' function. */
#define HAVE_GETRUSAGE 1

/* Define to 1 if you have the `getuid' function. */
#define HAVE_GETUID 1

/* Define if <sys/procfs.h> has gregset_t. */
/* #undef HAVE_GREGSET_T */

/* Define if Guile interpreter is being linked in. */
/* #undef HAVE_GUILE */

/* Define if Guile supports manual finalization. */
/* #undef HAVE_GUILE_MANUAL_FINALIZATION */

/* Define if you have the iconv() function. */
#define HAVE_ICONV 1

/* Define to 1 if you have the `iconvlist' function. */
/* #undef HAVE_ICONVLIST */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if your system has the kinfo_getfile function. */
/* #undef HAVE_KINFO_GETFILE */

/* Define to 1 if your system has the kinfo_getvmmap function. */
#define HAVE_KINFO_GETVMMAP 1

/* Define if you have <langinfo.h> and nl_langinfo(CODESET). */
#define HAVE_LANGINFO_CODESET 1

/* Define if your <locale.h> file defines LC_MESSAGES. */
#define HAVE_LC_MESSAGES 1

/* Define if you have the babeltrace library. */
/* #undef HAVE_LIBBABELTRACE */

/* Define to 1 if debuginfod is enabled. */
/* #undef HAVE_LIBDEBUGINFOD */

/* Define if you have the expat library. */
#define HAVE_LIBEXPAT 1

/* Define to 1 if you have the `libiconvlist' function. */
/* #undef HAVE_LIBICONVLIST */

/* Define if you have the ipt library. */
/* #undef HAVE_LIBIPT */

/* Define if you have the lzma library. */
/* #undef HAVE_LIBLZMA */

/* Define to 1 if you have the `m' library (-lm). */
#define HAVE_LIBM 1

/* Define if you have the mpfr library. */
/* #undef HAVE_LIBMPFR */

/* Define to 1 if you have the <libunwind-ia64.h> header file. */
/* #undef HAVE_LIBUNWIND_IA64_H */

/* Define if you have the xxhash library. */
/* #undef HAVE_LIBXXHASH */

/* Define to 1 if you have the <linux/elf.h> header file. */
/* #undef HAVE_LINUX_ELF_H */

/* Define to 1 if you have the <linux/perf_event.h> header file. */
/* #undef HAVE_LINUX_PERF_EVENT_H */

/* Define to 1 if you have the <locale.h> header file. */
#define HAVE_LOCALE_H 1

/* Define to 1 if the compiler supports long double. */
#define HAVE_LONG_DOUBLE 1

/* Define to 1 if the system has the type `long long'. */
#define HAVE_LONG_LONG 1

/* Define if <sys/procfs.h> has lwpid_t. */
/* #undef HAVE_LWPID_T */

/* Define to 1 if you have the <machine/reg.h> header file. */
#define HAVE_MACHINE_REG_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have a working `mmap' system call. */
#define HAVE_MMAP 1

/* Define to 1 if you have the `monstartup' function. */
#define HAVE_MONSTARTUP 1

/* Define to 1 if you have the <ncursesw/ncurses.h> header file. */
/* #undef HAVE_NCURSESW_NCURSES_H */

/* Define to 1 if you have the <ncurses.h> header file. */
/* #undef HAVE_NCURSES_H */

/* Define to 1 if you have the <ncurses/ncurses.h> header file. */
/* #undef HAVE_NCURSES_NCURSES_H */

/* Define to 1 if you have the <ncurses/term.h> header file. */
/* #undef HAVE_NCURSES_TERM_H */

/* Define to 1 if you have the <nlist.h> header file. */
#define HAVE_NLIST_H 1

/* Define if you support the personality syscall. */
/* #undef HAVE_PERSONALITY */

/* Define to 1 if you have the `pipe' function. */
#define HAVE_PIPE 1

/* Define to 1 if you have the `pipe2' function. */
#define HAVE_PIPE2 1

/* Define to 1 if you have the `poll' function. */
#define HAVE_POLL 1

/* Define to 1 if you have the <poll.h> header file. */
#define HAVE_POLL_H 1

/* Define to 1 if you have the `posix_madvise' function. */
#define HAVE_POSIX_MADVISE 1

/* Define to 1 if you have the `pread' function. */
#define HAVE_PREAD 1

/* Define to 1 if you have the `pread64' function. */
/* #undef HAVE_PREAD64 */

/* Define if <sys/procfs.h> has prfpregset_t. */
/* #undef HAVE_PRFPREGSET_T */

/* Define if <sys/procfs.h> has prgregset32_t. */
/* #undef HAVE_PRGREGSET32_T */

/* Define if <sys/procfs.h> has prgregset_t. */
/* #undef HAVE_PRGREGSET_T */

/* Define to 1 if you have the <proc_service.h> header file. */
/* #undef HAVE_PROC_SERVICE_H */

/* Define if <sys/procfs.h> has psaddr_t. */
/* #undef HAVE_PSADDR_T */

/* Have PTHREAD_PRIO_INHERIT. */
#define HAVE_PTHREAD_PRIO_INHERIT 1

/* Define to 1 if you have the `pthread_setname_np' function. */
#define HAVE_PTHREAD_SETNAME_NP 1

/* Define to 1 if you have the `pthread_sigmask' function. */
#define HAVE_PTHREAD_SIGMASK 1

/* Define to 1 if you have the `ptrace64' function. */
/* #undef HAVE_PTRACE64 */

/* Define if sys/ptrace.h defines the PTRACE_GETFPXREGS request. */
/* #undef HAVE_PTRACE_GETFPXREGS */

/* Define if sys/ptrace.h defines the PTRACE_GETREGS request. */
/* #undef HAVE_PTRACE_GETREGS */

/* Define to 1 if you have the <ptrace.h> header file. */
/* #undef HAVE_PTRACE_H */

/* Define if sys/ptrace.h defines the PT_GETDBREGS request. */
/* #undef HAVE_PT_GETDBREGS */

/* Define if sys/ptrace.h defines the PT_GETXMMREGS request. */
/* #undef HAVE_PT_GETXMMREGS */

/* Define to 1 if you have the `pt_insn_event' function. */
/* #undef HAVE_PT_INSN_EVENT */

/* Define to 1 if you have the `pwrite' function. */
#define HAVE_PWRITE 1

/* Define if Python interpreter is being linked in. */
/* #undef HAVE_PYTHON */

/* Define to 1 if you have the `resize_term' function. */
#define HAVE_RESIZE_TERM 1

/* Define to 1 if you have the `sbrk' function. */
#define HAVE_SBRK 1

/* Define to 1 if you have the `scm_new_smob' function. */
/* #undef HAVE_SCM_NEW_SMOB */

/* Define to 1 if you have the `setlocale' function. */
#define HAVE_SETLOCALE 1

/* Define to 1 if you have the `setns' function. */
/* #undef HAVE_SETNS */

/* Define to 1 if you have the `setpgid' function. */
#define HAVE_SETPGID 1

/* Define to 1 if you have the `setpgrp' function. */
#define HAVE_SETPGRP 1

/* Define to 1 if you have the `setrlimit' function. */
#define HAVE_SETRLIMIT 1

/* Define to 1 if you have the `setsid' function. */
#define HAVE_SETSID 1

/* Define to 1 if you have the `sigaction' function. */
#define HAVE_SIGACTION 1

/* Define to 1 if you have the `sigaltstack' function. */
#define HAVE_SIGALTSTACK 1

/* Define to 1 if you have the <signal.h> header file. */
#define HAVE_SIGNAL_H 1

/* Define to 1 if you have the `sigprocmask' function. */
#define HAVE_SIGPROCMASK 1

/* Define if sigsetjmp is available. */
#define HAVE_SIGSETJMP 1

/* Define to 1 if you have the `sigsetmask' function. */
#define HAVE_SIGSETMASK 1

/* Define to 1 if you have the `socketpair' function. */
#define HAVE_SOCKETPAIR 1

/* Define to 1 if the system has the type `socklen_t'. */
#define HAVE_SOCKLEN_T 1

/* Define to 1 if the source-highlight library is available */
/* #undef HAVE_SOURCE_HIGHLIGHT */

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if your system has struct lwp. */
#define HAVE_STRUCT_LWP 1

/* Define to 1 if `pl_syscall_code' is a member of `struct ptrace_lwpinfo'. */
/* #undef HAVE_STRUCT_PTRACE_LWPINFO_PL_SYSCALL_CODE */

/* Define to 1 if `pl_tdname' is a member of `struct ptrace_lwpinfo'. */
/* #undef HAVE_STRUCT_PTRACE_LWPINFO_PL_TDNAME */

/* Define to 1 if `enabled' is a member of `struct pt_insn'. */
/* #undef HAVE_STRUCT_PT_INSN_ENABLED */

/* Define to 1 if `resynced' is a member of `struct pt_insn'. */
/* #undef HAVE_STRUCT_PT_INSN_RESYNCED */

/* Define to 1 if your system has struct reg in <machine/reg.h>. */
#define HAVE_STRUCT_REG 1

/* Define to 1 if `r_fs' is a member of `struct reg'. */
/* #undef HAVE_STRUCT_REG_R_FS */

/* Define to 1 if `r_gs' is a member of `struct reg'. */
/* #undef HAVE_STRUCT_REG_R_GS */

/* Define to 1 if `st_blksize' is a member of `struct stat'. */
#define HAVE_STRUCT_STAT_ST_BLKSIZE 1

/* Define to 1 if `st_blocks' is a member of `struct stat'. */
#define HAVE_STRUCT_STAT_ST_BLOCKS 1

/* Define to 1 if `td_pcb' is a member of `struct thread'. */
/* #undef HAVE_STRUCT_THREAD_TD_PCB */

/* Define to 1 if you have the <sys/debugreg.h> header file. */
/* #undef HAVE_SYS_DEBUGREG_H */

/* Define to 1 if you have the <sys/file.h> header file. */
#define HAVE_SYS_FILE_H 1

/* Define to 1 if you have the <sys/filio.h> header file. */
#define HAVE_SYS_FILIO_H 1

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#define HAVE_SYS_IOCTL_H 1

/* Define to 1 if you have the <sys/param.h> header file. */
#define HAVE_SYS_PARAM_H 1

/* Define to 1 if you have the <sys/poll.h> header file. */
#define HAVE_SYS_POLL_H 1

/* Define to 1 if you have the <sys/procfs.h> header file. */
/* #undef HAVE_SYS_PROCFS_H */

/* Define to 1 if you have the <sys/ptrace.h> header file. */
#define HAVE_SYS_PTRACE_H 1

/* Define to 1 if you have the <sys/reg.h> header file. */
/* #undef HAVE_SYS_REG_H */

/* Define to 1 if you have the <sys/resource.h> header file. */
#define HAVE_SYS_RESOURCE_H 1

/* Define to 1 if you have the <sys/select.h> header file. */
#define HAVE_SYS_SELECT_H 1

/* Define to 1 if you have the <sys/socket.h> header file. */
#define HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/un.h> header file. */
#define HAVE_SYS_UN_H 1

/* Define to 1 if you have the <sys/user.h> header file. */
/* #undef HAVE_SYS_USER_H */

/* Define to 1 if you have the <sys/wait.h> header file. */
#define HAVE_SYS_WAIT_H 1

/* Define to 1 if you have the <termios.h> header file. */
#define HAVE_TERMIOS_H 1

/* Define to 1 if you have the <term.h> header file. */
#define HAVE_TERM_H 1

/* Define to 1 if you have the <thread_db.h> header file. */
/* #undef HAVE_THREAD_DB_H */

/* Define to 1 if you have the `ttrace' function. */
/* #undef HAVE_TTRACE */

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the `use_default_colors' function. */
#define HAVE_USE_DEFAULT_COLORS 1

/* Define to 1 if you have the `vfork' function. */
#define HAVE_VFORK 1

/* Define to 1 if you have the <vfork.h> header file. */
/* #undef HAVE_VFORK_H */

/* Define to 1 if you have the `waitpid' function. */
#define HAVE_WAITPID 1

/* Define to 1 if you have the <wait.h> header file. */
/* #undef HAVE_WAIT_H */

/* Define to 1 if you have the `wborder' function. */
#define HAVE_WBORDER 1

/* Define to 1 if you have the <windows.h> header file. */
/* #undef HAVE_WINDOWS_H */

/* Define to 1 if `fork' works. */
#define HAVE_WORKING_FORK 1

/* Define to 1 if `vfork' works. */
#define HAVE_WORKING_VFORK 1

/* Define to 1 if you have the `wresize' function. */
#define HAVE_WRESIZE 1

/* Define to 1 if you have the <ws2tcpip.h> header file. */
/* #undef HAVE_WS2TCPIP_H */

/* Define to 1 if you have the `XML_StopParser' function. */
#define HAVE_XML_STOPPARSER 1

/* Define to 1 if your system has the _etext variable. */
#define HAVE__ETEXT 1

/* Define to 1 if you have the `_mcleanup' function. */
#define HAVE__MCLEANUP 1

/* Path of directory of iconv program. */
/* #undef ICONV_BIN */

/* Define if the iconv directory should be relocated when GDB is moved. */
/* #undef ICONV_BIN_RELOCATABLE */

/* Define as const if the declaration of iconv() needs const. */
#define ICONV_CONST 

/* directory to load the JIT readers from */
#define JIT_READER_DIR "/usr/lib/gdb"

/* Define if the jit-reader-dir directory should be relocated when GDB is
   moved. */
#define JIT_READER_DIR_RELOCATABLE 1

/* Name of this package. */
#define PACKAGE "gdb"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME ""

/* Define to the full name and version of this package. */
#define PACKAGE_STRING ""

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME ""

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION ""

/* Additional package description */
#define PKGVERSION "(GDB) "

/* Define to 1 if the "%H, %D and %DD" formats work to print decfloats. */
/* #undef PRINTF_HAS_DECFLOAT */

/* Define to 1 if the "%Lg" format works to print long doubles. */
/* #undef PRINTF_HAS_LONG_DOUBLE */

/* Define to 1 if the "%ll" format works to print long longs. */
/* #undef PRINTF_HAS_LONG_LONG */

/* Define to necessary symbol if this constant uses a non-standard name on
   your system. */
/* #undef PTHREAD_CREATE_JOINABLE */

/* Define to the type of arg 1 for ptrace. */
#define PTRACE_TYPE_ARG1 int

/* Define to the type of arg 3 for ptrace. */
#define PTRACE_TYPE_ARG3 void *

/* Define to the type of arg 4 for ptrace. */
#define PTRACE_TYPE_ARG4 int

/* Define to the type of arg 5 for ptrace. */
/* #undef PTRACE_TYPE_ARG5 */

/* Define as the return type of ptrace. */
#define PTRACE_TYPE_RET int

/* Define if the python lib directory should be relocated when GDB is moved.
   */
/* #undef PYTHON_LIBDIR_RELOCATABLE */

/* Define if the python directory should be relocated when GDB is moved. */
/* #undef PYTHON_PATH_RELOCATABLE */

/* Relocated directory for source files. */
/* #undef RELOC_SRCDIR */

/* Bug reporting address */
#define REPORT_BUGS_TO "<https://www.gnu.org/software/gdb/bugs/>"

/* Define to 1 if the "%Lg" format works to scan long doubles. */
/* #undef SCANF_HAS_LONG_DOUBLE */

/* Define to 1 if the `setpgrp' function takes no argument. */
/* #undef SETPGRP_VOID */

/* The size of `long', as computed by sizeof. */
/* #undef SIZEOF_LONG */

/* The size of `long long', as computed by sizeof. */
#define SIZEOF_LONG_LONG 8

/* The size of `unsigned long', as computed by sizeof. */
#define SIZEOF_UNSIGNED_LONG 8

/* The size of `unsigned long long', as computed by sizeof. */
#define SIZEOF_UNSIGNED_LONG_LONG 8

/* The size of `unsigned __int128', as computed by sizeof. */
#define SIZEOF_UNSIGNED___INT128 16

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at runtime.
	STACK_DIRECTION > 0 => grows toward higher addresses
	STACK_DIRECTION < 0 => grows toward lower addresses
	STACK_DIRECTION = 0 => direction of growth unknown */
/* #undef STACK_DIRECTION */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* automatically load a system-wide gdbinit file */
#define SYSTEM_GDBINIT ""

/* automatically load system-wide gdbinit files from this directory */
#define SYSTEM_GDBINIT_DIR ""

/* Define if the system-gdbinit-dir directory should be relocated when GDB is
   moved. */
#define SYSTEM_GDBINIT_DIR_RELOCATABLE 0

/* Define if the system-gdbinit directory should be relocated when GDB is
   moved. */
#define SYSTEM_GDBINIT_RELOCATABLE 0

/* search for usr/lib et al within DIR */
#define TARGET_SYSTEM_ROOT ""

/* Define if the sysroot directory should be relocated when GDB is moved. */
#define TARGET_SYSTEM_ROOT_RELOCATABLE 0

/* Define if <thread_db.h> has the TD_NOTALLOC error code. */
/* #undef THREAD_DB_HAS_TD_NOTALLOC */

/* Define if <thread_db.h> has the TD_NOTLS error code. */
/* #undef THREAD_DB_HAS_TD_NOTLS */

/* Define if <thread_db.h> has the TD_VERSION error code. */
/* #undef THREAD_DB_HAS_TD_VERSION */

/* Define to 1 if the regex included in libiberty should be used. */
#define USE_INCLUDED_REGEX 1

/* Enable extensions on AIX 3, Interix.  */
#ifndef _ALL_SOURCE
# define _ALL_SOURCE 1
#endif
/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif
/* Enable threading extensions on Solaris.  */
#ifndef _POSIX_PTHREAD_SEMANTICS
# define _POSIX_PTHREAD_SEMANTICS 1
#endif
/* Enable extensions on HP NonStop.  */
#ifndef _TANDEM_SOURCE
# define _TANDEM_SOURCE 1
#endif
/* Enable general extensions on Solaris.  */
#ifndef __EXTENSIONS__
# define __EXTENSIONS__ 1
#endif


/* Define if we should use the Windows API, instead of the POSIX API. On
   Windows, we use the Windows API when building for MinGW, but the POSIX API
   when building for Cygwin. */
/* #undef USE_WIN32API */

/* Define if the PPC simulator is being linked in. */
/* #undef WITH_PPC_SIM */

/* Directory containing Python's standard libraries from --with-python-libdir.
   */
/* #undef WITH_PYTHON_LIBDIR */

/* Define if --with-python provides a path, either directly or via
   python-config.py --exec-prefix. */
/* #undef WITH_PYTHON_PATH */

/* Define if the simulator is being linked in. */
/* #undef WITH_SIM */

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
#  define WORDS_BIGENDIAN 1
# endif
#endif

/* Enable large inode numbers on Mac OS X 10.5.  */
#ifndef _DARWIN_USE_64_BIT_INODE
# define _DARWIN_USE_64_BIT_INODE 1
#endif

/* Number of bits in a file offset, on hosts where this is settable. */
/* #undef _FILE_OFFSET_BITS */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */

/* Define to 1 if on MINIX. */
/* #undef _MINIX */

/* Define to 2 if the system does not provide POSIX.1 features except with
   this defined. */
/* #undef _POSIX_1_SOURCE */

/* Define to 1 if you need to in order for `stat' and other things to work. */
/* #undef _POSIX_SOURCE */

/* Use structured /proc on Solaris. */
#define _STRUCTURED_PROC 1

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif

/* Define to `int' if <sys/types.h> does not define. */
/* #undef pid_t */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */

/* Define as `fork' if `vfork' does not work. */
/* #undef vfork */
