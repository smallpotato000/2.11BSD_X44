/* $NetBSD: read.c,v 1.45 2021/04/18 22:51:24 rillig Exp $ */

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All Rights Reserved.
 * Copyright (c) 1994, 1995 Jochen Pohl
 * All Rights Reserved.
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
 *      This product includes software developed by Jochen Pohl for
 *	The NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if defined(__RCSID) && !defined(lint)
__RCSID("$NetBSD: read.c,v 1.45 2021/04/18 22:51:24 rillig Exp $");
#endif

#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lint2.h"


/* index of current (included) source file */
static	int	srcfile;

/*
 * The array pointed to by inpfns maps the file name indices of input files
 * to the file name indices used in lint2
 */
static	short	*inpfns;
static	size_t	ninpfns;

/*
 * The array pointed to by *fnames maps file name indices to file names.
 * Indices of type short are used instead of pointers to save memory.
 */
const	char **fnames;
static	size_t *flines;
static	size_t	nfnames;

/*
 * Types are shared (to save memory for the types itself) and accessed
 * via indices (to save memory for references to types (indices are short)).
 * To share types, a equal type must be located fast. This is done by a
 * hash table. Access by indices is done via an array of pointers to the
 * types.
 */
typedef struct thtab {
	const	char *th_name;
	u_short	th_idx;
	struct	thtab *th_next;
} thtab_t;
static	thtab_t	**thtab;		/* hash table */
type_t	**tlst;				/* array for indexed access */
static	size_t	tlstlen;		/* length of tlst */

static	hte_t **renametab;

/* index of current C source file (as specified at the command line) */
static	int	csrcfile;


#define		inperr(fmt, args...) \
	inperror(__FILE__, __LINE__, fmt, ##args)
static	void	inperror(const char *, size_t, const char *, ...);
static	void	setsrc(const char *);
static	void	setfnid(int, const char *);
static	void	funccall(pos_t *, const char *);
static	void	decldef(pos_t *, const char *);
static	void	usedsym(pos_t *, const char *);
static	u_short	inptype(const char *, const char **);
static	int	gettlen(const char *, const char **);
static	u_short	findtype(const char *, size_t, int);
static	u_short	storetyp(type_t *, const char *, size_t, int);
static	int	thash(const char *, size_t);
static	char	*inpqstrg(const char *, const char **);
static	const	char *inpname(const char *, const char **);
static	int	getfnidx(const char *);

static bool
try_parse_int(const char **p, int *num)
{
	char *end;

	*num = (int)strtol(*p, &end, 10);
	if (end == *p)
		return false;
	*p = end;
	return true;
}

static int
parse_int(const char **p)
{
	char *end;
	int n;

	n = (int)strtol(*p, &end, 10);
	if (end == *p)
		inperr("not a number: %s", *p);
	*p = end;
	return n;
}

static short
parse_short(const char **p)
{

	return (short)parse_int(p);
}

void
readfile(const char *name)
{
	FILE	*inp;
	size_t	len;
	const	char *cp;
	char	*line, rt = '\0';
	int	cline, isrc, iline;
	pos_t	pos;

	if (inpfns == NULL)
		inpfns = xcalloc(ninpfns = 128, sizeof(*inpfns));
	if (fnames == NULL)
		fnames = xcalloc(nfnames = 256, sizeof(*fnames));
	if (flines == NULL)
		flines = xcalloc(nfnames, sizeof(*flines));
	if (tlstlen == 0)
		tlst = xcalloc(tlstlen = 256, sizeof(*tlst));
	if (thtab == NULL)
		thtab = xcalloc(THSHSIZ2, sizeof(*thtab));

	_inithash(&renametab);

	srcfile = getfnidx(name);

	if ((inp = fopen(name, "r")) == NULL)
		err(1, "cannot open %s", name);

	while ((line = fgetln(inp, &len)) != NULL) {
		flines[srcfile]++;

		if (len == 0 || line[len - 1] != '\n')
			inperr("%s", &line[len - 1]);
		line[len - 1] = '\0';
		cp = line;

		/* line number in csrcfile */
		if (!try_parse_int(&cp, &cline))
			cline = -1;

		/* record type */
		if (*cp == '\0')
			inperr("missing record type");
		rt = *cp++;

		if (rt == 'S') {
			setsrc(cp);
			continue;
		} else if (rt == 's') {
			setfnid(cline, cp);
			continue;
		}

		/*
		 * Index of (included) source file. If this index is
		 * different from csrcfile, it refers to an included
		 * file.
		 */
		isrc = parse_int(&cp);
		isrc = inpfns[isrc];

		/* line number in isrc */
		if (*cp++ != '.')
			inperr("bad line number");
		iline = parse_int(&cp);

		pos.p_src = (u_short)csrcfile;
		pos.p_line = (u_short)cline;
		pos.p_isrc = (u_short)isrc;
		pos.p_iline = (u_short)iline;

		/* process rest of this record */
		switch (rt) {
		case 'c':
			funccall(&pos, cp);
			break;
		case 'd':
			decldef(&pos, cp);
			break;
		case 'u':
			usedsym(&pos, cp);
			break;
		default:
			inperr("bad record type %c", rt);
		}

	}

	_destroyhash(renametab);

	if (ferror(inp) != 0)
		err(1, "read error on %s", name);

	(void)fclose(inp);
}


static void __attribute__((format(printf, 3, 4))) __attribute__((noreturn))
inperror(const char *file, size_t line, const char *fmt, ...)
{
	va_list ap;
	char buf[1024];

	va_start(ap, fmt);
	(void)vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	errx(1, "%s,%zu: input file error: %s,%zu (%s)", file, line,
	    fnames[srcfile], flines[srcfile], buf);
}

/*
 * Set the name of the C source file of the .ln file which is
 * currently read.
 */
static void
setsrc(const char *cp)
{

	csrcfile = getfnidx(cp);
}

/*
 * setfnid() gets as input an index as used in an input file and the
 * associated file name. If necessary, it creates a new lint2 file
 * name index for this file name and creates the mapping of the index
 * as used in the input file to the index used in lint2.
 */
static void
setfnid(int fid, const char *cp)
{

	if (fid < 0)
		inperr("bad fid");

	if ((size_t)fid >= ninpfns) {
		inpfns = xrealloc(inpfns, (ninpfns * 2) * sizeof(*inpfns));
		(void)memset(inpfns + ninpfns, 0, ninpfns * sizeof(*inpfns));
		ninpfns *= 2;
	}
	/*
	 * Should always be true because indices written in the output
	 * file by lint1 are always the previous index + 1.
	 */
	if ((size_t)fid >= ninpfns)
		errx(1, "internal error: setfnid()");
	inpfns[fid] = (u_short)getfnidx(cp);
}

/*
 * Process a function call record (c-record).
 */
static void
funccall(pos_t *posp, const char *cp)
{
	arginf_t *ai, **lai;
	char	c;
	bool	rused, rdisc;
	hte_t	*hte;
	fcall_t	*fcall;
	const char *name;

	fcall = xalloc(sizeof(*fcall));
	fcall->f_pos = *posp;

	/* read flags */
	rused = rdisc = false;
	lai = &fcall->f_args;
	while ((c = *cp) == 'u' || c == 'i' || c == 'd' ||
	       c == 'z' || c == 'p' || c == 'n' || c == 's') {
		cp++;
		switch (c) {
		case 'u':
			if (rused || rdisc)
				inperr("used or discovered: %c", c);
			rused = true;
			break;
		case 'i':
			if (rused || rdisc)
				inperr("used or discovered: %c", c);
			break;
		case 'd':
			if (rused || rdisc)
				inperr("used or discovered: %c", c);
			rdisc = true;
			break;
		case 'z':
		case 'p':
		case 'n':
		case 's':
			ai = xalloc(sizeof(*ai));
			ai->a_num = parse_int(&cp);
			if (c == 'z') {
				ai->a_pcon = ai->a_zero = true;
			} else if (c == 'p') {
				ai->a_pcon = true;
			} else if (c == 'n') {
				ai->a_ncon = true;
			} else {
				ai->a_fmt = true;
				ai->a_fstrg = inpqstrg(cp, &cp);
			}
			*lai = ai;
			lai = &ai->a_next;
			break;
		}
	}
	fcall->f_rused = rused;
	fcall->f_rdisc = rdisc;

	/* read name of function */
	name = inpname(cp, &cp);

	/* first look it up in the renaming table, then in the normal table */
	hte = _hsearch(renametab, name, false);
	if (hte != NULL)
		hte = hte->h_hte;
	else
		hte = hsearch(name, true);
	hte->h_used = true;

	fcall->f_type = inptype(cp, &cp);

	*hte->h_lcall = fcall;
	hte->h_lcall = &fcall->f_next;

	if (*cp != '\0')
		inperr("trailing line data: %s", cp);
}

/*
 * Process a declaration or definition (d-record).
 */
static void
decldef(pos_t *posp, const char *cp)
{
	sym_t	*symp, sym;
	char	c, *pos1, *tname;
	bool	used, renamed;
	hte_t	*hte, *renamehte = NULL;
	const char *name, *newname;

	(void)memset(&sym, 0, sizeof(sym));
	sym.s_pos = *posp;
	sym.s_def = NODECL;

	used = false;

	while (strchr("deiorstuvPS", (c = *cp)) != NULL) {
		cp++;
		switch (c) {
		case 'd':
			if (sym.s_def != NODECL)
				inperr("nodecl %c", c);
			sym.s_def = DEF;
			break;
		case 'e':
			if (sym.s_def != NODECL)
				inperr("nodecl %c", c);
			sym.s_def = DECL;
			break;
		case 'i':
			if (sym.s_inline != NODECL)
				inperr("inline %c", c);
			sym.s_inline = true;
			break;
		case 'o':
			if (sym.s_osdef)
				inperr("osdef");
			sym.s_osdef = true;
			break;
		case 'r':
			if (sym.s_rval)
				inperr("rval");
			sym.s_rval = true;
			break;
		case 's':
			if (sym.s_static)
				inperr("static");
			sym.s_static = true;
			break;
		case 't':
			if (sym.s_def != NODECL)
				inperr("nodecl %c", c);
			sym.s_def = TDEF;
			break;
		case 'u':
			if (used)
				inperr("used %c", c);
			used = true;
			break;
		case 'v':
			if (sym.s_va)
				inperr("va");
			sym.s_va = true;
			sym.s_nva = parse_short(&cp);
			break;
		case 'P':
			if (sym.s_prfl)
				inperr("prfl");
			sym.s_prfl = true;
			sym.s_nprfl = parse_short(&cp);
			break;
		case 'S':
			if (sym.s_scfl)
				inperr("scfl");
			sym.s_scfl = true;
			sym.s_nscfl = parse_short(&cp);
			break;
		}
	}

	/* read symbol name, doing renaming if necessary */
	name = inpname(cp, &cp);
	renamed = false;
	if (*cp == 'r') {
		cp++;
		tname = xstrdup(name);
		newname = inpname(cp, &cp);

		/* enter it and see if it's already been renamed */
		renamehte = _hsearch(renametab, tname, true);
		if (renamehte->h_hte == NULL) {
			hte = hsearch(newname, true);
			renamehte->h_hte = hte;
			renamed = true;
		} else if (hte = renamehte->h_hte,
		    strcmp(hte->h_name, newname) != 0) {
			pos1 = xstrdup(mkpos(&renamehte->h_syms->s_pos));
			/* %s renamed multiple times  \t%s  ::  %s */
			msg(18, tname, pos1, mkpos(&sym.s_pos));
			free(pos1);
		}
		free(tname);
	} else {
		/* it might be a previously-done rename */
		hte = _hsearch(renametab, name, false);
		if (hte != NULL)
			hte = hte->h_hte;
		else
			hte = hsearch(name, true);
	}
	hte->h_used |= used;
	if (sym.s_def == DEF || sym.s_def == TDEF)
		hte->h_def = true;

	sym.s_type = inptype(cp, &cp);

	/*
	 * Allocate memory for this symbol only if it was not already
	 * declared or tentatively defined at the same location with
	 * the same type. Works only for symbols with external linkage,
	 * because static symbols, tentatively defined at the same location
	 * but in different translation units are really different symbols.
	 */
	for (symp = hte->h_syms; symp != NULL; symp = symp->s_next) {
		if (symp->s_pos.p_isrc == sym.s_pos.p_isrc &&
		    symp->s_pos.p_iline == sym.s_pos.p_iline &&
		    symp->s_type == sym.s_type &&
		    ((symp->s_def == DECL && sym.s_def == DECL) ||
		     (!sflag && symp->s_def == TDEF && sym.s_def == TDEF)) &&
		    !symp->s_static && !sym.s_static) {
			break;
		}
	}

	if (symp == NULL) {
		/* allocsym does not reserve space for s_nva */
		if (sym.s_va || sym.s_prfl || sym.s_scfl) {
			symp = xalloc(sizeof(*symp));
			*symp = sym;
		} else {
			symp = xalloc(sizeof(symp->s_s));
			symp->s_s = sym.s_s;
		}
		*hte->h_lsym = symp;
		hte->h_lsym = &symp->s_next;

		/* XXX hack so we can remember where a symbol was renamed */
		if (renamed)
			renamehte->h_syms = symp;
	}

	if (*cp != '\0')
		inperr("trailing line: %s", cp);
}

/*
 * Read an u-record (emitted by lint1 if a symbol was used).
 */
static void
usedsym(pos_t *posp, const char *cp)
{
	usym_t	*usym;
	hte_t	*hte;
	const char *name;

	usym = xalloc(sizeof(*usym));
	usym->u_pos = *posp;

	/* needed as delimiter between two numbers */
	if (*cp++ != 'x')
		inperr("bad delim %c", cp[-1]);

	name = inpname(cp, &cp);
	hte = _hsearch(renametab, name, false);
	if (hte != NULL)
		hte = hte->h_hte;
	else
		hte = hsearch(name, true);
	hte->h_used = true;

	*hte->h_lusym = usym;
	hte->h_lusym = &usym->u_next;
}

/*
 * Read a type and return the index of this type.
 */
static u_short
inptype(const char *cp, const char **epp)
{
	char	c, s;
	const	char *ep;
	type_t	*tp;
	int	narg, i;
	bool	osdef = false;
	size_t	tlen;
	u_short	tidx, sidx;
	int	h;

	/* If we have this type already, return its index. */
	tlen = gettlen(cp, &ep);
	h = thash(cp, tlen);
	if ((tidx = findtype(cp, tlen, h)) != 0) {
		*epp = ep;
		return tidx;
	}

	/* No, we must create a new type. */
	tp = xalloc(sizeof(*tp));

	tidx = storetyp(tp, cp, tlen, h);

	c = *cp++;

	while (c == 'c' || c == 'v') {
		if (c == 'c') {
			tp->t_const = true;
		} else {
			tp->t_volatile = true;
		}
		c = *cp++;
	}

	switch (c) {
	case 's':
	case 'u':
	case 'l':
	case 'e':
		s = c;
		c = *cp++;
		break;
	default:
		s = '\0';
		break;
	}

	switch (c) {
	case 'B':
		tp->t_tspec = BOOL;
		break;
	case 'C':
		tp->t_tspec = s == 's' ? SCHAR : (s == 'u' ? UCHAR : CHAR);
		break;
	case 'S':
		tp->t_tspec = s == 'u' ? USHORT : SHORT;
		break;
	case 'I':
		tp->t_tspec = s == 'u' ? UINT : INT;
		break;
	case 'L':
		tp->t_tspec = s == 'u' ? ULONG : LONG;
		break;
	case 'Q':
		tp->t_tspec = s == 'u' ? UQUAD : QUAD;
		break;
	case 'D':
		tp->t_tspec = s == 's' ? FLOAT : (s == 'l' ? LDOUBLE : DOUBLE);
		break;
	case 'V':
		tp->t_tspec = VOID;
		break;
	case 'P':
		tp->t_tspec = PTR;
		break;
	case 'A':
		tp->t_tspec = ARRAY;
		break;
	case 'F':
	case 'f':
		osdef = c == 'f';
		tp->t_tspec = FUNC;
		break;
	case 'T':
		tp->t_tspec = s == 'e' ? ENUM : (s == 's' ? STRUCT : UNION);
		break;
	case 'X':
		tp->t_tspec = s == 's' ? FCOMPLEX
				       : (s == 'l' ? LCOMPLEX : DCOMPLEX);
		break;
	}

	switch (tp->t_tspec) {
	case ARRAY:
		tp->t_dim = parse_int(&cp);
		sidx = inptype(cp, &cp); /* force seq. point! (ditto below) */
		tp->t_subt = TP(sidx);
		break;
	case PTR:
		sidx = inptype(cp, &cp);
		tp->t_subt = TP(sidx);
		break;
	case FUNC:
		c = *cp;
		if (ch_isdigit(c)) {
			if (!osdef)
				tp->t_proto = true;
			narg = parse_int(&cp);
			tp->t_args = xcalloc((size_t)(narg + 1),
					     sizeof(*tp->t_args));
			for (i = 0; i < narg; i++) {
				if (i == narg - 1 && *cp == 'E') {
					tp->t_vararg = true;
					cp++;
				} else {
					sidx = inptype(cp, &cp);
					tp->t_args[i] = TP(sidx);
				}
			}
		}
		sidx = inptype(cp, &cp);
		tp->t_subt = TP(sidx);
		break;
	case ENUM:
		tp->t_tspec = INT;
		tp->t_is_enum = true;
		/* FALLTHROUGH */
	case STRUCT:
	case UNION:
		switch (*cp++) {
		case '1':
			tp->t_istag = true;
			tp->t_tag = hsearch(inpname(cp, &cp), true);
			break;
		case '2':
			tp->t_istynam = true;
			tp->t_tynam = hsearch(inpname(cp, &cp), true);
			break;
		case '3':
			tp->t_isuniqpos = true;
			tp->t_uniqpos.p_line = parse_int(&cp);
			cp++;
			/* xlate to 'global' file name. */
			tp->t_uniqpos.p_file =
			    addoutfile(inpfns[parse_int(&cp)]);
			cp++;
			tp->t_uniqpos.p_uniq = parse_int(&cp);
			break;
		}
		break;
	case LONG:
	case VOID:
	case LDOUBLE:
	case DOUBLE:
	case FLOAT:
	case UQUAD:
	case QUAD:
#ifdef INT128_SIZE
	case UINT128:
	case INT128:
#endif
	case ULONG:
	case UINT:
	case INT:
	case USHORT:
	case SHORT:
	case UCHAR:
	case SCHAR:
	case CHAR:
	case BOOL:
	case UNSIGN:
	case SIGNED:
	case NOTSPEC:
	case FCOMPLEX:
	case DCOMPLEX:
	case LCOMPLEX:
	case COMPLEX:
		break;
	}

	*epp = cp;
	return tidx;
}

/*
 * Get the length of a type string.
 */
static int
gettlen(const char *cp, const char **epp)
{
	const	char *cp1;
	char	c, s;
	tspec_t	t;
	int	narg, i;
	bool	cm, vm;

	cp1 = cp;

	c = *cp++;

	cm = vm = false;

	while (c == 'c' || c == 'v') {
		if (c == 'c') {
			if (cm)
				inperr("cm: %c", c);
			cm = true;
		} else {
			if (vm)
				inperr("vm: %c", c);
			vm = true;
		}
		c = *cp++;
	}

	switch (c) {
	case 's':
	case 'u':
	case 'l':
	case 'e':
		s = c;
		c = *cp++;
		break;
	default:
		s = '\0';
		break;
	}

	t = NOTSPEC;

	switch (c) {
	case 'B':
		if (s == '\0')
			t = BOOL;
		break;
	case 'C':
		if (s == 's') {
			t = SCHAR;
		} else if (s == 'u') {
			t = UCHAR;
		} else if (s == '\0') {
			t = CHAR;
		}
		break;
	case 'S':
		if (s == 'u') {
			t = USHORT;
		} else if (s == '\0') {
			t = SHORT;
		}
		break;
	case 'I':
		if (s == 'u') {
			t = UINT;
		} else if (s == '\0') {
			t = INT;
		}
		break;
	case 'L':
		if (s == 'u') {
			t = ULONG;
		} else if (s == '\0') {
			t = LONG;
		}
		break;
	case 'Q':
		if (s == 'u') {
			t = UQUAD;
		} else if (s == '\0') {
			t = QUAD;
		}
		break;
	case 'D':
		if (s == 's') {
			t = FLOAT;
		} else if (s == 'l') {
			t = LDOUBLE;
		} else if (s == '\0') {
			t = DOUBLE;
		}
		break;
	case 'V':
		if (s == '\0')
			t = VOID;
		break;
	case 'P':
		if (s == '\0')
			t = PTR;
		break;
	case 'A':
		if (s == '\0')
			t = ARRAY;
		break;
	case 'F':
	case 'f':
		if (s == '\0')
			t = FUNC;
		break;
	case 'T':
		if (s == 'e') {
			t = ENUM;
		} else if (s == 's') {
			t = STRUCT;
		} else if (s == 'u') {
			t = UNION;
		}
		break;
	case 'X':
		if (s == 's') {
			t = FCOMPLEX;
		} else if (s == 'l') {
			t = LCOMPLEX;
		} else if (s == '\0') {
			t = DCOMPLEX;
		}
		break;
	default:
		inperr("bad type: %c %c", c, s);
	}

	if (t == NOTSPEC) {
		inperr("undefined type: %c %c", c, s);
	}

	switch (t) {
	case ARRAY:
		(void)parse_int(&cp);
		(void)gettlen(cp, &cp);
		break;
	case PTR:
		(void)gettlen(cp, &cp);
		break;
	case FUNC:
		c = *cp;
		if (ch_isdigit(c)) {
			narg = parse_int(&cp);
			for (i = 0; i < narg; i++) {
				if (i == narg - 1 && *cp == 'E') {
					cp++;
				} else {
					(void)gettlen(cp, &cp);
				}
			}
		}
		(void)gettlen(cp, &cp);
		break;
	case ENUM:
	case STRUCT:
	case UNION:
		switch (*cp++) {
		case '1':
			(void)inpname(cp, &cp);
			break;
		case '2':
			(void)inpname(cp, &cp);
			break;
		case '3':
			/* unique position: line.file.uniquifier */
			(void)parse_int(&cp);
			if (*cp++ != '.')
				inperr("not dot: %c", cp[-1]);
			(void)parse_int(&cp);
			if (*cp++ != '.')
				inperr("not dot: %c", cp[-1]);
			(void)parse_int(&cp);
			break;
		default:
			inperr("bad value: %c\n", cp[-1]);
		}
		break;
	case FLOAT:
	case USHORT:
	case SHORT:
	case UCHAR:
	case SCHAR:
	case CHAR:
	case BOOL:
	case UNSIGN:
	case SIGNED:
	case NOTSPEC:
	case INT:
	case UINT:
	case DOUBLE:
	case LDOUBLE:
	case VOID:
	case ULONG:
	case LONG:
	case QUAD:
	case UQUAD:
#ifdef INT128_SIZE
	case INT128:
	case UINT128:
#endif
	case FCOMPLEX:
	case DCOMPLEX:
	case LCOMPLEX:
	case COMPLEX:
		break;
	}

	*epp = cp;
	return cp - cp1;
}

/*
 * Search a type by its type string.
 */
static u_short
findtype(const char *cp, size_t len, int h)
{
	thtab_t	*thte;

	for (thte = thtab[h]; thte != NULL; thte = thte->th_next) {
		if (strncmp(thte->th_name, cp, len) != 0)
			continue;
		if (thte->th_name[len] == '\0')
			return thte->th_idx;
	}

	return 0;
}

/*
 * Store a type and its type string so we can later share this type
 * if we read the same type string from the input file.
 */
static u_short
storetyp(type_t *tp, const char *cp, size_t len, int h)
{
	static	u_int	tidx = 1;	/* 0 is reserved */
	thtab_t	*thte;
	char	*name;

	if (tidx >= USHRT_MAX)
		errx(1, "sorry, too many types");

	if (tidx == tlstlen - 1) {
		tlst = xrealloc(tlst, (tlstlen * 2) * sizeof(*tlst));
		(void)memset(tlst + tlstlen, 0, tlstlen * sizeof(*tlst));
		tlstlen *= 2;
	}

	tlst[tidx] = tp;

	/* create a hash table entry */
	name = xalloc(len + 1);
	(void)memcpy(name, cp, len);
	name[len] = '\0';

	thte = xalloc(sizeof(*thte));
	thte->th_name = name;
	thte->th_idx = tidx;
	thte->th_next = thtab[h];
	thtab[h] = thte;

	return (u_short)tidx++;
}

/*
 * Hash function for types
 */
static int
thash(const char *s, size_t len)
{
	u_int	v;

	v = 0;
	while (len-- != 0) {
		v = (v << sizeof(v)) + (u_char)*s++;
		v ^= v >> (sizeof(v) * CHAR_BIT - sizeof(v));
	}
	return v % THSHSIZ2;
}

/*
 * Read a string enclosed by "". This string may contain quoted chars.
 */
static char *
inpqstrg(const char *src, const char **epp)
{
	char	*strg, *dst;
	size_t	slen;
	int	c;
	int	v;

	dst = strg = xmalloc(slen = 32);

	if ((c = *src++) != '"')
		inperr("not quote: %c", c);
	if ((c = *src++) == '\0')
		inperr("trailing data: %c", c);

	while (c != '"') {
		if (c == '\\') {
			if ((c = *src++) == '\0')
				inperr("missing after \\");
			switch (c) {
			case 'n':
				c = '\n';
				break;
			case 't':
				c = '\t';
				break;
			case 'v':
				c = '\v';
				break;
			case 'b':
				c = '\b';
				break;
			case 'r':
				c = '\r';
				break;
			case 'f':
				c = '\f';
				break;
			case 'a':
				c = '\a';
				break;
			case '\\':
				c = '\\';
				break;
			case '"':
				c = '"';
				break;
			case '\'':
				c = '\'';
				break;
			case '0': case '1': case '2': case '3':
				v = (c - '0') << 6;
				if ((c = *src++) < '0' || c > '7')
					inperr("not octal: %c", c);
				v |= (c - '0') << 3;
				if ((c = *src++) < '0' || c > '7')
					inperr("not octal: %c", c);
				v |= c - '0';
				c = (u_char)v;
				break;
			default:
				inperr("bad \\ escape: %c", c);
			}
		}
		/* keep space for trailing '\0' */
		if ((size_t)(dst - strg) == slen - 1) {
			strg = xrealloc(strg, slen * 2);
			dst = strg + (slen - 1);
			slen *= 2;
		}
		*dst++ = (char)c;
		if ((c = *src++) == '\0')
			inperr("missing closing quote");
	}
	*dst = '\0';

	*epp = src;
	return strg;
}

/*
 * Read the name of a symbol in static memory.
 */
static const char *
inpname(const char *cp, const char **epp)
{
	static	char	*buf;
	static	size_t	blen = 0;
	size_t	len, i;
	char	c;

	len = parse_int(&cp);
	if (len + 1 > blen)
		buf = xrealloc(buf, blen = len + 1);
	for (i = 0; i < len; i++) {
		c = *cp++;
		if (!ch_isalnum(c) && c != '_')
			inperr("not alnum or _: %c", c);
		buf[i] = c;
	}
	buf[i] = '\0';

	*epp = cp;
	return buf;
}

/*
 * Return the index of a file name. If the name cannot be found, create
 * a new entry and return the index of the newly created entry.
 */
static int
getfnidx(const char *fn)
{
	size_t	i;

	/* 0 is reserved */
	for (i = 1; fnames[i] != NULL; i++) {
		if (strcmp(fnames[i], fn) == 0)
			return i;
	}

	if (i == nfnames - 1) {
		size_t nlen = nfnames * 2;
		fnames = xrealloc(fnames, nlen * sizeof(*fnames));
		(void)memset(fnames + nfnames, 0, nfnames * sizeof(*fnames));
		flines = xrealloc(flines, nlen * sizeof(*flines));
		(void)memset(flines + nfnames, 0, nfnames * sizeof(*flines));
		nfnames = nlen;
	}

	fnames[i] = xstrdup(fn);
	flines[i] = 0;
	return i;
}

/*
 * Separate symbols with static and external linkage.
 */
void
mkstatic(hte_t *hte)
{
	sym_t	*sym1, **symp, *sym;
	fcall_t	**callp, *call;
	usym_t	**usymp, *usym;
	hte_t	*nhte;
	bool	ofnd;

	/* Look for first static definition */
	for (sym1 = hte->h_syms; sym1 != NULL; sym1 = sym1->s_next) {
		if (sym1->s_static)
			break;
	}
	if (sym1 == NULL)
		return;

	/* Do nothing if this name is used only in one translation unit. */
	ofnd = false;
	for (sym = hte->h_syms; sym != NULL && !ofnd; sym = sym->s_next) {
		if (sym->s_pos.p_src != sym1->s_pos.p_src)
			ofnd = true;
	}
	for (call = hte->h_calls; call != NULL && !ofnd; call = call->f_next) {
		if (call->f_pos.p_src != sym1->s_pos.p_src)
			ofnd = true;
	}
	for (usym = hte->h_usyms; usym != NULL && !ofnd; usym = usym->u_next) {
		if (usym->u_pos.p_src != sym1->s_pos.p_src)
			ofnd = true;
	}
	if (!ofnd) {
		hte->h_used = true;
		/* errors about undef. static symbols are printed in lint1 */
		hte->h_def = true;
		hte->h_static = true;
		return;
	}

	/*
	 * Create a new hash table entry
	 *
	 * XXX this entry should be put at the beginning of the list to
	 * avoid to process the same symbol twice.
	 */
	for (nhte = hte; nhte->h_link != NULL; nhte = nhte->h_link)
		continue;
	nhte->h_link = xmalloc(sizeof(*nhte->h_link));
	nhte = nhte->h_link;
	nhte->h_name = hte->h_name;
	nhte->h_used = true;
	nhte->h_def = true;	/* error in lint1 */
	nhte->h_static = true;
	nhte->h_syms = NULL;
	nhte->h_lsym = &nhte->h_syms;
	nhte->h_calls = NULL;
	nhte->h_lcall = &nhte->h_calls;
	nhte->h_usyms = NULL;
	nhte->h_lusym = &nhte->h_usyms;
	nhte->h_link = NULL;
	nhte->h_hte = NULL;

	/*
	 * move all symbols used in this translation unit into the new
	 * hash table entry.
	 */
	for (symp = &hte->h_syms; (sym = *symp) != NULL; ) {
		if (sym->s_pos.p_src == sym1->s_pos.p_src) {
			sym->s_static = true;
			(*symp) = sym->s_next;
			if (hte->h_lsym == &sym->s_next)
				hte->h_lsym = symp;
			sym->s_next = NULL;
			*nhte->h_lsym = sym;
			nhte->h_lsym = &sym->s_next;
		} else {
			symp = &sym->s_next;
		}
	}
	for (callp = &hte->h_calls; (call = *callp) != NULL; ) {
		if (call->f_pos.p_src == sym1->s_pos.p_src) {
			(*callp) = call->f_next;
			if (hte->h_lcall == &call->f_next)
				hte->h_lcall = callp;
			call->f_next = NULL;
			*nhte->h_lcall = call;
			nhte->h_lcall = &call->f_next;
		} else {
			callp = &call->f_next;
		}
	}
	for (usymp = &hte->h_usyms; (usym = *usymp) != NULL; ) {
		if (usym->u_pos.p_src == sym1->s_pos.p_src) {
			(*usymp) = usym->u_next;
			if (hte->h_lusym == &usym->u_next)
				hte->h_lusym = usymp;
			usym->u_next = NULL;
			*nhte->h_lusym = usym;
			nhte->h_lusym = &usym->u_next;
		} else {
			usymp = &usym->u_next;
		}
	}

	/* h_def must be recalculated for old hte */
	hte->h_def = nhte->h_def = false;
	for (sym = hte->h_syms; sym != NULL; sym = sym->s_next) {
		if (sym->s_def == DEF || sym->s_def == TDEF) {
			hte->h_def = true;
			break;
		}
	}

	mkstatic(hte);
}
