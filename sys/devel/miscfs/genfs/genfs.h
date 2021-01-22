/*	$NetBSD: genfs.h,v 1.15 2001/12/18 07:49:36 chs Exp $	*/

int	genfs_badop (void *);
int	genfs_nullop (void *);
int	genfs_enoioctl (void *);
int	genfs_enoextops (void *);
int	genfs_einval (void *);
int	genfs_eopnotsupp (void *);
int	genfs_eopnotsupp_rele (void *);
int	genfs_ebadf (void *);
int	genfs_nolock (void *);
int	genfs_noislocked (void *);
int	genfs_nounlock (void *);

int	genfs_poll (void *);
int	genfs_fcntl (void *);
int	genfs_fsync (void *);
int	genfs_seek (void *);
int	genfs_abortop (void *);
int	genfs_revoke (void *);
int	genfs_lease_check (void *);
int	genfs_lock (void *);
int	genfs_islocked (void *);
int	genfs_unlock (void *);
int	genfs_mmap (void *);
int	genfs_getpages (void *);
int	genfs_putpages	(void *);
int	genfs_null_putpages	(void *);
int	genfs_compat_getpages (void *);
