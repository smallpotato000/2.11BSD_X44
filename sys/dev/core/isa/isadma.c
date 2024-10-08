/*	$NetBSD: isadma.c,v 1.32.4.1 1998/11/23 03:12:57 cgd Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

/*
 * Device driver for the ISA on-board DMA controller.
 */
#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <vm/include/vm.h>
#include <vm/include/vm_param.h>

#include <machine/bus.h>
#include <machine/param.h>
#include <machine/isa/isa_machdep.h>

#include <dev/core/ic/i8237reg.h>
#include <dev/core/isa/isadmareg.h>
#include <dev/core/isa/isadmavar.h>
#include <dev/core/isa/isareg.h>
#include <dev/core/isa/isavar.h>

/*
 * High byte of DMA address is stored in this DMAPG register for
 * the Nth DMA channel.
 */
static int dmapageport[2][4] = {
	{0x7, 0x3, 0x1, 0x2},
	{0xf, 0xb, 0x9, 0xa}
};

static u_int8_t dmamode[] = {
	/* write to device/read from device */
	DMA37MD_READ | DMA37MD_SINGLE,
	DMA37MD_WRITE | DMA37MD_SINGLE,

	/* write to device/read from device */
	DMA37MD_READ | DMA37MD_DEMAND,
	DMA37MD_WRITE | DMA37MD_DEMAND,

	/* write to device/read from device - DMAMODE_LOOP */
	DMA37MD_READ | DMA37MD_SINGLE | DMA37MD_LOOP,
	DMA37MD_WRITE | DMA37MD_SINGLE | DMA37MD_LOOP,

	/* write to device/read from device - DMAMODE_LOOPDEMAND */
	DMA37MD_READ | DMA37MD_DEMAND | DMA37MD_LOOP,
	DMA37MD_WRITE | DMA37MD_DEMAND | DMA37MD_LOOP,
};

SIMPLEQ_HEAD(isa_mem_list, isa_mem) isa_mem_head = SIMPLEQ_HEAD_INITIALIZER(isa_mem_head);

static inline void
isa_dmaunmask(ic, chan)
	isa_chipset_tag_t ic;
	int chan;
{
	struct isa_dma_state *ids = (struct isa_dma_state *)ic;
	int ochan = chan & 3;

	ISA_DMA_MASK_CLR(ids, chan);

	/*
	 * If DMA is frozen, don't unmask it now.  It will be
	 * unmasked when DMA is thawed again.
	 */
	if (ids->ids_frozen)
		return;

	/* set dma channel mode, and set dma channel mode */
	if ((chan & 4) == 0)
		bus_space_write_1(ids->ids_bst, ids->ids_dma1h,
		    DMA1_SMSK, ochan | DMA37SM_CLEAR);
	else
		bus_space_write_1(ids->ids_bst, ids->ids_dma2h,
		    DMA2_SMSK, ochan | DMA37SM_CLEAR);
}

static inline void
isa_dmamask(ic, chan)
	isa_chipset_tag_t ic;
	int chan;
{
	struct isa_dma_state *ids = (struct isa_dma_state *)ic;
	int ochan = chan & 3;

	ISA_DMA_MASK_SET(ids, chan);

	/*
	 * XXX Should we avoid masking the channel if DMA is
	 * XXX frozen?  It seems like what we're doing should
	 * XXX be safe, and we do need to reset FFC...
	 */

	/* set dma channel mode, and set dma channel mode */
	if ((chan & 4) == 0) {
		bus_space_write_1(ids->ids_bst, ids->ids_dma1h,
		    DMA1_SMSK, ochan | DMA37SM_SET);
		bus_space_write_1(ids->ids_bst, ids->ids_dma1h,
		    DMA1_FFC, 0);
	} else {
		bus_space_write_1(ids->ids_bst, ids->ids_dma2h,
		    DMA2_SMSK, ochan | DMA37SM_SET);
		bus_space_write_1(ids->ids_bst, ids->ids_dma2h,
		    DMA2_FFC, 0);
	}
}

/*
 * _isa_dmainit(): Initialize the isa_dma_state for this chipset.
 */
void
isa_dmainit(ic, bst, dmat, dev)
	isa_chipset_tag_t ic;
	bus_space_tag_t bst;
	bus_dma_tag_t dmat;
	struct device *dev;
{
	struct isa_dma_state *ids = (struct isa_dma_state *)ic;
	int chan;

	ids->ids_dev = dev;

	if (ids->ids_initialized) {
		/*
		 * Some systems may have e.g. `ofisa' (OpenFirmware
		 * configuration of ISA bus) and a regular `isa'.
		 * We allow both to call the initialization function,
		 * and take the device name from the last caller
		 * (assuming it will be the indirect ISA bus).  Since
		 * `ofisa' and `isa' are the same bus with different
		 * configuration mechanisms, the space and dma tags
		 * must be the same!
		 */
		if (ids->ids_bst != bst || ids->ids_dmat != dmat)
			panic("_isa_dmainit: inconsistent ISA tags");
	} else {
		ids->ids_bst = bst;
		ids->ids_dmat = dmat;

		/*
		 * Map the registers used by the ISA DMA controller.
		 */
		if (bus_space_map(ids->ids_bst, IO_DMA1, DMA1_IOSIZE, 0,
		    &ids->ids_dma1h))
			panic("_isa_dmainit: unable to map DMA controller #1");
		if (bus_space_map(ids->ids_bst, IO_DMA2, DMA2_IOSIZE, 0,
		    &ids->ids_dma2h))
			panic("_isa_dmainit: unable to map DMA controller #2");
		if (bus_space_map(ids->ids_bst, IO_DMAPG, 0xf, 0,
		    &ids->ids_dmapgh))
			panic("_isa_dmainit: unable to map DMA page registers");

		/*
		 * All 8 DMA channels start out "masked".
		 */
		ids->ids_masked = 0xff;

		/*
		 * Initialize the max transfer size for each channel, if
		 * it is not initialized already (i.e. by a bus-dependent
		 * front-end).
		 */
		for (chan = 0; chan < 8; chan++) {
			if (ids->ids_maxsize[chan] == 0)
				ids->ids_maxsize[chan] =
				    ISA_DMA_MAXSIZE_DEFAULT(chan);
		}

		ids->ids_initialized = 1;

		/*
		 * DRQ 4 is used to chain the two 8237s together; make
		 * sure it's always cascaded, and that it will be unmasked
		 * when DMA is thawed.
		 */
		isa_dmacascade(ic, 4);
	}
}

/*
 * _isa_dmacascade(): program 8237 DMA controller channel to accept
 * external dma control by a board.
 */
int
isa_dmacascade(ic, chan)
	isa_chipset_tag_t ic;
	int chan;
{
	struct isa_dma_state *ids = (struct isa_dma_state *)ic;
	int ochan = chan & 3;

	if (chan < 0 || chan > 7) {
		printf("%s: bogus drq %d\n", ids->ids_dev->dv_xname, chan);
		return (EINVAL);
	}

	if (ISA_DMA_DRQ_ISFREE(ids, chan) == 0) {
		printf("%s: DRQ %d is not free\n", ids->ids_dev->dv_xname,
		    chan);
		return (EAGAIN);
	}

	ISA_DMA_DRQ_ALLOC(ids, chan);

	/* set dma channel mode, and set dma channel mode */
	if ((chan & 4) == 0)
		bus_space_write_1(ids->ids_bst, ids->ids_dma1h,
		    DMA1_MODE, ochan | DMA37MD_CASCADE);
	else
		bus_space_write_1(ids->ids_bst, ids->ids_dma2h,
		    DMA2_MODE, ochan | DMA37MD_CASCADE);

	isa_dmaunmask(ids, chan);
	return (0);
}

int
isa_drq_alloc(ic, chan)
	isa_chipset_tag_t ic;
	int chan;
{
	struct isa_dma_state *ids = (struct isa_dma_state *)ic;

	if (ISA_DMA_DRQ_ISFREE(ids, chan) == 0)
		return EBUSY;
	ISA_DMA_DRQ_ALLOC(ids, chan);
	return 0;
}

int
isa_drq_free(ic, chan)
	isa_chipset_tag_t ic;
	int chan;
{
	struct isa_dma_state *ids = (struct isa_dma_state *)ic;

	if (ISA_DMA_DRQ_ISFREE(ids, chan))
		return EINVAL;
	ISA_DMA_DRQ_FREE(ids, chan);
	return 0;
}

bus_size_t
isa_dmamaxsize(ic, chan)
	isa_chipset_tag_t ic;
	int chan;
{
	struct isa_dma_state *ids = (struct isa_dma_state *)ic;

	if (chan < 0 || chan > 7) {
		printf("%s: bogus drq %d\n", ids->ids_dev->dv_xname, chan);
		return (0);
	}

	return (ids->ids_maxsize[chan]);
}

int
isa_dmamap_create(ic, chan, size, flags)
	isa_chipset_tag_t ic;
	int chan;
	bus_size_t size;
	int flags;
{
	struct isa_dma_state *ids = (struct isa_dma_state *)ic;
	int error;

	if (chan < 0 || chan > 7) {
		printf("%s: bogus drq %d\n", ids->ids_dev->dv_xname, chan);
		return (EINVAL);
	}

	if (size > ids->ids_maxsize[chan])
		return (EINVAL);

	error = bus_dmamap_create(ids->ids_dmat, size, 1, size,
	    ids->ids_maxsize[chan], flags, &ids->ids_dmamaps[chan]);

	return (error);
}

void
isa_dmamap_destroy(ic, chan)
	isa_chipset_tag_t ic;
	int chan;
{
	struct isa_dma_state *ids = (struct isa_dma_state *)ic;

	if (chan < 0 || chan > 7) {
		printf("%s: bogus drq %d\n", ids->ids_dev->dv_xname, chan);
		goto lose;
	}

	bus_dmamap_destroy(ids->ids_dmat, ids->ids_dmamaps[chan]);
	return;

 lose:
	panic("_isa_dmamap_destroy");
}

/*
 * _isa_dmastart(): program 8237 DMA controller channel and set it
 * in motion.
 */
int
isa_dmastart(ic, chan, addr, nbytes, p, flags, busdmaflags)
	isa_chipset_tag_t ic;
	int chan;
	void *addr;
	bus_size_t nbytes;
	struct proc *p;
	int flags;
	int busdmaflags;
{
	struct isa_dma_state *ids = (struct isa_dma_state *)ic;
	bus_dmamap_t dmam;
	bus_addr_t dmaaddr;
	int waport;
	int ochan = chan & 3;
	int error;

	if (chan < 0 || chan > 7) {
		printf("%s: bogus drq %d\n", ids->ids_dev->dv_xname, chan);
		goto lose;
	}

#ifdef ISADMA_DEBUG
	printf("_isa_dmastart: drq %d, addr %p, nbytes 0x%lx, p %p, "
	    "flags 0x%x, dmaflags 0x%x\n",
	    chan, addr, nbytes, p, flags, busdmaflags);
#endif

	if (ISA_DMA_DRQ_ISFREE(ids, chan)) {
		printf("%s: dma start on free channel %d\n",
		    ids->ids_dev->dv_xname, chan);
		goto lose;
	}

	if (chan & 4) {
		if (nbytes > (1 << 17) || (nbytes & 1) || ((u_long)addr & 1)) {
			printf("%s: drq %d, nbytes 0x%lx, addr %p\n",
			    ids->ids_dev->dv_xname, chan,
			    (unsigned long) nbytes, addr);
			goto lose;
		}
	} else {
		if (nbytes > (1 << 16)) {
			printf("%s: drq %d, nbytes 0x%lx\n",
			    ids->ids_dev->dv_xname, chan,
			    (unsigned long) nbytes);
			goto lose;
		}
	}

	dmam = ids->ids_dmamaps[chan];
	if (dmam == NULL)
		panic("_isa_dmastart: no DMA map for chan %d", chan);

	error = bus_dmamap_load(ids->ids_dmat, dmam, addr, nbytes,
	    p, busdmaflags |
	    ((flags & DMAMODE_READ) ? BUS_DMA_READ : BUS_DMA_WRITE));
	if (error)
		return (error);

#ifdef ISADMA_DEBUG
	__asm(".globl isa_dmastart_afterload ; isa_dmastart_afterload:");
#endif

	if (flags & DMAMODE_READ) {
		bus_dmamap_sync(ids->ids_dmat, dmam, BUS_DMASYNC_PREREAD);
		ids->ids_dmareads |= (1 << chan);
	} else {
		bus_dmamap_sync(ids->ids_dmat, dmam, BUS_DMASYNC_PREWRITE);
		ids->ids_dmareads &= ~(1 << chan);
	}

	dmaaddr = dmam->dm_segs[0].ds_addr;

#ifdef ISADMA_DEBUG
	printf("     dmaaddr 0x%lx\n", dmaaddr);

	__asm(".globl isa_dmastart_aftersync ; isa_dmastart_aftersync:");
#endif

	ids->ids_dmalength[chan] = nbytes;

	isa_dmamask(ic, chan);
	ids->ids_dmafinished &= ~(1 << chan);

	if ((chan & 4) == 0) {
		/* set dma channel mode */
		bus_space_write_1(ids->ids_bst, ids->ids_dma1h, DMA1_MODE,
		    ochan | dmamode[flags]);

		/* send start address */
		waport = DMA1_CHN(ochan);
		bus_space_write_1(ids->ids_bst, ids->ids_dmapgh,
		    dmapageport[0][ochan], (dmaaddr >> 16) & 0xff);
		bus_space_write_1(ids->ids_bst, ids->ids_dma1h, waport,
		    dmaaddr & 0xff);
		bus_space_write_1(ids->ids_bst, ids->ids_dma1h, waport,
		    (dmaaddr >> 8) & 0xff);

		/* send count */
		bus_space_write_1(ids->ids_bst, ids->ids_dma1h, waport + 1,
		    (--nbytes) & 0xff);
		bus_space_write_1(ids->ids_bst, ids->ids_dma1h, waport + 1,
		    (nbytes >> 8) & 0xff);
	} else {
		/* set dma channel mode */
		bus_space_write_1(ids->ids_bst, ids->ids_dma2h, DMA2_MODE,
		    ochan | dmamode[flags]);

		/* send start address */
		waport = DMA2_CHN(ochan);
		bus_space_write_1(ids->ids_bst, ids->ids_dmapgh,
		    dmapageport[1][ochan], (dmaaddr >> 16) & 0xff);
		dmaaddr >>= 1;
		bus_space_write_1(ids->ids_bst, ids->ids_dma2h, waport,
		    dmaaddr & 0xff);
		bus_space_write_1(ids->ids_bst, ids->ids_dma2h, waport,
		    (dmaaddr >> 8) & 0xff);

		/* send count */
		nbytes >>= 1;
		bus_space_write_1(ids->ids_bst, ids->ids_dma2h, waport + 2,
		    (--nbytes) & 0xff);
		bus_space_write_1(ids->ids_bst, ids->ids_dma2h, waport + 2,
		    (nbytes >> 8) & 0xff);
	}

	isa_dmaunmask(ic, chan);
	return (0);

 lose:
	panic("_isa_dmastart");
}

void
isa_dmaabort(ic, chan)
	isa_chipset_tag_t ic;
	int chan;
{
	struct isa_dma_state *ids = (struct isa_dma_state *)ic;

	if (chan < 0 || chan > 7) {
		printf("%s: bogus drq %d\n", ids->ids_dev->dv_xname, chan);
		panic("_isa_dmaabort");
	}

	isa_dmamask(ic, chan);
	bus_dmamap_unload(ids->ids_dmat, ids->ids_dmamaps[chan]);
	ids->ids_dmareads &= ~(1 << chan);
}

bus_size_t
isa_dmacount(ic, chan)
	isa_chipset_tag_t ic;
	int chan;
{
	struct isa_dma_state *ids = (struct isa_dma_state *)ic;
	int waport;
	bus_size_t nbytes;
	int ochan = chan & 3;

	if (chan < 0 || chan > 7) {
		printf("%s: bogus drq %d\n", ids->ids_dev->dv_xname, chan);
		panic("isa_dmacount");
	}

	isa_dmamask(ic, chan);

	/*
	 * We have to shift the byte count by 1.  If we're in auto-initialize
	 * mode, the count may have wrapped around to the initial value.  We
	 * can't use the TC bit to check for this case, so instead we compare
	 * against the original byte count.
	 * If we're not in auto-initialize mode, then the count will wrap to
	 * -1, so we also handle that case.
	 */
	if ((chan & 4) == 0) {
		waport = DMA1_CHN(ochan);
		nbytes = bus_space_read_1(ids->ids_bst, ids->ids_dma1h,
		    waport + 1) + 1;
		nbytes += bus_space_read_1(ids->ids_bst, ids->ids_dma1h,
		    waport + 1) << 8;
		nbytes &= 0xffff;
	} else {
		waport = DMA2_CHN(ochan);
		nbytes = bus_space_read_1(ids->ids_bst, ids->ids_dma2h,
		    waport + 2) + 1;
		nbytes += bus_space_read_1(ids->ids_bst, ids->ids_dma2h,
		    waport + 2) << 8;
		nbytes <<= 1;
		nbytes &= 0x1ffff;
	}

	if (nbytes == ids->ids_dmalength[chan])
		nbytes = 0;

	isa_dmaunmask(ic, chan);
	return (nbytes);
}

int
isa_dmafinished(ic, chan)
	isa_chipset_tag_t ic;
	int chan;
{
	struct isa_dma_state *ids = (struct isa_dma_state *)ic;

	if (chan < 0 || chan > 7) {
		printf("%s: bogus drq %d\n", ids->ids_dev->dv_xname, chan);
		panic("_isa_dmafinished");
	}

	/* check that the terminal count was reached */
	if ((chan & 4) == 0)
		ids->ids_dmafinished |= bus_space_read_1(ids->ids_bst,
		    ids->ids_dma1h, DMA1_SR) & 0x0f;
	else
		ids->ids_dmafinished |= (bus_space_read_1(ids->ids_bst,
		    ids->ids_dma2h, DMA2_SR) & 0x0f) << 4;

	return ((ids->ids_dmafinished & (1 << chan)) != 0);
}

void
isa_dmadone(ic, chan)
	isa_chipset_tag_t ic;
	int chan;
{
	struct isa_dma_state *ids = (struct isa_dma_state *)ic;
	bus_dmamap_t dmam;

	if (chan < 0 || chan > 7) {
		printf("%s: bogus drq %d\n", ids->ids_dev->dv_xname, chan);
		panic("_isa_dmadone");
	}

	dmam = ids->ids_dmamaps[chan];

	isa_dmamask(ic, chan);

	if (isa_dmafinished(ic, chan) == 0)
		printf("%s: _isa_dmadone: channel %d not finished\n",
		    ids->ids_dev->dv_xname, chan);

	bus_dmamap_sync(ids->ids_dmat, dmam, (ids->ids_dmareads & (1 << chan)) ? BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);

	bus_dmamap_unload(ids->ids_dmat, dmam);
	ids->ids_dmareads &= ~(1 << chan);
}

void
isa_dmafreeze(ic)
	isa_chipset_tag_t ic;
{
	struct isa_dma_state *ids = (struct isa_dma_state *)ic;
	int s;

	s = splhigh();

	if (ids->ids_frozen == 0) {
		bus_space_write_1(ids->ids_bst, ids->ids_dma1h,
		    DMA1_MASK, 0x0f);
		bus_space_write_1(ids->ids_bst, ids->ids_dma2h,
		    DMA2_MASK, 0x0f);
	}

	ids->ids_frozen++;
	if (ids->ids_frozen < 1)
		panic("_isa_dmafreeze: overflow");

	splx(s);
}

void
isa_dmathaw(ic)
	isa_chipset_tag_t ic;
{
	struct isa_dma_state *ids = (struct isa_dma_state *)ic;
	int s;

	s = splhigh();

	ids->ids_frozen--;
	if (ids->ids_frozen < 0)
		panic("_isa_dmathaw: underflow");

	if (ids->ids_frozen == 0) {
		bus_space_write_1(ids->ids_bst, ids->ids_dma1h,
		    DMA1_MASK, ids->ids_masked & 0x0f);
		bus_space_write_1(ids->ids_bst, ids->ids_dma2h,
		    DMA2_MASK, (ids->ids_masked >> 4) & 0x0f);
	}

	splx(s);
}

int
isa_dmamem_alloc(ic, chan, size, addrp, flags)
	isa_chipset_tag_t ic;
	int chan;
	bus_size_t size;
	bus_addr_t *addrp;
	int flags;
{
	struct isa_dma_state *ids = (struct isa_dma_state *)ic;
	bus_dma_segment_t seg;
	int error, boundary, rsegs;

	if (chan < 0 || chan > 7) {
		printf("%s: bogus drq %d\n", ids->ids_dev->dv_xname, chan);
		panic("_isa_dmamem_alloc");
	}

	boundary = (chan & 4) ? (1 << 17) : (1 << 16);

	size = round_page(size);

	error = bus_dmamem_alloc(ids->ids_dmat, size, PAGE_SIZE, boundary,
	    &seg, 1, &rsegs, flags);
	if (error)
		return (error);

	*addrp = seg.ds_addr;
	return (0);
}

void
isa_dmamem_free(ic, chan, addr, size)
	isa_chipset_tag_t ic;
	int chan;
	bus_addr_t addr;
	bus_size_t size;
{
	struct isa_dma_state *ids = (struct isa_dma_state *)ic;
	bus_dma_segment_t seg;

	if (chan < 0 || chan > 7) {
		printf("%s: bogus drq %d\n", ids->ids_dev->dv_xname, chan);
		panic("_isa_dmamem_free");
	}

	seg.ds_addr = addr;
	seg.ds_len = size;

	bus_dmamem_free(ids->ids_dmat, &seg, 1);
}

int
isa_dmamem_map(ic, chan, addr, size, kvap, flags)
	isa_chipset_tag_t ic;
	int chan;
	bus_addr_t addr;
	bus_size_t size;
	caddr_t *kvap;
	int flags;
{
	struct isa_dma_state *ids = (struct isa_dma_state *)ic;
	bus_dma_segment_t seg;

	if (chan < 0 || chan > 7) {
		printf("%s: bogus drq %d\n", ids->ids_dev->dv_xname, chan);
		panic("isa_dmamem_map");
	}

	seg.ds_addr = addr;
	seg.ds_len = size;

	return (bus_dmamem_map(ids->ids_dmat, &seg, 1, size, kvap, flags));
}

void
isa_dmamem_unmap(ic, chan, kva, size)
	isa_chipset_tag_t ic;
	int chan;
	caddr_t kva;
	size_t size;
{
	struct isa_dma_state *ids = (struct isa_dma_state *)ic;

	if (chan < 0 || chan > 7) {
		printf("%s: bogus drq %d\n", ids->ids_dev->dv_xname, chan);
		panic("_isa_dmamem_unmap");
	}

	bus_dmamem_unmap(ids->ids_dmat, kva, size);
}

int
isa_dmamem_mmap(ic, chan, addr, size, off, prot, flags)
	isa_chipset_tag_t ic;
	int chan;
	bus_addr_t addr;
	bus_size_t size;
	int off, prot, flags;
{
	struct isa_dma_state *ids = (struct isa_dma_state *)ic;
	bus_dma_segment_t seg;

	if (chan < 0 || chan > 7) {
		printf("%s: bogus drq %d\n", ids->ids_dev->dv_xname, chan);
		panic("_isa_dmamem_mmap");
	}

	if (off < 0)
		return (-1);

	seg.ds_addr = addr;
	seg.ds_len = size;

	return (bus_dmamem_mmap(ids->ids_dmat, &seg, 1, off, prot, flags));
}

int
isa_drq_isfree(ic, chan)
	isa_chipset_tag_t ic;
	int chan;
{
	struct isa_dma_state *ids = (struct isa_dma_state *)ic;

	if (chan < 0 || chan > 7) {
		printf("%s: bogus drq %d\n", ids->ids_dev->dv_xname, chan);
		panic("_isa_drq_isfree");
	}

	return ISA_DMA_DRQ_ISFREE(ids, chan);
}

void *
isa_malloc(ic, chan, size, type, flags)
	isa_chipset_tag_t ic;
	int 			chan;
	size_t 			size;
	int 			type;
	int 			flags;
{
	struct isa_dma_state *ids = (struct isa_dma_state *)ic;
	struct isa_mem 	*m;
	bus_addr_t 		addr;
	caddr_t 		kva;
	int 			bflags;

	bflags = flags & M_WAITOK ? BUS_DMA_WAITOK : BUS_DMA_NOWAIT;

	if (isa_dmamem_alloc(ic, chan, size, &addr, bflags))
		return 0;
	if (isa_dmamem_map(ic, chan, addr, size, &kva, bflags)) {
		isa_dmamem_free(ic, chan, addr, size);
		return 0;
	}
	m = malloc(sizeof(*m), type, flags);
	if (m == 0) {
		isa_dmamem_unmap(ic, chan, kva, size);
		isa_dmamem_free(ic, chan, addr, size);
		return 0;
	}

	m->ic = ic;
	m->ids = ids;
	m->chan = chan;
	m->size = size;
	m->addr = addr;
	m->kva = kva;
	SIMPLEQ_INSERT_HEAD(&isa_mem_head, m, next);
	return ((void *)kva);
}

void
isa_free(addr, type)
	void *addr;
	int type;
{
	struct isa_mem **mp, *m;
	caddr_t kva = (caddr_t)addr;

	SIMPLEQ_FOREACH(m, &isa_mem_head, next) {
		if (m->kva != kva) {
			;
		}
	}
	if (!m) {
		printf("isa_free: freeing unallocted memory\n");
		return;
	}
	SIMPLEQ_REMOVE(&isa_mem_head, m, isa_mem, next);
	isa_dmamem_unmap(m->ic, m->chan, kva, m->size);
	isa_dmamem_free(m->ic, m->chan, m->addr, m->size);
	free(m, type);
}

int
isa_mappage(mem, off, prot)
	void *mem;
	int off;
	int prot;
{
	struct isa_mem 	*m;

	SIMPLEQ_FOREACH(m, &isa_mem_head, next) {
		;
	}
	if (!m) {
		printf("isa_mappage: mapping unallocted memory\n");
		return (-1);
	}
	return (isa_dmamem_mmap(m->ic, m->chan, m->addr, m->size, off, prot, BUS_DMA_WAITOK));
}
