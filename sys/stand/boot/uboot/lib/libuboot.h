/*-
 * Copyright (C) 2000 Benno Rice.
 * Copyright (C) 2007 Semihalf, Rafal Jaworowski <raj@semihalf.com>
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
 *
 * $FreeBSD$
 */

#include <lib/libsa/stand.h>
//#include <readin.h>

#define	D_SLICENONE		-1
#define	D_SLICEWILD		 0
#define	D_PARTNONE		-1
#define	D_PARTWILD		-2
#define	D_PARTISGPT		255

struct uboot_devdesc {
	struct devdesc      dd;
	union {
		struct {
			int			unit;
			int			adaptor;
			int			controller;
			int			slice;
			int			partition;
			uint64_t	offset;
		} d_stor;
	} d_kind;
};

/*
 * Default network packet alignment in memory.  On arm arches packets must be
 * aligned to cacheline boundaries.
 */
#if defined(__aarch64__)
#define	PKTALIGN	128
#elif defined(__arm__)
#define	PKTALIGN	64
#else
#define	PKTALIGN	32
#endif

int 		uboot_getdev(void **vdev, const char *devspec, const char **path);
char 		*uboot_fmtdev(void *vdev);
int 		uboot_setcurrdev(struct env_var *ev, int flags, const void *value);

extern int devs_no;
extern struct netif_driver uboot_net;
extern struct devsw uboot_storage;

extern uintptr_t uboot_heap_start;
extern uintptr_t uboot_heap_end;

uint64_t 	uboot_loadaddr(u_int type, void *data, uint64_t addr);
ssize_t		uboot_copyin(const void *src, vm_offset_t dest, const size_t len);
ssize_t		uboot_copyout(const vm_offset_t src, void *dest, const size_t len);
//ssize_t		uboot_readin(readin_handle_t fd, vm_offset_t dest, const size_t len);
extern int 	uboot_autoload(void);

struct preloaded_file;
struct file_format;

extern struct file_format uboot_elf;

void reboot(void);

int uboot_diskgetunit(int type, int type_unit);

struct ubcommand_table {
	int 	(*command_heap)(int argc, char *argv[]);
    int 	(*command_reboot)(int argc, char *argv[]);
    int 	(*command_devinfo)(int argc, char *argv[]);
    int 	(*command_sysinfo)(int argc, char *argv[]);
    int 	(*command_ubenv)(int argc, char *argv[]);
    int 	(*command_fdt)(int argc, char *argv[]);
};
extern struct ubcommand_table ubcmds;

#define COMMON_UBCOMMANDS																\
		{ "heap", "heap", "show heap usage", command_ubheap }							\
		{ "reboot", "reboot", "reboot the system", command_ubreboot },					\
		{ "devinfo", "devinfo", "show U-Boot devices", command_devinfo },				\
		{ "sysinfo", "sysinfo", "show U-Boot system info", command_sysinfo },			\
		{ "ubenv", "ubenv", "show or import U-Boot env vars", command_ubenv },			\
		{ "fdt", "fdt", "flattened device tree handling", command_fdt },				\

void ubcmds_init(void);
