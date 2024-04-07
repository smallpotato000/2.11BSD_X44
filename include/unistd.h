/*-
 * Copyright (c) 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)unistd.h	8.10.4 (2.11BSD) 1999/5/25
 */

/*
 * Modified for 2.11BSD by removing prototypes.  To save time and space
 * functions not returning 'int' and functions not present in the system
 * are not listed.
*/

#ifndef _UNISTD_H_
#define	_UNISTD_H_

#include <machine/ansi.h>
#include <machine/types.h>

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/unistd.h>

#if _FORTIFY_SOURCE > 0
#include <ssp/unistd.h>
#endif

#define STDIN_FILENO	0	/* standard input file descriptor */
#define	STDOUT_FILENO	1	/* standard output file descriptor */
#define	STDERR_FILENO	2	/* standard error file descriptor */

#include <sys/null.h>

__BEGIN_DECLS

void	 		_exit(int);
int	 			access(const char *, int);
unsigned int	alarm(int);
int	 			chdir(const char *);
int	 			chown(const char *, uid_t, gid_t);
int	 			close(int);
size_t			confstr(int, char *, size_t);
int	 			dup(int);
int	 			dup2(int, int);
int	            execl(const char *, const char *, ...) __null_sentinel;
int	            execle(const char *, const char *, ...);
int	            execlp(const char *, const char *, ...) __null_sentinel;
int	            execv(const char *, char * const *);
int	            execve(const char *, char * const *, char * const *);
int	            execvp(const char *, char * const *);
pid_t	 		fork(void);
long	 		fpathconf(int, int);
char			*getcwd(char *, size_t);
gid_t	 		getegid(void);
uid_t			geteuid(void);
gid_t	 		getegid(void);
uid_t	 		geteuid(void);
gid_t	 		getgid(void);
int	 		    getgroups(int, gid_t[]);
char			*getlogin(void);
pid_t	 		getpgrp(void);
pid_t	 		getpid(void);
pid_t	 		getppid(void);
uid_t	 		getuid(void);
int	 			isatty(int);
int	 			link(const char *, const char *);
off_t	 		lseek(int, off_t, int);
long	 		pathconf(const char *, int);
int	 			pause(void);
int	 			pipe(int *);
ssize_t	 		read(int, void *, size_t);
int	 			rmdir(const char *);
int	 			setgid(gid_t);
int	 			setpgid(pid_t, pid_t);
pid_t		 	setsid(void);
int	 			setuid(uid_t);
unsigned int	sleep(unsigned int);
long	 		sysconf(int);
char			*ttyname(int);
int	 			unlink(const char *);
ssize_t			write(int, const void *, size_t);

extern	char *sys_siglist[];
extern	char	*optarg;		/* getopt(3) external variables */
extern	int	    opterr, optind, optopt;
int	 			getopt(int, char * const [], const char *);

#ifndef	_POSIX_SOURCE
#ifdef	__STDC__
struct timeval;				/* select(2) */
#endif

int	 			acct(const char *, pid_t);
char			*brk(const char *);
int	 			chroot(const char *);
char			*crypt(char *, char *);
int	 			des_cipher(const char *, char *, long, int);
int	 			des_setkey(const char *key);
int	 			encrypt(char *, int);
void	 		endusershell(void);
int	 			exect(const char *, char * const *, char * const *);
int	 			fchdir(int);
int	 			fchown(int, int, int);
int	 			fsync(int);
int	 			ftruncate(int, off_t);
int	 			getdtablesize(void);
unsigned long	gethostid(void);
int	 			gethostname(char *, size_t);
mode_t	 		getmode(const void *, mode_t);
__pure int	 	getpagesize(void);
char			*getpass(char *);
char			*getusershell(void);
char			*getwd(char *);
int	 			initgroups(const char *, int);
int	 			iruserok(unsigned long, int, const char *, const char *);
int	 			mknod(const char *, mode_t, dev_t);
char			*mktemp(char *);
int	 			nice(int);
int	 			profil(char *, int, int, int);
int		 		rcmd(char **, int, const char *, const char *, const char *, int *);
char			*re_comp(char *);
int 			re_exec(char *);
char			*sbrk(int);
unsigned long	sethostid(unsigned long);
int	 			setkey(const char *);
int	 			setlogin(const char *);
void			*setmode(const char *);
int	 			setpgrp(pid_t pid, pid_t pgrp);	/* obsoleted by setpgid() */
int	 			setregid(gid_t, gid_t);
int	 			setreuid(uid_t, uid_t);
int	 			setrgid(gid_t);
int	 			setruid(uid_t);
void	 		setusershell(void);
int	 			symlink(const char *, const char *);
void	 		sync(void);
int	 			swapctl(int, void *, int);
int	 			syscall(int, ...);
quad_t	 		__syscall(quad_t, ...);
int	 			truncate(const char *, off_t);
int	 			ttyslot(void);
unsigned int	ualarm(unsigned int, unsigned int);
void	 		usleep(long);
pid_t	 		vfork(void);
#endif /* !_POSIX_SOURCE */

struct mpx;
/* mpx (multiplexor is a non-standard system call unique to 2.11BSD_X44); */
/* int 			mpx(int, struct mpx *, int, void *); */
int				mpx_create(struct mpx *, int, void *);
int				mpx_put(struct mpx *, int, void *);
int				mpx_get(struct mpx *, int, void *);
int				mpx_destroy(struct mpx *, int, void *);
int				mpx_remove(struct mpx *, int, void *);
__END_DECLS

#endif /* !_UNISTD_H_ */
