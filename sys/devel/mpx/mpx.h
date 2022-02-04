/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#ifndef SYS_MPX_H_
#define SYS_MPX_H_

#include <sys/lock.h>
#include <sys/queue.h>

#define	NINDEX		15 	/* comes from ufs inode */
#define	NGROUPS		10	/* number of group structures */
#define	NCHANS		20	/* number of channel structures */
#define	NLEVELS		4

/* mpx reader/buffer */
struct mpx_reader {
    struct mpx_chan 		*mpr_chan;      /* channel */
    int 				    mpr_state;      /* state */
    size_t		            mpr_size;		/* size of buffer */
    caddr_t		            mpr_buffer;		/* kva of buffer */
};

/* mpx writer/buffer */
struct mpx_writer {
    struct mpx_chan         *mpw_chan;      /* channel */
    int 				    mpw_state;      /* state */
    size_t		            mpw_size;		/* size of buffer */
    caddr_t		            mpw_buffer;		/* kva of buffer */
};

struct chanlist;
LIST_HEAD(chanlist, mpx_chan);
struct mpx_chan {
    LIST_ENTRY(mpx_chan)    mpc_node;       /* channel node in list */
    struct mpx_group	    *mpc_group;     /* this channels group */
    int 	                mpc_index;      /* channel index */
    int 			        mpc_flags;      /* channel flags */
};

struct grplist;
LIST_HEAD(grplist, mpx_group);
struct mpx_group {
    LIST_ENTRY(mpx_group)    mpg_node;     /* group node in list */
    struct mpx_chan          *mpg_chan;    /* channels in this group */
    int 					 mpg_index;    /* group index */
};

union mpxpair {
    struct mpx_writer       mpp_wmpx;
    struct mpx_reader       mpp_rmpx;
    struct lock_object		mpp_lock;
};

struct mpx {
    union mpxpair			*mpx_pair;
    struct mpx_chan 	    *mpx_chan;
    struct file				*mpx_file;
};

extern struct grplist       mpx_grps[];
extern struct chanlist      mpx_chans[];

#define MPX_LOCK(mpx)		simple_lock((mpx)->mpp_lock)
#define MPX_UNLOCK(mpx)		simple_unlock((mpx)->mpp_lock)

void                		mpx_init(void);
void                		mpx_create_group(struct mpx_group *, int);
struct mpx_group    		*mpx_get_group(int);
void                		mpx_create_channel(struct mpx_chan *, int, int);
struct mpx_chan     		*mpx_get_channel(int);

#endif /* SYS_MPX_H_ */
