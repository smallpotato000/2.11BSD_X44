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

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/malloc.h>
#include <sys/buf.h>

#include <ufs/ufml/ufml.h>
#include <ufs/ufml/ufml_extern.h>
#include <ufs/ufml/ufml_meta.h>
#include <ufs/ufml/ufml_ops.h>

void
ufml_meta_itimes(vp)
	struct vnode *vp;
{
	struct ufml_node *ip;
	struct ufml_metadata *meta;
	struct timespec ts;

	ip = VTOUFML(vp);
	meta = ip->ufml_meta;
	if ((meta->ufml_flag & (IN_ACCESS | IN_CHANGE | IN_UPDATE)) == 0) {
		return;
	}
	if ((vp->v_type == VBLK || vp->v_type == VCHR)) {
		meta->ufml_flag |= IN_LAZYMOD;
	} else {
		meta->ufml_flag |= IN_MODIFIED;
	}
	if ((vp->v_mount->mnt_flag & MNT_RDONLY) == 0) {
		vfs_timestamp(&ts);
		if (meta->ufml_flag & IN_ACCESS) {
			meta->ufml_atime = ts.tv_sec;
			meta->ufml_atimensec = ts.tv_nsec;
		}
		if (meta->ufml_flag & IN_UPDATE) {
			meta->ufml_mtime = ts.tv_sec;
			meta->ufml_mtimensec = ts.tv_nsec;
			meta->ufml_modrev++;
		}
		if (meta->ufml_flag & IN_CHANGE) {
			meta->ufml_ctime = ts.tv_sec;
			meta->ufml_ctimensec = ts.tv_nsec;
		}
	}
	meta->ufml_flag &= ~(IN_ACCESS | IN_CHANGE | IN_UPDATE);
}

/* Check filesystem types to see if the filesystem is supported */
int
ufml_check_filesystem(meta, fs)
	struct ufml_metadata *meta;
	int fs;
{
	meta->ufml_filesystem = 0;

	switch(fs) {
	case UFML_FFS:
		meta->ufml_filesystem = UFML_FFS;
		return (meta->ufml_filesystem);
		break;
	case UFML_MFS:
		meta->ufml_filesystem = UFML_MFS;
		return (meta->ufml_filesystem);
		break;
	case UFML_LFS:
		meta->ufml_filesystem = UFML_LFS;
		return (meta->ufml_filesystem);
		break;
	default:
		meta->ufml_filesystem = UFML_UFS;
		return (meta->ufml_filesystem);
		break;
	}

	return (0);
}

/* Check archive types to see if the archive format is supported */
int
ufml_check_archive(meta, type)
	struct ufml_metadata *meta;
	int			 	type;
{
	meta->ufml_archive = 0;

	switch (type) {
	case UFML_AR:
		meta->ufml_archive = UFML_AR;
		return (meta->ufml_archive);
		break;
	case UFML_CPIO:
		meta->ufml_archive = UFML_CPIO;
		return (meta->ufml_archive);
		break;
	default:
		meta->ufml_archive = UFML_TAR;
		return (meta->ufml_archive);
		break;
	}

	return (0);
}

/* Check compression types to see if the compression format is supported */
int
ufml_check_compression(meta, type)
	struct ufml_metadata *meta;
	int type;
{
	meta->ufml_compress = 0;

	switch (type) {
	case UFML_GZIP:
		meta->ufml_compress = UFML_GZIP;
		return (meta->ufml_compress);
		break;
	case UFML_LZIP:
		meta->ufml_compress = UFML_LZIP;
		return (meta->ufml_compress);
		break;
	case UFML_LZMA:
		meta->ufml_compress = UFML_LZMA;
		return (meta->ufml_compress);
		break;
	case UFML_XZ:
		meta->ufml_compress = UFML_XZ;
		return (meta->ufml_compress);
		break;
	default:
		meta->ufml_compress = UFML_BZIP2;
		return (meta->ufml_compress);
		break;
	}

	return (0);
}

/* Check encryption types to see if the encryption algorithm is supported */
int
ufml_check_encryption(meta, type)
	struct ufml_metadata *meta;
	int type;
{
	meta->ufml_encrypt = 0;

	switch (type) {
	case UFML_TWOFISH:
		meta->ufml_encrypt = UFML_TWOFISH;
		return (meta->ufml_encrypt);
	}

	return (0);
}
