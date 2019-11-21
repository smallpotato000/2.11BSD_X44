//
// Created by marti on 13/11/2019.
//
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/user.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <mx.h>

struct fileops mpxops =
        { mpx_read, mpx_write, mpx_ioctl, mpx_select, mpx_close };

struct chan	chans[NCHANS];
struct schan schans[NPORTS];
struct group *groups[NGROUPS];

int	mpxline;
struct chan *xcp();
dev_t mpxdev = -1;
char mcdebugs[NDEBUGS];

int
mpx_read(fp, uio, cred)
    struct file *fp;
    struct uio *uio;
    struct ucred *cred;
{
    return (mpxreceive((struct mpx *)fp->f_data, (struct mbuf **)0, uio, (struct mbuf **)0, (struct mbuf **)0, (int *)0));
}

int
mpx_write(fp, uio, cred)
    struct file *fp;
    struct uio *uio;
    struct ucred *cred;
{
    mpx_read();
    return mpxsend((struct mpx *)fp->f_data, (struct mbuf *)0, uio, (struct mbuf *)0, (struct mbuf *)0, 0));
}

int
mpx_ioctl(fp, cmd, data, p)
    struct file *fp;
    u_long cmd;
    register caddr_t data;
    struct proc *p;
{
    register struct mpx *mpx = (struct mpx *)fp->f_data;
    return ((*mpx->mpx_proto->pr_usrreq)(mpx, PRU_CONTROL, (struct mbuf *)cmd, (struct mbuf *)data, (struct mbuf *)0));
}

int
mpx_select(fp, which, p)
    struct file *fp;
    int which;
    struct proc *p;
{
    register struct mpx *mpx = (struct mpx *)fp->f_data;
    return (0);
}

int
mpx_stat(mpx, ub)
    register struct mpx *mpx;
    register struct stat *ub;
{
    bzero((caddr_t)ub, sizeof (*ub));
    ub->st_mode = S_IFCHAN;
    return ((*mpx->mpx_proto->pr_usrreq)(mpx, PRU_SENSE, (struct mbuf *)ub, (struct mbuf *)0, (struct mbuf *)0));
}

int
mpx_close(fp, p)
    struct file *fp;
    struct proc *p;
{
    int error = 0;
    if(fp->f_data)
        error = mpxclose((struct mpx *)fp->f_data);
    fp->f_data = 0;
    return (error);
}