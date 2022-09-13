/*	$NetBSD: exec_conf.c,v 1.93 2006/08/30 14:41:06 cube Exp $	*/

/*
 * Copyright (c) 1993, 1994 Christopher G. Demetriou
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christopher G. Demetriou.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/exec.h>
#include <sys/exec_linker.h>
#include <sys/exec_script.h>
#include <sys/exec_aout.h>
#include <sys/exec_coff.h>
#include <sys/exec_ecoff.h>
#include <sys/exec_macho.h>
#include <sys/exec_pecoff.h>
#include <sys/exec_elf.h>
#include <sys/exec_xcoff.h>

#ifdef SYSCALL_DEBUG
extern char *syscallnames[];
#endif
/*
const struct execsw script_exec;
const struct execsw aout_exec;
const struct execsw coff_exec;
const struct execsw ecoff_exec;
const struct execsw pecoff_exec;
const struct execsw macho_exec;
const struct execsw elf32_exec;
const struct execsw elf64_exec;
const struct execsw xcoff32_exec;
const struct execsw xcoff64_exec;
*/

const struct execsw execsw[] = {
	/* shell scripts */
	{ &script_exec.ex_hdrsz, &script_exec.ex_makecmds, &script_exec.ex_emul, &script_exec.ex_prio, &script_exec.ex_arglen, &script_exec.ex_copyargs, &script_exec.ex_setup_stack },

	/* a.out binaries */
	{ &aout_exec.ex_hdrsz, &aout_exec.ex_makecmds, &aout_exec.ex_emul, &aout_exec.ex_prio, &aout_exec.ex_arglen, &aout_exec.ex_copyargs, &aout_exec.ex_setup_stack },

	/* coff binaries */
	{ &coff_exec.ex_hdrsz, &coff_exec.ex_makecmds, &coff_exec.ex_emul, &coff_exec.ex_prio, &coff_exec.ex_arglen, &coff_exec.ex_copyargs, &coff_exec.ex_setup_stack },

	/* ecoff binaries */
	{ &ecoff_exec.ex_hdrsz, &ecoff_exec.ex_makecmds, &ecoff_exec.ex_emul, &ecoff_exec.ex_prio, &ecoff_exec.ex_arglen, &ecoff_exec.ex_copyargs, &ecoff_exec.ex_setup_stack },
	
	/* pecoff binaries */
	{ &pecoff_exec.ex_hdrsz, &pecoff_exec.ex_makecmds, &pecoff_exec.ex_emul, &pecoff_exec.ex_prio, &pecoff_exec.ex_arglen, &pecoff_exec.ex_copyargs, &pecoff_exec.ex_setup_stack },

	/* mach-o binaries */
	{ &macho_exec.ex_hdrsz, &macho_exec.ex_makecmds, &macho_exec.ex_emul, &macho_exec.ex_prio, &macho_exec.ex_arglen, &macho_exec.ex_copyargs, &macho_exec.ex_setup_stack },

	/* 32-Bit ELF binaries */
	{ &elf32_exec.ex_hdrsz, &elf32_exec.ex_makecmds, &elf32_exec.ex_emul, &elf32_exec.ex_prio, &elf32_exec.ex_arglen, &elf32_exec.ex_copyargs, &elf32_exec.ex_setup_stack },

	/* 64-Bit ELF binaries */
	{ &elf64_exec.ex_hdrsz, &elf64_exec.ex_makecmds, &elf64_exec.ex_emul, &elf64_exec.ex_prio, &elf64_exec.ex_arglen, &elf64_exec.ex_copyargs, &elf64_exec.ex_setup_stack },
	
	/* 32-Bit xcoff binaries */
	{ &xcoff32_exec.ex_hdrsz, &xcoff32_exec.ex_makecmds, &xcoff32_exec.ex_emul, &xcoff32_exec.ex_prio, &xcoff32_exec.ex_arglen, &xcoff32_exec.ex_copyargs, &xcoff32_exec.ex_setup_stack },

	/* 64-Bit xcoff binaries */
	{ &xcoff64_exec.ex_hdrsz, &xcoff64_exec.ex_makecmds, &xcoff64_exec.ex_emul, &xcoff64_exec.ex_prio, &xcoff64_exec.ex_arglen, &xcoff64_exec.ex_copyargs, &xcoff64_exec.ex_setup_stack },
};

int nexecs = (sizeof execsw / sizeof(struct execsw));
int exec_maxhdrsz;

static void
link_exec(listp, exp)
	struct execsw_entry **listp;
	const struct execsw *exp;
{
	struct execsw_entry *et, *e1;

	et = (struct execsw_entry *)malloc(sizeof(struct execsw_entry), M_TEMP, M_WAITOK);
	et->next = NULL;
	et->ex = exp;
	if (*listp == NULL) {
		*listp = et;
		return;
	}
	switch(et->ex->ex_prio) {
	case EXECSW_PRIO_FIRST:
		/* put new entry as the first */
		et->next = *listp;
		*listp = et;
		break;
	case EXECSW_PRIO_ANY:
		/* put new entry after all *_FIRST and *_ANY entries */
		for (e1 = *listp; e1->next && e1->next->ex->ex_prio != EXECSW_PRIO_LAST;
				e1 = e1->next) {
			;
		}
		et->next = e1->next;
		e1->next = et;
		break;
	case EXECSW_PRIO_LAST:
		/* put new entry as the last one */
		for (e1 = *listp; e1->next; e1 = e1->next) {
			;
		}
		e1->next = et;
		break;
	default:
#ifdef DIAGNOSTIC
		panic("execw[] entry with unknown priority %d found",
			et->ex->ex_prio);
#else
		free(et, M_TEMP);
#endif
		break;
	}
}

void
exec_init(void)
{
	struct execsw_entry	*list;
	int	i;

	/*
	 * Build execsw[] array from builtin entries and entries added
	 * at runtime.
	 */
	list = NULL;
	for(i = 0; i < nexecs; i++) {
		link_exec(&list, &execsw[i]);
	}

	/*
	 * figure out the maximum size of an exec header.
	 */
	for (i = 0; i < nexecs; i++) {
		if (execsw[i].ex_makecmds != NULL && execsw[i].ex_hdrsz > exec_maxhdrsz) {
			exec_maxhdrsz = execsw[i].ex_hdrsz;
		}
	}
}
