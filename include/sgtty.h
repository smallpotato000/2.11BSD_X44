/*	sgtty.h	4.2	85/01/03	*/

#ifndef	_SGTTY_H_
#define	_SGTTY_H_

#ifndef	_IOCTL_
#include <sys/ioctl.h>
#endif

__BEGIN_DECLS
int gtty (int, struct sgttyb *);
int stty (int, struct sgttyb *);
__END_DECLS

#endif	/*! _SGTTY_H_ */
