/* $NetBSD: pckbc_isa.c,v 1.13 2004/03/24 17:26:53 drochner Exp $ */

/*
 * Copyright (c) 1998
 *	Matthias Drochner.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 */

#include <sys/cdefs.h>
/* __KERNEL_RCSID(0, "$NetBSD: pckbc_isa.c,v 1.13 2004/03/24 17:26:53 drochner Exp $"); */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/malloc.h> 
#include <sys/errno.h>
#include <sys/queue.h>
#include <sys/lock.h>
#include <sys/user.h>

#include <machine/bus.h>

#include <dev/core/isa/isareg.h>
#include <dev/core/isa/isavar.h>

#include <dev/core/ic/i8042reg.h>
#include <dev/misc/pccons/pckbcvar.h>

int		pckbc_isa_match (struct device *, struct cfdata *, void *);
void	pckbc_isa_attach (struct device *, struct device *, void *);

struct pckbc_isa_softc {
	struct pckbc_softc 	sc_pckbc;

	isa_chipset_tag_t 	sc_ic;
	int 				sc_irq[PCKBC_NSLOTS];
};

CFOPS_DECL(pckbc_isa, pckbc_isa_match, pckbc_isa_attach, NULL, NULL);
CFDRIVER_DECL(NULL, pckbc_isa, &pckbc_isa_cops, DV_DULL, sizeof(struct pckbc_isa_softc));

void	pckbc_isa_intr_establish(struct pckbc_softc *, pckbc_slot_t);

int
pckbc_isa_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct isa_attach_args *ia = aux;
	bus_space_tag_t iot = ia->ia_iot;
	bus_space_handle_t ioh_d, ioh_c;
	int res, ok = 1;

	if (ISA_DIRECT_CONFIG(ia))
		return (0);

	/* If values are hardwired to something that they can't be, punt. */
	if (ia->ia_nio < 1 || (ia->ia_iobase != IOBASEUNK && ia->ia_iobase != IO_KBD))
		return (0);

	if (ia->ia_niomem > 0 && (ia->ia_maddr != MADDRUNK))
		return (0);

	if (ia->ia_nirq < 1 || (ia->ia_irq != IRQUNK && ia->ia_irq != 1 /*XXX*/))
		return (0);

	if (ia->ia_ndrq > 0 && (ia->ia_drq != DRQUNK))
		return (0);

	if (pckbc_is_console(iot, IO_KBD) == 0) {
		struct pckbc_internal t;

		if (bus_space_map(iot, IO_KBD + KBDATAP, 1, 0, &ioh_d))
			return (0);
		if (bus_space_map(iot, IO_KBD + KBCMDP, 1, 0, &ioh_c)) {
			bus_space_unmap(iot, ioh_d, 1);
			return (0);
		}

		memset(&t, 0, sizeof(t));
		t.t_iot = iot;
		t.t_ioh_d = ioh_d;
		t.t_ioh_c = ioh_c;

		/* flush KBC */
		(void) pckbc_poll_data1(&t, PCKBC_KBD_SLOT);

		/* KBC selftest */
		if (pckbc_send_cmd(iot, ioh_c, KBC_SELFTEST) == 0) {
			ok = 0;
			goto out;
		}
		res = pckbc_poll_data1(&t, PCKBC_KBD_SLOT);
		if (res != 0x55) {
#ifdef DEBUG
			printf("kbc selftest: %x\n", res);
#endif
			ok = 0;
		}
 out:
		bus_space_unmap(iot, ioh_d, 1);
		bus_space_unmap(iot, ioh_c, 1);
	}

	if (ok) {
		ia->ia_iobase = IO_KBD;
		ia->ia_iosize = 5;
		ia->ia_nio = 1;

		ia->ia_niomem = 0;
		ia->ia_nirq = 0;
		ia->ia_ndrq = 0;
	}
	return (ok);
}

void
pckbc_isa_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pckbc_isa_softc *isc = (void *)self;
	struct pckbc_softc *sc = &isc->sc_pckbc;
	struct isa_attach_args *ia = aux;
	struct pckbc_internal *t;
	bus_space_tag_t iot;
	bus_space_handle_t ioh_d, ioh_c;

	isc->sc_ic = ia->ia_ic;
	iot = ia->ia_iot;

	switch (ia->ia_nirq) {
	case 1:
		/* Both channels use the same IRQ. */
		isc->sc_irq[PCKBC_KBD_SLOT] = isc->sc_irq[PCKBC_AUX_SLOT] = ia->ia_irq[0];
		break;

	case 2:
		/* First IRQ is kbd, second IRQ is aux port. */
		isc->sc_irq[PCKBC_KBD_SLOT] = ia->ia_irq[0];
		isc->sc_irq[PCKBC_AUX_SLOT] = ia->ia_irq[1];
		break;

	default:
		/* Set up IRQs for "normal" ISA. */
		isc->sc_irq[PCKBC_KBD_SLOT] = 1;
		isc->sc_irq[PCKBC_AUX_SLOT] = 12;
		break;
	}

	sc->intr_establish = pckbc_isa_intr_establish;

	if (pckbc_is_console(iot, IO_KBD)) {
		t = &pckbc_consdata;
		ioh_d = t->t_ioh_d;
		ioh_c = t->t_ioh_c;
		pckbc_console_attached = 1;
		/* t->t_cmdbyte was initialized by cnattach */
	} else {
		if (bus_space_map(iot, IO_KBD + KBDATAP, 1, 0, &ioh_d) ||
		    bus_space_map(iot, IO_KBD + KBCMDP, 1, 0, &ioh_c))
			panic("pckbc_attach: couldn't map");

		t = malloc(sizeof(struct pckbc_internal), M_DEVBUF, M_WAITOK|M_ZERO);
		t->t_iot = iot;
		t->t_ioh_d = ioh_d;
		t->t_ioh_c = ioh_c;
		t->t_addr = IO_KBD;
		t->t_cmdbyte = KC8_CPU; /* Enable ports */
		callout_init(&t->t_cleanup);
	}

	t->t_sc = sc;
	sc->id = t;

	printf("\n");

	/* Finish off the attach. */
	pckbc_attach(sc);
}

void
pckbc_isa_intr_establish(sc, slot)
	struct pckbc_softc *sc;
	pckbc_slot_t slot;
{
	struct pckbc_isa_softc *isc = (void *) sc;
	void *rv;

	rv = isa_intr_establish(isc->sc_ic, isc->sc_irq[slot], IST_EDGE, IPL_TTY, pckbcintr, sc);
	if (rv == NULL) {
		printf("%s: unable to establish interrupt for %s slot\n",
		    sc->sc_dv.dv_xname, pckbc_slot_names[slot]);
	} else {
		printf("%s: using irq %d for %s slot\n", sc->sc_dv.dv_xname, isc->sc_irq[slot], pckbc_slot_names[slot]);
	}
}
