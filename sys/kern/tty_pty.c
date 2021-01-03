/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)tty_pty.c	1.3 (2.11BSD GTE) 1997/5/2
 */

/*
 * Pseudo-teletype Driver
 * (Actually two drivers, requiring two entries in 'cdevsw')
 */
#include <sys/pty.h>

#if NPTY > 0
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/user.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/poll.h>

#if NPTY == 1
#undef NPTY
#define	NPTY	16		/* crude XXX */
#endif

#define BUFSIZ 	100		/* Chunk size iomoved to/from user */

/*
 * pts == /dev/tty[pqrs]?
 * ptc == /dev/pty[pqrs]?
 */
struct tty pt_tty[NPTY];
struct pt_ioctl {
	struct	tty 	*pt_tty;
	int				pt_flags;
	struct	proc 	*pt_selr, *pt_selw;
	u_char			pt_send;
	u_char			pt_ucntl;
} pt_ioctl[NPTY];
int	npty = NPTY;		/* for pstat -t */

#define	PF_RCOLL	0x01
#define	PF_WCOLL	0x02
#define	PF_PKT		0x08		/* packet mode */
#define	PF_STOPPED	0x10		/* user told stopped */
#define	PF_REMOTE	0x20		/* remote and flow controlled input */
#define	PF_NOSTOP	0x40
#define PF_UCNTL	0x80		/* user control mode */

dev_type_open(ptsopen);
dev_type_close(ptsclose);
dev_type_read(ptsread);
dev_type_write(ptswrite);
dev_type_start(ptsstart);
dev_type_stop(ptsstop);

dev_type_open(ptcopen);
dev_type_close(ptcclose);
dev_type_read(ptcread);
dev_type_write(ptcwrite);
dev_type_select(ptcselect);

dev_type_ioctl(ptyioctl);
dev_type_tty(ptytty);

const struct cdevsw ptc_cdevsw = {
		.d_open = ptcopen,
		.d_close = ptcclose,
		.d_read = ptcread,
		.d_write = ptcwrite,
		.d_ioctl = ptyioctl,
		.d_stop = nodev,
		.d_tty = ptytty,
		.d_select = ptcselect,
		.d_poll = ptcpoll,
		.d_mmap = nodev,
		.d_discard = nodev,
		.d_type = D_TTY
};

const struct cdevsw pts_cdevsw = {
		.d_open = ptsopen,
		.d_close = ptsclose,
		.d_read = ptsread,
		.d_write = ptswrite,
		.d_ioctl = ptyioctl,
		.d_stop = ptsstop,
		.d_tty = ptytty,
		.d_poll = ptspoll,
		.d_mmap = nodev,
		.d_discard = nodev,
		.d_type = D_TTY
};

/* initialize pty structures */
void
pty_init(devsw)
	struct devswtable *devsw;
{
	DEVSWIO_CONFIG_INIT(devsw, NPTY, NULL, &ptc_cdevsw, NULL);	/* ptc */
	DEVSWIO_CONFIG_INIT(devsw, NPTY, NULL, &pts_cdevsw, NULL);	/* pts */
}

/*ARGSUSED*/
int
ptsopen(dev, flag)
	dev_t dev;
{
	register struct tty *tp;
	int error;

#ifdef lint
	npty = npty;
#endif
	if (minor(dev) >= NPTY)
		return (ENXIO);
	tp = &pt_tty[minor(dev)];
	if ((tp->t_state & TS_ISOPEN) == 0) {
		ttychars(tp);		/* Set up default chars */
		tp->t_ispeed = tp->t_ospeed = EXTB;
		tp->t_flags = 0;	/* No features (nor raw mode) */
	} else if ((tp->t_state&TS_XCLUDE) && u->u_uid != 0)
		return (EBUSY);
	if (tp->t_oproc)			/* Ctrlr still around. */
		tp->t_state |= TS_CARR_ON;
	while ((tp->t_state & TS_CARR_ON) == 0) {
		tp->t_state |= TS_WOPEN;
		sleep((caddr_t)&tp->t_rawq, TTIPRI);
	}
	error = (*linesw[tp->t_line].l_open)(dev, tp);
	ptcwakeup(tp, FREAD|FWRITE);
	return (error);
}

int
ptsclose(dev, flag)
	dev_t dev;
	int flag;
{
	register struct tty *tp;
	int err;

	tp = &pt_tty[minor(dev)];
	err = (*linesw[tp->t_line].l_close)(tp, flag);
	ttyclose(tp);
	ptcwakeup(tp, FREAD|FWRITE);
	return (err);
}
int
ptsread(dev, uio, flag)
	dev_t dev;
	register struct uio *uio;
	int flag;
{
	register struct tty *tp = &pt_tty[minor(dev)];
	register struct pt_ioctl *pti = &pt_ioctl[minor(dev)];
	int error = 0;

again:
	if (pti->pt_flags & PF_REMOTE) {
		while (tp == u->u_ttyp && u->u_procp->p_pgrp != tp->t_pgrp) {
			if ((u->u_procp->p_sigignore & sigmask(SIGTTIN)) ||
			    (u->u_procp->p_sigmask & sigmask(SIGTTIN)) ||
			    (u->u_procp->p_flag&P_SVFORK))
				return (EIO);
			gsignal(u->u_procp->p_pgrp, SIGTTIN);
			sleep((caddr_t)&lbolt, TTIPRI);
		}
		if (tp->t_canq.c_cc == 0) {
			if (flag & IO_NDELAY)
				return (EWOULDBLOCK);
			sleep((caddr_t)&tp->t_canq, TTIPRI);
			goto again;
		}
		while (tp->t_canq.c_cc > 1 && uio->uio_resid)
			if (ureadc(getc(&tp->t_canq), uio) < 0) {
				error = EFAULT;
				break;
			}
		if (tp->t_canq.c_cc == 1)
			(void) getc(&tp->t_canq);
		if (tp->t_canq.c_cc)
			return (error);
	} else
		if (tp->t_oproc)
			error = (*linesw[tp->t_line].l_read)(tp, uio, flag);
	ptcwakeup(tp, FWRITE);
	return (error);
}

/*
 * Write to pseudo-tty.
 * Wakeups of controlling tty will happen
 * indirectly, when tty driver calls ptsstart.
 */
int
ptswrite(dev, uio, flag)
	dev_t dev;
	register struct uio *uio;
	int flag;
{
	register struct tty *tp;

	tp = &pt_tty[minor(dev)];
	if (tp->t_oproc == 0)
		return (EIO);
	return ((*linesw[tp->t_line].l_write)(tp, uio, flag));
}

/*
 * Start output on pseudo-tty.
 * Wake up process selecting or sleeping for input from controlling tty.
 */
void
ptsstart(tp)
	struct tty *tp;
{
	register struct pt_ioctl *pti = &pt_ioctl[minor(tp->t_dev)];

	if (tp->t_state & TS_TTSTOP)
		return;
	if (pti->pt_flags & PF_STOPPED) {
		pti->pt_flags &= ~PF_STOPPED;
		pti->pt_send = TIOCPKT_START;
	}
	ptcwakeup(tp, FREAD);
}

int
ptspoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	register struct pt_ioctl *pti = &pt_ioctl[minor(dev)];
	struct tty *tp = pti->pt_tty;

	if (tp->t_oproc == NULL) {
		return (POLLHUP);
	}

	return ((*linesw[tp->t_line].l_poll)(tp, events, p));
}

void
ptcwakeup(tp, flag)
	struct tty *tp;
	int flag;
{
	struct pt_ioctl *pti = &pt_ioctl[minor(tp->t_dev)];

	if (flag & FREAD) {
		if (pti->pt_selr) {
			selwakeup(pti->pt_selr, (long)(pti->pt_flags & PF_RCOLL));
			pti->pt_selr = 0;
			pti->pt_flags &= ~PF_RCOLL;
		}
		wakeup((caddr_t)&tp->t_outq.c_cf);
	}
	if (flag & FWRITE) {
		if (pti->pt_selw) {
			selwakeup(pti->pt_selw, (long)(pti->pt_flags & PF_WCOLL));
			pti->pt_selw = 0;
			pti->pt_flags &= ~PF_WCOLL;
		}
		wakeup((caddr_t)&tp->t_rawq.c_cf);
	}
}

/*ARGSUSED*/
int
ptcopen(dev, flag)
	dev_t dev;
	int flag;
{
	register struct tty *tp;
	struct pt_ioctl *pti;

	if (minor(dev) >= NPTY)
		return (ENXIO);
	tp = &pt_tty[minor(dev)];
	if (tp->t_oproc)
		return (EIO);
	tp->t_oproc = ptsstart;
	(void)(*linesw[tp->t_line].l_modem)(tp, 1);
	pti = &pt_ioctl[minor(dev)];
	pti->pt_flags = 0;
	pti->pt_send = 0;
	pti->pt_ucntl = 0;
	return (0);
}

int
ptcclose(dev, flag)
	dev_t dev;
	int flag;
{
	register struct tty *tp;

	tp = &pt_tty[minor(dev)];
	(void)(*linesw[tp->t_line].l_modem)(tp, 0);
	tp->t_state &= ~TS_CARR_ON;
	tp->t_oproc = 0;		/* mark closed */
	return (0);
}

int
ptcread(dev, uio, flag)
	dev_t dev;
	register struct uio *uio;
	int flag;
{
	register struct tty *tp = &pt_tty[minor(dev)];
	struct pt_ioctl *pti = &pt_ioctl[minor(dev)];
	char buf[BUFSIZ];
	int error = 0, cc;

	/*
	 * We want to block until the slave
	 * is open, and there's something to read;
	 * but if we lost the slave or we're NBIO,
	 * then return the appropriate error instead.
	 */
	for (;;) {
		if (tp->t_state&TS_ISOPEN) {
			if ((pti->pt_flags&PF_PKT) && pti->pt_send) {
				error = ureadc((int)pti->pt_send, uio);
				if (error)
					return (error);
				pti->pt_send = 0;
				return (0);
			}
			if ((pti->pt_flags&PF_UCNTL) && pti->pt_ucntl) {
				error = ureadc((int)pti->pt_ucntl, uio);
				if (error)
					return (error);
				pti->pt_ucntl = 0;
				return (0);
			}
			if (tp->t_outq.c_cc && (tp->t_state&TS_TTSTOP) == 0)
				break;
		}
		if ((tp->t_state&TS_CARR_ON) == 0)
			return (0);	/* EOF */
		if (flag & IO_NDELAY)
			return (EWOULDBLOCK);
		sleep((caddr_t)&tp->t_outq.c_cf, TTIPRI);
	}
	if (pti->pt_flags & (PF_PKT|PF_UCNTL))
		error = ureadc(0, uio);
	while (uio->uio_resid && error == 0) {
		cc = q_to_b(&tp->t_outq, buf, MIN(uio->uio_resid, BUFSIZ));
		if (cc <= 0)
			break;
		error = uiomove(buf, cc, uio);
	}
	if (tp->t_outq.c_cc <= TTLOWAT(tp)) {
		if (tp->t_state&TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup((caddr_t)&tp->t_outq);
		}
		if (tp->t_wsel) {
			selwakeup(tp->t_wsel, tp->t_state & TS_WCOLL);
			tp->t_wsel = 0;
			tp->t_state &= ~TS_WCOLL;
		}
	}
	return (error);
}

void
ptsstop(tp, flush)
	register struct tty *tp;
	int flush;
{
	struct pt_ioctl *pti = &pt_ioctl[minor(tp->t_dev)];
	int flag;

	/* note: FLUSHREAD and FLUSHWRITE already ok */
	if (flush == 0) {
		flush = TIOCPKT_STOP;
		pti->pt_flags |= PF_STOPPED;
	} else
		pti->pt_flags &= ~PF_STOPPED;
	pti->pt_send |= flush;
	/* change of perspective */
	flag = 0;
	if (flush & FREAD)
		flag |= FWRITE;
	if (flush & FWRITE)
		flag |= FREAD;
	ptcwakeup(tp, flag);
}

int
ptcselect(dev, rw)
	dev_t dev;
	int rw;
{
	register struct tty *tp = &pt_tty[minor(dev)];
	struct pt_ioctl *pti = &pt_ioctl[minor(dev)];
	struct proc *p;
	int s;

	if ((tp->t_state&TS_CARR_ON) == 0)
		return (1);
	switch (rw) {

	case FREAD:
		/*
		 * Need to block timeouts (ttrstart).
		 */
		s = spltty();
		if ((tp->t_state&TS_ISOPEN) &&
		     tp->t_outq.c_cc && (tp->t_state&TS_TTSTOP) == 0) {
			splx(s);
			return (1);
		}
		splx(s);
		break;
		/* FALLTHROUGH */

	case 0:					/* exceptional */
		if ((tp->t_state&TS_ISOPEN) &&
		    (((pti->pt_flags&PF_PKT) && pti->pt_send) ||
		     ((pti->pt_flags&PF_UCNTL) && pti->pt_ucntl)))
			return (1);
		if ((p = pti->pt_selr) && p->p_wchan == (caddr_t)&selwait)
			pti->pt_flags |= PF_RCOLL;
		else
			pti->pt_selr = u->u_procp;
		break;


	case FWRITE:
		if (tp->t_state&TS_ISOPEN) {
			if (pti->pt_flags & PF_REMOTE) {
			    if (tp->t_canq.c_cc == 0)
				return (1);
			} else {
			    if (tp->t_rawq.c_cc + tp->t_canq.c_cc < TTYHOG-2)
				    return (1);
			    if (tp->t_canq.c_cc == 0 &&
			        (tp->t_flags & (RAW|CBREAK)) == 0)
				    return (1);
			}
		}
		if ((p = pti->pt_selw) && p->p_wchan == (caddr_t)&selwait)
			pti->pt_flags |= PF_WCOLL;
		else
			pti->pt_selw = u->u_procp;
		break;

	}
	return (0);
}

int
ptcpoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	struct pt_ioctl *pti = pt_ioctl[minor(dev)];
	struct tty *tp = pti->pt_tty;
	int revents = 0;
}

int
ptcwrite(dev, uio, flag)
	dev_t dev;
	register struct uio *uio;
	int flag;
{
	register struct tty *tp = &pt_tty[minor(dev)];
	register char *cp;
	register int cc = 0;
	char locbuf[BUFSIZ];
	int cnt = 0;
	struct pt_ioctl *pti = &pt_ioctl[minor(dev)];
	int error = 0;

again:
	if ((tp->t_state&TS_ISOPEN) == 0)
		goto block;
	if (pti->pt_flags & PF_REMOTE) {
		if (tp->t_canq.c_cc)
			goto block;
		while (uio->uio_resid && tp->t_canq.c_cc < TTYHOG - 1) {
			if (cc == 0) {
				cc = MIN(uio->uio_resid, BUFSIZ);
				cc = MIN(cc, TTYHOG - 1 - tp->t_canq.c_cc);
				cp = locbuf;
				error = uiomove(cp, cc, uio);
				if (error)
					return (error);
				/* check again for safety */
				if ((tp->t_state&TS_ISOPEN) == 0)
					return (EIO);
			}
			if (cc)
				(void) b_to_q(cp, cc, &tp->t_canq);
			cc = 0;
		}
		(void) putc(0, &tp->t_canq);
		ttwakeup(tp);
		wakeup((caddr_t)&tp->t_canq);
		return (0);
	}
	while (uio->uio_resid > 0) {
		if (cc == 0) {
			cc = MIN(uio->uio_resid, BUFSIZ);
			cp = locbuf;
			error = uiomove(cp, cc, uio);
			if (error)
				return (error);
			/* check again for safety */
			if ((tp->t_state&TS_ISOPEN) == 0)
				return (EIO);
		}
		while (cc > 0) {
			if ((tp->t_rawq.c_cc + tp->t_canq.c_cc) >= TTYHOG - 2 &&
			   (tp->t_canq.c_cc > 0 ||
			      (tp->t_flags & (RAW|CBREAK)))) {
				wakeup((caddr_t)&tp->t_rawq);
				goto block;
			}
			(*linesw[tp->t_line].l_rint)(*cp++, tp);
			cnt++;
			cc--;
		}
		cc = 0;
	}
	return (0);
block:
	/*
	 * Come here to wait for slave to open, for space
	 * in outq, or space in rawq.
	 */
	if ((tp->t_state&TS_CARR_ON) == 0)
		return (EIO);
	if (flag & IO_NDELAY) {
		/* adjust for data copied in but not written */
		uio->uio_resid += cc;
		if (cnt == 0)
			return (EWOULDBLOCK);
		return (0);
	}
	sleep((caddr_t)&tp->t_rawq.c_cf, TTOPRI);
	goto again;
}

struct tty *
ptytty(dev_t dev)
{
	struct pt_ioctl *pti = pt_ioctl[minor(dev)];
	struct tty *tp = pti->pt_tty;

	return tp;
}

/*ARGSUSED*/
int
ptyioctl(dev, cmd, data, flag)
	caddr_t data;
	u_int cmd;
	dev_t dev;
	int flag;
{
	register struct tty *tp = &pt_tty[minor(dev)];
	register struct pt_ioctl *pti = &pt_ioctl[minor(dev)];
	int stop, error;
	extern ttyinput();

	/*
	 * IF CONTROLLER STTY THEN MUST FLUSH TO PREVENT A HANG.
	 * ttywflush(tp) will hang if there are characters in the outq.
	 */
	if (cdevsw[major(dev)].d_open == ptcopen)
		switch (cmd) {

		case TIOCPKT:
			if (*(int *)data) {
				if (pti->pt_flags & PF_UCNTL)
					return (EINVAL);
				pti->pt_flags |= PF_PKT;
			} else
				pti->pt_flags &= ~PF_PKT;
			return (0);

		case TIOCUCNTL:
			if (*(int *)data) {
				if (pti->pt_flags & PF_PKT)
					return (EINVAL);
				pti->pt_flags |= PF_UCNTL;
			} else
				pti->pt_flags &= ~PF_UCNTL;
			return (0);

		case TIOCREMOTE:
			if (*(int *)data)
				pti->pt_flags |= PF_REMOTE;
			else
				pti->pt_flags &= ~PF_REMOTE;
			ttyflush(tp, FREAD|FWRITE);
			return (0);

		case TIOCSETP:
		case TIOCSETN:
		case TIOCSETD:
			while (getc(&tp->t_outq) >= 0)
				;
			break;
		}
/*
 * Unsure if the comment below still applies or not.  For now put the
 * new code in ifdef'd out.
*/

#ifdef	four_four_bsd
	error = (*linesw[t->t_line].l_ioctl(tp, cmd, data, flag);
	if (error < 0)
		error = ttioctl(tp, cmd, data, flag);
#else
	error = ttioctl(tp, cmd, data, flag);
	/*
	 * Since we use the tty queues internally,
	 * pty's can't be switched to disciplines which overwrite
	 * the queues.  We can't tell anything about the discipline
	 * from here...
	 */
	if (linesw[tp->t_line].l_rint != ttyinput) {
		(*linesw[tp->t_line].l_close)(tp, flag);
		tp->t_line = 0;
		(void)(*linesw[tp->t_line].l_open)(dev, tp);
		error = ENOTTY;
	}
#endif
	if (error < 0) {
		if ((pti->pt_flags & PF_UCNTL) &&
		    (cmd & ~0xff) == UIOCCMD(0)) {
			if (cmd & 0xff) {
				pti->pt_ucntl = (u_char)cmd;
				ptcwakeup(tp, FREAD);
			}
			return (0);
		}
		error = ENOTTY;
	}
	stop = (tp->t_flags & RAW) == 0 &&
	    tp->t_stopc == CTRL(s) && tp->t_startc == CTRL(q);
	if (pti->pt_flags & PF_NOSTOP) {
		if (stop) {
			pti->pt_send &= ~TIOCPKT_NOSTOP;
			pti->pt_send |= TIOCPKT_DOSTOP;
			pti->pt_flags &= ~PF_NOSTOP;
			ptcwakeup(tp, FREAD);
		}
	} else {
		if (!stop) {
			pti->pt_send &= ~TIOCPKT_DOSTOP;
			pti->pt_send |= TIOCPKT_NOSTOP;
			pti->pt_flags |= PF_NOSTOP;
			ptcwakeup(tp, FREAD);
		}
	}
	return (error);
}
#endif
