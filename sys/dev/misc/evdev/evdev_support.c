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

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/sysctl.h>
#include <sys/systm.h>

#include <dev/misc/evdev/evdev.h>
#include <dev/misc/evdev/evdev_private.h>
#include <dev/misc/evdev/input.h>
#include <dev/misc/evdev/freebsd-bitstring.h>

#ifdef EVDEV_DEBUG
#define	debugf(evdev, fmt, args...)	kprintf("evdev: " fmt "\n", ##args)
#else
#define	debugf(evdev, fmt, args...)
#endif

enum evdev_sparse_result {
	EV_SKIP_EVENT,		/* Event value not changed */
	EV_REPORT_EVENT,	/* Event value changed */
	EV_REPORT_MT_SLOT,	/* Event value and MT slot number changed */
};

int evdev_rcpt_mask = EVDEV_RCPT_WSMOUSE | EVDEV_RCPT_WSKBD;
int evdev_sysmouse_t_axis = 0;

static void evdev_start_repeat(struct evdev_dev *, uint16_t);
static void evdev_stop_repeat(struct evdev_dev *);
static int evdev_check_event(struct evdev_dev *, uint16_t, uint16_t, int32_t);

static inline void
bit_change(bitstr_t *bitstr, int bit, int value)
{
	if (value)
		bit_set(bitstr, bit);
	else
		bit_clear(bitstr, bit);
}

struct evdev_dev *
evdev_alloc(void)
{
	return (malloc(sizeof(struct evdev_dev), M_EVDEV, M_WAITOK | M_ZERO));
}

void
evdev_free(struct evdev_dev *evdev)
{

	if(evdev != NULL) {
		evdev_unregister(evdev);
	}
	free(evdev, M_EVDEV);
}

struct evdev_client *
evdev_client_alloc(evdev, buffer_size)
	struct evdev_dev 	*evdev;
	size_t 				buffer_size;
{
	evdev->ev_client = (struct evdev_client *)malloc(offsetof(struct evdev_client, ec_buffer) + sizeof(struct input_event) * buffer_size, M_EVDEV, M_WAITOK | M_ZERO);

	return (evdev->ev_client);
}

void
evdev_client_free(client)
	struct evdev_client 	*client;
{
	free(client, M_EVDEV);
}

static struct input_absinfo *
evdev_alloc_absinfo(void)
{
	return (malloc(sizeof(struct input_absinfo) * ABS_CNT, M_EVDEV, M_WAITOK | M_ZERO));
}

static void
evdev_free_absinfo(struct input_absinfo *absinfo)
{
	free(absinfo, M_EVDEV);
}

int
evdev_set_report_size(struct evdev_dev *evdev, size_t report_size)
{
	if (report_size > KEY_CNT + REL_CNT + ABS_CNT + MAX_MT_SLOTS * MT_CNT +
	MSC_CNT + LED_CNT + SND_CNT + SW_CNT + FF_CNT)
		return (EINVAL);

	evdev->ev_report_size = report_size;
	return (0);
}

static size_t
evdev_estimate_report_size(struct evdev_dev *evdev)
{
	size_t size = 0;
	int res;

	/*
	 * Keyboards generate one event per report but other devices with
	 * buttons like mouses can report events simultaneously
	 */
	bit_ffs_at(evdev->ev_key_flags, KEY_OK, KEY_CNT - KEY_OK, &res);
	if (res == -1)
		bit_ffs(evdev->ev_key_flags, BTN_MISC, &res);
	size += (res != -1);
	bit_count(evdev->ev_key_flags, BTN_MISC, KEY_OK - BTN_MISC, &res);
	size += res;

	/* All relative axes can be reported simultaneously */
	bit_count(evdev->ev_rel_flags, 0, REL_CNT, &res);
	size += res;

	/*
	 * All absolute axes can be reported simultaneously.
	 * Multitouch axes can be reported ABS_MT_SLOT times
	 */
	if (evdev->ev_absinfo != NULL) {
		bit_count(evdev->ev_abs_flags, 0, ABS_CNT, &res);
		size += res;
		bit_count(evdev->ev_abs_flags, ABS_MT_FIRST, MT_CNT, &res);
		if (res > 0) {
			res++; /* ABS_MT_SLOT or SYN_MT_REPORT */
			if (bit_test(evdev->ev_abs_flags, ABS_MT_SLOT))
				/* MT type B */
				size += res * MAXIMAL_MT_SLOT(evdev);
			else
				/* MT type A */
				size += res * (MAX_MT_REPORTS - 1);
		}
	}

	/* All misc events can be reported simultaneously */
	bit_count(evdev->ev_msc_flags, 0, MSC_CNT, &res);
	size += res;

	/* All leds can be reported simultaneously */
	bit_count(evdev->ev_led_flags, 0, LED_CNT, &res);
	size += res;

	/* Assume other events are generated once per report */
	bit_ffs(evdev->ev_snd_flags, SND_CNT, &res);
	size += (res != -1);

	bit_ffs(evdev->ev_sw_flags, SW_CNT, &res);
	size += (res != -1);

	/* XXX: FF part is not implemented yet */

	size++; /* SYN_REPORT */
	return (size);
}

static int
evdev_register_common(struct evdev_dev *evdev)
{
	int ret;

	debugf(evdev, "%s: registered evdev provider: %s <%s>\n",
			evdev->ev_shortname, evdev->ev_name, evdev->ev_serial);

	/* Initialize internal structures */
	if (evdev_event_supported(evdev, EV_REP)
			&& bit_test(evdev->ev_flags, EVDEV_FLAG_SOFTREPEAT)) {
		/* Initialize callout */
		callout_init(&evdev->ev_rep_callout);
		if (evdev->ev_rep[REP_DELAY] == 0 && evdev->ev_rep[REP_PERIOD] == 0) {
			/* Supply default values */
			evdev->ev_rep[REP_DELAY] = 250;
			evdev->ev_rep[REP_PERIOD] = 33;
		}
	}
	/* Initialize multitouch protocol type B states */
	if (bit_test(evdev->ev_abs_flags, ABS_MT_SLOT) && evdev->ev_absinfo != NULL
			&& MAXIMAL_MT_SLOT(evdev) > 0) {
		evdev_mt_init(evdev);
	}

	/* Estimate maximum report size */
	if (evdev->ev_report_size == 0) {
		ret = evdev_set_report_size(evdev, evdev_estimate_report_size(evdev));
		if (ret != 0) {
			return (ret);
		}
	}

	return (0);
}

int
evdev_register(struct evdev_dev *evdev)
{
	int ret;

	evdev->ev_lock_type = EV_LOCK_INTERNAL;
	evdev->ev_lock.lk_lnterlock = evdev->ev_mtx;
	simple_lock_init(&evdev->ev_mtx, "evmtx");

	ret = evdev_register_common(evdev);
	return (ret);
}

int
evdev_register_mtx(struct evdev_dev *evdev, struct lock *mtx)
{
	evdev->ev_lock_type = EV_LOCK_MTX;
//	evdev->ev_lock = mtx;
	return (evdev_register_common(evdev));
}

int
evdev_unregister(struct evdev_dev *evdev)
{
	struct evdev_client *client;

	debugf(evdev, "%s: unregistered evdev provider: %s\n", evdev->ev_shortname, evdev->ev_name);

	EVDEV_LOCK(evdev);
	/* Wake up sleepers */
	client = evdev->ev_client;
	evdev_revoke_client(client);
	evdev_dispose_client(evdev, client);
	EVDEV_CLIENT_LOCKQ(client);
	evdev_notify_event(client);
	EVDEV_CLIENT_UNLOCKQ(client);
	EVDEV_UNLOCK(evdev);

	/*
	 * For some devices, e.g. keyboards, ev_absinfo and ev_mt
	 * may be NULL, so check before freeing them.
	 */
	if (evdev->ev_absinfo != NULL)
	    evdev_free_absinfo(evdev->ev_absinfo);
	if (evdev->ev_mt != NULL)
	    evdev_mt_free(evdev);

	return (0);
}

inline void
evdev_set_name(struct evdev_dev *evdev, const char *name)
{
	snprintf(evdev->ev_name, NAMELEN, "%s", name);
}

inline void
evdev_set_id(struct evdev_dev *evdev, uint16_t bustype, uint16_t vendor,
    uint16_t product, uint16_t version)
{
	evdev->ev_id = (struct input_id) {
		.bustype = bustype,
		.vendor = vendor,
		.product = product,
		.version = version
	};
}

inline void
evdev_set_phys(struct evdev_dev *evdev, const char *name)
{

	snprintf(evdev->ev_shortname, NAMELEN, "%s", name);
}

inline void
evdev_set_serial(struct evdev_dev *evdev, const char *serial)
{

	snprintf(evdev->ev_serial, NAMELEN, "%s", serial);
}

inline void
evdev_set_methods(struct evdev_dev *evdev, void *softc, const struct evdev_methods *methods)
{
	evdev->ev_methods = methods;
	evdev->ev_softc = softc;
}

inline void
evdev_support_prop(struct evdev_dev *evdev, uint16_t prop)
{

	KASSERTMSG(prop < INPUT_PROP_CNT, "invalid evdev input property");
	bit_set(evdev->ev_prop_flags, prop);
}

inline void
evdev_support_event(struct evdev_dev *evdev, uint16_t type)
{

	KASSERTMSG(type < EV_CNT, "invalid evdev event property");
	bit_set(evdev->ev_type_flags, type);
}

inline void
evdev_support_key(struct evdev_dev *evdev, uint16_t code)
{

	KASSERTMSG(code < KEY_CNT, "invalid evdev key property");
	bit_set(evdev->ev_key_flags, code);
}

inline void
evdev_support_rel(struct evdev_dev *evdev, uint16_t code)
{

	KASSERTMSG(code < REL_CNT, "invalid evdev rel property");
	bit_set(evdev->ev_rel_flags, code);
}

inline void
evdev_support_abs(struct evdev_dev *evdev, uint16_t code, int32_t value,
    int32_t minimum, int32_t maximum, int32_t fuzz, int32_t flat,
    int32_t resolution)
{
	struct input_absinfo absinfo;

	KASSERTMSG(code < ABS_CNT, "invalid evdev abs property");

	absinfo = (struct input_absinfo) {
		.value = value,
		.minimum = minimum,
		.maximum = maximum,
		.fuzz = fuzz,
		.flat = flat,
		.resolution = resolution,
	};
	evdev_set_abs_bit(evdev, code);
	evdev_set_absinfo(evdev, code, &absinfo);
}

inline void
evdev_set_abs_bit(struct evdev_dev *evdev, uint16_t code)
{

	KASSERTMSG(code < ABS_CNT, "invalid evdev abs property");
	if (evdev->ev_absinfo == NULL)
		evdev->ev_absinfo = evdev_alloc_absinfo();
	bit_set(evdev->ev_abs_flags, code);
}

inline void
evdev_support_msc(struct evdev_dev *evdev, uint16_t code)
{

	KASSERTMSG(code < MSC_CNT, "invalid evdev msc property");
	bit_set(evdev->ev_msc_flags, code);
}

inline void
evdev_support_led(struct evdev_dev *evdev, uint16_t code)
{

	KASSERTMSG(code < LED_CNT, "invalid evdev led property");
	bit_set(evdev->ev_led_flags, code);
}

inline void
evdev_support_snd(struct evdev_dev *evdev, uint16_t code)
{

	KASSERTMSG(code < SND_CNT, "invalid evdev snd property");
	bit_set(evdev->ev_snd_flags, code);
}

inline void
evdev_support_sw(struct evdev_dev *evdev, uint16_t code)
{

	KASSERTMSG(code < SW_CNT, "invalid evdev sw property");
	bit_set(evdev->ev_sw_flags, code);
}

bool_t
evdev_event_supported(struct evdev_dev *evdev, uint16_t type)
{

	KASSERTMSG(type < EV_CNT, "invalid evdev event property");
	return (bit_test(evdev->ev_type_flags, type));
}

inline void
evdev_set_absinfo(struct evdev_dev *evdev, uint16_t axis, struct input_absinfo *absinfo)
{

	KASSERTMSG(axis < ABS_CNT, "invalid evdev abs property");

	if (axis == ABS_MT_SLOT &&
	    (absinfo->maximum < 1 || absinfo->maximum >= MAX_MT_SLOTS))
		return;

	if (evdev->ev_absinfo == NULL)
		evdev->ev_absinfo = evdev_alloc_absinfo();

	if (axis == ABS_MT_SLOT)
		evdev->ev_absinfo[ABS_MT_SLOT].maximum = absinfo->maximum;
	else
		memcpy(&evdev->ev_absinfo[axis], absinfo, sizeof(struct input_absinfo));
}

inline void
evdev_set_repeat_params(struct evdev_dev *evdev, uint16_t property, int value)
{

	KASSERTMSG(property < REP_CNT, "invalid evdev repeat property");
	evdev->ev_rep[property] = value;
}

inline void
evdev_set_flag(struct evdev_dev *evdev, uint16_t flag)
{

	KASSERTMSG(flag < EVDEV_FLAG_CNT, "invalid evdev flag property");
	bit_set(evdev->ev_flags, flag);
}

static int
evdev_check_event(struct evdev_dev *evdev, uint16_t type, uint16_t code,
    int32_t value)
{

	if (type >= EV_CNT)
		return (EINVAL);

	/* Allow SYN events implicitly */
	if (type != EV_SYN && !evdev_event_supported(evdev, type))
		return (EINVAL);

	switch (type) {
	case EV_SYN:
		if (code >= SYN_CNT)
			return (EINVAL);
		break;

	case EV_KEY:
		if (code >= KEY_CNT)
			return (EINVAL);
		if (!bit_test(evdev->ev_key_flags, code))
			return (EINVAL);
		break;

	case EV_REL:
		if (code >= REL_CNT)
			return (EINVAL);
		if (!bit_test(evdev->ev_rel_flags, code))
			return (EINVAL);
		break;

	case EV_ABS:
		if (code >= ABS_CNT)
			return (EINVAL);
		if (!bit_test(evdev->ev_abs_flags, code))
			return (EINVAL);
		if (code == ABS_MT_SLOT &&
		    (value < 0 || value > MAXIMAL_MT_SLOT(evdev)))
			return (EINVAL);
		if (ABS_IS_MT(code) && evdev->ev_mt == NULL &&
		    bit_test(evdev->ev_abs_flags, ABS_MT_SLOT))
			return (EINVAL);
		break;

	case EV_MSC:
		if (code >= MSC_CNT)
			return (EINVAL);
		if (!bit_test(evdev->ev_msc_flags, code))
			return (EINVAL);
		break;

	case EV_LED:
		if (code >= LED_CNT)
			return (EINVAL);
		if (!bit_test(evdev->ev_led_flags, code))
			return (EINVAL);
		break;

	case EV_SND:
		if (code >= SND_CNT)
			return (EINVAL);
		if (!bit_test(evdev->ev_snd_flags, code))
			return (EINVAL);
		break;

	case EV_SW:
		if (code >= SW_CNT)
			return (EINVAL);
		if (!bit_test(evdev->ev_sw_flags, code))
			return (EINVAL);
		break;

	case EV_REP:
		if (code >= REP_CNT)
			return (EINVAL);
		break;

	default:
		return (EINVAL);
	}

	return (0);
}

static void
evdev_modify_event(struct evdev_dev *evdev, uint16_t type, uint16_t code, int32_t *value)
{
	EVDEV_LOCK_ASSERT(evdev);

	switch (type) {
	case EV_KEY:
		if (!evdev_event_supported(evdev, EV_REP))
			break;

		if (!bit_test(evdev->ev_flags, EVDEV_FLAG_SOFTREPEAT)) {
			/* Detect driver key repeats. */
			if (bit_test(evdev->ev_key_states, code)
					&& *value == KEY_EVENT_DOWN)
				*value = KEY_EVENT_REPEAT;
		} else {
			/* Start/stop callout for evdev repeats */
			if (bit_test(evdev->ev_key_states, code) == !*value) {
				if (*value == KEY_EVENT_DOWN)
					evdev_start_repeat(evdev, code);
				else
					evdev_stop_repeat(evdev);
			}
		}
		break;

	case EV_ABS:
		/* TBD: implement fuzz */
		break;
	}
}

static enum evdev_sparse_result
evdev_sparse_event(struct evdev_dev *evdev, uint16_t type, uint16_t code, int32_t value)
{
	int32_t last_mt_slot;

	EVDEV_LOCK_ASSERT(evdev);

	/*
	 * For certain event types, update device state bits
	 * and convert level reporting to edge reporting
	 */
	switch (type) {
	case EV_KEY:
		switch (value) {
		case KEY_EVENT_UP:
		case KEY_EVENT_DOWN:
			if (bit_test(evdev->ev_key_states, code) == value)
				return (EV_SKIP_EVENT);
			bit_change(evdev->ev_key_states, code, value);
			break;

		case KEY_EVENT_REPEAT:
			if (bit_test(evdev->ev_key_states, code) == 0
					|| !evdev_event_supported(evdev, EV_REP))
				return (EV_SKIP_EVENT);
			break;

		default:
			return (EV_SKIP_EVENT);
		}
		break;

	case EV_LED:
		if (bit_test(evdev->ev_led_states, code) == value)
			return (EV_SKIP_EVENT);
		bit_change(evdev->ev_led_states, code, value);
		break;

	case EV_SND:
		if (bit_test(evdev->ev_snd_states, code) == value)
			return (EV_SKIP_EVENT);
		bit_change(evdev->ev_snd_states, code, value);
		break;

	case EV_SW:
		if (bit_test(evdev->ev_sw_states, code) == value)
			return (EV_SKIP_EVENT);
		bit_change(evdev->ev_sw_states, code, value);
		break;

	case EV_REP:
		if (evdev->ev_rep[code] == value)
			return (EV_SKIP_EVENT);
		evdev_set_repeat_params(evdev, code, value);
		break;

	case EV_REL:
		if (value == 0)
			return (EV_SKIP_EVENT);
		break;

		/* For EV_ABS, save last value in absinfo and ev_mt_states */
	case EV_ABS:
		switch (code) {
		case ABS_MT_SLOT:
			/* Postpone ABS_MT_SLOT till next event */
			evdev_set_last_mt_slot(evdev, value);
			return (EV_SKIP_EVENT);

		case ABS_MT_FIRST ... ABS_MT_LAST:
			/* Pass MT protocol type A events as is */
			if (!bit_test(evdev->ev_abs_flags, ABS_MT_SLOT))
				break;
			/* Don`t repeat MT protocol type B events */
			last_mt_slot = evdev_get_last_mt_slot(evdev);
			if (evdev_get_mt_value(evdev, last_mt_slot, code) == value)
				return (EV_SKIP_EVENT);
			evdev_set_mt_value(evdev, last_mt_slot, code, value);
			if (last_mt_slot != CURRENT_MT_SLOT(evdev)) {
				CURRENT_MT_SLOT (evdev) = last_mt_slot;
				evdev->ev_report_opened = TRUE;
				return (EV_REPORT_MT_SLOT);
			}
			break;

		default:
			if (evdev->ev_absinfo[code].value == value)
				return (EV_SKIP_EVENT);
			evdev->ev_absinfo[code].value = value;
		}
		break;

	case EV_SYN:
		if (code == SYN_REPORT) {
			/* Count empty reports as well as non empty */
			evdev->ev_report_count++;
			/* Skip empty reports */
			if (!evdev->ev_report_opened)
				return (EV_SKIP_EVENT);
			evdev->ev_report_opened = FALSE;
			return (EV_REPORT_EVENT);
		}
		break;
	}

	evdev->ev_report_opened = TRUE;
	return (EV_REPORT_EVENT);
}

static void
evdev_propagate_event(struct evdev_dev *evdev, uint16_t type, uint16_t code, int32_t value)
{
	struct evdev_client *client;

	debugf(evdev, "%s pushed event %d/%d/%d",
			evdev->ev_shortname, type, code, value);

	EVDEV_LOCK_ASSERT(evdev);

	/* Propagate event through all clients */
	client = evdev->ev_client;
	if (evdev->ev_grabber != NULL && evdev->ev_grabber != client) {
	//	continue;
    }

	EVDEV_CLIENT_LOCKQ(client);
	evdev_client_push(client, type, code, value);
	if (type == EV_SYN && code == SYN_REPORT)
		evdev_notify_event(client);
	EVDEV_CLIENT_UNLOCKQ(client);

	evdev->ev_event_count++;
}

void
evdev_send_event(struct evdev_dev *evdev, uint16_t type, uint16_t code, int32_t value)
{
	enum evdev_sparse_result sparse;

	sparse = evdev_sparse_event(evdev, type, code, value);
	switch (sparse) {
	case EV_REPORT_MT_SLOT:
		/* report postponed ABS_MT_SLOT */
		evdev_propagate_event(evdev, EV_ABS, ABS_MT_SLOT, CURRENT_MT_SLOT(evdev));
		/* FALLTHROUGH */
	case EV_REPORT_EVENT:
		evdev_propagate_event(evdev, type, code, value);
		/* FALLTHROUGH */
	case EV_SKIP_EVENT:
		break;
	}
}

#ifdef notyet
void
evdev_restore_after_kdb(struct evdev_dev *evdev)
{
	int code;

	EVDEV_LOCK_ASSERT(evdev);

	/* Report postponed leds */
	bit_foreach(evdev->ev_kdb_led_states, LED_CNT, code);
	evdev_send_event(evdev, EV_LED, code, !bit_test(evdev->ev_led_states, code));
	bit_nclear(evdev->ev_kdb_led_states, 0, LED_MAX);

	/* Release stuck keys (CTRL + ALT + ESC) */
	evdev_stop_repeat(evdev);
	bit_foreach(evdev->ev_key_states, KEY_CNT, code)
	evdev_send_event(evdev, EV_KEY, code, KEY_EVENT_UP);
	evdev_send_event(evdev, EV_SYN, SYN_REPORT, 1);
}
#endif

int
evdev_push_event(struct evdev_dev *evdev, uint16_t type, uint16_t code, int32_t value)
{

	if (evdev_check_event(evdev, type, code, value) != 0)
		return (EINVAL);

	EVDEV_ENTER(evdev);

	evdev_modify_event(evdev, type, code, &value);
	if (type == EV_SYN && code == SYN_REPORT
			&& bit_test(evdev->ev_flags, EVDEV_FLAG_MT_AUTOREL))
		evdev_send_mt_autorel(evdev);
	if (type == EV_SYN && code == SYN_REPORT && evdev->ev_report_opened
			&& bit_test(evdev->ev_flags, EVDEV_FLAG_MT_STCOMPAT))
		evdev_send_mt_compat(evdev);
	evdev_send_event(evdev, type, code, value);

	EVDEV_EXIT(evdev);

	return (0);
}

int
evdev_inject_event(struct evdev_dev *evdev, uint16_t type, uint16_t code, int32_t value)
{
	int ret = 0;

	switch (type) {
	case EV_REP:
		/* evdev repeats should not be processed by hardware driver */
		if (bit_test(evdev->ev_flags, EVDEV_FLAG_SOFTREPEAT))
			goto push;
		/* FALLTHROUGH */
	case EV_LED:
	case EV_MSC:
	case EV_SND:
	case EV_FF:

		if (evdev->ev_methods != NULL) {
			evdev_method_event(evdev, type, code, value);
        }
		/*
		 * Leds and driver repeats should be reported in ev_event
		 * method body to interoperate with kbdmux states and rates
		 * propagation so both ways (ioctl and evdev) of changing it
		 * will produce only one evdev event report to client.
		 */
		if (type == EV_LED || type == EV_REP)
			break;
		/* FALLTHROUGH */
	case EV_SYN:
	case EV_KEY:
	case EV_REL:
	case EV_ABS:
	case EV_SW:
push:
		ret = evdev_push_event(evdev, type, code, value);
		break;

	default:
		ret = EINVAL;
	}

	return (ret);
}

int
evdev_register_client(struct evdev_dev *evdev, struct evdev_client *client)
{
	int ret = 0;

	EVDEV_LOCK_ASSERT(evdev);
	if (evdev->ev_client && evdev->ev_methods != NULL) {
		debugf(evdev, "adding new client for device %s", evdev->ev_shortname);
		ret = evdev_doopen(evdev);
	}
	if (ret == 0) {
		client = evdev->ev_client;
	}
	return (1);
}

void
evdev_dispose_client(struct evdev_dev *evdev, struct evdev_client *client)
{
	int ret;

	debugf(evdev, "removing client for device %s", evdev->ev_shortname);

	KASSERT(evdev);
	client = NULL;
	evdev->ev_client = client;
	if(evdev->ev_client == NULL) {
		if (evdev->ev_methods != NULL) {
			ret = evdev_doclose(evdev);
		}
		if (evdev_event_supported(evdev, EV_REP) && bit_test(evdev->ev_flags, EVDEV_FLAG_SOFTREPEAT)) {
			evdev_stop_repeat(evdev);
		}
	}
	if (ret == 0) {
		evdev_release_client(evdev, client);		
	}
}

int
evdev_grab_client(struct evdev_dev *evdev, struct evdev_client *client)
{
	KASSERT(evdev);

	if (evdev->ev_grabber != NULL) {
		return (EBUSY);
	}
	evdev->ev_grabber = client;
	return (0);
}

int
evdev_release_client(struct evdev_dev *evdev, struct evdev_client *client)
{
	KASSERT(evdev);

	if (evdev->ev_grabber != client) {
		return (EINVAL);
	}
	evdev->ev_grabber = NULL;
	return (0);
}

static void
evdev_repeat_callout(void *arg)
{
	struct evdev_dev *evdev = (struct evdev_dev *)arg;

	evdev_send_event(evdev, EV_KEY, evdev->ev_rep_key, KEY_EVENT_REPEAT);
	evdev_send_event(evdev, EV_SYN, SYN_REPORT, 1);

	if (evdev->ev_rep[REP_PERIOD])
		callout_reset(&evdev->ev_rep_callout,
				evdev->ev_rep[REP_PERIOD] * hz / 1000, evdev_repeat_callout,
				evdev);
	else
		evdev->ev_rep_key = KEY_RESERVED;
}

static void
evdev_start_repeat(struct evdev_dev *evdev, uint16_t key)
{
	if (evdev->ev_rep[REP_DELAY]) {
		evdev->ev_rep_key = key;
		callout_reset(&evdev->ev_rep_callout,
		    evdev->ev_rep[REP_DELAY] * hz / 1000,
		    evdev_repeat_callout, evdev);
	}
}

static void
evdev_stop_repeat(struct evdev_dev *evdev)
{
	if (evdev->ev_rep_key != KEY_RESERVED) {
		callout_stop(&evdev->ev_rep_callout);
		evdev->ev_rep_key = KEY_RESERVED;
	}
}
