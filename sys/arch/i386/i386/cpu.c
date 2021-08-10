/*	$NetBSD: cpu.c,v 1.16 2019/10/01 18:00:07 chs Exp $	*/

/*
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 *
 * Author:
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
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/user.h>

#include <machine/cpu.h>

struct cpu_softc {
	struct device 		*sc_dev;		/* device tree glue */
	struct cpu_info 	*sc_info;		/* pointer to CPU info */
};

static int cpu_match(struct device *, struct cfdata *, void *);
static void cpu_attach(struct device *, struct device *, void *);

CFDRIVER_DECL(NULL, cpu, &cpu_cops, DV_DULL, sizeof(struct cpu_softc));
CFOPS_DECL(cpu, cpu_match, cpu_attach, NULL, NULL);

void
cpu_init_first()
{
	int cpunum = lapic_cpu_number();

	if (cpunum != 0) {
		cpu_info[0] = NULL;
		cpu_info[cpunum] = &cpu_info;
	}
}

static int
cpu_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct cpu_attach_args *caa = aux;
	if (strcmp(caa->caa_name, match->cf_driver->cd_name) != 0) {
		return (0);
	}
	return (1);
}

static void
cpu_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct cpu_softc *sc = (struct cpu_softc *)self;
	struct cpu_attach_args *caa = (struct cpu_attach_args *)aux;
	struct cpu_info *ci;
#ifdef SMP
	int cpunum = caa->cpu_apic_id;
#endif
	caa->cpu_acpi_id = 0xffffffff;

	if (caa->cpu_role == CPU_ROLE_AP) {
		cpu_alloc(ci);
		memset(ci, 0, sizeof(*ci));
#ifdef SMP
		if (cpu_info[cpunum] != NULL) {
			panic("cpu at apic id %d already attached?", cpunum);
		}
		cpu_info[cpunum] = ci;
#endif
	} else {
		ci = &cpu_info;
#ifdef SMP
		if(cpunum != lapic_cpu_number()) {
			panic("%s: running CPU is at apic %d instead of at expected %d", sc->sc_dev->dv_xname, lapic_cpu_number(), cpunum);
		}
#endif
	}

	sc->sc_dev = self;

	ci->cpu_self = ci;
	sc->sc_info = ci;
	ci->cpu_dev = self;
	ci->cpu_apic_id = caa->cpu_apic_id;
	//ci->cpu_acpi_id = caa->cpu_acpi_id;
#ifdef SMP
	cpu_init(ci, caa->cpu_apic_id, sizeof(struct cpu_info));
#else
	cpu_init(ci, 0, sizeof(struct cpu_info));
#endif

	printf(": ");
	switch (caa->cpu_role) {
	case CPU_ROLE_SP:
		printf("(uniprocessor)\n");
		ci->cpu_flags |= CPUF_PRESENT | CPUF_SP | CPUF_PRIMARY;

		break;
	case CPU_ROLE_BP:
		printf("apid %d (boot processor)\n", caa->cpu_apic_id);
		ci->cpu_flags |= CPUF_PRESENT | CPUF_BSP | CPUF_PRIMARY;
#if NLAPIC > 0
		/*
		 * Enable local apic
		 */
		lapic_enable();
		lapic_calibrate_timer(ci);
#endif
#if NIOAPIC > 0
		ioapic_bsp_id = caa->cpu_apic_id;
#endif
		break;
	case CPU_ROLE_AP:
		/*
		 * report on an AP
		 */
		printf("apid %d (application processor)\n", caa->cpu_apic_id);
#ifdef SMP
		cpu_smp_init(ci);
#else
		printf("%s: not started\n", sc->sc_dev->dv_xname);
#endif
		break;
	default:
		panic("unknown processor type??\n");
	}
	return;
}

void
cpu_init(ci, cpuid, size)
	struct cpu_info *ci;
	int cpuid;
	size_t size;
{
	ci->cpu_cpuid = cpuid;
	ci->cpu_cpumask = 1 << cpuid;
	ci->cpu_size = size;
}

void
cpu_hatch(void *v)
{
	struct cpu_info *ci = (struct cpu_info *)v;
	int s;

#if NLAPIC > 0
	lapic_enable();
	lapic_set_lvt();
	lapic_write_tpri(0);
#endif
}

#ifdef SMP
u_int all_cpus;
int mp_ncpus;
/* export this for libkvm consumers. */
int mp_maxcpus = NCPUS;

int smp_disabled = 0;			/* has smp been disabled? */
int smp_cpus = 1;				/* how many cpu's running */
int smp_threads_per_core = 1;	/* how many SMT threads are running per core */
int mp_ncores = -1;				/* how many physical cores running */
int smp_topology = 0;			/* Which topology we're using. */

void
cpu_smp_init(ci)
	struct cpu_info *ci;
{
	//init_secondary(ci);
	ci->cpu_flags |= CPUF_PRESENT | CPUF_AP;

	cpu_mp_setmaxid();
	KASSERT(mp_ncpus >= 1 ("%s: CPU count < 1", __func__));
	KASSERT(mp_ncpus > 1 || mp_maxid == 0 ("%s: one CPU but mp_maxid is not zero", __func__));
	KASSERT(mp_maxid >= mp_ncpus - 1 ("%s: counters out of sync: max %d, count %d", __func__, mp_maxid, mp_ncpus));

	if (smp_disabled != 0 || cpu_mp_probe() == 0) {
		mp_ncores = 1;
		mp_ncpus = 1;
		all_cpus = PERCPU_GET(ci->cpu_percpu, cpumask);
		return;
	}

	cpu_mp_start(ci->cpu_percpu);
	printf("211BSD/SMP: Multiprocessor System Detected: %d CPUs\n", mp_ncpus);

	if (mp_ncores < 0) {
		mp_ncores = mp_ncpus;
	}

	cpu_mp_announce();
}
#endif
