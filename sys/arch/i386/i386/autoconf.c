/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	@(#)autoconf.c	8.1 (Berkeley) 6/11/93
 */

/*
 * Setup the system to run on the current machine.
 *
 * Configure() is called at boot time and initializes the vba
 * device tables and the memory controller monitoring.  Available
 * devices are determined (from possibilities mentioned in ioconf.c),
 * and the drivers are initialized.
 */
#include <sys/param.h>

#include <sys/systm.h>
#include <sys/map.h>
#include <sys/buf.h>
#include <sys/dk.h>
#include <sys/conf.h>
#include <sys/dmap.h>
#include <sys/reboot.h>
#include <sys/null.h>
#include <sys/devsw.h>

#include <machine/pte.h>

#include <dev/core/isa/isareg.h>
#include <dev/core/isa/isavar.h>

#if NIOAPIC > 0
#include <machine/apic/ioapicvar.h>
#endif

#if NLAPIC > 0
#include <machine/apic/lapicvar.h>
#endif

/*
 * Generic configuration;  all in one
 */
dev_t	rootdev = makedev(0,0);
dev_t	dumpdev = makedev(0,1);
int		nswap;

struct swdevt swdevt[] = {
		{ 1, 0,	0 },
		{ NODEV, 1,	0 },
};
long dumplo;
int	dmmin, dmmax, dmtext;

#define	DOSWAP				/* change swdevt and dumpdev */
u_long	bootdev = 0;		/* should be dev_t, but not until 32 bits */

static	char devname[][2] = {
	'w','d',	/* 0 = wd */
	's','w',	/* 1 = sw */
	'f','d',	/* 2 = fd */
	'w','t',	/* 3 = wt */
	'x','d',	/* 4 = xd */
};

#define	PARTITIONMASK	0x7
#define	PARTITIONSHIFT	3

/*
 * The following several variables are related to
 * the configuration process, and are used in initializing
 * the machine.
 */
int			dkn;		/* number of iostat dk numbers assigned so far */
extern int	cold;		/* cold start flag initialized in locore.s */

/*
 * Determine i/o configuration for a machine.
 */
void
configure()
{
	startrtclock();

	bios32_init();
	k6_mem_drvinit();

	i386_proc0_tss_ldt_init();

	if(config_rootfound("mainbus", NULL) == NULL) {
		panic("cpu_configure: mainbus not configured");
	}

#if NIOAPIC > 0
	ioapic_enable();
#endif

	/*
	 * Configure device structures
	 */
	mi_device_init(&sys_devsw);
	md_device_init(&sys_devsw);

	/*
	 * Configure swap area and related system
	 * parameter based on device(s) used.
	 */
	swapconf();

	spl0();

#if NLAPIC > 0
	lapic_write_tpri(0);
#endif

	cold = 0;
}

/*
 * Configure MD (Machine-Dependent) Devices
 */
void
md_device_init(devsw)
	struct devswtable *devsw;
{
	DEVSWIO_CONFIG_INIT(devsw, 1, NULL, &cmos_cdevsw, NULL);			/* CMOS Interface */
	DEVSWIO_CONFIG_INIT(devsw, 1, NULL, &apm_cdevsw, NULL);				/* Power Management (APM) Interface */

}

/*
 * Configure swap space and related parameters.
 */
void
swapconf()
{
	register struct swdevt *swp;
	register int nblks;
	extern int Maxmem;

	for (swp = swdevt; swp->sw_dev != NODEV; swp++)	{
		if ( (u_int)swp->sw_dev >= nblkdev ){
			break;	/* XXX */
		}
		if (bdevsw[major(swp->sw_dev)].d_psize) {
			nblks = (*bdevsw[major(swp->sw_dev)].d_psize)(swp->sw_dev);
			if (nblks != -1 && (swp->sw_nblks == 0 || swp->sw_nblks > nblks)) {
				swp->sw_nblks = nblks;
			}
		}
	}
	if (dumplo == 0 && bdevsw[major(dumpdev)].d_psize) {
		dumplo = (*bdevsw[major(dumpdev)].d_psize)(dumpdev) - (Maxmem * NBPG/512);
		/*dumplo = (*bdevsw[major(dumpdev)].d_psize)(dumpdev) - physmem;*/
	}
	if (dumplo < 0) {
		dumplo = 0;
	}
}

/*
 * Attempt to find the device from which we were booted.
 * If we can do so, and not instructed not to do so,
 * change rootdev to correspond to the load device.
 */
void
setroot()
{
	int majdev, mindev, unit, part, adaptor;
	dev_t temp, orootdev;
	struct swdevt *swp;

	if ((boothowto & RB_DFLTROOT) || (bootdev & B_MAGICMASK) != (u_long) B_DEVMAGIC) {
		return;
	}
	majdev = (bootdev >> B_TYPESHIFT) & B_TYPEMASK;
	if (majdev > sizeof(devname) / sizeof(devname[0])) {
		return;
	}
	adaptor = (bootdev >> B_ADAPTORSHIFT) & B_ADAPTORMASK;
	part = (bootdev >> B_PARTITIONSHIFT) & B_PARTITIONMASK;
	unit = (bootdev >> B_UNITSHIFT) & B_UNITMASK;
	mindev = (unit << PARTITIONSHIFT) + part;
	orootdev = rootdev;
	rootdev = makedev(majdev, mindev);

	/*
	 * If the original rootdev is the same as the one
	 * just calculated, don't need to adjust the swap configuration.
	 */
	if (rootdev == orootdev)
		return;
	printf("changing root device to %c%c%d%c\n", devname[majdev][0], devname[majdev][1], mindev >> PARTITIONSHIFT, part + 'a');
#ifdef DOSWAP
	mindev &= ~PARTITIONMASK;
	for (swp = swdevt; swp->sw_dev != NODEV; swp++) {
		if (majdev == major(swp->sw_dev) && mindev == (minor(swp->sw_dev) & ~PARTITIONMASK)) {
			temp = swdevt[0].sw_dev;
			swdevt[0].sw_dev = swp->sw_dev;
			swp->sw_dev = temp;
			break;
		}
	}
	if (swp->sw_dev == NODEV)
		return;
	/*
	 * If dumpdev was the same as the old primary swap
	 * device, move it to the new primary swap device.
	 */
	if (temp == dumpdev)
		dumpdev = swdevt[0].sw_dev;
#endif
}
