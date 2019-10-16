/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)tty.h	7.1.2 (2.11BSD GTE) 1997/4/10
 */

#ifndef _SYS_TTY_H_
#define _SYS_TTY_H_

#include <sys/termios.h>
#include <sys/select.h>

/*
 * A clist structure is the head of a linked list queue
 * of characters.  The characters are stored in blocks
 * containing a link and CBSIZE (param.h) characters. 
 * The routines in tty_subr.c manipulate these structures.
 */
struct clist {
	int	c_cc;			/* character count */
	char	*c_cf;		/* pointer to first char */
	char	*c_cl;		/* pointer to last char */
};

/*
 * Per-tty structure.
 *
 * Should be split in two, into device and tty drivers.
 * Glue could be masks of what to echo and circular buffer
 * (low, high, timeout).
 */
struct tty {
	union {
		struct {
			struct	clist T_rawq;
			struct	clist T_canq;
		} t_t;

#define	t_rawq	t_nu.t_t.T_rawq		/* raw characters or partial line */
#define	t_canq	t_nu.t_t.T_canq		/* raw characters or partial line */
		struct {
			struct	buf *T_bufp;
			char	*T_cp;
			int	T_inbuf;
			int	T_rec;
		} t_n;

#define	t_bufp	t_nu.t_n.T_bufp		/* buffer allocated to protocol */
#define	t_cp	t_nu.t_n.T_cp		/* pointer into the ripped off buffer */
#define	t_inbuf	t_nu.t_n.T_inbuf	/* number chars in the buffer */
#define	t_rec	t_nu.t_n.T_rec		/* have a complete record */
	} t_nu;

	struct	clist t_outq;			/* device */
	int	(*t_oproc)();				/* device */
	struct	proc *t_rsel;			/* tty */
	struct	proc *t_wsel;
	caddr_t	T_LINEP;				/* ### */
	caddr_t	t_addr;					/* ??? */
	dev_t	t_dev;					/* device */
	long	t_flags;				/* some of both */
	long	t_state;				/* some of both */
	short	t_pgrp;					/* tty */
	char	t_delct;				/* tty */
	char	t_line;					/* glue */
	char	t_col;					/* tty */
	char	t_ispeed, t_ospeed;		/* device */
	char	t_rocount, t_rocol;		/* tty */
	struct	ttychars t_chars;		/* tty */
	struct	winsize t_winsize;		/* window size */
/* be careful of tchars & co. */
#define	t_erase		t_chars.tc_erase
#define	t_kill		t_chars.tc_kill
#define	t_intrc		t_chars.tc_intrc
#define	t_quitc		t_chars.tc_quitc
#define	t_startc	t_chars.tc_startc
#define	t_stopc		t_chars.tc_stopc
#define	t_eofc		t_chars.tc_eofc
#define	t_brkc		t_chars.tc_brkc
#define	t_suspc		t_chars.tc_suspc
#define	t_dsuspc	t_chars.tc_dsuspc
#define	t_rprntc	t_chars.tc_rprntc
#define	t_flushc	t_chars.tc_flushc
#define	t_werasc	t_chars.tc_werasc
#define	t_lnextc	t_chars.tc_lnextc
};

#define	TTIPRI	28
#define	TTOPRI	29

/* limits */
#define	NSPEEDS	16
#define	TTMASK	15
#define	OBUFSIZ	100

#if defined(KERNEL) && !defined(SUPERVISOR)
short	tthiwat[NSPEEDS], ttlowat[NSPEEDS];
#define	TTHIWAT(tp)	tthiwat[(tp)->t_ospeed&TTMASK]
#define	TTLOWAT(tp)	ttlowat[(tp)->t_ospeed&TTMASK]
extern	struct ttychars ttydefaults;
#endif

/* internal state bits */
#define	TS_TIMEOUT	0x000001L	/* delay timeout in progress */
#define	TS_WOPEN	0x000002L	/* waiting for open to complete */
#define	TS_ISOPEN	0x000004L	/* device is open */
#define	TS_FLUSH	0x000008L	/* outq has been flushed during DMA */
#define	TS_CARR_ON	0x000010L	/* software copy of carrier-present */
#define	TS_BUSY		0x000020L	/* output in progress */
#define	TS_ASLEEP	0x000040L	/* wakeup when output done */
#define	TS_XCLUDE	0x000080L	/* exclusive-use flag against open */
#define	TS_TTSTOP	0x000100L	/* output stopped by ctl-s */
#define	TS_HUPCLS	0x000200L	/* hang up upon last close */
#define	TS_TBLOCK	0x000400L	/* tandem queue blocked */
#define	TS_RCOLL	0x000800L	/* collision in read select */
#define	TS_WCOLL	0x001000L	/* collision in write select */
#define	TS_ASYNC	0x004000L	/* tty in async i/o mode */
/* state for intra-line fancy editing work */
#define	TS_ERASE	0x040000L	/* within a \.../ for PRTRUB */
#define	TS_LNCH		0x080000L	/* next character is literal */
#define	TS_TYPEN	0x100000L	/* retyping suspended input (PENDIN) */
#define	TS_CNTTB	0x200000L	/* counting tab width; leave FLUSHO alone */

#define	TS_LOCAL	(TS_ERASE|TS_LNCH|TS_TYPEN|TS_CNTTB)

/* define partab character types */
#define	ORDINARY	0
#define	CONTROL		1
#define	BACKSPACE	2
#define	NEWLINE		3
#define	TAB			4
#define	VTAB		5
#define	RETURN		6

#ifdef KERNEL
extern	struct ttychars ttydefaults;

/* Symbolic sleep message strings. */
extern	 char ttyin[], ttyout[], ttopen[], ttclos[], ttybg[], ttybuf[];

void cblock_alloc_cblocks __P((int));
void cblock_free_cblocks __P((int));
int	 b_to_q __P((char *cp, int cc, struct clist *q));
void catq __P((struct clist *from, struct clist *to));
/* void	 clist_init __P((void)); */ /* defined in systm.h for main() */
int	 getc __P((struct clist *q));
void  ndflush __P((struct clist *q, int cc));
int	 ndqb __P((struct clist *q, int flag));
char *nextc __P((struct clist *q, char *cp, int *c));
int	 putc __P((int c, struct clist *q));
int	 q_to_b __P((struct clist *q, char *cp, int cc));
int	 unputc __P((struct clist *q));

int	 nullmodem __P((struct tty *tp, int flag));
int	 tputchar __P((int c, struct tty *tp));
int	 ttioctl __P((struct tty *tp, int com, void *data, int flag));
int	 ttread __P((struct tty *tp, struct uio *uio, int flag));
void ttrstrt __P((void *tp));
int	 ttselect __P((dev_t device, int rw, struct proc *p));
void ttsetwater __P((struct tty *tp));
int	 ttspeedtab __P((int speed, struct speedtab *table));
int	 ttstart __P((struct tty *tp));
void ttwakeup __P((struct tty *tp));
int	 ttwrite __P((struct tty *tp, struct uio *uio, int flag));
void ttychars __P((struct tty *tp));
int	 ttycheckoutq __P((struct tty *tp, int wait));
int	 ttyclose __P((struct tty *tp));
void ttyflush __P((struct tty *tp, int rw));
void ttyinfo __P((struct tty *tp));
int	 ttyinput __P((int c, struct tty *tp));
int	 ttylclose __P((struct tty *tp, int flag));
int	 ttymodem __P((struct tty *tp, int flag));
int	 ttyopen __P((dev_t device, struct tty *tp));
int	 ttyoutput __P((int c, struct tty *tp));
void ttypend __P((struct tty *tp));
void ttyretype __P((struct tty *tp));
void ttyrub __P((int c, struct tty *tp));
int	 ttysleep __P((struct tty *tp,
	    void *chan, int pri, char *wmesg, int timeout));
int	 ttywait __P((struct tty *tp));
int	 ttywflush __P((struct tty *tp));
#endif

#endif
