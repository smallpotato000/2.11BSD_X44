/* $NetBSD: mount_msdos.c,v 1.34 2004/01/05 23:27:16 jmmv Exp $ */

/*
 * Copyright (c) 1994 Christopher G. Demetriou
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.NetBSD.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 * 
 * <<Id: LICENSE,v 1.2 2000/06/14 15:57:33 cgd Exp>>
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: mount_msdos.c,v 1.34 2004/01/05 23:27:16 jmmv Exp $");
#endif /* not lint */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <msdosfs/msdosfsmount.h>
#include <ctype.h>
#include <err.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <util.h>

#include <mntopts.h>
#include <fattr.h>

static const struct mntopt mopts[] = {
	MOPT_STDOPTS,
	MOPT_ASYNC,
	MOPT_SYNC,
	MOPT_UPDATE,
	{ NULL }
};

int	main(int, char *[]);
int	mount_msdos(int argc, char **argv);
static void	usage(void)) __attribute__((__noreturn__);

#ifndef MOUNT_NOMAIN
int
main(argc, argv)
	int argc;
	char **argv;
{
	return mount_msdos(argc, argv);
}
#endif

int
mount_msdos(argc, argv)
	int argc;
	char **argv;
{
	struct msdosfs_args args;
	struct stat sb;
	int c, mntflags, set_gid, set_uid, set_mask, set_dirmask, set_gmtoff;
	char *dev, *dir, ndir[MAXPATHLEN+1];
	time_t now;
	struct tm *tm;

	mntflags = set_gid = set_uid = set_mask = set_dirmask = set_gmtoff = 0;
	(void)memset(&args, '\0', sizeof(args));

	while ((c = getopt(argc, argv, "Gsl9u:g:m:M:o:t:")) != -1) {
		switch (c) {
		case 'G':
			args.flags |= MSDOSFSMNT_GEMDOSFS;
			break;
		case 's':
			args.flags |= MSDOSFSMNT_SHORTNAME;
			break;
		case 'l':
			args.flags |= MSDOSFSMNT_LONGNAME;
			break;
		case '9':
			args.flags |= MSDOSFSMNT_NOWIN95;
			break;
		case 'u':
			args.uid = a_uid(optarg);
			set_uid = 1;
			break;
		case 'g':
			args.gid = a_gid(optarg);
			set_gid = 1;
			break;
		case 'm':
			args.mask = a_mask(optarg);
			set_mask = 1;
			break;
		case 'M':
			args.dirmask = a_mask(optarg);
			set_dirmask = 1;
			break;
		case 'o':
			getmntopts(optarg, mopts, &mntflags, 0);
			break;
		case 't':
			args.gmtoff = atoi(optarg);
			set_gmtoff = 1;
			break;
		case '?':
		default:
			usage();
			break;
		}
	}

	if (optind + 2 != argc)
		usage();

	if (set_mask && !set_dirmask) {
		args.dirmask = args.mask;
		set_dirmask = 1;
	} else if (set_dirmask && !set_mask) {
		args.mask = args.dirmask;
		set_mask = 1;
	}

	dev = argv[optind];
	dir = argv[optind + 1];
	if (dir[0] != '/') {
		warnx("\"%s\" is a relative path.", dir);
		if (getcwd(ndir, sizeof(ndir)) == NULL)
			err(1, "getcwd");
		strncat(ndir, "/", sizeof(ndir) - strlen(ndir) - 1);
		strncat(ndir, dir, sizeof(ndir) - strlen(ndir) - 1);
		dir = ndir;
		warnx("using \"%s\" instead.", dir);
	}

	args.fspec = dev;
	args.export.ex_root = -2;	/* unchecked anyway on DOS fs */
	if (mntflags & MNT_RDONLY)
		args.export.ex_flags = MNT_EXRDONLY;
	else
		args.export.ex_flags = 0;
	if (!set_gid || !set_uid || !set_mask) {
		if (stat(dir, &sb) == -1)
			err(1, "stat %s", dir);

		if (!set_uid)
			args.uid = sb.st_uid;
		if (!set_gid)
			args.gid = sb.st_gid;
		if (!set_mask) {
			args.mask = args.dirmask =
				sb.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);
		}
	}

	if (!set_gmtoff) {
		/* use user's time zone as default */
		time(&now);
		tm = localtime(&now);
		args.gmtoff = tm->tm_gmtoff;

	}
	args.flags |= MSDOSFSMNT_VERSIONED;
	args.version = MSDOSFSMNT_VERSION;

	if (mount(MOUNT_MSDOS, dir, mntflags, &args) < 0)
		err(1, "%s on %s", dev, dir);

	exit (0);
}

static void
usage()
{

	fprintf(stderr, "usage: mount_msdos [-9Gls] [-g gid] [-M mask] [-m mask] [-o options]\n"
			"\t[-t gmtoff] [-u uid] special node\n");
	exit(1);
}
