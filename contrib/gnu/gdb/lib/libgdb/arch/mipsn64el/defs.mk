# This file is automatically generated.  DO NOT EDIT!
# Generated from: NetBSD: mknative-gdb,v 1.15 2020/12/05 21:27:31 christos Exp 
# Generated from: NetBSD: mknative.common,v 1.16 2018/04/15 15:13:37 christos Exp 
#
G_INTERNAL_CFLAGS=   -I. -I${GNUHOSTDIST}/gdb -I${GNUHOSTDIST}/gdb/config -DLOCALEDIR=\"/usr/share/locale\" -DHAVE_CONFIG_H -I${GNUHOSTDIST}/gdb/../include/opcode -I${GNUHOSTDIST}/gdb/../readline/readline/.. -I${GNUHOSTDIST}/gdb/../zlib -I../bfd -I${GNUHOSTDIST}/gdb/../bfd -I${GNUHOSTDIST}/gdb/../include -I../libdecnumber -I${GNUHOSTDIST}/gdb/../libdecnumber -I./../intl -I${GNUHOSTDIST}/gdb/../gnulib/import -I../gnulib/import -I${GNUHOSTDIST}/gdb/.. -I..     -I${GNUHOSTDIST}/gdb/.. -pthread  -Wall -Wpointer-arith -Wno-unused -Wunused-value -Wunused-variable -Wunused-function -Wno-switch -Wno-char-subscripts -Wempty-body -Wunused-but-set-parameter -Wunused-but-set-variable -Wno-sign-compare -Wno-error=maybe-uninitialized -Wno-mismatched-tags -Wsuggest-override -Wimplicit-fallthrough=3 -Wduplicated-cond -Wshadow=local -Wdeprecated-copy -Wdeprecated-copy-dtor -Wredundant-move -Wmissing-declarations -Wstrict-null-sentinel -Wformat -Wformat-nonliteral -Werror
G_LIBGDB_OBS=ada-exp.o ada-lang.o ada-tasks.o ada-typeprint.o ada-valprint.o ada-varobj.o addrmap.o agent.o alloc.o annotate.o arch-utils.o async-event.o auto-load.o auxv.o ax-gdb.o ax-general.o bcache.o bfd-target.o block.o blockframe.o break-catch-sig.o break-catch-syscall.o break-catch-throw.o breakpoint.o bsd-kvm.o btrace.o build-id.o buildsym-legacy.o buildsym.o c-exp.o c-lang.o c-typeprint.o c-valprint.o c-varobj.o charset.o cli-out.o cli-cmds.o cli-decode.o cli-dump.o cli-interp.o cli-logging.o cli-option.o cli-script.o cli-setshow.o cli-style.o cli-utils.o coff-pe-read.o coffread.o compile-c-support.o compile-c-symbols.o compile-c-types.o compile-cplus-symbols.o compile-cplus-types.o compile-loc2c.o compile-object-load.o compile-object-run.o compile.o complaints.o completer.o continuations.o copying.o corefile.o corelow.o cp-abi.o cp-name-parser.o cp-namespace.o cp-support.o cp-valprint.o ctfread.o d-exp.o d-lang.o d-namespace.o d-valprint.o dbxread.o dcache.o debug.o debuginfod-support.o dictionary.o disasm-selftests.o disasm.o dtrace-probe.o dummy-frame.o abbrev.o attribute.o comp-unit.o dwz.o expr.o frame-tailcall.o frame.o index-cache.o index-common.o index-write.o leb.o line-header.o loc.o macro.o read.o section.o stringify.o elfread.o eval.o event-top.o exceptions.o exec.o expprint.o extension.o f-exp.o f-lang.o f-typeprint.o f-valprint.o filename-seen-cache.o filesystem.o findcmd.o findvar.o fork-child.o frame-base.o frame-unwind.o frame.o gcore.o gdb-demangle.o gdb_bfd.o gdb_obstack.o gdb_regex.o gdbarch-selftests.o gdbarch.o gdbtypes.o gnu-v2-abi.o gnu-v3-abi.o go-exp.o go-lang.o go-typeprint.o go-valprint.o guile.o inf-child.o inf-loop.o inf-ptrace.o infcall.o infcmd.o inferior.o inflow.o infrun.o inline-frame.o interps.o jit.o language.o linespec.o location.o m2-exp.o m2-lang.o m2-typeprint.o m2-valprint.o macrocmd.o macroexp.o macroscope.o macrotab.o main.o maint-test-options.o maint-test-settings.o maint.o mdebugread.o mem-break.o memattr.o memory-map.o memrange.o mi-cmd-break.o mi-cmd-catch.o mi-cmd-disas.o mi-cmd-env.o mi-cmd-file.o mi-cmd-info.o mi-cmd-stack.o mi-cmd-target.o mi-cmd-var.o mi-cmds.o mi-common.o mi-console.o mi-getopt.o mi-interp.o mi-main.o mi-out.o mi-parse.o mi-symbol-cmds.o minidebug.o minsyms.o mips-nbsd-nat.o mips-nbsd-tdep.o mips-tdep.o mipsread.o namespace.o fork-inferior.o netbsd-nat.o nbsd-nat.o nbsd-tdep.o objc-lang.o objfiles.o observable.o opencl-lang.o osabi.o osdata.o p-exp.o p-lang.o p-typeprint.o p-valprint.o parse.o posix-hdep.o printcmd.o probe.o process-stratum-target.o producer.o progspace-and-thread.o progspace.o prologue-value.o psymtab.o python.o record-btrace.o record-full.o record.o regcache-dump.o regcache.o reggroups.o registry.o remote-fileio.o remote-notif.o remote.o reverse.o run-on-main-thread.o rust-exp.o rust-lang.o selftest-arch.o sentinel-frame.o ser-base.o ser-event.o ser-pipe.o ser-tcp.o ser-uds.o ser-unix.o serial.o skip.o solib-svr4.o solib-target.o solib.o source-cache.o source.o stabsread.o stack.o stap-probe.o std-regs.o stub-termcap.o symfile-debug.o symfile.o symmisc.o symtab.o target-connection.o target-dcache.o target-descriptions.o target-float.o target-memory.o target.o waitstatus.o test-target.o thread-iter.o thread.o tid-parse.o top.o tracectf.o tracefile-tfile.o tracefile.o tracepoint.o trad-frame.o tramp-frame.o type-stack.o typeprint.o ui-file.o ui-out.o ui-style.o array-view-selftests.o child-path-selftests.o cli-utils-selftests.o command-def-selftests.o common-utils-selftests.o copy_bitwise-selftests.o environ-selftests.o filtered_iterator-selftests.o format_pieces-selftests.o function-view-selftests.o lookup_name_info-selftests.o main-thread-selftests.o memory-map-selftests.o memrange-selftests.o mkdir-recursive-selftests.o observable-selftests.o offset-type-selftests.o optional-selftests.o parse-connection-spec-selftests.o ptid-selftests.o rsp-low-selftests.o scoped_fd-selftests.o scoped_mmap-selftests.o scoped_restore-selftests.o string_view-selftests.o style-selftests.o tracepoint-selftests.o tui-selftests.o unpack-selftests.o utils-selftests.o vec-utils-selftests.o xml-utils-selftests.o user-regs.o utils.o valarith.o valops.o valprint.o value.o varobj.o version.o xml-builtin.o xml-support.o xml-syscall.o xml-tdesc.o init.o
G_SIM_OBS=
