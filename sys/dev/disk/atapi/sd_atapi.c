/*	$NetBSD: sd_atapi.c,v 1.3 1998/08/31 22:28:07 cgd Exp $	*/

/*
 * Copyright 1998
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

/*
 * ATAPI disk attachment for the 'sd' driver.
 *
 * Chris Demetriou, January 10, 1998.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/conf.h>
#include <sys/user.h>

#include <dev/disk/scsi/scsi_all.h>
#include <dev/disk/scsi/scsi_disk.h>
#include <dev/disk/atapi/atapi_all.h>
#include <dev/disk/atapi/atapi_disk.h>
#include <dev/disk/atapi/atapiconf.h>
#include <dev/disk/scsi/sdvar.h>

int	sd_atapibus_match (struct device *, struct cfdata *, void *);
void	sd_atapibus_attach (struct device *, struct device *, void *);
/*
struct cfdriver sd_atapibus_cd = {
	NULL, "sd_atapibus", sd_atapibus_match, sd_atapibus_attach, DV_DULL, sizeof(struct sd_softc)
};
*/
CFDRIVER_DECL(NULL, sd_atapibus, &sd_atapibus_cops, DV_DULL, sizeof(struct sd_softc));
CFOPS_DECL(sd_atapibus, sd_atapibus_match, sd_atapibus_attach, NULL, NULL);

struct scsipi_inquiry_pattern sd_atapibus_patterns[] = {
	{T_DIRECT, T_FIXED,				/* do these exist? */
	 "",         "",                 ""},
	{T_DIRECT, T_REMOV,
	 "",         "",                 ""},
	{T_OPTICAL, T_FIXED,			/* do these exist? */
	 "",         "",                 ""},
	{T_OPTICAL, T_REMOV,			/* do these exist? */
	 "",         "",                 ""},
};

static int	sd_atapibus_get_parms (struct sd_softc *, struct disk_parms *, int);

const struct sd_ops sd_atapibus_ops = {
	sd_atapibus_get_parms,
};

int
sd_atapibus_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct scsibus_attach_args *sa = aux;
	int priority;

	if (sa->sa_sc_link->type != BUS_ATAPI)
		return (0);

	(void) scsi_inqmatch(&sa->sa_inqbuf, (caddr_t) sd_atapibus_patterns,
			sizeof(sd_atapibus_patterns) / sizeof(sd_atapibus_patterns[0]),
			sizeof(sd_atapibus_patterns[0]), &priority);
	return (priority);
}

/*
 * The routine called by the low level scsipi routine when it discovers
 * a device suitable for this driver.
 */
void
sd_atapibus_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct sd_softc *sd = (void *)self;
	struct scsibus_attach_args *sa = aux;
	struct scsi_link *sc_link = sa->sa_sc_link;

	SC_DEBUG(sc_link, SDEV_DB2, ("sd_atapi_attach: "));

	sd->type = (sa->sa_inqbuf.type & SID_TYPE);
	scsi_strvis(sd->name, 16, sa->sa_inqbuf.product, 16);

	sdattach(parent, sd, sc_link, &sd_atapibus_ops);
}

static int
sd_atapibus_get_parms(sd, dp, flags)
	struct sd_softc *sd;
	struct disk_parms *dp;
	int flags;
{
	struct atapi_read_format_capacities scsi_cmd;
	struct atapi_capacity_descriptor *descp;
	struct atapi_sd_mode_data sense_data;
	char capacity_data[ATAPI_CAP_DESC_SIZE(1)];
	int error;

	bzero(&scsi_cmd, sizeof scsi_cmd);
	scsi_cmd.opcode = ATAPI_READ_FORMAT_CAPACITIES;
	_lto2b(ATAPI_CAP_DESC_SIZE(1), scsi_cmd.length);

	error = scsi_command(sd->sc_link, (struct scsi_generic*) &scsi_cmd,
			sizeof(scsi_cmd), (void*) capacity_data, ATAPI_CAP_DESC_SIZE(1),
			SDRETRIES, 20000,
			NULL, flags | SCSI_DATA_IN);
	SC_DEBUG(sd->sc_link, SDEV_DB2,
			("sd_atapibus_get_parms: read format capacities error=%d\n",
					error));
	if (error != 0)
		return (SDGP_RESULT_OFFLINE);

	descp =
			(struct atapi_capacity_descriptor*) &capacity_data[ATAPI_CAP_DESC_OFFSET_DESC(
					0)];

	switch (descp->byte5 & ATAPI_CAP_DESC_CODE_MASK) {
	case ATAPI_CAP_DESC_CODE_UNFORMATTED:
		return SDGP_RESULT_UNFORMATTED;

	case ATAPI_CAP_DESC_CODE_FORMATTED:
		break;

	default:
#ifdef DIAGNOSTIC
		printf("%s: strange capacity descriptor byte5 0x%x\n",
		    sd->sc_dev.dv_xname, (u_int)descp->byte5);
#endif
		/* FALLTHROUGH */
	case ATAPI_CAP_DESC_CODE_NONE:
		return SDGP_RESULT_OFFLINE;
	}

	dp->disksize = _4btol(descp->nblks);
	dp->blksize = _3btol(descp->blklen);

	/*
	 * First, set up standard fictitious geometry, a la sd_scsi.c.
	 */
	dp->heads = 64;
	dp->sectors = 32;
	dp->cyls = dp->disksize / (64 * 32);
	dp->rot_rate = 3600;

	/*
	 * Then try to get something better.  If we can't, that's
	 * still OK.
	 *
	 * XXX Rigid geometry page?
	 */
	error = atapi_mode_sense(sd->sc_link, ATAPI_FLEX_GEOMETRY_PAGE,
			(struct atapi_mode_header*) &sense_data, FLEXGEOMETRYPAGESIZE,
			flags, SDRETRIES, 20000);
	SC_DEBUG(sd->sc_link, SDEV_DB2,
			("sd_atapibus_get_parms: mode sense (flex) error=%d\n", error));
	if (error != 0)
		return (SDGP_RESULT_OK);

	dp->heads = sense_data.pages.flex_geometry.nheads;
	dp->sectors = sense_data.pages.flex_geometry.ph_sec_tr;
	dp->cyls = _2btol(sense_data.pages.flex_geometry.ncyl);
	dp->rot_rate = _2btol(sense_data.pages.flex_geometry.rot_rate);

	return (SDGP_RESULT_OK);
}
