/*
 * imgact_coff.h
 *
 *  Created on: 26 Nov 2019
 *      Author: marti
 */

#ifndef _IMGACT_COFF_H_
#define _IMGACT_COFF_H_

/*
 * COFF file header
 */

struct coff_filehdr {
	u_short	f_magic;	/* magic number */
	u_short	f_nscns;	/* # of sections */
	long	f_timdat;	/* timestamp */
	long	f_symptr;	/* file offset of symbol table */
	long	f_nsyms;	/* # of symbol table entries */
	u_short	f_opthdr;	/* size of optional header */
	u_short	f_flags;	/* flags */
};

/* f_flags */
#define COFF_F_RELFLG	0x1
#define COFF_F_EXEC		0x2
#define COFF_F_LNNO		0x4
#define COFF_F_LSYMS	0x8
#define COFF_F_SWABD	0x40
#define COFF_F_AR16WR	0x80
#define COFF_F_AR32WR	0x100
#define COFF_F_AR32W	0x200

/*
 * COFF system header
 */

struct coff_aouthdr {
	short	a_magic;
	short	a_vstamp;
	long	a_tsize;
	long	a_dsize;
	long	a_bsize;
	long	a_entry;
	long	a_tstart;
	long	a_dstart;
};

/*
 * COFF section header
 */

struct coff_scnhdr {
	char	s_name[8];
	long	s_paddr;
	long	s_vaddr;
	long	s_size;
	long	s_scnptr;
	long	s_relptr;
	long	s_lnnoptr;
	u_short	s_nreloc;
	u_short	s_nlnno;
	long	s_flags;
};

/*
 * COFF shared library header
 */

struct coff_slhdr {
	long	entry_len;	/* in words */
	long	path_index;	/* in words */
	char	sl_name[1];
};

struct coff_exechdr {
	struct coff_filehdr f;
	struct coff_aouthdr a;
};

#define COFF_ROUND(val, by)     (((val) + by - 1) & ~(by - 1))

#define COFF_ALIGN(a) ((a) & ~(COFF_LDPGSZ - 1))

#define COFF_HDR_SIZE \
	(sizeof(struct coff_filehdr) + sizeof(struct coff_aouthdr))

#define COFF_BLOCK_ALIGN(ap, value) \
        ((ap)->a_magic == COFF_ZMAGIC ? COFF_ROUND(value, COFF_LDPGSZ) : \
         value)

#define COFF_TXTOFF(fp, ap) \
        ((ap)->a_magic == COFF_ZMAGIC ? 0 : \
         COFF_ROUND(COFF_HDR_SIZE + (fp)->f_nscns * \
		    sizeof(struct coff_scnhdr), \
		    COFF_SEGMENT_ALIGNMENT(fp, ap)))

#define COFF_DATOFF(fp, ap) \
        (COFF_BLOCK_ALIGN(ap, COFF_TXTOFF(fp, ap) + (ap)->a_tsize))

#define COFF_SEGMENT_ALIGN(fp, ap, value) \
        (COFF_ROUND(value, ((ap)->a_magic == COFF_ZMAGIC ? COFF_LDPGSZ : \
         COFF_SEGMENT_ALIGNMENT(fp, ap))))

#endif /* _IMGACT_COFF_H_ */
