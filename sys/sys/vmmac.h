/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)vmmac.h	1.1 (2.10 Berkeley) 12/1/86
 */

#ifndef _SYS_VMMAC_H_
#define _SYS_VMMAC_H_

/* Average new into old with aging factor time */
#define	ave(smooth, cnt, time) \
	smooth = ((time - 1) * (smooth) + (cnt)) / (time)

#endif /* _SYS_VMMAC_H_ */
