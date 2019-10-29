/*
 * kern_exec2.c
 *
 *  Created on: 26 Oct 2019
 *      Author: marti
 */


#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/resourcevar.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/filedesc.h>
#include <sys/file.h>
#include <sys/acct.h>
#include <sys/exec.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/map.h>
#include <sys/syslog.h>


#include <vm/vm.h>
#include <vm/vm_kern.h>

#include <machine/reg.h>

int *exec_copyout_strings __P((struct image_params *));

static int exec_check_permissions(struct image_params *);

/*
 * execve() system call.
 */
int
execve(p, uap, retval)
	struct proc *p;
	register struct execve_args *uap;
	int *retval;
{
	struct nameidata nd, *ndp;
	int *stack_base;
	int error, len, i;
	struct image_params image_params, *iparams;
	struct vnode *vnodep;
	struct vattr attr;
	char *image_header;

	iparams = &image_params;
	bzero((caddr_t)iparams, sizeof(struct image_params));
	image_header = (char *)0;

	/*
	 * Initialize a few constants in the common area
	 */
	iparams->proc = p;
	iparams->uap = uap;
	iparams->attr = &attr;

	/*
	 * Allocate temporary demand zeroed space for argument and
	 *	environment strings
	 */
	error = vm_allocate(kernel_map, (vm_offset_t *)&iparams->stringbase,
			    ARG_MAX, TRUE);
	if (error) {
		log(LOG_WARNING, "execve: failed to allocate string space\n");
		return (error);
	}

	if (!iparams->stringbase) {
		error = ENOMEM;
		goto exec_fail;
	}
	iparams->stringp = iparams->stringbase;
	iparams->stringspace = ARG_MAX;

	/*
	 * Translate the file name. namei() returns a vnode pointer
	 *	in ni_vp amoung other things.
	 */
	ndp = &nd;
	ndp->ni_cnd.cn_nameiop = LOOKUP;
	ndp->ni_cnd.cn_flags = LOCKLEAF | FOLLOW | SAVENAME;
	ndp->ni_cnd.cn_proc = curproc;
	ndp->ni_cnd.cn_cred = curproc->p_cred->pc_ucred;
	ndp->ni_segflg = UIO_USERSPACE;
	ndp->ni_dirp = uap->fname;

interpret:

	error = namei(ndp);
	if (error) {
		vm_deallocate(kernel_map, (vm_offset_t)iparams->stringbase, ARG_MAX);
		goto exec_fail;
	}

	iparams->vnodep = vnodep = ndp->ni_vp;

	if (vnodep == NULL) {
		error = ENOEXEC;
		goto exec_fail_dealloc;
	}

	/*
	 * Check file permissions (also 'opens' file)
	 */
	error = exec_check_permissions(iparams);
	if (error)
		goto exec_fail_dealloc;

	/*
	 * Map the image header (first page) of the file into
	 *	kernel address space
	 */
	error = vm_mmap(kernel_map,				/* map */
			(vm_offset_t *)&image_header,	/* address */
			PAGE_SIZE,						/* size */
			VM_PROT_READ, 					/* protection */
			VM_PROT_READ, 					/* max protection */
			0,	 							/* flags */
			(caddr_t)vnodep,				/* vnode */
			0);								/* offset */
	if (error) {
		uprintf("mmap failed: %d\n",error);
		goto exec_fail_dealloc;
	}
	iparams->image_header = image_header;

	/*
	 * Loop through list of image activators, calling each one.
	 *	If there is no match, the activator returns -1. If there
	 *	is a match, but there was an error during the activation,
	 *	the error is returned. Otherwise 0 means success. If the
	 *	image is interpreted, loop back up and try activating
	 *	the interpreter.
	 */
	for (i = 0; execsw[i]; ++i) {
		if (execsw[i]->ex_imgact)
			error = (*execsw[i]->ex_imgact)(iparams);
		else
			continue;

		if (error == -1)
			continue;
		if (error)
			goto exec_fail_dealloc;
		if (iparams->interpreted) {
			/* free old vnode and name buffer */
			vput(ndp->ni_vp);
			//FREE(ndp->ni_cnd.cn_pnbuf, M_NAMEI);
			mfree(ndp->ni_cnd.cn_pnbuf);
			if (vm_deallocate(kernel_map, (vm_offset_t)image_header, PAGE_SIZE))
				panic("execve: header dealloc failed (1)");

			/* set new name to that of the interpreter */
			ndp->ni_segflg = UIO_SYSSPACE;
			ndp->ni_dirp = iparams->interpreter_name;
			ndp->ni_cnd.cn_nameiop = LOOKUP;
			ndp->ni_cnd.cn_flags = LOCKLEAF | FOLLOW | SAVENAME;
			ndp->ni_cnd.cn_proc = curproc;
			ndp->ni_cnd.cn_cred = curproc->p_cred->pc_ucred;
			goto interpret;
		}
		break;
	}
	/* If we made it through all the activators and none matched, exit. */
	if (error == -1) {
		error = ENOEXEC;
		goto exec_fail_dealloc;
	}

	/*
	 * Copy out strings (args and env) and initialize stack base
	 */
	stack_base = exec_copyout_strings(iparams);
	p->p_vmspace->vm_minsaddr = (char *)stack_base;

	/*
	 * Stuff argument count as first item on stack
	 */
	*(--stack_base) = iparams->argc;

	/* close files on exec */
	fdcloseexec(p);

	/* reset caught signals */
	execsigs(p);

	/* name this process - nameiexec(p, ndp) */
	len = min(ndp->ni_cnd.cn_namelen, MAXCOMLEN);
	bcopy(ndp->ni_cnd.cn_nameptr, p->p_comm, len);
	p->p_comm[len] = 0;

	/*
	 * mark as executable, wakeup any process that was vforked and tell
	 * it that it now has it's own resources back
	 */
	p->p_flag |= P_EXEC;
	if (p->p_pptr && (p->p_flag & P_PPWAIT)) {
		p->p_flag &= ~P_PPWAIT;
		wakeup((caddr_t)p->p_pptr);
	}

	/* implement set userid/groupid */
	p->p_flag &= ~P_SUGID;

	/*
	 * Turn off kernel tracing for set-id programs, except for
	 * root.
	 */
	if (p->p_tracep && (attr.va_mode & (VSUID | VSGID)) &&
	    suser(p->p_ucred, &p->p_acflag)) {
		p->p_traceflag = 0;
		vrele(p->p_tracep);
		p->p_tracep = 0;
	}
	if ((attr.va_mode & VSUID) && (p->p_flag & P_TRACED) == 0) {
		p->p_ucred = crcopy(p->p_ucred);
		p->p_ucred->cr_uid = attr.va_uid;
		p->p_flag |= P_SUGID;
	}
	if ((attr.va_mode & VSGID) && (p->p_flag & P_TRACED) == 0) {
		p->p_ucred = crcopy(p->p_ucred);
		p->p_ucred->cr_groups[0] = attr.va_gid;
		p->p_flag |= P_SUGID;
	}

	/*
	 * Implement correct POSIX saved uid behavior.
	 */
	p->p_cred->p_svuid = p->p_ucred->cr_uid;
	p->p_cred->p_svgid = p->p_ucred->cr_gid;

	/* mark vnode pure text */
 	ndp->ni_vp->v_flag |= VTEXT;

	/*
	 * Store the vp for use in procfs
	 */
	if (p->p_textvp)		/* release old reference */
		vrele(p->p_textvp);
	VREF(ndp->ni_vp);
	p->p_textvp = ndp->ni_vp;

	/*
	 * If tracing the process, trap to debugger so breakpoints
	 * 	can be set before the program executes.
	 */
	if (p->p_flag & P_TRACED)
		psignal(p, SIGTRAP);

	/* clear "fork but no exec" flag, as we _are_ execing */
	p->p_acflag &= ~AFORK;

	/* Set entry address */
	setregs(p, iparams->entry_addr, (u_long)stack_base);

	/*
	 * free various allocated resources
	 */
	if (vm_deallocate(kernel_map, (vm_offset_t)iparams->stringbase, ARG_MAX))
		panic("execve: string buffer dealloc failed (1)");
	if (vm_deallocate(kernel_map, (vm_offset_t)image_header, PAGE_SIZE))
		panic("execve: header dealloc failed (2)");
	vput(ndp->ni_vp);
	//FREE(ndp->ni_cnd.cn_pnbuf, M_NAMEI);
	mfree(ndp->ni_cnd.cn_pnbuf);

	return (0);

exec_fail_dealloc:
	if (iparams->stringbase && iparams->stringbase != (char *)-1)
		if (vm_deallocate(kernel_map, (vm_offset_t)iparams->stringbase,
				  ARG_MAX))
			panic("execve: string buffer dealloc failed (2)");
	if (iparams->image_header && iparams->image_header != (char *)-1)
		if (vm_deallocate(kernel_map,
				  (vm_offset_t)iparams->image_header, PAGE_SIZE))
			panic("execve: header dealloc failed (3)");
	vput(ndp->ni_vp);
	//FREE(ndp->ni_cnd.cn_pnbuf, M_NAMEI);
	mfree(ndp->ni_cnd.cn_pnbuf);

exec_fail:
	if (iparams->vmspace_destroyed) {
		/* sorry, no more process anymore. exit gracefully */
#if 0	/* XXX */
		vm_deallocate(&vs->vm_map, USRSTACK - MAXSSIZ, MAXSSIZ);
#endif
		exit1(p, W_EXITCODE(0, SIGABRT));
		/* NOT REACHED */
		return(0);
	} else {
		return(error);
	}

