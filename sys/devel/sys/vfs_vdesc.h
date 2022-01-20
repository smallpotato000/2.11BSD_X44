/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed
 * to Berkeley by John Heidemann of the UCLA Ficus project.
 *
 * Source: * @(#)i405_init.c 2.10 92/04/27 UCLA Ficus project
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
 *	@(#)vnode.h	8.17 (Berkeley) 5/20/95
 */

#ifndef _SYS_VFS_VDESC_H_
#define _SYS_VFS_VDESC_H_

#include <sys/vnode.h>

/*
 * This structure describes the vnode operation taking place.
 */
struct vnodeop_desc {
	int							vdesc_offset;
	char    					*vdesc_name;
	int							vdesc_flags;		/* VDESC_* flags */
};

union vnodeopv_entry_desc {
	struct vnodeops				**opve_vops;			/* vnode operations */
	struct vnodeop_desc 		**opve_op;  			/* which operation this is */
};

struct vnodeopv_desc_list;
LIST_HEAD(vnodeopv_desc_list, vnodeopv_desc);
struct vnodeopv_desc {
	LIST_ENTRY(vnodeopv_desc)	opv_entry;
    const char                  *opv_fsname;
    int                         opv_voptype;
    union vnodeopv_entry_desc 	opv_desc_ops;   	/* null terminated list */
};

/* vnodeops voptype */
#define D_VNODEOPS  0   /* vops vnodeops */
#define D_SPECOPS   1   /* vops specops */
#define D_FIFOOPS   2   /* vops fifoops */

void vnodeopv_desc_init(struct vnodeopv_desc *, const char *, int, struct vnodeops *, struct vnodeop_desc *);
struct vnodeopv_desc 	*vnodeopv_desc_lookup(const char *, int);
struct vnodeops 		*vnodeopv_desc_get_vnodeops(const char *, int);
struct vnodeop_desc 	*vnodeopv_desc_get_vnodeop_desc(const char *, int);

#define VNODEOPV_DESC_NAME(name, voptype)         name##_##voptype##_opv_desc
#define VNODEOPV_DESC_STRUCT(name, voptype) \
		struct vnodeopv_desc VNODEOPV_DESC_NAME(name, voptype);

extern struct vnodeopv_desc_list vfs_opv_descs;

/*
extern struct vnodeop_desc vop_lookup_desc;
extern struct vnodeop_desc vop_create_desc;
extern struct vnodeop_desc vop_whiteout_desc;
extern struct vnodeop_desc vop_mknod_desc;
extern struct vnodeop_desc vop_open_desc;
extern struct vnodeop_desc vop_close_desc;
extern struct vnodeop_desc vop_access_desc;
extern struct vnodeop_desc vop_getattr_desc;
extern struct vnodeop_desc vop_setattr_desc;
extern struct vnodeop_desc vop_read_desc;
extern struct vnodeop_desc vop_write_desc;
extern struct vnodeop_desc vop_lease_desc;
extern struct vnodeop_desc vop_ioctl_desc;
extern struct vnodeop_desc vop_select_desc;
extern struct vnodeop_desc vop_poll_desc;
extern struct vnodeop_desc vop_kqfilter_desc;
extern struct vnodeop_desc vop_revoke_desc;
extern struct vnodeop_desc vop_mmap_desc;
extern struct vnodeop_desc vop_fsync_desc;
extern struct vnodeop_desc vop_seek_desc;
extern struct vnodeop_desc vop_remove_desc;
extern struct vnodeop_desc vop_link_desc;
extern struct vnodeop_desc vop_rename_desc;
extern struct vnodeop_desc vop_mkdir_desc;
extern struct vnodeop_desc vop_rmdir_desc;
extern struct vnodeop_desc vop_symlink_desc;
extern struct vnodeop_desc vop_readdir_desc;
extern struct vnodeop_desc vop_readlink_desc;
extern struct vnodeop_desc vop_aborttop_desc;
extern struct vnodeop_desc vop_inactive_desc;
extern struct vnodeop_desc vop_reclaim_desc;
extern struct vnodeop_desc vop_lock_desc;
extern struct vnodeop_desc vop_unlock_desc;
extern struct vnodeop_desc vop_bmap_desc;
extern struct vnodeop_desc vop_print_desc;
extern struct vnodeop_desc vop_islocked_desc;
extern struct vnodeop_desc vop_pathconf_desc;
extern struct vnodeop_desc vop_advlock_desc;
extern struct vnodeop_desc vop_blkatoff_desc;
extern struct vnodeop_desc vop_valloc_desc;
extern struct vnodeop_desc vop_reallocblks_desc;
extern struct vnodeop_desc vop_vfree_desc;
extern struct vnodeop_desc vop_truncate_desc;
extern struct vnodeop_desc vop_update_desc;
extern struct vnodeop_desc vop_strategy_desc;
extern struct vnodeop_desc vop_bwrite_desc;

#define VDESCNAME(name)	 (vop_##name_desc)
#define VNODEOP_DESC_INIT(name)	 						\
	struct vnodeop_desc VDESCNAME(name) = 				\
	{ __offsetof(struct vnodeops, vop_##name), #name }

*/

#endif /* _SYS_VFS_VDESC_H_ */
