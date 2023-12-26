/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)dk.h	5.1 (Berkeley) 12/13/86
 */
#ifndef _DK_H_
#define	_DK_H_
/*
 * switch commands
 */
#define	RAM_ON	0226
#define	ROM_ON	0322
#define	R_CNTL	0264
#define	W_CNTL	0170

/*
 * library routine declarations
 */
extern long cmread();
extern long ocmread();

/*
 * call setup struct
 */

struct	dialout {
	char	srv;
	char	area;
	char	sw;
	char	mch;
	char	chan;
	char	other;
	short	check;
};

struct	indial {
	char	i_lchan;
	char	i_rhost;
	char	i_rchan;
	char	i_srv;
	char	i_area;
	char	i_sw;
	char	i_mch;
	char	i_chan;
	char	i_other;
	short	i_check;
};
#define	D_SH	1
#define	D_FS	2
#define	D_LSTNR	3

struct	ring {
	char	r_lchan;
	char	r_srv;
};

#endif /* !_DK_H_ */
