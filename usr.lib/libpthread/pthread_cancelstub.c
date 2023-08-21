/*	$NetBSD: pthread_cancelstub.c,v 1.8 2003/11/24 23:23:17 cl Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: pthread_cancelstub.c,v 1.8 2003/11/24 23:23:17 cl Exp $");

/*
 * This is necessary because the fsync_range() name is always weak (it is
 * not a POSIX function).
 */
#define	fsync_range	_fsync_range

#include <sys/msg.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <poll.h>
#include <stdarg.h>
#include <unistd.h>

int	pthread__cancel_stub_binder;

/*
 * XXX this is necessary to get the prototypes for the __sigsuspend14
 * XXX and __msync13 internal names, instead of the application-visible
 * XXX sigsuspend and msync names. It's kind of gross, but we're pretty
 * XXX intimate with libc already.
 */
#define __LIBC12_SOURCE__
#include <signal.h>
#include <sys/mman.h>
#include <sys/socket.h>

#include "pthread.h"
#include "pthread_int.h"

int	_sys_accept(int, struct sockaddr *, socklen_t *);
int	_sys_close(int);
int	_sys_connect(int, const struct sockaddr *, socklen_t);
int	_sys_fcntl(int, int, ...);
int	_sys_fsync(int);
int	_sys_fsync_range(int, int, off_t, off_t);
ssize_t	_sys_msgrcv(int, void *, size_t, long, int);
int	_sys_msgsnd(int, const void *, size_t, int);
int	_sys___msync13(void *, size_t, int);
int	_sys_open(const char *, int, ...);
int	_sys_poll(struct pollfd *, nfds_t, int);
ssize_t	_sys_pread(int, void *, size_t, off_t);
ssize_t	_sys_pwrite(int, const void *, size_t, off_t);
ssize_t	_sys_read(int, void *, size_t);
ssize_t	_sys_readv(int, const struct iovec *, int);
int	_sys_select(int, fd_set *, fd_set *, fd_set *, struct timeval *);
int	_sys_wait4(pid_t, int *, int, struct rusage *);
ssize_t	_sys_write(int, const void *, size_t);
ssize_t	_sys_writev(int, const struct iovec *, int);

#define TESTCANCEL(id) 	do {						\
	if (__predict_false((id)->pt_cancel))				\
		pthread_exit(PTHREAD_CANCELED);				\
	} while (/*CONSTCOND*/0)


int
accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_accept(s, addr, addrlen);
	TESTCANCEL(self);
	
	return retval;
}

int
close(int d)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_close(d);
	TESTCANCEL(self);
	
	return retval;
}

int
connect(int s, const struct sockaddr *addr, socklen_t namelen)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_connect(s, addr, namelen);
	TESTCANCEL(self);
	
	return retval;
}

int
fcntl(int fd, int cmd, ...)
{
	int retval;
	pthread_t self;
	va_list ap;

	self = pthread__self();
	TESTCANCEL(self);
	va_start(ap, cmd);
	retval = _sys_fcntl(fd, cmd, va_arg(ap, void *));
	va_end(ap);
	TESTCANCEL(self);

	return retval;
}

int
fsync(int d)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_fsync(d);
	TESTCANCEL(self);
	
	return retval;
}

int
fsync_range(int d, int f, off_t s, off_t e)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_fsync_range(d, f, s, e);
	TESTCANCEL(self);

	return retval;
}

ssize_t
msgrcv(int msgid, void *msgp, size_t msgsz, long msgtyp, int msgflg)
{
	ssize_t retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_msgrcv(msgid, msgp, msgsz, msgtyp, msgflg);
	TESTCANCEL(self);

	return retval;
}

int
msgsnd(int msgid, const void *msgp, size_t msgsz, int msgflg)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_msgsnd(msgid, msgp, msgsz, msgflg);
	TESTCANCEL(self);

	return retval;
}

int
__msync13(void *addr, size_t len, int flags)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys___msync13(addr, len, flags);
	TESTCANCEL(self);

	return retval;
}

int
open(const char *path, int flags, ...)
{
	int retval;
	pthread_t self;
	va_list ap;

	self = pthread__self();
	TESTCANCEL(self);
	va_start(ap, flags);
	retval = _sys_open(path, flags, va_arg(ap, mode_t));
	va_end(ap);
	TESTCANCEL(self);

	return retval;
}

int
poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_poll(fds, nfds, timeout);
	TESTCANCEL(self);

	return retval;
}

ssize_t
pread(int d, void *buf, size_t nbytes, off_t offset)
{
	ssize_t retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_pread(d, buf, nbytes, offset);
	TESTCANCEL(self);

	return retval;
}

ssize_t
pwrite(int d, const void *buf, size_t nbytes, off_t offset)
{
	ssize_t retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_pwrite(d, buf, nbytes, offset);
	TESTCANCEL(self);

	return retval;
}

ssize_t
read(int d, void *buf, size_t nbytes)
{
	ssize_t retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_read(d, buf, nbytes);
	TESTCANCEL(self);

	return retval;
}

ssize_t
readv(int d, const struct iovec *iov, int iovcnt)
{
	ssize_t retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_readv(d, iov, iovcnt);
	TESTCANCEL(self);

	return retval;
}

int
select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, 
    struct timeval *timeout)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_select(nfds, readfds, writefds, exceptfds, timeout);
	TESTCANCEL(self);

	return retval;
}

pid_t
wait4(pid_t wpid, int *status, int options, struct rusage *rusage)
{
	pid_t retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_wait4(wpid, status, options, rusage);
	TESTCANCEL(self);

	return retval;
}

ssize_t
write(int d, const void *buf, size_t nbytes)
{
	ssize_t retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_write(d, buf, nbytes);
	TESTCANCEL(self);

	return retval;
}

ssize_t
writev(int d, const struct iovec *iov, int iovcnt)
{
	ssize_t retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = _sys_writev(d, iov, iovcnt);
	TESTCANCEL(self);

	return retval;
}


__strong_alias(_close, close)
__strong_alias(_fcntl, fcntl)
__strong_alias(_fsync, fsync)
__weak_alias(fsync_range, _fsync_range)
__strong_alias(_msgrcv, msgrcv)
__strong_alias(_msgsnd, msgsnd)
__strong_alias(___msync13, __msync13)
__strong_alias(_open, open)
__strong_alias(_poll, poll)
__strong_alias(_pread, pread)
__strong_alias(_pwrite, pwrite)
__strong_alias(_read, read)
__strong_alias(_readv, readv)
__strong_alias(_select, select)
__strong_alias(_wait4, wait4)
__strong_alias(_write, write)
__strong_alias(_writev, writev)
