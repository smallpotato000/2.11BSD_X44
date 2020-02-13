/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)exec.h	8.3 (Berkeley) 1/21/94
 * 	$Id: exec.h,v 1.8 1994/10/02 17:24:41 phk Exp $
 * 	@(#)exec.h	1.2 (2.11BSD GTE) 10/31/93: a_out contents from this file were moved to imgact_aout.h
 */
#ifndef _EXEC_
#define _EXEC_


#define	NOVL	15						/* number of overlays */
struct	ovlhdr {
		 int			max_ovl;		/* maximum overlay size */
		 unsigned int	ov_siz[NOVL];	/* size of i'th overlay */
};

/*
 * eXtended header definition for use with the new macros in a.out.h
*/
struct	xexec {
	struct	exec	e;
	struct	ovlhdr	o;
};

/*
 * The following structure is found at the top of the user stack of each
 * user process. The ps program uses it to locate argv and environment
 * strings. Programs that wish ps to display other information may modify
 * it; normally ps_argvstr points to the text for argv[0], and ps_nargvstr
 * is the same as the program's argc. The fields ps_envstr and ps_nenvstr
 * are the equivalent for the environment.
 */
struct ps_strings {
	char	*ps_argvstr;	/* first of 0 or more argument strings */
	int		ps_nargvstr;	/* the number of argument strings */
	char	*ps_envstr;		/* first of 0 or more environment strings */
	int		ps_nenvstr;		/* the number of environment strings */
};

/*
 * Address of ps_strings structure (in user space).
 */
#define SPARE_USRSPACE	256
#define	PS_STRINGS ((struct ps_strings *) \
		(USRSTACK - sizeof(struct ps_strings) - SPARE_USRSPACE))


/*
 * exec system call, with and without environments.
 */
struct execa {
	char	*fname;
	char	**argp;
	char	**envp;
};

typedef int (*exec_makecmds_fcn)(struct proc *, struct exec_linker *);

struct execsw {
	u_int				ex_hdrsz;		/* size of header for this format */
	exec_makecmds_fcn	ex_makecmds;	/* function to setup vmcmds */

	struct  emul 		*ex_emul;		/* os emulation */
	int					ex_prio;		/* entry priority */
	int					ex_arglen;		/* Extra argument size in words */
										/* Copy arguments on the new stack */
	int					(*ex_setup_stack)(struct exec_linker *);
};

#define EXECSW_PRIO_ANY		0x000	/* default, no preference */
#define EXECSW_PRIO_FIRST	0x001	/* this should be among first */
#define EXECSW_PRIO_LAST	0x002	/* this should be among last */

#ifdef KERNEL
extern const struct execsw **execsw;
extern struct execsw execsw[];
extern int	nexecs;
#endif

#include <machine/exec.h>
#endif
