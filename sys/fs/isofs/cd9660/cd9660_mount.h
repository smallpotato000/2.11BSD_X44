/*
 * Copyright (c) 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley
 * by Pace Willisson (pace@blitz.com).  The Rock Ridge Extension
 * Support code is derived from software contributed to Berkeley
 * by Atsushi Murai (amurai@spec.co.jp).
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
 *	@(#)cd9660_mount.h	8.1 (Berkeley) 5/24/95
 */

#ifndef _ISOFS_CD9660_CD9660_MOUNT_H_
#define _ISOFS_CD9660_CD9660_MOUNT_H_

/*
 * Arguments to mount ISO 9660 filesystems.
 */
struct iso_args {
	char				*fspec;		/* block special device to mount */
	struct	export_args export;		/* network export info */
	int					flags;		/* mounting flags, see below */
};
#define	ISOFSMNT_NORRIP			0x00000001	/* disable Rock Ridge Ext.*/
#define	ISOFSMNT_GENS			0x00000002	/* enable generation numbers */
#define	ISOFSMNT_EXTATT			0x00000004	/* enable extended attributes */
#define	ISOFSMNT_NOJOLIET		0x00000008 	/* disable Joliet extensions */
#define	ISOFSMNT_NOCASETRANS	0x00000010 	/* do not make names lower case */
#define	ISOFSMNT_RRCASEINS		0x00000020 	/* case insensitive Rock Ridge */

#endif /* !_ISOFS_CD9660_CD9660_MOUNT_H_ */
