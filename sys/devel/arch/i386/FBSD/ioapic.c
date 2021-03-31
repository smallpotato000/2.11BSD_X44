/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2003 John Baldwin <jhb@FreeBSD.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/lock.h>
#include <sys/user.h>
#include <sys/malloc.h>

#include <devel/sys/malloctypes.h>
#include <devel/arch/i386/include/cpu.h>

#include <devel/arch/i386/FBSD/apicreg.h>
#include <devel/arch/i386/FBSD/apicvar.h>
#include <arch/i386/isa/isa_machdep.h>

#define ioapic_lock_init(lock) 	simple_lock_init(lock, "ioapic_lock")
#define ioapic_lock(lock) 		simple_lock(lock)
#define ioapic_unlock(lock) 	simple_unlock(lock)

#define IOAPIC_ISA_INTS			16
#define	IOAPIC_MEM_REGION		32
#define	IOAPIC_REDTBL_LO(i)		(IOAPIC_REDTBL + (i) * 2)
#define	IOAPIC_REDTBL_HI(i)		(IOAPIC_REDTBL_LO(i) + 1)

/*
 * I/O APIC interrupt source driver.  Each pin is assigned an IRQ cookie
 * as laid out in the ACPI System Interrupt number model where each I/O
 * APIC has a contiguous chunk of the System Interrupt address space.
 * We assume that IRQs 1 - 15 behave like ISA IRQs and that all other
 * IRQs behave as PCI IRQs by default.  We also assume that the pin for
 * IRQ 0 is actually an ExtINT pin.  The apic enumerators override the
 * configuration of individual pins as indicated by their tables.
 *
 * Documentation for the I/O APIC: "82093AA I/O Advanced Programmable
 * Interrupt Controller (IOAPIC)", May 1996, Intel Corp.
 * ftp://download.intel.com/design/chipsets/datashts/29056601.pdf
 */

struct ioapic_intsrc {
	struct intsrc 			io_intsrc;
	int 					io_irq;
	u_int 					io_intpin:8;
	u_int 					io_vector:8;
	u_int 					io_cpu;
	u_int 					io_activehi:1;
	u_int 					io_edgetrigger:1;
	u_int 					io_masked:1;
	int 					io_bus:4;
	uint32_t 				io_lowreg;
	u_int 					io_remap_cookie;
};

struct ioapic {
	struct pic 				io_pic;
	u_int 					io_id:8;			/* logical ID */
	u_int 					io_apic_id:8;		/* Id as enumerated by MADT */
	u_int 					io_hw_apic_id:8;	/* Content of APIC ID register */
	u_int 					io_intbase:8;		/* System Interrupt base */
	u_int 					io_numintr:8;
	u_int 					io_haseoi:1;
	volatile ioapic_t 		*io_addr;			/* XXX: should use bus_space */
	caddr_t 				io_paddr;
	SIMPLEQ_ENTRY(ioapic) 	io_next;
	struct device			*io_dev;			/* matched pci device, if found */

	struct ioapic_intsrc 	io_pins[0];
};

static u_int		ioapic_read(volatile ioapic_t *apic, int reg);
static void			ioapic_write(volatile ioapic_t *apic, int reg, u_int val);
static const char 	*ioapic_bus_string(int bus_type);
static void			ioapic_print_irq(struct ioapic_intsrc *intpin);
static void			ioapic_register_sources(struct pic *pic);
static void			ioapic_enable_source(struct intsrc *isrc);
static void			ioapic_disable_source(struct intsrc *isrc, int eoi);
static void			ioapic_eoi_source(struct intsrc *isrc);
static void			ioapic_enable_intr(struct intsrc *isrc);
static void			ioapic_disable_intr(struct intsrc *isrc);
static int			ioapic_vector(struct intsrc *isrc);
static int			ioapic_source_pending(struct intsrc *isrc);
static int			ioapic_config_intr(struct intsrc *isrc, enum intr_trigger trig, enum intr_polarity pol);
static void			ioapic_resume(struct pic *pic, boolean_t suspend_cancelled);
static int			ioapic_assign_cpu(struct intsrc *isrc, u_int apic_id);
static void			ioapic_program_intpin(struct ioapic_intsrc *intpin);
static void			ioapic_reprogram_intpin(struct intsrc *isrc);

static SIMPLEQ_HEAD(,ioapic) ioapic_list = SIMPLEQ_HEAD_INITIALIZER(ioapic_list);
struct pic ioapic_template = {
		.pic_register_sources 	= ioapic_register_sources,
		.pic_enable_source 		= ioapic_enable_source,
		.pic_disable_source 	= ioapic_disable_source,
		.pic_eoi_source 		= ioapic_eoi_source,
		.pic_enable_intr 		= ioapic_enable_intr,
		.pic_disable_intr 		= ioapic_disable_intr,
		.pic_vector 			= ioapic_vector,
		.pic_source_pending 	= ioapic_source_pending,
		.pic_suspend 			= NULL,
		.pic_resume 			= ioapic_resume,
		.pic_config_intr 		= ioapic_config_intr,
		.pic_assign_cpu 		= ioapic_assign_cpu,
		.pic_reprogram_pin 		= ioapic_reprogram_intpin,
};

struct lock_object *icu_lock;
static u_int next_ioapic_base;
static u_int next_id;
static int enable_extint;

static void
_ioapic_eoi_source(struct intsrc *isrc, int locked)
{
	struct ioapic_intsrc *src;
	struct ioapic *io;
	volatile uint32_t *apic_eoi;
	uint32_t low1;

	lapic_eoi();
	if (!lapic_eoi_suppression)
		return;
	src = (struct ioapic_intsrc *)isrc;
	if (src->io_edgetrigger)
		return;
	io = (struct ioapic *)isrc->is_pic;

	/*
	 * Handle targeted EOI for level-triggered pins, if broadcast
	 * EOI suppression is supported by LAPICs.
	 */
	if (io->io_haseoi) {
		/*
		 * If IOAPIC has EOI Register, simply write vector
		 * number into the reg.
		 */
		apic_eoi = (volatile uint32_t *)((volatile char *)io->io_addr + IOAPIC_EOIR);
		*apic_eoi = src->io_vector;
	} else {
		/*
		 * Otherwise, if IO-APIC is too old to provide EOIR,
		 * do what Intel did for the Linux kernel. Temporary
		 * switch the pin to edge-trigger and back, masking
		 * the pin during the trick.
		 */
		if (!locked)
			ioapic_lock(&icu_lock);
		low1 = src->io_lowreg;
		low1 &= ~IOART_TRGRLVL;
		low1 |= IOART_TRGREDG | IOART_INTMSET;
		ioapic_write(io->io_addr, IOAPIC_REDTBL_LO(src->io_intpin), low1);
		low1 = src->io_lowreg;
		if (src->io_masked != 0)
			low1 |= IOART_INTMSET;
		ioapic_write(io->io_addr, IOAPIC_REDTBL_LO(src->io_intpin), low1);
		if (!locked)
			ioapic_unlock(&icu_lock);
	}
}

static u_int
ioapic_read(volatile ioapic_t *apic, int reg)
{
	apic->ioregsel = reg;
	return (apic->iowin);
}

static void
ioapic_write(volatile ioapic_t *apic, int reg, u_int val)
{
	apic->ioregsel = reg;
	apic->iowin = val;
}

static const char *
ioapic_bus_string(int bus_type)
{
	switch (bus_type) {
	case APIC_BUS_ISA:
		return ("ISA");
	case APIC_BUS_EISA:
		return ("EISA");
	case APIC_BUS_PCI:
		return ("PCI");
	default:
		return ("unknown");
	}
}

static void
ioapic_print_irq(struct ioapic_intsrc *intpin)
{
	switch (intpin->io_irq) {
	case IRQ_DISABLED:
		printf("disabled");
		break;
	case IRQ_EXTINT:
		printf("ExtINT");
		break;
	case IRQ_NMI:
		printf("NMI");
		break;
	case IRQ_SMI:
		printf("SMI");
		break;
	default:
		printf("%s IRQ %d", ioapic_bus_string(intpin->io_bus), intpin->io_irq);
	}
}

static void
ioapic_enable_source(struct intsrc *isrc)
{
	struct ioapic_intsrc *intpin = (struct ioapic_intsrc *)isrc;
	struct ioapic *io = (struct ioapic *)isrc->is_pic;
	uint32_t flags;

	ioapic_lock(&icu_lock);
	if (intpin->io_masked) {
		flags = intpin->io_lowreg & ~IOART_INTMASK;
		ioapic_write(io->io_addr, IOAPIC_REDTBL_LO(intpin->io_intpin), flags);
		intpin->io_masked = 0;
	}
	ioapic_unlock(&icu_lock);
}

static void
ioapic_disable_source(struct intsrc *isrc, int eoi)
{
	struct ioapic_intsrc *intpin = (struct ioapic_intsrc *)isrc;
	struct ioapic *io = (struct ioapic *)isrc->is_pic;
	uint32_t flags;

	ioapic_lock(&icu_lock);
	if (!intpin->io_masked && !intpin->io_edgetrigger) {
		flags = intpin->io_lowreg | IOART_INTMSET;
		ioapic_write(io->io_addr, IOAPIC_REDTBL_LO(intpin->io_intpin), flags);
		intpin->io_masked = 1;
	}

	if (eoi == PIC_EOI)
		_ioapic_eoi_source(isrc, 1);

	ioapic_unlock(&icu_lock);
}

static void
ioapic_eoi_source(struct intsrc *isrc)
{
	_ioapic_eoi_source(isrc, 0);
}

/*
 * Completely program an intpin based on the data in its interrupt source
 * structure.
 */
static void
ioapic_program_intpin(struct ioapic_intsrc *intpin)
{
	struct ioapic *io = (struct ioapic *)intpin->io_intsrc.is_pic;
	uint32_t low, high;
#ifdef IOMMU
	int error;
#endif

	/*
	 * If a pin is completely invalid or if it is valid but hasn't
	 * been enabled yet, just ensure that the pin is masked.
	 */
	//mtx_assert(&icu_lock, MA_OWNED);
	if (intpin->io_irq == IRQ_DISABLED || (intpin->io_irq >= 0 &&
	    intpin->io_vector == 0)) {
		low = ioapic_read(io->io_addr,
		    IOAPIC_REDTBL_LO(intpin->io_intpin));
		if ((low & IOART_INTMASK) == IOART_INTMCLR)
			ioapic_write(io->io_addr, IOAPIC_REDTBL_LO(intpin->io_intpin), low | IOART_INTMSET);
#ifdef IOMMU
		ioapic_unlock(&icu_lock);
		iommu_unmap_ioapic_intr(io->io_apic_id, &intpin->io_remap_cookie);
		ioapic_lock(&icu_lock);
#endif
		return;
	}

#ifdef IOMMU
	ioapic_unlock(&icu_lock);
	error = iommu_map_ioapic_intr(io->io_apic_id,
	    intpin->io_cpu, intpin->io_vector, intpin->io_edgetrigger,
	    intpin->io_activehi, intpin->io_irq, &intpin->io_remap_cookie,
	    &high, &low);
	ioapic_lock(&icu_lock);
	if (error == 0) {
		ioapic_write(io->io_addr, IOAPIC_REDTBL_HI(intpin->io_intpin),
		    high);
		intpin->io_lowreg = low;
		ioapic_write(io->io_addr, IOAPIC_REDTBL_LO(intpin->io_intpin),
		    low);
		return;
	} else if (error != EOPNOTSUPP) {
		return;
	}
#endif

	/*
	 * Set the destination.  Note that with Intel interrupt remapping,
	 * the previously reserved bits 55:48 now have a purpose so ensure
	 * these are zero.
	 */
	low = IOART_DESTPHY;
	high = intpin->io_cpu << APIC_ID_SHIFT;

	/* Program the rest of the low word. */
	if (intpin->io_edgetrigger)
		low |= IOART_TRGREDG;
	else
		low |= IOART_TRGRLVL;
	if (intpin->io_activehi)
		low |= IOART_INTAHI;
	else
		low |= IOART_INTALO;
	if (intpin->io_masked)
		low |= IOART_INTMSET;
	switch (intpin->io_irq) {
	case IRQ_EXTINT:
		KASSERT(intpin->io_edgetrigger,
		    ("ExtINT not edge triggered"));
		low |= IOART_DELEXINT;
		break;
	case IRQ_NMI:
		KASSERT(intpin->io_edgetrigger,
		    ("NMI not edge triggered"));
		low |= IOART_DELNMI;
		break;
	case IRQ_SMI:
		KASSERT(intpin->io_edgetrigger,
		    ("SMI not edge triggered"));
		low |= IOART_DELSMI;
		break;
	default:
		KASSERT(intpin->io_vector != 0, ("No vector for IRQ %u",
		    intpin->io_irq));
		low |= IOART_DELFIXED | intpin->io_vector;
	}

	/* Write the values to the APIC. */
	ioapic_write(io->io_addr, IOAPIC_REDTBL_HI(intpin->io_intpin), high);
	intpin->io_lowreg = low;
	ioapic_write(io->io_addr, IOAPIC_REDTBL_LO(intpin->io_intpin), low);
}

static void
ioapic_reprogram_intpin(struct intsrc *isrc)
{
	ioapic_lock(&icu_lock);
	ioapic_program_intpin((struct ioapic_intsrc *)isrc);
	ioapic_unlock(&icu_lock);
}

static int
ioapic_assign_cpu(struct intsrc *isrc, u_int apic_id)
{
	struct ioapic_intsrc *intpin = (struct ioapic_intsrc *)isrc;
	struct ioapic *io = (struct ioapic *)isrc->is_pic;
	u_int old_vector, new_vector;
	u_int old_id;

	/*
	 * keep 1st core as the destination for NMI
	 */
	if (intpin->io_irq == IRQ_NMI)
		apic_id = 0;

	/*
	 * Set us up to free the old irq.
	 */
	old_vector = intpin->io_vector;
	old_id = intpin->io_cpu;
	if (old_vector && apic_id == old_id)
		return (0);

	/*
	 * Allocate an APIC vector for this interrupt pin.  Once
	 * we have a vector we program the interrupt pin.
	 */
	new_vector = apic_alloc_vector(apic_id, intpin->io_irq);
	if (new_vector == 0)
		return (ENOSPC);

	/*
	 * Mask the old intpin if it is enabled while it is migrated.
	 *
	 * At least some level-triggered interrupts seem to need the
	 * extra DELAY() to avoid being stuck in a non-EOI'd state.
	 */
	ioapic_lock(&icu_lock);
	if (!intpin->io_masked && !intpin->io_edgetrigger) {
		ioapic_write(io->io_addr, IOAPIC_REDTBL_LO(intpin->io_intpin), intpin->io_lowreg | IOART_INTMSET);
		ioapic_unlock(&icu_lock);
		DELAY(100);
		ioapic_lock(&icu_lock);
	}

	intpin->io_cpu = apic_id;
	intpin->io_vector = new_vector;
	if (isrc->is_handlers > 0)
		apic_enable_vector(intpin->io_cpu, intpin->io_vector);
	if (bootverbose) {
		printf("ioapic%u: routing intpin %u (", io->io_id, intpin->io_intpin);
		ioapic_print_irq(intpin);
		printf(") to lapic %u vector %u\n", intpin->io_cpu, intpin->io_vector);
	}
	ioapic_program_intpin(intpin);
	ioapic_unlock(&icu_lock);

	/*
	 * Free the old vector after the new one is established.  This is done
	 * to prevent races where we could miss an interrupt.
	 */
	if (old_vector) {
		if (isrc->is_handlers > 0)
			apic_disable_vector(old_id, old_vector);
		apic_free_vector(old_id, old_vector, intpin->io_irq);
	}
	return (0);
}

static void
ioapic_enable_intr(struct intsrc *isrc)
{
	struct ioapic_intsrc *intpin = (struct ioapic_intsrc *)isrc;

	if (intpin->io_vector == 0)
		if (ioapic_assign_cpu(isrc, intr_next_cpu(isrc->is_domain)) != 0)
			panic("Couldn't find an APIC vector for IRQ %d", intpin->io_irq);
	apic_enable_vector(intpin->io_cpu, intpin->io_vector);
}

static void
ioapic_disable_intr(struct intsrc *isrc)
{
	struct ioapic_intsrc *intpin = (struct ioapic_intsrc *)isrc;
	u_int vector;

	if (intpin->io_vector != 0) {
		/* Mask this interrupt pin and free its APIC vector. */
		vector = intpin->io_vector;
		apic_disable_vector(intpin->io_cpu, vector);
		ioapic_lock(&icu_lock);
		intpin->io_masked = 1;
		intpin->io_vector = 0;
		ioapic_program_intpin(intpin);
		ioapic_unlock(&icu_lock);
		apic_free_vector(intpin->io_cpu, vector, intpin->io_irq);
	}
}

static int
ioapic_vector(struct intsrc *isrc)
{
	struct ioapic_intsrc *pin;

	pin = (struct ioapic_intsrc *)isrc;
	return (pin->io_irq);
}

static int
ioapic_source_pending(struct intsrc *isrc)
{
	struct ioapic_intsrc *intpin = (struct ioapic_intsrc *)isrc;

	if (intpin->io_vector == 0)
		return 0;
	return (lapic_intr_pending(intpin->io_vector));
}

static int
ioapic_config_intr(struct intsrc *isrc, enum intr_trigger trig,
    enum intr_polarity pol)
{
	struct ioapic_intsrc *intpin = (struct ioapic_intsrc *)isrc;
	struct ioapic *io = (struct ioapic *)isrc->is_pic;
	int changed;

	KASSERT(!(trig == INTR_TRIGGER_CONFORM || pol == INTR_POLARITY_CONFORM), ("%s: Conforming trigger or polarity\n", __func__));

	/*
	 * EISA interrupts always use active high polarity, so don't allow
	 * them to be set to active low.
	 *
	 * XXX: Should we write to the ELCR if the trigger mode changes for
	 * an EISA IRQ or an ISA IRQ with the ELCR present?
	 */
	ioapic_lock(&icu_lock);
	if (intpin->io_bus == APIC_BUS_EISA)
		pol = INTR_POLARITY_HIGH;
	changed = 0;
	if (intpin->io_edgetrigger != (trig == INTR_TRIGGER_EDGE)) {
		if (bootverbose)
			printf("ioapic%u: Changing trigger for pin %u to %s\n", io->io_id, intpin->io_intpin, trig == INTR_TRIGGER_EDGE ? "edge" : "level");
		intpin->io_edgetrigger = (trig == INTR_TRIGGER_EDGE);
		changed++;
	}
	if (intpin->io_activehi != (pol == INTR_POLARITY_HIGH)) {
		if (bootverbose)
			printf("ioapic%u: Changing polarity for pin %u to %s\n", io->io_id, intpin->io_intpin, pol == INTR_POLARITY_HIGH ? "high" : "low");
		intpin->io_activehi = (pol == INTR_POLARITY_HIGH);
		changed++;
	}
	if (changed)
		ioapic_program_intpin(intpin);
	ioapic_unlock(&icu_lock);
	return (0);
}

static void
ioapic_resume(struct pic *pic, bool suspend_cancelled)
{
	struct ioapic *io = (struct ioapic *)pic;
	int i;

	ioapic_lock(&icu_lock);
	for (i = 0; i < io->io_numintr; i++) {
		ioapic_program_intpin(&io->io_pins[i]);
	}
	ioapic_unlock(&icu_lock);
}

/*
 * Create a plain I/O APIC object.
 */
void *
ioapic_create(caddr_t addr, int32_t apic_id, int intbase)
{
	struct ioapic *io;
	struct ioapic_intsrc *intpin;
	volatile ioapic_t *apic;
	u_int numintr, i;
	uint32_t value;

	/* Map the register window so we can access the device. */
	apic =  pmap_mapdev(addr, IOAPIC_MEM_REGION);
	ioapic_lock(&icu_lock);
	value = ioapic_read(apic, IOAPIC_VER);
	ioapic_unlock(&icu_lock);

	/* If it's version register doesn't seem to work, punt. */
	if (value == 0xffffffff) {
		pmap_unmapdev((vm_offset_t)apic, IOAPIC_MEM_REGION);
		return (NULL);
	}

	/* Determine the number of vectors and set the APIC ID. */
	numintr = ((value & IOART_VER_MAXREDIR) >> MAXREDIRSHIFT) + 1;
	io = malloc(sizeof(struct ioapic) + numintr * sizeof(struct ioapic_intsrc), M_IOAPIC, M_WAITOK);
	io->io_pic = ioapic_template;
	io->io_dev = NULL;
	ioapic_lock(&icu_lock);
	io->io_id = next_id++;
	io->io_hw_apic_id = ioapic_read(apic, IOAPIC_ID) >> APIC_ID_SHIFT;
	io->io_apic_id = apic_id == -1 ? io->io_hw_apic_id : apic_id;
	ioapic_unlock(&icu_lock);
	if (io->io_hw_apic_id != apic_id) {
		printf("ioapic%u: MADT APIC ID %d != hw id %d\n", io->io_id, apic_id, io->io_hw_apic_id);
	}
	if (intbase == -1) {
		intbase = next_ioapic_base;
		printf("ioapic%u: Assuming intbase of %d\n", io->io_id, intbase);
	} else if (intbase != next_ioapic_base && bootverbose) {
		printf("ioapic%u: WARNING: intbase %d != expected base %d\n", io->io_id, intbase, next_ioapic_base);
	}
	io->io_intbase = intbase;
	next_ioapic_base = intbase + numintr;
	if (next_ioapic_base > num_io_irqs) {
		num_io_irqs = next_ioapic_base;
	}
	io->io_numintr = numintr;
	io->io_addr = apic;
	io->io_paddr = addr;

	if (bootverbose) {
		printf("ioapic%u: ver 0x%02x maxredir 0x%02x\n", io->io_id, (value & IOART_VER_VERSION), (value & IOART_VER_MAXREDIR) >> MAXREDIRSHIFT);
	}
	/*
	 * The  summary information about IO-APIC versions is taken from
	 * the Linux kernel source:
	 *     0Xh     82489DX
	 *     1Xh     I/OAPIC or I/O(x)APIC which are not PCI 2.2 Compliant
	 *     2Xh     I/O(x)APIC which is PCI 2.2 Compliant
	 *     30h-FFh Reserved
	 * IO-APICs with version >= 0x20 have working EOIR register.
	 */
	io->io_haseoi = (value & IOART_VER_VERSION) >= 0x20;

	/*
	 * Initialize pins.  Start off with interrupts disabled.  Default
	 * to active-hi and edge-triggered for ISA interrupts and active-lo
	 * and level-triggered for all others.
	 */
	bzero(io->io_pins, sizeof(struct ioapic_intsrc) * numintr);
	ioapic_lock(&icu_lock);
	for (i = 0, intpin = io->io_pins; i < numintr; i++, intpin++) {
		intpin->io_intsrc.is_pic = (struct pic *)io;
		intpin->io_intpin = i;
		intpin->io_irq = intbase + i;

		/*
		 * Assume that pin 0 on the first I/O APIC is an ExtINT pin.
		 * Assume that pins 1-15 are ISA interrupts and that all
		 * other pins are PCI interrupts.
		 */
		if (intpin->io_irq == 0) {
			ioapic_set_extint(io, i);
		} else if (intpin->io_irq < IOAPIC_ISA_INTS) {
			intpin->io_bus = APIC_BUS_ISA;
			intpin->io_activehi = 1;
			intpin->io_edgetrigger = 1;
			intpin->io_masked = 1;
		} else {
			intpin->io_bus = APIC_BUS_PCI;
			intpin->io_activehi = 0;
			intpin->io_edgetrigger = 0;
			intpin->io_masked = 1;
		}

		/*
		 * Route interrupts to the BSP by default.  Interrupts may
		 * be routed to other CPUs later after they are enabled.
		 */
		intpin->io_cpu = PERCPU_GET(apic_id);
		value = ioapic_read(apic, IOAPIC_REDTBL_LO(i));
		ioapic_write(apic, IOAPIC_REDTBL_LO(i), value | IOART_INTMSET);
#ifdef IOMMU
		/* dummy, but sets cookie */
		ioapic_unlock(&icu_lock);
		iommu_map_ioapic_intr(io->io_apic_id,
		    intpin->io_cpu, intpin->io_vector, intpin->io_edgetrigger,
		    intpin->io_activehi, intpin->io_irq,
		    &intpin->io_remap_cookie, NULL, NULL);
		ioapic_lock(&icu_lock);
#endif
	}
	ioapic_unlock(&icu_lock);

	return (io);
}

int
ioapic_get_vector(void *cookie, u_int pin)
{
	struct ioapic *io;

	io = (struct ioapic *)cookie;
	if (pin >= io->io_numintr)
		return (-1);
	return (io->io_pins[pin].io_irq);
}

int
ioapic_disable_pin(void *cookie, u_int pin)
{
	struct ioapic *io;

	io = (struct ioapic *)cookie;
	if (pin >= io->io_numintr)
		return (EINVAL);
	if (io->io_pins[pin].io_irq == IRQ_DISABLED)
		return (EINVAL);
	io->io_pins[pin].io_irq = IRQ_DISABLED;
	if (bootverbose)
		printf("ioapic%u: intpin %d disabled\n", io->io_id, pin);
	return (0);
}

int
ioapic_remap_vector(void *cookie, u_int pin, int vector)
{
	struct ioapic *io;

	io = (struct ioapic *)cookie;
	if (pin >= io->io_numintr || vector < 0)
		return (EINVAL);
	if (io->io_pins[pin].io_irq < 0)
		return (EINVAL);
	io->io_pins[pin].io_irq = vector;
	if (bootverbose)
		printf("ioapic%u: Routing IRQ %d -> intpin %d\n", io->io_id, vector, pin);
	return (0);
}

int
ioapic_set_bus(void *cookie, u_int pin, int bus_type)
{
	struct ioapic *io;

	if (bus_type < 0 || bus_type > APIC_BUS_MAX)
		return (EINVAL);
	io = (struct ioapic *)cookie;
	if (pin >= io->io_numintr)
		return (EINVAL);
	if (io->io_pins[pin].io_irq < 0)
		return (EINVAL);
	if (io->io_pins[pin].io_bus == bus_type)
		return (0);
	io->io_pins[pin].io_bus = bus_type;
	if (bootverbose)
		printf("ioapic%u: intpin %d bus %s\n", io->io_id, pin, ioapic_bus_string(bus_type));
	return (0);
}

int
ioapic_set_nmi(void *cookie, u_int pin)
{
	struct ioapic *io;

	io = (struct ioapic *)cookie;
	if (pin >= io->io_numintr)
		return (EINVAL);
	if (io->io_pins[pin].io_irq == IRQ_NMI)
		return (0);
	if (io->io_pins[pin].io_irq < 0)
		return (EINVAL);
	io->io_pins[pin].io_bus = APIC_BUS_UNKNOWN;
	io->io_pins[pin].io_irq = IRQ_NMI;
	io->io_pins[pin].io_masked = 0;
	io->io_pins[pin].io_edgetrigger = 1;
	io->io_pins[pin].io_activehi = 1;
	if (bootverbose)
		printf("ioapic%u: Routing NMI -> intpin %d\n", io->io_id, pin);
	return (0);
}

int
ioapic_set_smi(void *cookie, u_int pin)
{
	struct ioapic *io;

	io = (struct ioapic *)cookie;
	if (pin >= io->io_numintr)
		return (EINVAL);
	if (io->io_pins[pin].io_irq == IRQ_SMI)
		return (0);
	if (io->io_pins[pin].io_irq < 0)
		return (EINVAL);
	io->io_pins[pin].io_bus = APIC_BUS_UNKNOWN;
	io->io_pins[pin].io_irq = IRQ_SMI;
	io->io_pins[pin].io_masked = 0;
	io->io_pins[pin].io_edgetrigger = 1;
	io->io_pins[pin].io_activehi = 1;
	if (bootverbose)
		printf("ioapic%u: Routing SMI -> intpin %d\n", io->io_id, pin);
	return (0);
}

int
ioapic_set_extint(void *cookie, u_int pin)
{
	struct ioapic *io;

	io = (struct ioapic *)cookie;
	if (pin >= io->io_numintr)
		return (EINVAL);
	if (io->io_pins[pin].io_irq == IRQ_EXTINT)
		return (0);
	if (io->io_pins[pin].io_irq < 0)
		return (EINVAL);
	io->io_pins[pin].io_bus = APIC_BUS_UNKNOWN;
	io->io_pins[pin].io_irq = IRQ_EXTINT;
	if (enable_extint)
		io->io_pins[pin].io_masked = 0;
	else
		io->io_pins[pin].io_masked = 1;
	io->io_pins[pin].io_edgetrigger = 1;
	io->io_pins[pin].io_activehi = 1;
	if (bootverbose)
		printf("ioapic%u: Routing external 8259A's -> intpin %d\n", io->io_id, pin);
	return (0);
}

int
ioapic_set_polarity(void *cookie, u_int pin, enum intr_polarity pol)
{
	struct ioapic *io;
	int activehi;

	io = (struct ioapic *)cookie;
	if (pin >= io->io_numintr || pol == INTR_POLARITY_CONFORM)
		return (EINVAL);
	if (io->io_pins[pin].io_irq < 0)
		return (EINVAL);
	activehi = (pol == INTR_POLARITY_HIGH);
	if (io->io_pins[pin].io_activehi == activehi)
		return (0);
	io->io_pins[pin].io_activehi = activehi;
	if (bootverbose)
		printf("ioapic%u: intpin %d polarity: %s\n", io->io_id, pin, pol == INTR_POLARITY_HIGH ? "high" : "low");
	return (0);
}

int
ioapic_set_triggermode(void *cookie, u_int pin, enum intr_trigger trigger)
{
	struct ioapic *io;
	int edgetrigger;

	io = (struct ioapic *)cookie;
	if (pin >= io->io_numintr || trigger == INTR_TRIGGER_CONFORM)
		return (EINVAL);
	if (io->io_pins[pin].io_irq < 0)
		return (EINVAL);
	edgetrigger = (trigger == INTR_TRIGGER_EDGE);
	if (io->io_pins[pin].io_edgetrigger == edgetrigger)
		return (0);
	io->io_pins[pin].io_edgetrigger = edgetrigger;
	if (bootverbose)
		printf("ioapic%u: intpin %d trigger: %s\n", io->io_id, pin, trigger == INTR_TRIGGER_EDGE ? "edge" : "level");
	return (0);
}

/*
 * Register a complete I/O APIC object with the interrupt subsystem.
 */
void
ioapic_register(void *cookie)
{
	struct ioapic_intsrc *pin;
	struct ioapic *io;
	volatile ioapic_t *apic;
	uint32_t flags;
	int i;

	io = (struct ioapic *)cookie;
	apic = io->io_addr;
	ioapic_lock(&icu_lock);
	flags = ioapic_read(apic, IOAPIC_VER) & IOART_VER_VERSION;
	SIMPLEQ_INSERT_TAIL(&ioapic_list, io, io_next);
	ioapic_unlock(&icu_lock);
	printf("ioapic%u <Version %u.%u> irqs %u-%u\n", io->io_id, flags >> 4, flags & 0xf, io->io_intbase, io->io_intbase + io->io_numintr - 1);

	/*
	 * Reprogram pins to handle special case pins (such as NMI and
	 * SMI) and disable normal pins until a handler is registered.
	 */
	intr_register_pic(&io->io_pic);
	for (i = 0, pin = io->io_pins; i < io->io_numintr; i++, pin++) {
		ioapic_reprogram_intpin(&pin->io_intsrc);
	}
}

/*
 * Add interrupt sources for I/O APIC interrupt pins.
 */
static void
ioapic_register_sources(struct pic *pic)
{
	struct ioapic_intsrc *pin;
	struct ioapic *io;
	int i;

	io = (struct ioapic*) pic;
	for (i = 0, pin = io->io_pins; i < io->io_numintr; i++, pin++) {
		if (pin->io_irq >= 0) {
			intr_register_source(&pin->io_intsrc);
		}
	}
}

typedef struct INTENTRY {
	uint8_t	type;
	uint8_t	int_type;
	uint16_t int_flags;
	uint8_t	src_bus_id;
	uint8_t	src_bus_irq;
	uint8_t	dst_apic_id;
	uint8_t	dst_apic_int;
} *int_entry_ptr;

static void
mptable_parse_io_int(int_entry_ptr intr)
{
	void *ioapic;
	u_int pin, apic_id;

	apic_id = intr->dst_apic_id;
	ioapic = ioapics[apic_id];
	pin = intr->dst_apic_int;
	switch (intr->int_type) {
	case INTENTRY_TYPE_INT:
		switch (busses[intr->src_bus_id].bus_type) {
		case NOBUS:
			panic("interrupt from missing bus");
		case ISA:
		case EISA:
			if (busses[intr->src_bus_id].bus_type == ISA)
				ioapic_set_bus(ioapic, pin, APIC_BUS_ISA);
			else
				ioapic_set_bus(ioapic, pin, APIC_BUS_EISA);
			if (intr->src_bus_irq == pin)
				break;
			ioapic_remap_vector(ioapic, pin, intr->src_bus_irq);
			if (ioapic_get_vector(ioapic, intr->src_bus_irq) == intr->src_bus_irq) {
				ioapic_disable_pin(ioapic, intr->src_bus_irq);
			}
			break;
		case PCI:
			ioapic_set_bus(ioapic, pin, APIC_BUS_PCI);
			break;
		default:
			ioapic_set_bus(ioapic, pin, APIC_BUS_UNKNOWN);
			break;
		}
		break;
	case INTENTRY_TYPE_NMI:
		ioapic_set_nmi(ioapic, pin);
		break;
	case INTENTRY_TYPE_SMI:
		ioapic_set_smi(ioapic, pin);
		break;
	case INTENTRY_TYPE_EXTINT:
		ioapic_set_extint(ioapic, pin);
		break;

	}
	if (intr->int_type == INTENTRY_TYPE_INT || (intr->int_flags & INTENTRY_FLAGS_TRIGGER) != INTENTRY_FLAGS_TRIGGER_CONFORM)
		ioapic_set_triggermode(ioapic, pin, intentry_trigger(intr));
	if (intr->int_type == INTENTRY_TYPE_INT || (intr->int_flags & INTENTRY_FLAGS_POLARITY) !=  INTENTRY_FLAGS_POLARITY_CONFORM)
		ioapic_set_polarity(ioapic, pin, intentry_polarity(intr));
}
