/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)subr_log.c	2.1 (2.11BSD) 1999/4/29
 */

/*
 * logioctl() had the wrong number of arguments.  Argh!  Apparently this
 * driver was overlooked when 'dev' was added to ioctl entry points.
 *
 * logclose() returned garbage.  this went unnoticed because most programs
 * don't check status when doing a close.

 * Remove vax ifdefs - this driver is never going back to vaxland.
 *
 * Add support for multiple log devices.  Minor device 0 is the traditional
 * kernel logger (/dev/klog), minor device 1 is reserved for the future device 
 * error logging daemon.  Minor device 2 is used by the 'accounting' daemon
 * 'acctd'.
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/msgbuf.h>
#include <sys/file.h>
#include <sys/vnode.h>
#include <sys/uio.h>
#include <sys/map.h>

#define	NLOG	3
int	nlog = NLOG;

#define LOG_RDPRI	(PZERO + 1)

#define	LOG_OPEN	0x01
#define LOG_ASYNC	0x04
#define LOG_RDWAIT	0x08

/*
 * This is an efficiency hack.  This global is provided for exit() to
 * test and avoid the overhead of function calls when accounting is
 * turned off.
*/
int	Acctopen;
struct	msgbuf	msgbuf[NLOG];
static struct logsoftc {
	int				sc_state;			/* see above for possibilities */
	struct	proc 	*sc_selp;			/* process waiting on select call */
	int				sc_pgid;			/* process/group for async I/O */
	int				sc_overrun;			/* full buffer count */
} logsoftc[NLOG];

/*ARGSUSED*/
int
logopen(dev, mode)
	dev_t dev;
	int mode;
{
	register int	unit = minor(dev);

	if	(unit >= NLOG)
		return(ENODEV);
	if	(logisopen(unit))
		return(EBUSY);
	if	(msgbuf[unit].msg_click == 0)	/* no buffer allocated */
		return(ENOMEM);
	logsoftc[unit].sc_state |= LOG_OPEN;
	if	(unit == logACCT)
		Acctopen = 1;
	logsoftc[unit].sc_pgid = u->u_procp->p_pid;  /* signal process only */
	logsoftc[unit].sc_overrun = 0;
	return(0);
}

/*ARGSUSED*/
int
logclose(dev, flag)
	dev_t	dev;
	int	flag;
{
	register int unit = minor(dev);

	logsoftc[unit].sc_state = 0;
	if	(unit == logACCT)
		Acctopen = 0;
	return(0);
}

/*
 * This is a helper function to keep knowledge of this driver's data
 * structures away from the rest of the kernel.
*/
int
logisopen(unit)
	int	unit;
{
	if	(logsoftc[unit].sc_state & LOG_OPEN)
		return(1);
	return(0);
}

/*ARGSUSED*/
int
logread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	register int l;
	register struct logsoftc *lp;
	register struct msgbuf *mp;
	int	s, error = 0;
	char	buf[ctob(2)];

	l = minor(dev);
	lp = &logsoftc[l];
	mp = &msgbuf[l];
	s = splhigh();
	while	(mp->msg_bufr == mp->msg_bufx) {
		if	(flag & IO_NDELAY) {
			splx(s);
			return(EWOULDBLOCK);
		}
		lp->sc_state |= LOG_RDWAIT;
		if (error == sleep((caddr_t)mp, LOG_RDPRI | PCATCH, "klog", 0)) {
			splx(0);
			return (error);
		}
	}

	splx(s);
	lp->sc_state &= ~LOG_RDWAIT;

	while	(uio->uio_resid > 0) {
		l = mp->msg_bufx - mp->msg_bufr;
/*
 * If the reader and writer are equal then we have caught up and there
 * is nothing more to transfer.
*/
		if	(l == 0)
			break;

/*
 * If the write pointer is behind the reader then only consider as
 * available for now the bytes from the read pointer thru the end of 
 * the buffer.
*/
		if	(l < 0) {
			l = MSG_BSIZE - mp->msg_bufr;
/*
 * If the reader is exactly at the end of the buffer it is
 * time to wrap it around to the beginning and recalculate the
 * amount of data to transfer.
*/
			if	(l == 0) {
				mp->msg_bufr = 0;
				continue;
			}
		}
		l = MIN(l, uio->uio_resid);
		l = MIN(l, sizeof buf);
		bcopy(&mp->msg_bufc[mp->msg_bufr], buf, l);
		error = uiomove(buf, l, uio);
		if	(error)
			break;
		mp->msg_bufr += l;
	}
	splx(s);
	return(error);
}

/*ARGSUSED*/
int
logselect(dev, rw)
	dev_t dev;
	int rw;
{
	register int s = splhigh();
	int	unit = minor(dev);

	switch	(rw) {
	case FREAD:
		if	(msgbuf[unit].msg_bufr != msgbuf[unit].msg_bufx) {
			splx(s);
			return(1);
		}
		logsoftc[unit].sc_selp = u->u_procp;
		break;
	}
	splx(s);
	return(0);
}

void
logwakeup(unit)
	int	unit;
{
	register struct proc *p;
	register struct logsoftc *lp;
	register struct msgbuf *mp;

	if	(!logisopen(unit))
		return;
	lp = &logsoftc[unit];
	mp = &msgbuf[unit];
	if	(lp->sc_selp) {
		selwakeup(lp->sc_selp, (long) 0);
		lp->sc_selp = 0;
	}
	if	((lp->sc_state & LOG_ASYNC) && (mp->msg_bufx != mp->msg_bufr)) {
		if	(lp->sc_pgid < 0)
			gsignal(-lp->sc_pgid, SIGIO); 
		else if (p == pfind(lp->sc_pgid))
			psignal(p, SIGIO);
		}
	if	(lp->sc_state & LOG_RDWAIT) {
		wakeup((caddr_t)mp);
		lp->sc_state &= ~LOG_RDWAIT;
	}
}

/*ARGSUSED*/
int
logioctl(dev, com, data, flag)
	dev_t	dev;
	u_int	com;
	caddr_t data;
	int	flag;
{
	long l;
	register int s;
	int	unit;
	register struct logsoftc *lp;
	register struct msgbuf *mp;

	unit = minor(dev);
	lp = &logsoftc[unit];
	mp = &msgbuf[unit];

	switch	(com) {
	case	FIONREAD:
		s = splhigh();
		l = mp->msg_bufx - mp->msg_bufr;
		splx(s);
		if	(l < 0)
			l += MSG_BSIZE;
			*(off_t *)data = l;
		break;
	case	FIONBIO:
			break;
	case	FIOASYNC:
		if (*(int *)data)
			lp->sc_state |= LOG_ASYNC;
		else
			lp->sc_state &= ~LOG_ASYNC;
		break;
	case	TIOCSPGRP:
		lp->sc_pgid = *(int *)data;
		break;
	case	TIOCGPGRP:
		*(int *)data = lp->sc_pgid;
		break;
	default:
		return(-1);
	}
	return(0);
}

/*
 * This is inefficient for single character writes.  Alas, changing this
 * to be buffered would affect the networking code's use of printf.  
*/
int
logwrt(buf,len,log)
	char	*buf;
	int	len;
	int	log;
{
	register struct msgbuf *mp = &msgbuf[log];
	struct	logsoftc *lp = &logsoftc[log];
	register int	infront;
	int  s, n, writer, err = 0;

	if	(mp->msg_magic != MSG_MAGIC || (len > MSG_BSIZE))
		return(-1);
/*
 * Hate to do this but since this can be called from anywhere in the kernel
 * we have to hold off any interrupt service routines so they don't change
 * things.  This looks like a lot of code but it isn't really.
*/
	s = splhigh();
	while	(len) {
again:
	infront = MSG_BSIZE - mp->msg_bufx;
		if	(infront <= 0) {
			mp->msg_bufx = 0;
			infront = MSG_BSIZE - mp->msg_bufr;
		}
		n = mp->msg_bufr - mp->msg_bufx;
		if	(n < 0)		/* bufr < bufx */
			writer = (MSG_BSIZE - mp->msg_bufx) + mp->msg_bufr;
		else if	(n == 0)
			writer = MSG_BSIZE;
		else {
			writer = n;
			infront = n;
		}
		if	(len > writer) {
			/*
			 * won't fit.  the total number of bytes to be written is
			 * greater than the number available.  the buffer is full.
			 * throw away the old data and keep the current data by resetting
			 * the 'writer' pointer to the current 'reader' position.  Bump the
			 * overrun counter in case anyone wants to look at it for debugging.
			*/
			lp->sc_overrun++;
			mp->msg_bufx = mp->msg_bufr;
			goto	again;
		}
		if	(infront > len)
			infront = len;
		bcopy(buf, &mp->msg_bufc[mp->msg_bufx], infront);
		mp->msg_bufx += infront;
		len -= infront;
		buf += infront;
	}
out:	splx(s);
	return(err);
}

void
loginit(void)
{
	register struct msgbuf *mp;
	long new_bufs;

	/* Sanity-check the given size. */
	if (MSG_BSIZE < sizeof(struct msgbuf)) {
		return;
	}

	new_bufs = MSG_BSIZE - offsetof(struct msgbuf, msg_bufc);
	for	(mp = &msgbuf[0]; mp < &msgbuf[NLOG]; mp++) {
		mp->msg_click = rmalloc(coremap, btoc(MSG_BSIZE));
		if (!mp->msg_click) {
			return;
		}
		mp->msg_magic = MSG_MAGIC;
		mp->msg_bufc = new_bufs;
		mp->msg_bufx = mp->msg_bufr = 0;
	}
}
