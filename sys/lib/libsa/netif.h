/*	$NetBSD: netif.h,v 1.5 2003/03/12 14:49:19 drochner Exp $	*/

#ifndef __SYS_LIBNETBOOT_NETIF_H
#define __SYS_LIBNETBOOT_NETIF_H

#include "iodesc.h"

struct netif; /* forward */

struct netif_driver {
	char	*netif_bname;
	int		(*netif_match)(struct netif *, void *);
	int		(*netif_probe)(struct netif *, void *);
	void	(*netif_init)(struct iodesc *, void *);
	int		(*netif_get)(struct iodesc *, void *, size_t, time_t);
	int		(*netif_put)(struct iodesc *, void *, size_t);
	void	(*netif_end)(struct netif *);
	struct	netif_dif *netif_ifs;
	int		netif_nifs;
};

struct netif_dif {
	int					dif_unit;
	int					dif_nsel;
	struct netif_stats 	*dif_stats;
	void				*dif_private;
	/* the following fields are used internally by the netif layer */
	u_long				dif_used;
};

struct netif_stats {
	int	collisions;
	int	collision_error;
	int	missed;
	int	sent;
	int	received;
	int	deferred;
	int	overflow;
};

struct netif {
	struct netif_driver	*nif_driver;
	int					nif_unit;
	int					nif_sel;
	void				*nif_devdata;
};

extern struct netif_driver	*netif_drivers[];	/* machdep */
extern int			n_netif_drivers;

extern int			netif_debug;

void			netif_init(void);
struct netif	*netif_select(void *);
int				netif_probe(struct netif *, void *);
void			netif_attach(struct netif *, struct iodesc *, void *);
void			netif_detach(struct netif *);

int				netif_open(void *);
int				netif_close(int);

#endif /* __SYS_LIBNETBOOT_NETIF_H */
