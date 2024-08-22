/* $211BSD$ */

/*
 * System call switch table.
 *
 * DO NOT EDIT-- this file is automatically generated.
 * created from;	@(#)syscalls.master	8.6 (Berkeley) 3/30/95
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signal.h>
#include <sys/mount.h>
int	nosys();
int	rexit();
int	fork();
int	read();
int	write();
int	open();
int	close();
int	wait4();
int	link();
int	unlink();
int	execv();
int	chdir();
int	fchdir();
int	mknod();
int	chmod();
int	chown();
int	chflags();
int	fchflags();
int	lseek();
int	getpid();
int	mount();
int	unmount();
int	__sysctl();
int	getuid();
int	geteuid();
int	ptrace();
int	getppid();
int	statfs();
int	fstatfs();
int	getfsstat();
int	sigaction();
int	sigprocmask();
int	access();
int	sigpending();
int	sigaltstack();
int	sync();
int	kill();
int	stat();
int	getlogin();
int	lstat();
int	dup();
int	pipe();
int	setlogin();
int	profil();
int	setuid();
int	seteuid();
int	getgid();
int	getegid();
int	setgid();
int	setegid();
int	acct();
int	getucontext();
int	setucontext();
int	ioctl();
int	reboot();
int	sigwait();
int	symlink();
int	readlink();
int	execve();
int	umask();
int	chroot();
int	fstat();
int	undelete();
int	pselect();
int	vfork();
int	obreak();
int	sbrk();
int	sstk();
int	mmap();
int	msync();
int	munmap();
int	mprotect();
int	madvise();
int	minherit();
int	mincore();
int	mlock();
int	munlock();
int	getgroups();
int	setgroups();
int	getpgrp();
int	setpgrp();
int	setitimer();
int	mpx();
int	pathconf();
int	getitimer();
int	setsid();
int	setpgid();
int	getdtablesize();
int	dup2();
int	fcntl();
int	select();
int	mkfifo();
int	fsync();
int	setpriority();
int	socket();
int	connect();
int	accept();
int	getpriority();
int	send();
int	recv();
int	sigreturn();
int	bind();
int	setsockopt();
int	listen();
int	sigsuspend();
int	sigstack();
int	recvmsg();
int	sendmsg();
int	revoke();
int	gettimeofday();
int	getrusage();
int	getsockopt();
int	getdirentries();
int	readv();
int	writev();
int	settimeofday();
int	fchown();
int	fchmod();
int	recvfrom();
int	setreuid();
int	setregid();
int	rename();
int	truncate();
int	ftruncate();
int	flock();
int	sigtimedwait();
int	sendto();
int	shutdown();
int	socketpair();
int	mkdir();
int	rmdir();
int	utimes();
int	adjtime();
int	getpeername();
int	getrlimit();
int	setrlimit();
int	killpg();
int	quotactl();
int	getsockname();
int	uuidgen();
int	nostk();
int	lfs_bmapv();
int	lfs_markv();
int	lfs_segclean();
int	lfs_segwait();
int	sysarch();
int	kenv();
int	kevent();
int	kqueue();
int	swapctl();
int	nosys();
#ifdef KTRACE
int	ktrace();
#else 
#endif
#ifdef TRACE
int	vtrace();
#else
#endif
#define	s(type)	sizeof(type)

struct sysent sysent[] = {
	{ 0, 0,
	    nosys },				/* 0 = syscall */
	{ 0, 0,
	    rexit },				/* 1 = rexit */
	{ 0, 0,
	    fork },				/* 2 = fork */
	{ 0, 0,
	    read },				/* 3 = read */
	{ 0, 0,
	    write },				/* 4 = write */
	{ 0, 0,
	    open },				/* 5 = open */
	{ 0, 0,
	    close },				/* 6 = close */
	{ 0, 0,
	    wait4 },				/* 7 = wait4 */
	{ 0, 0,
	    nosys },				/* 8 = unused old creat */
	{ 0, 0,
	    link },				/* 9 = link */
	{ 0, 0,
	    unlink },				/* 10 = unlink */
	{ 0, 0,
	    execv },				/* 11 = execv */
	{ 0, 0,
	    chdir },				/* 12 = chdir */
	{ 0, 0,
	    fchdir },				/* 13 = fchdir */
	{ 0, 0,
	    mknod },				/* 14 = mknod */
	{ 0, 0,
	    chmod },				/* 15 = chmod */
	{ 0, 0,
	    chown },				/* 16 = chown */
	{ 0, 0,
	    chflags },				/* 17 = chflags */
	{ 0, 0,
	    fchflags },				/* 18 = fchflags */
	{ 0, 0,
	    lseek },				/* 19 = lseek */
	{ 0, 0,
	    getpid },				/* 20 = getpid */
	{ 0, 0,
	    mount },				/* 21 = mount */
	{ 0, 0,
	    unmount },				/* 22 = unmount */
	{ 0, 0,
	    __sysctl },				/* 23 = __sysctl */
	{ 0, 0,
	    getuid },				/* 24 = getuid */
	{ 0, 0,
	    geteuid },				/* 25 = geteuid */
	{ 0, 0,
	    ptrace },				/* 26 = ptrace */
	{ 0, 0,
	    getppid },				/* 27 = getppid */
	{ 0, 0,
	    statfs },				/* 28 = statfs */
	{ 0, 0,
	    fstatfs },				/* 29 = fstatfs */
	{ 0, 0,
	    getfsstat },			/* 30 = getfsstat */
	{ 0, 0,
	    sigaction },			/* 31 = sigaction */
	{ 0, 0,
	    sigprocmask },			/* 32 = sigprocmask */
	{ 0, 0,
	    access },				/* 33 = access */
	{ 0, 0,
	    sigpending },			/* 34 = sigpending */
	{ 0, 0,
	    sigaltstack },			/* 35 = sigaltstack */
	{ 0, 0,
	    sync },				/* 36 = sync */
	{ 0, 0,
	    kill },				/* 37 = kill */
	{ 0, 0,
	    stat },				/* 38 = stat */
	{ 0, 0,
	    getlogin },				/* 39 = getlogin */
	{ 0, 0,
	    lstat },				/* 40 = lstat */
	{ 0, 0,
	    dup },				/* 41 = dup */
	{ 0, 0,
	    pipe },				/* 42 = pipe */
	{ 0, 0,
	    setlogin },				/* 43 = setlogin */
	{ 0, 0,
	    profil },				/* 44 = profil */
	{ 0, 0,
	    setuid },				/* 45 = setuid */
	{ 0, 0,
	    seteuid },				/* 46 = seteuid */
	{ 0, 0,
	    getgid },				/* 47 = getgid */
	{ 0, 0,
	    getegid },				/* 48 = getegid */
	{ 0, 0,
	    setgid },				/* 49 = setgid */
	{ 0, 0,
	    setegid },				/* 50 = setegid */
	{ 0, 0,
	    acct },				/* 51 = acct */
	{ 0, 0,
	    getucontext },			/* 52 = getucontext */
	{ 0, 0,
	    setucontext },			/* 53 = setucontext */
	{ 0, 0,
	    ioctl },				/* 54 = ioctl */
	{ 0, 0,
	    reboot },				/* 55 = reboot */
	{ 0, 0,
	    sigwait },				/* 56 = sigwait */
	{ 0, 0,
	    symlink },				/* 57 = symlink */
	{ 0, 0,
	    readlink },				/* 58 = readlink */
	{ 0, 0,
	    execve },				/* 59 = execve */
	{ 0, 0,
	    umask },				/* 60 = umask */
	{ 0, 0,
	    chroot },				/* 61 = chroot */
	{ 0, 0,
	    fstat },				/* 62 = fstat */
	{ 0, 0,
	    undelete },				/* 63 = undelete */
	{ 0, 0,
	    nosys },				/* 64 = unused old getpagesize */
	{ 0, 0,
	    pselect },				/* 65 = pselect */
	{ 0, 0,
	    vfork },				/* 66 = vfork */
	{ 0, 0,
	    obreak },				/* 67 = break */
	{ 0, 0,
	    sbrk },				/* 68 = sbrk */
	{ 0, 0,
	    sstk },				/* 69 = sstk */
	{ 0, 0,
	    mmap },				/* 70 = mmap */
	{ 0, 0,
	    msync },				/* 71 = msync */
	{ 0, 0,
	    munmap },				/* 72 = munmap */
	{ 0, 0,
	    mprotect },				/* 73 = mprotect */
	{ 0, 0,
	    madvise },				/* 74 = madvise */
	{ 0, 0,
	    minherit },				/* 75 = minherit */
	{ 0, 0,
	    mincore },				/* 76 = mincore */
	{ 0, 0,
	    mlock },				/* 77 = mlock */
	{ 0, 0,
	    munlock },				/* 78 = munlock */
	{ 0, 0,
	    getgroups },			/* 79 = getgroups */
	{ 0, 0,
	    setgroups },			/* 80 = setgroups */
	{ 0, 0,
	    getpgrp },				/* 81 = getpgrp */
	{ 0, 0,
	    setpgrp },				/* 82 = setpgrp */
	{ 0, 0,
	    setitimer },			/* 83 = setitimer */
	{ 0, 0,
	    mpx },				/* 84 = mpx */
	{ 0, 0,
	    pathconf },				/* 85 = pathconf */
	{ 0, 0,
	    getitimer },			/* 86 = getitimer */
	{ 0, 0,
	    setsid },				/* 87 = setsid */
	{ 0, 0,
	    setpgid },				/* 88 = setpgid */
	{ 0, 0,
	    getdtablesize },			/* 89 = getdtablesize */
	{ 0, 0,
	    dup2 },				/* 90 = dup2 */
	{ 0, 0,
	    nosys },				/* 91 = unused */
	{ 0, 0,
	    fcntl },				/* 92 = fcntl */
	{ 0, 0,
	    select },				/* 93 = select */
	{ 0, 0,
	    mkfifo },				/* 94 = mkfifo */
	{ 0, 0,
	    fsync },				/* 95 = fsync */
	{ 0, 0,
	    setpriority },			/* 96 = setpriority */
	{ 0, 0,
	    socket },				/* 97 = socket */
	{ 0, 0,
	    connect },				/* 98 = connect */
	{ 0, 0,
	    accept },				/* 99 = accept */
	{ 0, 0,
	    getpriority },			/* 100 = getpriority */
	{ 0, 0,
	    send },				/* 101 = send */
	{ 0, 0,
	    recv },				/* 102 = recv */
	{ 0, 0,
	    sigreturn },			/* 103 = sigreturn */
	{ 0, 0,
	    bind },				/* 104 = bind */
	{ 0, 0,
	    setsockopt },			/* 105 = setsockopt */
	{ 0, 0,
	    listen },				/* 106 = listen */
	{ 0, 0,
	    sigsuspend },			/* 107 = sigsuspend */
	{ 0, 0,
	    nosys },				/* 108 = obsolete old sigvec */
	{ 0, 0,
	    nosys },				/* 109 = obsolete old sigblock */
	{ 0, 0,
	    nosys },				/* 110 = obsolete old sigsetmask */
	{ 0, 0,
	    nosys },				/* 111 = obsolete old sigpause */
	{ 0, 0,
	    sigstack },				/* 112 = sigstack */
	{ 0, 0,
	    recvmsg },				/* 113 = recvmsg */
	{ 0, 0,
	    sendmsg },				/* 114 = sendmsg */
	{ 0, 0,
	    revoke },				/* 115 = revoke */
	{ 0, 0,
	    gettimeofday },			/* 116 = gettimeofday */
	{ 0, 0,
	    getrusage },			/* 117 = getrusage */
	{ 0, 0,
	    getsockopt },			/* 118 = getsockopt */
	{ 0, 0,
	    getdirentries },			/* 119 = getdirentries */
	{ 0, 0,
	    readv },				/* 120 = readv */
	{ 0, 0,
	    writev },				/* 121 = writev */
	{ 0, 0,
	    settimeofday },			/* 122 = settimeofday */
	{ 0, 0,
	    fchown },				/* 123 = fchown */
	{ 0, 0,
	    fchmod },				/* 124 = fchmod */
	{ 0, 0,
	    recvfrom },				/* 125 = recvfrom */
	{ 0, 0,
	    setreuid },				/* 126 = setreuid */
	{ 0, 0,
	    setregid },				/* 127 = setregid */
	{ 0, 0,
	    rename },				/* 128 = rename */
	{ 0, 0,
	    truncate },				/* 129 = truncate */
	{ 0, 0,
	    ftruncate },			/* 130 = ftruncate */
	{ 0, 0,
	    flock },				/* 131 = flock */
	{ 0, 0,
	    sigtimedwait },			/* 132 = sigtimedwait */
	{ 0, 0,
	    sendto },				/* 133 = sendto */
	{ 0, 0,
	    shutdown },				/* 134 = shutdown */
	{ 0, 0,
	    socketpair },			/* 135 = socketpair */
	{ 0, 0,
	    mkdir },				/* 136 = mkdir */
	{ 0, 0,
	    rmdir },				/* 137 = rmdir */
	{ 0, 0,
	    utimes },				/* 138 = utimes */
	{ 0, 0,
	    nosys },				/* 139 = unused */
	{ 0, 0,
	    adjtime },				/* 140 = adjtime */
	{ 0, 0,
	    getpeername },			/* 141 = getpeername */
	{ 0, 0,
	    nosys },				/* 142 = obsolete old gethostid */
	{ 0, 0,
	    nosys },				/* 143 = obsolete old sethostid */
	{ 0, 0,
	    getrlimit },			/* 144 = getrlimit */
	{ 0, 0,
	    setrlimit },			/* 145 = setrlimit */
	{ 0, 0,
	    killpg },				/* 146 = killpg */
	{ 0, 0,
	    quotactl },				/* 147 = quotactl */
	{ 0, 0,
	    nosys },				/* 148 = obsolete old setquota */
	{ 0, 0,
	    nosys },				/* 149 = obsolete old quota */
	{ 0, 0,
	    getsockname },			/* 150 = getsockname */
	{ 0, 0,
	    uuidgen },				/* 151 = uuidgen */
	{ 0, 0,
	    nostk },				/* 152 = nostk */
	{ 0, 0,
	    nosys },				/* 153 = obsolete old fetchi */
	{ 0, 0,
	    nosys },				/* 154 = obsolete old ucall */
	{ 0, 0,
	    nosys },				/* 155 = obsolete old fperr */
	{ 0, 0,
	    lfs_bmapv },			/* 156 = lfs_bmapv */
	{ 0, 0,
	    lfs_markv },			/* 157 = lfs_markv */
	{ 0, 0,
	    lfs_segclean },			/* 158 = lfs_segclean */
	{ 0, 0,
	    lfs_segwait },			/* 159 = lfs_segwait */
	{ 0, 0,
	    sysarch },				/* 160 = sysarch */
	{ 0, 0,
	    kenv },				/* 161 = kenv */
	{ 0, 0,
	    kevent },				/* 162 = kevent */
	{ 0, 0,
	    kqueue },				/* 163 = kqueue */
	{ 0, 0,
	    swapctl },				/* 164 = swapctl */
	{ 0, 0,
	    nosys },				/* 165 = _syscall */
#ifdef KTRACE
	{ 0, 0,
	    ktrace },				/* 166 = ktrace */
#else 
	{ 0, 0,
	    nosys },				/* 166 = unused ktrace */
#endif
#ifdef TRACE
	{ 0, 0,
	    vtrace },				/* 167 = vtrace */
#else
	{ 0, 0,
	    nosys },				/* 167 = unused vtrace */
#endif
};

int	nsysent= sizeof(sysent) / sizeof(sysent[0]);
