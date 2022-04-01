/*	$211BSD$	*/

/*
 * System call names.
 *
 * DO NOT EDIT-- this file is automatically generated.
 * created from;	@(#)syscalls.master	8.6 (Berkeley) 3/30/95
 */

char *syscallnames[] = {
	"syscall",			/* 0 = syscall */
	"rexit",			/* 1 = rexit */
	"fork",			/* 2 = fork */
	"read",			/* 3 = read */
	"write",			/* 4 = write */
	"open",			/* 5 = open */
	"close",			/* 6 = close */
	"wait4",			/* 7 = wait4 */
	"compat_43_creat",	/* 8 = compat_43 creat */
	"link",			/* 9 = link */
	"unlink",			/* 10 = unlink */
	"execv",			/* 11 = execv */
	"chdir",			/* 12 = chdir */
	"fchdir",			/* 13 = fchdir */
	"mknod",			/* 14 = mknod */
	"chmod",			/* 15 = chmod */
	"chown",			/* 16 = chown */
	"chflags",			/* 17 = chflags */
	"fchflags",			/* 18 = fchflags */
	"lseek",			/* 19 = lseek */
	"getpid",			/* 20 = getpid */
	"mount",			/* 21 = mount */
	"unmount",			/* 22 = unmount */
	"__sysctl",			/* 23 = __sysctl */
	"getuid",			/* 24 = getuid */
	"geteuid",			/* 25 = geteuid */
	"ptrace",			/* 26 = ptrace */
	"getppid",			/* 27 = getppid */
	"statfs",			/* 28 = statfs */
	"fstatfs",			/* 29 = fstatfs */
	"getfsstat",			/* 30 = getfsstat */
	"sigaction",			/* 31 = sigaction */
	"sigprocmask",			/* 32 = sigprocmask */
	"access",			/* 33 = access */
	"sigpending",			/* 34 = sigpending */
	"sigaltstack",			/* 35 = sigaltstack */
	"sync",			/* 36 = sync */
	"kill",			/* 37 = kill */
	"stat",			/* 38 = stat */
	"getlogin",			/* 39 = getlogin */
	"lstat",			/* 40 = lstat */
	"dup",			/* 41 = dup */
	"pipe",			/* 42 = pipe */
	"setlogin",			/* 43 = setlogin */
	"profil",			/* 44 = profil */
	"setuid",			/* 45 = setuid */
	"seteuid",			/* 46 = seteuid */
	"getgid",			/* 47 = getgid */
	"getegid",			/* 48 = getegid */
	"setgid",			/* 49 = setgid */
	"setegid",			/* 50 = setegid */
	"acct",			/* 51 = acct */
	"phys",			/* 52 = phys */
	"lock",			/* 53 = lock */
	"ioctl",			/* 54 = ioctl */
	"reboot",			/* 55 = reboot */
	"sigwait",			/* 56 = sigwait */
	"symlink",			/* 57 = symlink */
	"readlink",			/* 58 = readlink */
	"execve",			/* 59 = execve */
	"umask",			/* 60 = umask */
	"chroot",			/* 61 = chroot */
	"fstat",			/* 62 = fstat */
	"undelete",			/* 63 = undelete */
	"compat_43_getpagesize",	/* 64 = compat_43 getpagesize */
	"pselect",			/* 65 = pselect */
	"vfork",			/* 66 = vfork */
	"compat_43_lseek",	/* 67 = compat_43 lseek */
	"compat_43_mmap",	/* 68 = compat_43 mmap */
	"sbrk",			/* 69 = sbrk */
	"sstk",			/* 70 = sstk */
	"mmap",			/* 71 = mmap */
	"msync",			/* 72 = msync */
	"munmap",			/* 73 = munmap */
	"mprotect",			/* 74 = mprotect */
	"madvise",			/* 75 = madvise */
	"mincore",			/* 76 = mincore */
	"mlock",			/* 77 = mlock */
	"munlock",			/* 78 = munlock */
	"getgroups",			/* 79 = getgroups */
	"setgroups",			/* 80 = setgroups */
	"getpgrp",			/* 81 = getpgrp */
	"setpgrp",			/* 82 = setpgrp */
	"setitimer",			/* 83 = setitimer */
	"compat_43_lstat",	/* 84 = compat_43 lstat */
	"pathconf",			/* 85 = pathconf */
	"getitimer",			/* 86 = getitimer */
	"setsid",			/* 87 = setsid */
	"setpgid",			/* 88 = setpgid */
	"getdtablesize",			/* 89 = getdtablesize */
	"dup2",			/* 90 = dup2 */
	"compat_43_stat",	/* 91 = compat_43 stat */
	"fcntl",			/* 92 = fcntl */
	"select",			/* 93 = select */
	"mkfifo",			/* 94 = mkfifo */
	"fsync",			/* 95 = fsync */
	"setpriority",			/* 96 = setpriority */
	"socket",			/* 97 = socket */
	"connect",			/* 98 = connect */
	"accept",			/* 99 = accept */
	"getpriority",			/* 100 = getpriority */
	"send",			/* 101 = send */
	"recv",			/* 102 = recv */
	"sigreturn",			/* 103 = sigreturn */
	"bind",			/* 104 = bind */
	"setsockopt",			/* 105 = setsockopt */
	"listen",			/* 106 = listen */
	"sigsuspend",			/* 107 = sigsuspend */
	"#108 (obsolete old sigvec)",		/* 108 = obsolete old sigvec */
	"#109 (obsolete old sigblock)",		/* 109 = obsolete old sigblock */
	"#110 (obsolete old sigsetmask)",		/* 110 = obsolete old sigsetmask */
	"#111 (obsolete old sigpause)",		/* 111 = obsolete old sigpause */
	"compat_43_sigstack",	/* 112 = compat_43 sigstack */
	"recvmsg",			/* 113 = recvmsg */
	"sendmsg",			/* 114 = sendmsg */
	"revoke",			/* 115 = revoke */
	"gettimeofday",			/* 116 = gettimeofday */
	"getrusage",			/* 117 = getrusage */
	"getsockopt",			/* 118 = getsockopt */
	"getdirentries",			/* 119 = getdirentries */
	"readv",			/* 120 = readv */
	"writev",			/* 121 = writev */
	"settimeofday",			/* 122 = settimeofday */
	"fchown",			/* 123 = fchown */
	"fchmod",			/* 124 = fchmod */
	"recvfrom",			/* 125 = recvfrom */
	"setreuid",			/* 126 = setreuid */
	"setregid",			/* 127 = setregid */
	"rename",			/* 128 = rename */
	"truncate",			/* 129 = truncate */
	"ftruncate",			/* 130 = ftruncate */
	"flock",			/* 131 = flock */
	"compat_43_truncate",	/* 132 = compat_43 truncate */
	"sendto",			/* 133 = sendto */
	"shutdown",			/* 134 = shutdown */
	"socketpair",			/* 135 = socketpair */
	"mkdir",			/* 136 = mkdir */
	"rmdir",			/* 137 = rmdir */
	"utimes",			/* 138 = utimes */
	"compat_43_ftruncate",	/* 139 = compat_43 ftruncate */
	"adjtime",			/* 140 = adjtime */
	"getpeername",			/* 141 = getpeername */
	"#142 (obsolete old gethostid)",		/* 142 = obsolete old gethostid */
	"#143 (obsolete old sethostid)",		/* 143 = obsolete old sethostid */
	"getrlimit",			/* 144 = getrlimit */
	"setrlimit",			/* 145 = setrlimit */
	"killpg",			/* 146 = killpg */
	"quotactl",			/* 147 = quotactl */
	"#148 (obsolete old setquota)",		/* 148 = obsolete old setquota */
	"quota",			/* 149 = quota */
	"getsockname",			/* 150 = getsockname */
	"compat_43_getdirentries",	/* 151 = compat_43 getdirentries */
	"#152 (obsolete old nostk)",		/* 152 = obsolete old nostk */
	"#153 (obsolete old fetchi)",		/* 153 = obsolete old fetchi */
	"#154 (obsolete old ucall)",		/* 154 = obsolete old ucall */
	"#155 (obsolete old fperr)",		/* 155 = obsolete old fperr */
	"lfs_bmapv",			/* 156 = lfs_bmapv */
	"lfs_markv",			/* 157 = lfs_markv */
	"lfs_segclean",			/* 158 = lfs_segclean */
	"lfs_segwait",			/* 159 = lfs_segwait */
	"sysarch",			/* 160 = sysarch */
	"kenv",			/* 161 = kenv */
	"kevent",			/* 162 = kevent */
	"kqueue",			/* 163 = kqueue */
	"swapon",			/* 164 = swapon */
	"#165 (unimplemented { int swapctl ( ) ; })",		/* 165 = unimplemented { int swapctl ( ) ; } */
#ifdef KTRACE
	"ktrace",			/* 166 = ktrace */
#else 
	"#166 (unimplemented ktrace)",		/* 166 = unimplemented ktrace */
#endif
#ifdef TRACE
	"vtrace",			/* 167 = vtrace */
#else
	"#167 (unimplemented vtrace)",		/* 167 = unimplemented vtrace */
#endif
};
