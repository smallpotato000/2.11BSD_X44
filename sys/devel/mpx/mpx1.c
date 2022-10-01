/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

/*
 * Copyright (c) 1996 John S. Dyson
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice immediately at the beginning of the file, without modification,
 *    this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Absolutely no warranty of function or purpose is made by the author
 *    John S. Dyson.
 * 4. Modifications may be freely made to this file if the above conditions
 *    are met.
 *
 * $FreeBSD: src/sys/kern/sys_pipe.c,v 1.95 2002/03/09 22:06:31 alfred Exp $
 */

#include <sys/cdefs.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/conf.h>
#include <sys/lock.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/null.h>

#include <vm/include/vm_extern.h>

#include <devel/mpx/mpx.h>
#include <devel/sys/malloctypes.h>

int
mpx()
{
	/*
	register struct mpx_args {
		syscallarg(int *) fdp;
	} *uap = (struct mpx_args *) u.u_ap;
	*/
	struct file *rf, *wf;
	struct mpx *rmpx, *wmpx;
	struct mpxpair *mm;
	int fd, error;

	mm = mpx_paircreate();
	if(mm == NULL) {
		return (ENOMEM);
	}
	rmpx = &mm->mpp_rmpx;
	wmpx = &mm->mpp_wmpx;

	rf = falloc();
	if (rf != NULL) {
		u.u_r.r_val1 = fd;
		rf->f_flag = FREAD;
		rf->f_type = DTYPE_PIPE;
		rf->f_mpx = rmpx;
		rf->f_ops = &mpxops;
		error = ufdalloc(rf);
		if(error != 0) {
			goto free2;
		}
	} else {
		u.u_error = ENFILE;
		goto free2;
	}
	wf = falloc();
	if (wf != NULL) {
		u.u_r.r_val2 = fd;
		wf->f_flag = FWRITE;
		wf->f_type = DTYPE_PIPE;
		wf->f_mpx = wmpx;
		wf->f_ops = &mpxops;
		error = ufdalloc(wf);
		if(error != 0) {
			goto free3;
		}
	} else {
		u.u_error = ENFILE;
		goto free3;
	}

	rmpx->mpx_peer = wmpx;
	wmpx->mpx_peer = rmpx;

	FILE_UNUSE(rf, u.u_procp);
	FILE_UNUSE(wf, u.u_procp);
	return (0);

free3:
	FILE_UNUSE(rf, u.u_procp);
	ffree(rf);
	fdremove(u.u_r.r_val1);

free2:
	mpxclose(NULL, rmpx);
	mpxclose(NULL, wmpx);
	return (u.u_error);
}


mpxn_open()
{

}

mpxn_ioctl()
{
	struct mpx 			*mpx;
	struct mpx_group 	*grp;
	struct mpx_channel 	*chan;

	switch (cmd) {
	case MPXIOATTACH:
		mpx_attach(chan, grp);
		break;
	case MPXIODETACH:
		mpx_detach(chan, grp);
		break;
	case MPXIOCONNECT:
		mpx_connect(chan, chan1);
		break;
	case MPXIODISCONNECT:
		mpx_disconnect(chan, nchans);
		break;
	}
}
