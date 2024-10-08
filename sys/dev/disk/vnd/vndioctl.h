/*	$NetBSD: vndvar.h,v 1.12.6.1 2005/05/13 18:06:35 riz Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: fdioctl.h 1.1 90/07/09$
 *
 *	@(#)vnioctl.h	8.1 (Berkeley) 6/10/93
 */

/*
 * Copyright (c) 1988 University of Utah.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: fdioctl.h 1.1 90/07/09$
 *
 *	@(#)vnioctl.h	8.1 (Berkeley) 6/10/93
 */

/*
 * Vnode disk pseudo-geometry information.
 */
struct vndgeom {
	u_int32_t		vng_secsize;	/* # bytes per sector */
	u_int32_t		vng_nsectors;	/* # data sectors per track */
	u_int32_t		vng_ntracks;	/* # tracks per cylinder */
	u_int32_t		vng_ncylinders;	/* # cylinders per unit */
};

/*
 * Ioctl definitions for file (vnode) disk pseudo-device.
 */
struct vnd_ioctl {
	char				*vnd_file;	/* pathname of file to mount */
	int					vnd_flags;	/* flags; see below */
	struct vndgeom		vnd_geom;	/* geometry to emulate */
	int					vnd_size;	/* (returned) size of disk */
};

/* vnd_flags */
#define	VNDIOF_HASGEOM	0x01		/* use specified geometry */
#define	VNDIOF_READONLY	0x02		/* as read-only device */
#define	VNDIOF_FORCE	0x04		/* force close */
#define	VNDIOF_FILEIO	0x08		/* have to use read/write */

#ifdef _KERNEL

struct vnode;
struct ucred;

/*
 * A vnode disk's state information.
 */
struct vnd_softc {
	int		 			sc_unit;		/* logical unit number */
	int			 		sc_flags;		/* flags */
	size_t		 		sc_size;		/* size of vnd */
	struct vnode		*sc_vp;			/* vnode */
	struct ucred		*sc_cred;		/* credentials */
	int		 			sc_maxactive;	/* max # of active requests */
	struct bufq_state 	sc_tab;			/* transfer queue */
	int				 	sc_active;		/* number of active transfers */
	char		 		sc_xname[8];	/* XXX external name */
	struct dkdevice		sc_dkdev;		/* generic disk device info */
	struct vndgeom	 	sc_geom;		/* virtual geometry */
	struct proc 		*sc_kthread;	/* kernel thread */
};
#endif

/* sc_flags */
#define	VNF_INITED		0x01	/* unit has been initialized */
#define	VNF_WLABEL		0x02	/* label area is writable */
#define	VNF_LABELLING	0x04	/* unit is currently being labelled */
#define	VNF_WANTED		0x08	/* someone is waiting to obtain a lock */
#define	VNF_LOCKED		0x10	/* unit is locked */
#define	VNF_READONLY	0x020	/* unit is read-only */
#define	VNF_KLABEL		0x040	/* keep label on close */
#define	VNF_VLABEL		0x080	/* label is valid */
#define	VNF_KTHREAD		0x100	/* thread is running */
#define	VNF_VUNCONF		0x200	/* device is unconfiguring */

/*
 * A simple structure for describing which vnd units are in use.
 */
struct vnd_user {
	int					vnu_unit;	/* which vnd unit */
	dev_t				vnu_dev;	/* file is on this device... */
	ino_t				vnu_ino;	/* ...at this inode */
};

/*
 * Before you can use a unit, it must be configured with VNDIOCSET.
 * The configuration persists across opens and closes of the device;
 * an VNDIOCCLR must be used to reset a configuration.  An attempt to
 * VNDIOCSET an already active unit will return EBUSY.
 */
#define VNDIOCSET	_IOWR('F', 0, struct vnd_ioctl)	/* enable disk */
#define VNDIOCCLR	_IOW('F', 1, struct vnd_ioctl)	/* disable disk */
#define VNDIOCGET	_IOWR('F', 2, struct vnd_user)	/* get list */
