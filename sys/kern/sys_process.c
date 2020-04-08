/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)sys_process.c	1.2 (2.11BSD) 1999/9/5
 */

#define IPCREG
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/vnode.h>
#include <sys/buf.h>
#include <sys/ptrace.h>

#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <vm/include/vm.h>
#include <vm/include/vm_page.h>

#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/seg.h>

/*
 * Priority for tracing
 */
#define	IPCPRI	PZERO

/*
 * Tracing variables.
 * Used to pass trace command from
 * parent to child being traced.
 * This data base cannot be
 * shared and is locked
 * per user.
 */
struct
{
	int	ip_lock;
	int	ip_req;
	int	*ip_addr;
	int	ip_data;
} ipc;


/*
 * sys-trace system call.
 */
void
ptrace()
{
	register struct proc *p;

	struct ptrace_args *uap;

	uap = (struct a *)u->u_ap;
	if (uap->req <= 0) {
		u->u_procp->p_flag |= P_TRACED;
		return;
	}
	p = pfind(uap->pid);
	if (p == 0 || p->p_stat != SSTOP || p->p_ppid != u->u_procp->p_pid ||
	    !(p->p_flag & P_TRACED)) {
		u->u_error = ESRCH;
		return;
	}
	while (ipc.ip_lock)
		sleep((caddr_t)&ipc, PZERO);
	ipc.ip_lock = p->p_pid;
	ipc.ip_data = uap->data;
	ipc.ip_addr = uap->addr;
	ipc.ip_req = uap->req;
	p->p_flag &= ~P_WAITED;
	setrun(p);
	while (ipc.ip_req > 0)
		sleep((caddr_t)&ipc, PZERO);
	u->u_r.r_val1 = (short)ipc.ip_data;
	if (ipc.ip_req < 0)
		u->u_error = EIO;
	ipc.ip_lock = 0;
	wakeup((caddr_t)&ipc);
}

#define	PHYSOFF(p, o) ((caddr_t)(p) + (o))
#define PHYSALIGNED(a) (((int)(a) & (sizeof(int) - 1)) == 0)

int
procxmt(p)
	register struct proc *p;
{
	register int i, *poff;
	extern char kstack[];

	if (ipc.ip_lock != u->u_procp->p_pid)
		return(0);
	u->u_procp->p_slptime = 0;
	i = ipc.ip_req;
	ipc.ip_req = 0;
	switch (i) {

	/* read user I */
	case PT_READ_I:
		useracc();
		if (fuibyte((caddr_t)ipc.ip_addr) == -1 || !useracc((caddr_t)ipc.ip_addr, 4, B_READ))
			goto error;
		ipc.ip_data = fuiword((caddr_t)ipc.ip_addr);
		break;

	/* read user D */
	case PT_READ_D:
		if (fubyte((caddr_t)ipc.ip_addr) == -1 || !useracc((caddr_t)ipc.ip_addr, 4, B_READ))
			goto error;
		ipc.ip_data = fuword((caddr_t)ipc.ip_addr);
		break;

	/* read u */
	case PT_READ_U:
		i = (int)ipc.ip_addr;
		if (i < 0 || i > ctob(USIZE)-sizeof(int) || !PHYSALIGNED(i))
			goto error;
		ipc.ip_data = *(int *)PHYSOFF(p->p_addr, i);
		break;

	/* write user I */
	/* Must set up to allow writing */
	case PT_WRITE_I:
		if ((i = suiword((caddr_t)ipc.ip_addr, ipc.ip_data)) < 0) {
			vm_offset_t sa, ea;
			int rv;

			sa = trunc_page((vm_offset_t)ipc.ip_addr);
			ea = round_page((vm_offset_t)ipc.ip_addr+sizeof(int));
			rv = vm_map_protect(&p->p_vmspace->vm_map, sa, ea, VM_PROT_DEFAULT, FALSE);

			if (rv == KERN_SUCCESS) {
				//estabur(u->u_tsize, u->u_dsize, u->u_ssize, u->u_sep, RW);
				i = suiword((caddr_t)ipc.ip_addr, 0);
				suiword((caddr_t)ipc.ip_addr, ipc.ip_data);
				(void) vm_map_protect(&p->p_vmspace->vm_map, sa, ea, VM_PROT_READ|VM_PROT_EXECUTE, FALSE);
				//estabur(u->u_tsize, u->u_dsize, u->u_ssize, u->u_sep, RO);
			}
		}
		if (i<0)
			goto error;
		break;

	/* write user D */
	case PT_WRITE_D:
		if (suword((caddr_t)ipc.ip_addr, 0) < 0)
			goto error;
		(void)  suword((caddr_t)ipc.ip_addr, ipc.ip_data);
		break;

	case PT_WRITE_U:
		i = (int)ipc.ip_addr;
		poff = (int *)PHYSOFF(kstack, i);
		for (i=0; i < NIPCREG; i++)
			if (poff == &u->u_ar0[ipcreg[i]])
				goto ok;

		if (poff == &u->u_ar0[PS]) {
			ipc.ip_data |= PSL_USERSET;
			ipc.ip_data &= ~PSL_USERCLR;

#ifdef PSL_CM_CLR
			if (ipc.ip_data & PSL_CM)
				ipc.ip_data &= ~PSL_CM_CLR;
#endif
			goto ok;
		}
		goto error;
	ok:
		*poff = ipc.ip_data;
		break;

	/* set signal and continue */
	/*  one version causes a trace-trap */
	case PT_STEP:
		u->u_ar0[PS] |= PSL_T;
		break;
		/* FALL THROUGH TO ... */

	case PT_CONTINUE:
		if ((int)ipc.ip_addr != 1)
			u->u_ar0[PC] = (int)ipc.ip_addr;
		if (ipc.ip_data > NSIG)
			goto error;
		u->u_procp->p_ptracesig = ipc.ip_data; 		/* see issignal */

		wakeup((caddr_t)&ipc);
		return(1);

	/* force exit */
	case PT_KILL:
		wakeup((caddr_t)&ipc);
		exit(u->u_procp->p_ptracesig);
		/*NOTREACHED*/

	case PT_DETACH:									/* stop tracing the child */
		u->u_ar0 = (int *)((short *)u->u_ar0 + 1);
		if ((unsigned)ipc.ip_data >= NSIG)
			goto error;
		if ((int)ipc.ip_addr != 1)
			u->u_ar0[PC] = (int)ipc.ip_addr;
		u->u_procp->p_ptracesig = ipc.ip_data;		/* see issignal */
		p->p_flag &= ~P_TRACED;
		if (p->p_oppid != p->p_pptr->p_pid) {
			register struct proc *pp = pfind(p->p_oppid);
			 if (pp)
				 proc_reparent(p, pp);
		}
		p->p_oppid = 0;
		wakeup((caddr_t)&ipc);
		return (1);

	default:
	error:
			ipc.ip_req = -1;
	}
	wakeup((caddr_t)&ipc);
	return (0);
}
