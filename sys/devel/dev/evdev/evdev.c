/*-
 * Copyright (c) 2014 Jakub Wojciech Klama <jceel@FreeBSD.org>
 * Copyright (c) 2015-2016 Vladimir Kondratyev <wulf@FreeBSD.org>
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

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/devsw.h>
#include <sys/device.h>
#include <sys/fcntl.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/poll.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/stddef.h>
#include <sys/uio.h>

#include <dev/misc/wscons/wsconsio.h>
#include <dev/misc/wscons/wseventvar.h>

#include <dev/evdev/evdev.h>
#include <dev/evdev/evdev_private.h>
#include <dev/evdev/input.h>
#include "freebsd-bitstring.h"

#ifdef EVDEV_DEBUG
#define	debugf(client, fmt, args...)	printf("evdev: "fmt"\n", ##args)
#else
#define	debugf(client, fmt, args...)
#endif

CFDRIVER_DECL(NULL, evdev, &evdev_cops, DV_DULL, sizeof(struct evdev_softc));
CFOPS_DECL(evdev, evdev_match, evdev_attach, NULL, NULL);

#define	DEF_RING_REPORTS	8

dev_type_read(evdev_read);
dev_type_write(evdev_write);
dev_type_ioctl(evdev_ioctl);
dev_type_poll(evdev_poll);
dev_type_kqfilter(evdev_kqfilter);

//extern struct cdevsw evdev_cdevsw;
const struct cdevsw evdev_cdevsw = {
	.d_open = noopen,
	.d_close = noclose,
	.d_read = evdev_read,
	.d_write = evdev_write,
	.d_ioctl = evdev_ioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = evdev_poll,
	.d_mmap = nommap,
	.d_kqfilter = evdev_kqfilter,
	.d_discard = nodiscard,
	.d_type = D_OTHER
};

size_t
evdev_buffer_size(evdev)
	struct evdev_dev *evdev;
{
	size_t buffer_size = evdev->ev_report_size * DEF_RING_REPORTS;

	return (buffer_size);
}

int
evdev_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	return (1);
}

void
evdev_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct evdev_softc *sc = (struct evdev_softc *)self;
	struct evdev_dev *evdev = (struct evdev_dev *)aux;
	struct evdev_client *client;
	size_t buffer_size;

	/* Initialize internal structures */
	ret = evdev_register(evdev);
	if (ret != 0) {
		printf("evdev_attach: evdev_register error", ret);
	}

	/* Initialize ring buffer */
	buffer_size = evdev_buffer_size(evdev);
	client = evdev_client_alloc(evdev, buffer_size);

	client->ec_buffer_size = buffer_size;
	client->ec_buffer_ready = 0;

	client->ec_evdev = evdev;
	lockinit(&client->ec_buffer_lock, "evclient", 0, LK_CANRECURSE);
}

int
evdev_open(evdev, p)
	struct evdev_dev 	*evdev;
	struct proc 		*p;
{
	struct evdev_client *client;
	struct wseventvar 	*evar;
	size_t buffer_size;
	int ret;

	client = evdev->ev_client;

	if (client == NULL) {
		client = evdev_client_alloc(evdev, (evdev->ev_report_size * DEF_RING_REPORTS));
	}

	EVDEV_LOCK(evdev);
	evar = evdev_register_wsevent(client, p);

	/* check event buffer */
	if(buffer_size > wsevent_avail(evar)) {
		return (EINVAL);
	}
	ret = evdev_register_client(evdev, client);

	if(ret != 0) {
		evdev_revoke_client(client);
		evdev_unregister_wsevent(client);
	}

	EVDEV_UNLOCK(evdev);

	return (ret);
}

int
evdev_close(evdev, p)
	struct evdev_dev 	*evdev;
	struct proc 		*p;
{
	struct evdev_client *client;

	client = evdev->ev_client;

	EVDEV_LOCK(evdev);
	if (client != NULL) {
		evdev_revoke_client(client);
		evdev_unregister_wsevent(client);
		/* free client */
		evdev_client_free(client);
		evdev->ev_client = client;
	}
	EVDEV_UNLOCK(evdev);
	return (0);
}

int
evdev_read(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{
	struct evdev_softc *sc;
	struct evdev_dev 	*evdev;
	struct evdev_client *client;
	struct wseventvar 	*evar;
	struct wscons_event *we;
	int unit, error, remaining;

	unit = minor(dev);
	sc = (struct evdev_softc *)evdev_cd.cd_devs[unit];

	if(sc == NULL) {
		return (ENXIO);
	}

	evdev = sc->sc_evdev;
	client = evdev->ev_client;
	evar = client->ec_base.me_evp;

	if (sc->sc_dying) {
		return (EIO);
	}
	if (evar->revoked)
		return (ENODEV);

	/* Zero-sized reads are allowed for error checking */
	if (uio->uio_resid != 0 && uio->uio_resid < sizeof(struct input_event)) {
		return (EINVAL);
	}

	remaining = uio->uio_resid / sizeof(struct input_event);

	EVDEV_CLIENT_LOCKQ(client);
	if (EVDEV_CLIENT_EMPTYQ(client)) {
		if (flags & O_NONBLOCK) {
			error = EWOULDBLOCK;
		} else {
			if (remaining != 0) {
				evar->blocked = TRUE;
				tsleep(client, &client->ec_buffer_lock, PCATCH, "evread", 0);
				if (error == 0 && evar->revoked) {
					error = ENODEV;
				}
			}
		}
	}
	while (error == 0 && !EVDEV_CLIENT_EMPTYQ(client) && remaining > 0) {
		memcpy(&we, &client->ec_buffer[evar->put], sizeof(struct input_event));
		wsevent_put(evar, 1);
		remaining--;

		EVDEV_CLIENT_UNLOCKQ(client);
		sc->sc_refcnt++;
		error = wsevent_read(evar, uio, flags);
		if (--sc->sc_refcnt < 0) {
			wakeup(sc);
			error = EIO;
		}
		EVDEV_CLIENT_LOCKQ(client);
	}
	EVDEV_CLIENT_UNLOCKQ(client);
	return (error);
}

int
evdev_write(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{
	struct evdev_softc *sc;
	struct evdev_dev *evdev;
	struct evdev_client *client;
	struct wseventvar 	*evar;
	struct input_event 	event;
	int unit, error, remaining;

	unit = minor(dev);
	sc = (struct evdev_softc*) evdev_cd.cd_devs[unit];

	if (sc == NULL) {
		return (ENXIO);
	}

	evdev = sc->sc_evdev;
	client = evdev->ev_client;
	evar = client->ec_base.me_evp;

	if (evar->revoked || evdev == NULL) {
		return (ENODEV);
	}
	if (uio->uio_resid % sizeof(struct input_event) != 0) {
		debugf(sc, "write size not multiple of input_event size");
		return (EINVAL);
	}

	while (uio->uio_resid > 0 && ret == 0) {
		error = uiomove(&event, sizeof(struct input_event), uio);
		if (error == 0) {
			error = evdev_inject_event(evdev, event.type, event.code, event.value);
			if (error == 0) {
				error = evdev_wsevent_inject(evar, &event, 1);
			}
		}
	}
	return (error);
}

int
evdev_poll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	struct evdev_softc *sc;
	struct evdev_dev *evdev;
	struct evdev_client *client;
	int unit, error;

	unit = minor(dev);
	sc = (struct evdev_softc*) evdev_cd.cd_devs[unit];

	if (sc == NULL) {
		return (ENXIO);
	}
	evdev = sc->sc_evdev;
	client = evdev->ev_client;

	if (client->ec_base.me_evp == NULL)
		return (EINVAL);

	EVDEV_CLIENT_LOCKQ(client);
	if(!EVDEV_CLIENT_EMPTYQ(client)) {
		error = wsevent_poll(client->ec_base.me_evp, events & (POLLIN | POLLRDNORM), p);
	} else {
		error = wsevent_poll(client->ec_base.me_evp, events, p);
	}
	EVDEV_CLIENT_UNLOCKQ(client);
	return (error);
}

int
evdev_kqfilter(dev, kn)
	dev_t dev;
	struct knote *kn;
{
	struct evdev_softc *sc;
	struct evdev_dev *evdev;
	struct evdev_client *client;
	int unit, error;

	unit = minor(dev);
	sc = (struct evdev_softc*) evdev_cd.cd_devs[unit];
	if (sc == NULL) {
		return (ENXIO);
	}
	evdev = sc->sc_evdev;
	client = evdev->ev_client;
	if (client->ec_base.me_evp == NULL)
		return (EINVAL);
	return (wsevent_kqfilter(client->ec_base.me_evp, kn));
}

int
evdev_ioctl(dev, cmd, data, flag, p)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	return (evdev_do_ioctl(evdev_cd.cd_devs[minor(dev)], cmd, data, flag, p));
}

int
evdev_do_ioctl(dv, cmd, data, fflag, p)
	struct device *dv;
	int cmd;
	caddr_t data;
	int fflag;
	struct proc *p;
{
	struct evdev_softc *sc;
	struct evdev_dev *evdev;
	int error;

	sc = (struct evdev_softc*) dv;
	evdev = sc->sc_dv;
	sc->sc_refcnt++;
	error = evdev_do_ioctl_sc(evdev, cmd, data, fflag, p);
	if (--sc->sc_refcnt < 0)
		wakeup(sc);
	return (error);
}

int
evdev_do_ioctl_sc(evdev, cmd, data, fflag, p)
	struct evdev_dev *evdev;
	int cmd;
	caddr_t data;
	int fflag;
	struct proc *p;
{
	struct evdev_client *client;
	struct input_keymap_entry *ke;
	int ret, len, limit, type_num;
	uint32_t code;
	size_t nvalues;

	client = evdev->ev_client;

	if (client->ec_base->me_evp->revoked || evdev == NULL) {
		return (ENODEV);
	}

	/* file I/O ioctl handling */
	switch (cmd) {
	case FIONBIO:
		return (0);

	case FIONREAD:
		if (client->ec_base.me_evp == NULL)
			return (EINVAL);
		EVDEV_CLIENT_LOCKQ(client);
		*(int*) data = EVDEV_CLIENT_SIZEQ(client) * sizeof(struct input_event);
		EVDEV_CLIENT_UNLOCKQ(client);
		return (0);

	case FIOASYNC:
		if (client->ec_base.me_evp == NULL)
			return (EINVAL);
		client->ec_base.me_evp->async = *(int*) data != 0;
		return (0);

	case FIOSETOWN:
		if (client->ec_base.me_evp == NULL)
			return (EINVAL);
		if (-*(int*) data != client->ec_base.me_evp->io->p_pgid
				&& *(int*) data != client->ec_base.me_evp->io->p_pid)
			return (EPERM);
		return (0);

	case TIOCSPGRP:
		if (client->ec_base.me_evp == NULL)
			return (EINVAL);
		if (*(int*) data != client->ec_base.me_evp->io->p_pgid)
			return (EPERM);
		return (0);
	}

	len = IOCPARM_LEN(cmd);
	debugf(client, "ioctl called: cmd=0x%08lx, data=%p", cmd, data);

	/* evdev fixed-length ioctls handling */
	switch (cmd) {
	case EVIOCGVERSION:
		*(int*) data = EV_VERSION;
		return (0);

	case EVIOCGID:
		debugf(client, "EVIOCGID: bus=%d vendor=0x%04x product=0x%04x",
				evdev->ev_id.bustype, evdev->ev_id.vendor,
				evdev->ev_id.product);
		memcpy(data, &evdev->ev_id, sizeof(struct input_id));
		return (0);

	case EVIOCGREP:
		if (!evdev_event_supported(evdev, EV_REP))
			return (ENOTSUP);

		memcpy(data, evdev->ev_rep, sizeof(evdev->ev_rep));
		return (0);

	case EVIOCSREP:
		if (!evdev_event_supported(evdev, EV_REP))
			return (ENOTSUP);

		evdev_inject_event(evdev, EV_REP, REP_DELAY, ((int*) data)[0]);
		evdev_inject_event(evdev, EV_REP, REP_PERIOD, ((int*) data)[1]);
		return (0);

	case EVIOCGKEYCODE:
		/* Fake unsupported ioctl */
		return (0);

	case EVIOCGKEYCODE_V2:
		if (evdev->ev_methods == NULL ||
				    evdev->ev_methods->ev_get_keycode == NULL)
					return (ENOTSUP);

		ke = (struct input_keymap_entry*) data;
		evdev->ev_methods->ev_get_keycode(evdev, ke);
		return (0);

	case EVIOCSKEYCODE:
		/* Fake unsupported ioctl */
		return (0);

	case EVIOCSKEYCODE_V2:
		if (evdev->ev_methods == NULL
				|| evdev->ev_methods->ev_set_keycode == NULL)
			return (ENOTSUP);

		ke = (struct input_keymap_entry*) data;
		evdev->ev_methods->ev_set_keycode(evdev, ke);
		return (0);

	case EVIOCGABS(0) ... EVIOCGABS(ABS_MAX):
		if (evdev->ev_absinfo == NULL)
			return (EINVAL);

		memcpy(data, &evdev->ev_absinfo[cmd - EVIOCGABS(0)],
				sizeof(struct input_absinfo));
		return (0);

	case EVIOCSABS(0) ... EVIOCSABS(ABS_MAX):
		if (evdev->ev_absinfo == NULL)
			return (EINVAL);

		code = cmd - EVIOCSABS(0);
		/* mt-slot number can not be changed */
		if (code == ABS_MT_SLOT)
			return (EINVAL);

		EVDEV_LOCK(evdev);
		evdev_set_absinfo(evdev, code, (struct input_absinfo*) data);
		EVDEV_UNLOCK(evdev);
		return (0);

	case EVIOCSFF:
	case EVIOCRMFF:
	case EVIOCGEFFECTS:
		/* Fake unsupported ioctls */
		return (0);

	case EVIOCGRAB:
		EVDEV_LOCK(evdev);
		if (*(int*) data)
			ret = evdev_grab_client(evdev, client);
		else
			ret = evdev_release_client(evdev, client);
		EVDEV_UNLOCK(evdev);
		return (ret);

	case EVIOCREVOKE:
		if (*(int*) data != 0)
			return (EINVAL);

		EVDEV_LIST_LOCK(evdev);
		if (client->ec_base.me_dv != NULL && !client->ec_base.me_evp->revoked) {
			evdev_dispose_client(evdev, client);
			evdev_revoke_client(client);
		}
		EVDEV_LIST_UNLOCK(evdev);
		return (0);

	case EVIOCSCLOCKID:
		switch (*(int*) data) {
		case CLOCK_REALTIME:
			client->ec_clock_id = EV_CLOCK_REALTIME;
			return (0);
		case CLOCK_MONOTONIC:
			client->ec_clock_id = EV_CLOCK_MONOTONIC;
			return (0);
		default:
			return (EINVAL);
		}
	}

	/* evdev variable-length ioctls handling */
	switch (IOCBASECMD(cmd)) {
	case EVIOCGNAME(0):
		/* Linux evdev does not terminate truncated strings with 0 */
		limit = MIN(strlen(evdev->ev_name) + 1, len);
		memcpy(data, evdev->ev_name, limit);
		return (0);

	case EVIOCGPHYS(0):
		if (evdev->ev_shortname[0] == 0)
			return (ENOENT);

		limit = MIN(strlen(evdev->ev_shortname) + 1, len);
		memcpy(data, evdev->ev_shortname, limit);
		return (0);

	case EVIOCGUNIQ(0):
		if (evdev->ev_serial[0] == 0)
			return (ENOENT);

		limit = MIN(strlen(evdev->ev_serial) + 1, len);
		memcpy(data, evdev->ev_serial, limit);
		return (0);

	case EVIOCGPROP(0):
		limit = MIN(len, bitstr_size(INPUT_PROP_CNT));
		memcpy(data, evdev->ev_prop_flags, limit);
		return (0);

	case EVIOCGMTSLOTS(0):
		/* EVIOCGMTSLOTS always returns 0 on success */
		if (evdev->ev_mt == NULL)
			return (EINVAL);
		if (len < sizeof(uint32_t))
			return (EINVAL);
		code = *(uint32_t*) data;
		if (!ABS_IS_MT(code))
			return (EINVAL);

		nvalues = MIN(len / sizeof(int32_t) - 1, MAXIMAL_MT_SLOT(evdev) + 1);
		for (int i = 0; i < nvalues; i++)
			((int32_t*) data)[i + 1] = evdev_get_mt_value(evdev, i, code);
		return (0);

	case EVIOCGKEY(0):
		limit = MIN(len, bitstr_size(KEY_CNT));
		EVDEV_LOCK(evdev);
		evdev_client_filter_queue(client, EV_KEY);
		memcpy(data, evdev->ev_key_states, limit);
		EVDEV_UNLOCK(evdev);
		return (0);

	case EVIOCGLED(0):
		limit = MIN(len, bitstr_size(LED_CNT));
		EVDEV_LOCK(evdev);
		evdev_client_filter_queue(client, EV_LED);
		memcpy(data, evdev->ev_led_states, limit);
		return (0);

	case EVIOCGSND(0):
		limit = MIN(len, bitstr_size(SND_CNT));
		EVDEV_LOCK(evdev);
		evdev_client_filter_queue(client, EV_SND);
		memcpy(data, evdev->ev_snd_states, limit);
		EVDEV_UNLOCK(evdev);
		return (0);

	case EVIOCGSW(0):
		limit = MIN(len, bitstr_size(SW_CNT));
		EVDEV_LOCK(evdev);
		evdev_client_filter_queue(client, EV_SW);
		memcpy(data, evdev->ev_sw_states, limit);
		EVDEV_UNLOCK(evdev);
		return (0);

	case EVIOCGBIT(0, 0) ... EVIOCGBIT(EV_MAX, 0):
		type_num = IOCBASECMD(cmd) - EVIOCGBIT(0, 0);
		debugf(client, "EVIOCGBIT(%d): data=%p, len=%d", type_num, data, len);
		return (evdev_ioctl_eviocgbit(evdev, type_num, len, data, p));
	}

	return (EINVAL);
}

static int
evdev_ioctl_eviocgbit(evdev, type, len, data, p)
	struct evdev_dev *evdev;
	int type, len;
	caddr_t data;
	struct proc *p;
{
	unsigned long *bitmap;
	int limit;

	switch (type) {
	case 0:
		bitmap = evdev->ev_type_flags;
		limit = EV_CNT;
		break;
	case EV_KEY:
		bitmap = evdev->ev_key_flags;
		limit = KEY_CNT;
		break;
	case EV_REL:
		bitmap = evdev->ev_rel_flags;
		limit = REL_CNT;
		break;
	case EV_ABS:
		bitmap = evdev->ev_abs_flags;
		limit = ABS_CNT;
		break;
	case EV_MSC:
		bitmap = evdev->ev_msc_flags;
		limit = MSC_CNT;
		break;
	case EV_LED:
		bitmap = evdev->ev_led_flags;
		limit = LED_CNT;
		break;
	case EV_SND:
		bitmap = evdev->ev_snd_flags;
		limit = SND_CNT;
		break;
	case EV_SW:
		bitmap = evdev->ev_sw_flags;
		limit = SW_CNT;
		break;
	case EV_FF:
		/*
		 * We don't support EV_FF now, so let's
		 * just fake it returning only zeros.
		 */
		bzero(data, len);
		return (0);
	default:
		return (ENOTTY);
	}

	/*
	 * Clear ioctl data buffer in case it's bigger than
	 * bitmap size
	 */
	bzero(data, len);

	limit = bitstr_size(limit);
	len = MIN(limit, len);
	memcpy(data, bitmap, len);
	return (0);
}

struct wseventvar *
evdev_register_wsevent(client, p)
	struct evdev_client *client;
	struct proc *p;
{
	struct wseventvar *evar;

	if (client->ec_base.me_evp != NULL) {
		return (EBUSY);
	}

	evar = &client->ec_base.me_evar;
	wsevent_init(evar);
	client->ec_base.me_evp = evar;
	evar->io = p;

	return (evar);
}

void
evdev_unregister_wsevent(client)
	struct evdev_client *client;
{
	struct wseventvar *evar;

	evar = client->ec_base.me_evp;
	client->ec_base.me_evp = NULL;
	wsevent_fini(evar);
}

int
evdev_wsevent_inject(ev, events, nevents)
	struct wseventvar *ev;
	struct input_event *events;
	size_t nevents;
{
#define EVARRAY(ev, idx) (&(ev)->q[(idx)])
	struct timeval tv;
	struct timespec ts;
	size_t avail, i;

	avail = wsevent_avail(ev);

	/* Fail if there is all events will not fit in the queue. */
	if (avail < nevents) {
		return (ENOSPC);
	}

	/* Use the current time for all events. */
	if (events->time != NULL) {
		TIMEVAL_TO_TIMESPEC(events->time, ts);
	} else {
		microtime(&tv);
		TIMEVAL_TO_TIMESPEC(&tv, &ts);
	}

	/* Inject the events. */
	for (i = 0; i < nevents; i++) {
		struct wscons_event *we;
	    we = EVARRAY(ev, ev->put);
	    we->type = events[i].type;
	    we->code = events[i].code;
	    we->value = events[i].value;
	    we->time = ts;

	    wsevent_put(ev, 1);
	}
	wsevent_wakeup(ev);
	return (0);
}

void
evdev_revoke_client(client)
	struct evdev_client *client;
{
	struct wseventvar *evar;

	KASSERT(client->ec_evdev);

	evar = &client->ec_base.me_evar;
	evar->revoked = TRUE;
	client->ec_base.me_evp = evar;
}

void
evdev_notify_event(client)
	struct evdev_client *client;
{
	struct wseventvar *evar;

	KASSERT(sc);
	evar = &client->ec_base.me_evar;

	if (evar->blocked) {
		evar->blocked = FALSE;
		wsevent_wakeup(evar);
	}
	if (evar->selected) {
		evar->selected = FALSE;
		wsevent_wakeup(evar);
	}

	KNOTE(&evar->sel.si_klist, 0);

	if (evar->async) {
		psignal(evar->io, SIGIO);
	}
	client->ec_base.me_evp = evar;
}

static void
evdev_client_filter_queue(client, type)
	struct evdev_client *client;
	uint16_t type;
{
	struct input_event 	*event;
	struct wseventvar 	*ev;
	size_t head, tail, count, i;
	bool_t last_was_syn = FALSE;

	ev = client->ec_base.me_evp;

	i = head = ev->put;
	tail = ev->get;
	if(head < tail) {
		count = WSEVENT_QSIZE - ev->get;
	} else {
		count = ev->put - ev->get;
	}
	client->ec_buffer_ready = ev->get;

	while (i != ev->get) {
		event = &client->ec_buffer[ev->q[i]];
		i = (i + 1) % count;

		/* Skip event of given type */
		if (event->type == type) {
			continue;
		}

		/* Remove empty SYN_REPORT events */
		if (event->type == EV_SYN && event->code == SYN_REPORT) {
			if (last_was_syn) {
				continue;
			} else {
				client->ec_buffer_ready = (tail + 1) % count;
			}
		}

		/* Rewrite entry */
		memcpy(ev->q[tail], event, sizeof(struct input_event));

		last_was_syn = (event->type == EV_SYN && event->code == SYN_REPORT);

		tail = (tail + 1) % count;
	}
	wsevent_put(ev, i);
	wsevent_get(ev, tail);
}
