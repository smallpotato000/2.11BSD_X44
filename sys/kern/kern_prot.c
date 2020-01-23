/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)kern_prot.c	1.4 (2.11BSD GTE) 1997/11/28
 */

/*
 * System calls related to processes and protection
 */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/malloc.h>

void
getpid()
{

	u->u_r.r_val1 = u->u_procp->p_pid;
	u->u_r.r_val2 = u->u_procp->p_ppid;	/* XXX - compatibility */
}

void
getppid()
{

	u->u_r.r_val1 = u->u_procp->p_ppid;
}

void
getpgrp()
{
	register struct a {
		int	pid;
	} *uap = (struct a *)u->u_ap;
	register struct proc *p;

	if (uap->pid == 0)		/* silly... */
		uap->pid = u->u_procp->p_pid;
	p = pfind(uap->pid);
	if (p == 0) {
		u->u_error = ESRCH;
		return;
	}
	u->u_r.r_val1 = p->p_pgrp;
}

void
getuid()
{

	u->u_r.r_val1 = u->u_ruid;
	u->u_r.r_val2 = u->u_uid;		/* XXX */
}

void
geteuid()
{

	u->u_r.r_val1 = u->u_uid;
}

void
getgid()
{

	u->u_r.r_val1 = u->u_rgid;
	u->u_r.r_val2 = u->u_groups[0];		/* XXX */
}

void
getegid()
{

	u->u_r.r_val1 = u->u_groups[0];
}

/*
 * getgroups and setgroups differ from 4.X because the VAX stores group
 * entries in the user structure as shorts and has to convert them to ints.
 */
void
getgroups()
{
	register struct	a {
		u_int	gidsetsize;
		int	*gidset;
	} *uap = (struct a *)u->u_ap;
	register gid_t *gp;

	for (gp = &u->u_groups[NGROUPS]; gp > u->u_groups; gp--)
		if (gp[-1] != NOGROUP)
			break;
	if (uap->gidsetsize < gp - u->u_groups) {
		u->u_error = EINVAL;
		return;
	}
	uap->gidsetsize = gp - u->u_groups;
	u->u_error = copyout((caddr_t)u->u_groups, (caddr_t)uap->gidset,
	    uap->gidsetsize * sizeof(u->u_groups[0]));
	if (u->u_error)
		return;
	u->u_r.r_val1 = uap->gidsetsize;
}

void
setpgrp()
{
	register struct proc *p;
	register struct a {
		int	pid;
		int	pgrp;
	} *uap = (struct a *)u->u_ap;

	if (uap->pid == 0)		/* silly... */
		uap->pid = u->u_procp->p_pid;
	p = pfind(uap->pid);
	if (p == 0) {
		u->u_error = ESRCH;
		return;
	}
/* need better control mechanisms for process groups */
	if (p->p_uid != u->u_uid && u->u_uid && !inferior(p)) {
		u->u_error = EPERM;
		return;
	}
	p->p_pgrp = uap->pgrp;
}

void
setgroups()
{
	register struct	a {
		u_int	gidsetsize;
		int	*gidset;
	} *uap = (struct a *)u->u_ap;

	register gid_t *gp;

	if (!suser())
		return;
	if (uap->gidsetsize > sizeof (u->u_groups) / sizeof (u->u_groups[0])) {
		u->u_error = EINVAL;
		return;
	}
	u->u_error = copyin((caddr_t)uap->gidset, (caddr_t)u->u_groups,
	uap->gidsetsize * sizeof (u->u_groups[0]));
	if (u->u_error)
		return;
	for (gp = &u->u_groups[uap->gidsetsize]; gp < &u->u_groups[NGROUPS]; gp++)
		*gp = NOGROUP;
}

/*
 * Check if gid is a member of the group set.
 */
int
groupmember(gid)
	gid_t gid;
{
	register gid_t *gp;

	for (gp = u->u_groups; gp < &u->u_groups[NGROUPS] && *gp != NOGROUP; gp++)
		if (*gp == gid)
			return (1);
	return (0);
}

/*
 * Get login name, if available.
*/
int
getlogin()
{
	register struct a {
		char *namebuf;
		u_int namelen;
	} *uap = (struct a *)u->u_ap;

	register int error;
	if	(uap->namelen > sizeof (u->u_login))
		uap->namelen = sizeof (u->u_login);
	error = copyout(u->u_login, uap->namebuf, uap->namelen);
	return(u->u_error = error);
}

/*
 * Set login name.
 * It is not clear whether this should be allowed if the process
 * is not the "session leader" (the 'login' process).  But since 2.11
 * doesn't have sessions and it's almost impossible to know if a process
 * is "login" or not we simply restrict this call to the super user.
*/

int
setlogin()
{
	register struct a {
		char *namebuf;
	} *uap = (struct a *)u->u_ap;

	register int error;
	char	newname[MAXLOGNAME + 1];

	if	(!suser())
		return(u->u_error);	/* XXX - suser should be changed! */
/*
 * copinstr() wants to copy a string including a nul but u_login is not
 * necessarily nul-terminated.  Copy into a temp that is one character
 * longer then copy into place if that fit.
*/

	bzero(newname, sizeof (newname));
	error = copyinstr(uap->namebuf, newname, sizeof(newname), NULL);
	if	(error == 0)
		bcopy(newname, u->u_login, sizeof (u->u_login));
	return(u->u_error = error);
}



/*
 * Test whether the specified credentials imply "super-user"
 * privilege; if so, and we have accounting info, set the flag
 * indicating use of super-powers.
 * Returns 0 or error.
 */
int
suser(cred, acflag)
	struct ucred *cred;
	u_short *acflag;
{
	if (cred->cr_uid == 0) {
		if (acflag)
			*acflag |= ASU;
		return (0);
	}
	return (EPERM);
}

/*
 * Allocate a zeroed cred structure.
 */
struct ucred *
crget()
{
	register struct ucred *cr;

	MALLOC(cr, struct ucred *, sizeof(*cr), M_CRED, M_WAITOK);
	bzero((caddr_t)cr, sizeof(*cr));
	cr->cr_ref = 1;
	return (cr);
}

/*
 * Free a cred structure.
 * Throws away space when ref count gets to 0.
 */
void
crfree(cr)
	struct ucred *cr;
{
	int s;

	s = splimp();				/* ??? */
	if (--cr->cr_ref == 0)
		FREE((caddr_t)cr, M_CRED);
	(void) splx(s);
}

/*
 * Copy cred structure to a new one and free the old one.
 */
struct ucred *
crcopy(cr)
	struct ucred *cr;
{
	struct ucred *newcr;

	if (cr->cr_ref == 1)
		return (cr);
	newcr = crget();
	*newcr = *cr;
	crfree(cr);
	newcr->cr_ref = 1;
	return (newcr);
}

/*
 * Dup cred struct to a new held one.
 */
struct ucred *
crdup(cr)
	struct ucred *cr;
{
	struct ucred *newcr;

	newcr = crget();
	*newcr = *cr;
	newcr->cr_ref = 1;
	return (newcr);
}

