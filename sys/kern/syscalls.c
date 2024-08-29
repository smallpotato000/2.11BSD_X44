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
	"#8",		/* 8 = unused old creat */
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
	"getucontext",			/* 52 = getucontext */
	"setucontext",			/* 53 = setucontext */
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
	"#64",		/* 64 = unused old getpagesize */
	"pselect",			/* 65 = pselect */
	"vfork",			/* 66 = vfork */
	"break",			/* 67 = break */
	"sbrk",			/* 68 = sbrk */
	"sstk",			/* 69 = sstk */
	"mmap",			/* 70 = mmap */
	"msync",			/* 71 = msync */
	"munmap",			/* 72 = munmap */
	"mprotect",			/* 73 = mprotect */
	"madvise",			/* 74 = madvise */
	"minherit",			/* 75 = minherit */
	"mincore",			/* 76 = mincore */
	"mlock",			/* 77 = mlock */
	"munlock",			/* 78 = munlock */
	"getgroups",			/* 79 = getgroups */
	"setgroups",			/* 80 = setgroups */
	"getpgrp",			/* 81 = getpgrp */
	"setpgrp",			/* 82 = setpgrp */
	"setitimer",			/* 83 = setitimer */
	"mpx",			/* 84 = mpx */
	"pathconf",			/* 85 = pathconf */
	"getitimer",			/* 86 = getitimer */
	"setsid",			/* 87 = setsid */
	"setpgid",			/* 88 = setpgid */
	"getdtablesize",			/* 89 = getdtablesize */
	"dup2",			/* 90 = dup2 */
	"#91",		/* 91 = unused */
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
	"nanosleep",			/* 108 = nanosleep */
	"clock_gettime",			/* 109 = clock_gettime */
	"clock_settime",			/* 110 = clock_settime */
	"poll",			/* 111 = poll */
	"sigstack",			/* 112 = sigstack */
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
	"sigtimedwait",			/* 132 = sigtimedwait */
	"sendto",			/* 133 = sendto */
	"shutdown",			/* 134 = shutdown */
	"socketpair",			/* 135 = socketpair */
	"mkdir",			/* 136 = mkdir */
	"rmdir",			/* 137 = rmdir */
	"utimes",			/* 138 = utimes */
	"#139",		/* 139 = unused */
	"adjtime",			/* 140 = adjtime */
	"getpeername",			/* 141 = getpeername */
	"#142",		/* 142 = obsolete old gethostid */
	"#143",		/* 143 = obsolete old sethostid */
	"getrlimit",			/* 144 = getrlimit */
	"setrlimit",			/* 145 = setrlimit */
	"killpg",			/* 146 = killpg */
	"quotactl",			/* 147 = quotactl */
	"#148",		/* 148 = obsolete old setquota */
	"#149",		/* 149 = obsolete old quota */
	"getsockname",			/* 150 = getsockname */
	"uuidgen",			/* 151 = uuidgen */
	"nostk",			/* 152 = nostk */
	"#153",		/* 153 = obsolete old fetchi */
	"#154",		/* 154 = obsolete old ucall */
	"#155",		/* 155 = obsolete old fperr */
	"lfs_bmapv",			/* 156 = lfs_bmapv */
	"lfs_markv",			/* 157 = lfs_markv */
	"lfs_segclean",			/* 158 = lfs_segclean */
	"lfs_segwait",			/* 159 = lfs_segwait */
	"sysarch",			/* 160 = sysarch */
	"kenv",			/* 161 = kenv */
	"kevent",			/* 162 = kevent */
	"kqueue",			/* 163 = kqueue */
	"swapctl",			/* 164 = swapctl */
	"_syscall",			/* 165 = _syscall */
#ifdef KTRACE
	"ktrace",			/* 166 = ktrace */
#else 
	"#166",		/* 166 = unused ktrace */
#endif
#ifdef TRACE
	"vtrace",			/* 167 = vtrace */
#else
	"#167",		/* 167 = unused vtrace */
#endif
};
