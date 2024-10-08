/*	$NetBSD: usb_subr.c,v 1.111.2.1 2004/07/02 17:23:33 he Exp $	*/
/*	$FreeBSD: src/sys/dev/usb/usb_subr.c,v 1.18 1999/11/17 22:33:47 n_hibma Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifdef _KERNEL_OPT
//#include "opt_compat_netbsd.h"
//#include "opt_usb.h"
#include "opt_usbverbose.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/select.h>
#include <sys/user.h>

#include <dev/usb/usb.h>

#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usbdevs.h>
#include <dev/usb/usb_quirks.h>

#ifdef USB_DEBUG
#define DPRINTF(x)		if (usbdebug) printf x
#define DPRINTFN(n,x)	if (usbdebug>(n)) printf x
extern int usbdebug;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

static usbd_status	usbd_set_config (usbd_device_handle, int);
static char *usbd_get_string (usbd_device_handle, int, char *);
static int usbd_getnewaddr (usbd_bus_handle bus);

static int usbd_print (void *aux, const char *pnp);
static int usbd_submatch (struct device *, struct cfdata *cf, void *);
static void usbd_free_iface_data (usbd_device_handle dev, int ifcno);
static void usbd_kill_pipe (usbd_pipe_handle);
static usbd_status usbd_probe_and_attach (struct device *parent, usbd_device_handle dev, int port, int addr);

#ifdef USBVERBOSE
typedef u_int16_t usb_vendor_id_t;
typedef u_int16_t usb_product_id_t;

/*
 * Descriptions of of known vendors and devices ("products").
 */
struct usb_knowndev {
	usb_vendor_id_t		vendor;
	usb_product_id_t	product;
	int					flags;
	char				*vendorname, *productname;
};
#define	USB_KNOWNDEV_NOPROD	0x01		/* match on vendor only */

#include <dev/usb/usbdevs_data.h>
#endif /* USBVERBOSE */

#ifdef USB_DEBUG
static char *usbd_error_strs[] = {
	"NORMAL_COMPLETION",
	"IN_PROGRESS",
	"PENDING_REQUESTS",
	"NOT_STARTED",
	"INVAL",
	"IS_IDLE",
	"NOMEM",
	"CANCELLED",
	"BAD_ADDRESS",
	"IN_USE",
	"INTERFACE_NOT_ACTIVE",
	"NO_ADDR",
	"SET_ADDR_FAILED",
	"NO_POWER",
	"TOO_DEEP",
	"IOERROR",
	"NOT_CONFIGURED",
	"TIMEOUT",
	"SHORT_XFER",
	"STALLED",
	"XXX",
};
#endif

const char *
usbd_errstr(err)
	usbd_status err;
{
	static char buffer[5];

	if (err < USBD_ERROR_MAX) {
		return usbd_error_strs[err];
	} else {
		snprintf(buffer, sizeof buffer, "%d", err);
		return buffer;
	}
}

usbd_status
usbd_get_string_desc(dev, sindex, langid, sdesc, sizep)
	usbd_device_handle dev;
	int sindex, langid;
	usb_string_descriptor_t *sdesc;
	int *sizep;
{
	usb_device_request_t req;
	usbd_status err;
	int actlen;

	req.bmRequestType = UT_READ_DEVICE;
	req.bRequest = UR_GET_DESCRIPTOR;
	USETW2(req.wValue, UDESC_STRING, sindex);
	USETW(req.wIndex, langid);
	USETW(req.wLength, 2);	/* only size byte first */
	err = usbd_do_request_flags(dev, &req, sdesc, USBD_SHORT_XFER_OK,
		&actlen, USBD_DEFAULT_TIMEOUT);
	if (err)
		return (err);

	if (actlen < 2)
		return (USBD_SHORT_XFER);

	USETW(req.wLength, sdesc->bLength);	/* the whole string */
	err = usbd_do_request_flags(dev, &req, sdesc, USBD_SHORT_XFER_OK,
		&actlen, USBD_DEFAULT_TIMEOUT);
	if (err)
		return (err);

	if (actlen != sdesc->bLength) {
		DPRINTFN(-1, ("usbd_get_string_desc: expected %d, got %d\n",
		    sdesc->bLength, actlen));
	}

	*sizep = actlen;
	return (USBD_NORMAL_COMPLETION);
}

char *
usbd_get_string(dev, si, buf)
	usbd_device_handle dev;
	int si;
	char *buf;
{
	int swap = dev->quirks->uq_flags & UQ_SWAP_UNICODE;
	usb_string_descriptor_t us;
	char *s;
	int i, n;
	u_int16_t c;
	usbd_status err;
	int size;

	if (si == 0)
		return (0);
	if (dev->quirks->uq_flags & UQ_NO_STRINGS)
		return (0);
	if (dev->langid == USBD_NOLANG) {
		/* Set up default language */
		err = usbd_get_string_desc(dev, USB_LANGUAGE_TABLE, 0, &us,
		    &size);
		if (err || size < 4) {
			dev->langid = 0; /* Well, just pick something then */
		} else {
			/* Pick the first language as the default. */
			dev->langid = UGETW(us.bString[0]);
		}
	}
	err = usbd_get_string_desc(dev, si, dev->langid, &us, &size);
	if (err)
		return (0);
	s = buf;
	n = size / 2 - 1;
	for (i = 0; i < n; i++) {
		c = UGETW(us.bString[i]);
		/* Convert from Unicode, handle buggy strings. */
		if ((c & 0xff00) == 0)
			*s++ = c;
		else if ((c & 0x00ff) == 0 && swap)
			*s++ = c >> 8;
		else
			*s++ = '?';
	}
	*s++ = 0;
	return (buf);
}

static void
usbd_trim_spaces(p)
	char *p;
{
	char *q, *e;

	if (p == NULL)
		return;
	q = e = p;
	while (*q == ' ')	/* skip leading spaces */
		q++;
	while ((*p = *q++))	/* copy string */
		if (*p++ != ' ') /* remember last non-space */
			e = p;
	*e = 0;			/* kill trailing spaces */
}

void
usbd_devinfo_vp(dev, v, p, usedev)
	usbd_device_handle dev;
	char *v, *p;
	int usedev;
{
	usb_device_descriptor_t *udd = &dev->ddesc;
	char *vendor = NULL, *product = NULL;
#ifdef USBVERBOSE
	const struct usb_knowndev *kdp;
#endif

	if (dev == NULL) {
		v[0] = p[0] = '\0';
		return;
	}

	if (usedev) {
		vendor = usbd_get_string(dev, udd->iManufacturer, v);
		usbd_trim_spaces(vendor);
		product = usbd_get_string(dev, udd->iProduct, p);
		usbd_trim_spaces(product);
		if (vendor && !*vendor)
			vendor = NULL;
		if (product && !*product)
			product = NULL;
	} else {
		vendor = NULL;
		product = NULL;
	}
#ifdef USBVERBOSE
	if (vendor == NULL || product == NULL) {
		for(kdp = usb_knowndevs;
		    kdp->vendorname != NULL;
		    kdp++) {
			if (kdp->vendor == UGETW(udd->idVendor) &&
			    (kdp->product == UGETW(udd->idProduct) ||
			     (kdp->flags & USB_KNOWNDEV_NOPROD) != 0))
				break;
		}
		if (kdp->vendorname != NULL) {
			if (vendor == NULL)
			    vendor = kdp->vendorname;
			if (product == NULL)
			    product = (kdp->flags & USB_KNOWNDEV_NOPROD) == 0 ?
				kdp->productname : NULL;
		}
	}
#endif
	if (vendor != NULL && *vendor)
		strcpy(v, vendor);
	else
		sprintf(v, "vendor 0x%04x", UGETW(udd->idVendor));
	if (product != NULL && *product)
		strcpy(p, product);
	else
		sprintf(p, "product 0x%04x", UGETW(udd->idProduct));
}


int
usbd_printBCD(cp, bcd)
	char *cp;
	int bcd;
{
	return (sprintf(cp, "%x.%02x", bcd >> 8, bcd & 0xff));
}

void
usbd_devinfo(dev, showclass, cp)
	usbd_device_handle dev;
	int showclass;
	char *cp;
{
	usb_device_descriptor_t *udd = &dev->ddesc;
	char vendor[USB_MAX_STRING_LEN];
	char product[USB_MAX_STRING_LEN];
	int bcdDevice, bcdUSB;

	usbd_devinfo_vp(dev, vendor, product);
	cp += sprintf(cp, "%s %s", vendor, product);
	if (showclass)
		cp += sprintf(cp, ", class %d/%d",
			      udd->bDeviceClass, udd->bDeviceSubClass);
	bcdUSB = UGETW(udd->bcdUSB);
	bcdDevice = UGETW(udd->bcdDevice);
	cp += sprintf(cp, ", rev ");
	cp += usbd_printBCD(cp, bcdUSB);
	*cp++ = '/';
	cp += usbd_printBCD(cp, bcdDevice);
	cp += sprintf(cp, ", addr %d", dev->address);
	*cp = 0;
}

/* Delay for a certain number of ms */
void
usb_delay_ms(bus, ms)
	usbd_bus_handle bus;
	u_int ms;
{
	/* Wait at least two clock ticks so we know the time has passed. */
	if (bus->use_polling)
		delay((ms+1) * 1000);
	else
		tsleep(&ms, PRIBIO, "usbdly", (ms*hz+999)/1000 + 1);
}

/* Delay given a device handle. */
void
usbd_delay_ms(dev, ms)
	usbd_device_handle dev;
	u_int ms;
{
	usb_delay_ms(dev->bus, ms);
}

usbd_status
usbd_reset_port(dev, port, ps)
	usbd_device_handle dev;
	int port;
	usb_port_status_t *ps;
{
	usb_device_request_t req;
	usbd_status r;
	int n;
	
	req.bmRequestType = UT_WRITE_CLASS_OTHER;
	req.bRequest = UR_SET_FEATURE;
	USETW(req.wValue, UHF_PORT_RESET);
	USETW(req.wIndex, port);
	USETW(req.wLength, 0);
	r = usbd_do_request(dev, &req, 0);
	DPRINTFN(1,("usbd_reset_port: port %d reset done, error=%d(%s)\n", port, r, usbd_error_strs[r]));
	if (r != USBD_NORMAL_COMPLETION)
		return (r);
	n = 10;
	do {
		/* Wait for device to recover from reset. */
		usbd_delay_ms(dev, USB_PORT_RESET_DELAY);
		r = usbd_get_port_status(dev, port, ps);
		if (r != USBD_NORMAL_COMPLETION) {
			DPRINTF(("usbd_reset_port: get status failed %d\n",r));
			return (r);
		}
	} while ((UGETW(ps->wPortChange) & UPS_C_PORT_RESET) == 0 && --n > 0);
	if (n == 0) {
		printf("usbd_reset_port: timeout\n");
		return (USBD_IOERROR);
	}
	r = usbd_clear_port_feature(dev, port, UHF_C_PORT_RESET);
#ifdef USB_DEBUG
	if (r != USBD_NORMAL_COMPLETION)
		DPRINTF(("usbd_reset_port: clear port feature failed %d\n",r));
#endif

	/* Wait for the device to recover from reset. */
	usbd_delay_ms(dev, USB_PORT_RESET_RECOVERY);
	return (r);
}

usb_interface_descriptor_t *
usbd_find_idesc(cd, ifaceidx, altidx)
	usb_config_descriptor_t *cd;
	int ifaceidx;
	int altidx;
{
	char *p = (char *)cd;
	char *end = p + UGETW(cd->wTotalLength);
	usb_interface_descriptor_t *d;
	int curidx, lastidx, curaidx = 0;

	for (curidx = lastidx = -1; p < end; ) {
		d = (usb_interface_descriptor_t *)p;
		DPRINTFN(4,("usbd_find_idesc: idx=%d(%d) altidx=%d(%d) len=%d "
			    "type=%d\n", 
			    ifaceidx, curidx, altidx, curaidx,
			    d->bLength, d->bDescriptorType));
		if (d->bLength == 0) /* bad descriptor */
			break;
		p += d->bLength;
		if (p <= end && d->bDescriptorType == UDESC_INTERFACE) {
			if (d->bInterfaceNumber != lastidx) {
				lastidx = d->bInterfaceNumber;
				curidx++;
				curaidx = 0;
			} else
				curaidx++;
			if (ifaceidx == curidx && altidx == curaidx)
				return (d);
		}
	}
	return (NULL);
}

usb_endpoint_descriptor_t *
usbd_find_edesc(cd, ifaceidx, altidx, endptidx)
	usb_config_descriptor_t *cd;
	int ifaceidx;
	int altidx;
	int endptidx;
{
	char *p = (char *)cd;
	char *end = p + UGETW(cd->wTotalLength);
	usb_interface_descriptor_t *d;
	usb_endpoint_descriptor_t *e;
	int curidx;

	d = usbd_find_idesc(cd, ifaceidx, altidx);
	if (!d)
		return (NULL);
	if (endptidx >= d->bNumEndpoints) /* quick exit */
		return (NULL);

	curidx = -1;
	for (p = (char *)d + d->bLength; p < end; ) {
		e = (usb_endpoint_descriptor_t *)p;
		if (e->bLength == 0) /* bad descriptor */
			break;
		p += e->bLength;
		if (p <= end && e->bDescriptorType == UDESC_INTERFACE)
			return (NULL);
		if (p <= end && e->bDescriptorType == UDESC_ENDPOINT) {
			curidx++;
			if (curidx == endptidx)
				return (e);
		}
	}
	return (NULL);
}

usbd_status
usbd_fill_iface_data(dev, ifaceidx, altidx)
	usbd_device_handle dev;
	int ifaceidx;
	int altidx;
{
	usbd_interface_handle ifc = &dev->ifaces[ifaceidx];
	char *p, *end;
	int endpt, nendpt;

	DPRINTFN(4,("usbd_fill_iface_data: ifaceidx=%d altidx=%d\n", ifaceidx, altidx));
	ifc->device = dev;
	ifc->idesc = usbd_find_idesc(dev->cdesc, ifaceidx, altidx);
	if (ifc->idesc == NULL)
		return (USBD_INVAL);
	ifc->index = ifaceidx;
	ifc->altindex = altidx;
	nendpt = ifc->idesc->bNumEndpoints;
	DPRINTFN(10,("usbd_fill_iface_data: found idesc n=%d\n", nendpt));
	if (nendpt != 0) {
		ifc->endpoints = malloc(nendpt * sizeof(struct usbd_endpoint), M_USB, M_NOWAIT);
		if (ifc->endpoints == NULL)
			return (USBD_NOMEM);
	} else
		ifc->endpoints = NULL;
	ifc->priv = NULL;
	p = (char *)ifc->idesc + ifc->idesc->bLength;
	end = (char *)dev->cdesc + UGETW(dev->cdesc->wTotalLength);
#define ed ((usb_endpoint_descriptor_t *)p)
	for (endpt = 0; endpt < nendpt; endpt++) {
		DPRINTFN(10,("usbd_fill_iface_data: endpt=%d\n", endpt));
		for (; p < end; p += ed->bLength) {
			ed = (usb_endpoint_descriptor_t *)p;
			DPRINTFN(10,("usbd_fill_iface_data: p=%p end=%p len=%d type=%d\n", p, end, ed->bLength, ed->bDescriptorType));
			if (p + ed->bLength <= end && ed->bLength != 0 &&
			    ed->bDescriptorType == UDESC_ENDPOINT)
				goto found;
			if (ed->bDescriptorType == UDESC_INTERFACE ||
			    ed->bLength == 0)
				break;
		}
		/* passed end, or bad desc */
		goto bad;
	found:
		ifc->endpoints[endpt].edesc = ed;
		ifc->endpoints[endpt].state = USBD_ENDPOINT_ACTIVE;
		ifc->endpoints[endpt].refcnt = 0;
		ifc->endpoints[endpt].toggle = 0;
		p += ed->bLength;
	}
#undef ed
	LIST_INIT(&ifc->pipes);
	ifc->state = USBD_INTERFACE_ACTIVE;
	return (USBD_NORMAL_COMPLETION);

 bad:
 if (ifc->endpoints != NULL) {
 		free(ifc->endpoints, M_USB);
 		ifc->endpoints = NULL;
 	}
	return (USBD_INVAL);
}

void
usbd_free_iface_data(dev, ifcno)
	usbd_device_handle dev;
	int ifcno;
{
	usbd_interface_handle ifc = &dev->ifaces[ifcno];
	if (ifc->endpoints)
		free(ifc->endpoints, M_USB);
}

static usbd_status
usbd_set_config(dev, conf)
	usbd_device_handle dev;
	int conf;
{
	usb_device_request_t req;

	req.bmRequestType = UT_WRITE_DEVICE;
	req.bRequest = UR_SET_CONFIG;
	USETW(req.wValue, conf);
	USETW(req.wIndex, 0);
	USETW(req.wLength, 0);
	return (usbd_do_request(dev, &req, 0));
}

usbd_status
usbd_set_config_no(dev, no, msg)
	usbd_device_handle dev;
	int no;
	int msg;
{
	int index;
	usb_config_descriptor_t cd;
	usbd_status r;

	DPRINTFN(5,("usbd_set_config_no: %d\n", no));
	/* Figure out what config index to use. */
	for (index = 0; index < dev->ddesc.bNumConfigurations; index++) {
		r = usbd_get_config_desc(dev, index, &cd);
		if (r != USBD_NORMAL_COMPLETION)
			return (r);
		if (cd.bConfigurationValue == no)
			return (usbd_set_config_index(dev, index, msg));
	}
	return (USBD_INVAL);
}

usbd_status
usbd_set_config_index(dev, index, msg)
	usbd_device_handle dev;
	int index;
	int msg;
{
	usb_status_t ds;
	usb_hub_status_t hs;
	usb_config_descriptor_t cd, *cdp;
	usbd_status r;
	int ifcidx, nifc, len, selfpowered, power;

	DPRINTFN(5,("usbd_set_config_index: dev=%p index=%d\n", dev, index));

	/* XXX check that all interfaces are idle */
	if (dev->config != 0) {
		DPRINTF(("usbd_set_config_index: free old config\n"));
		/* Free all configuration data structures. */
		nifc = dev->cdesc->bNumInterface;
		for (ifcidx = 0; ifcidx < nifc; ifcidx++)
			usbd_free_iface_data(dev, ifcidx);
		free(dev->ifaces, M_USB);
		free(dev->cdesc, M_USB);
		dev->ifaces = 0;
		dev->cdesc = NULL;
		dev->config = 0;
		dev->state = USBD_DEVICE_ADDRESSED;
	}

	/* Figure out what config number to use. */
	r = usbd_get_config_desc(dev, index, &cd);
	if (r != USBD_NORMAL_COMPLETION)
		return (r);
	len = UGETW(cd.wTotalLength);
	cdp = malloc(len, M_USB, M_NOWAIT);
	if (cdp == NULL)
		return (USBD_NOMEM);
	r = usbd_get_desc(dev, UDESC_CONFIG, index, len, cdp);
	if (r != USBD_NORMAL_COMPLETION)
		goto bad;
	if (cdp->bDescriptorType != UDESC_CONFIG) {
		DPRINTFN(-1,("usbd_set_config_index: bad desc %d\n",
			     cdp->bDescriptorType));
		r = USBD_INVAL;
		goto bad;
	}
	selfpowered = 0;
	if (cdp->bmAttributes & UC_SELF_POWERED) {
		/* May be self powered. */
		if (cdp->bmAttributes & UC_BUS_POWERED) {
			/* Must ask device. */
			if (dev->quirks->uq_flags & UQ_HUB_POWER) {
				/* Buggy hub, use hub descriptor. */
				r = usbd_get_hub_status(dev, &hs);
				if (r == USBD_NORMAL_COMPLETION && 
				    !(UGETW(hs.wHubStatus) & UHS_LOCAL_POWER))
					selfpowered = 1;
			} else {
				r = usbd_get_device_status(dev, &ds);
				if (r == USBD_NORMAL_COMPLETION && 
				    (UGETW(ds.wStatus) & UDS_SELF_POWERED))
					selfpowered = 1;
			}
			DPRINTF(("usbd_set_config_index: status=0x%04x, "
				 "error=%d(%s)\n",
				 UGETW(ds.wStatus), r, usbd_error_strs[r]));
		} else
			selfpowered = 1;
	}
	DPRINTF(("usbd_set_config_index: (addr %d) attr=0x%02x, "
		 "selfpowered=%d, power=%d, powerquirk=%x\n", 
		 dev->address, cdp->bmAttributes, 
		 selfpowered, cdp->bMaxPower * 2,
		 dev->quirks->uq_flags & UQ_HUB_POWER));
#ifdef USB_DEBUG
	if (!dev->powersrc) {
		printf("usbd_set_config_index: No power source?\n");
		return (USBD_IOERROR);
	}
#endif
	power = cdp->bMaxPower * 2;
	if (power > dev->powersrc->power) {
		/* XXX print nicer message. */
		if (msg)
			printf("%s: device addr %d (config %d) exceeds power "
				 "budget, %d mA > %d mA\n",
			       dev->bus->bdev.dv_xname, dev->address,
			       cdp->bConfigurationValue, 
			       power, dev->powersrc->power);
		r = USBD_NO_POWER;
		goto bad;
	}
	dev->power = power;
	dev->self_powered = selfpowered;

	DPRINTF(("usbd_set_config_index: set config %d\n",
		 cdp->bConfigurationValue));
	r = usbd_set_config(dev, cdp->bConfigurationValue);
	if (r != USBD_NORMAL_COMPLETION) {
		DPRINTF(("usbd_set_config_index: setting config=%d failed, "
			 "error=%d(%s)\n",
			 cdp->bConfigurationValue, r, usbd_error_strs[r]));
		goto bad;
	}
	DPRINTF(("usbd_set_config_index: setting new config %d\n",
		 cdp->bConfigurationValue));
	nifc = cdp->bNumInterface;
	dev->ifaces = malloc(nifc * sizeof(struct usbd_interface), M_USB, M_NOWAIT);
	if (dev->ifaces == 0) {
		r = USBD_NOMEM;
		goto bad;
	}
	DPRINTFN(5,("usbd_set_config_index: dev=%p cdesc=%p\n", dev, cdp));
	dev->cdesc = cdp;
	dev->config = cdp->bConfigurationValue;
	dev->state = USBD_DEVICE_CONFIGURED;
	for (ifcidx = 0; ifcidx < nifc; ifcidx++) {
		r = usbd_fill_iface_data(dev, ifcidx, 0);
		if (r != USBD_NORMAL_COMPLETION) {
			while (--ifcidx >= 0)
				usbd_free_iface_data(dev, ifcidx);
			goto bad;
		}
	}

	return (USBD_NORMAL_COMPLETION);

 bad:
	free(cdp, M_USB);
	return (r);
}

/* XXX add function for alternate settings */

usbd_status
usbd_setup_pipe(dev, iface, ep, ival, pipe)
	usbd_device_handle dev;
	usbd_interface_handle iface; 
	struct usbd_endpoint *ep;
	int ival;
	usbd_pipe_handle *pipe;
{
	usbd_pipe_handle p;
	usbd_status r;

	DPRINTFN(1,("usbd_setup_pipe: dev=%p iface=%p ep=%p pipe=%p\n", dev, iface, ep, pipe));
	p = malloc(dev->bus->pipe_size, M_USB, M_NOWAIT);
	if (p == 0)
		return (USBD_NOMEM);
	p->device = dev;
	p->iface = iface;
	p->state = USBD_PIPE_ACTIVE;
	p->endpoint = ep;
	ep->refcnt++;
	p->refcnt = 1;
	p->intrxfer = NULL;
	p->running = 0;
	p->aborting = 0;
	p->repeat = 0;
	p->interval = ival;
	SIMPLEQ_INIT(&p->queue);
	r = dev->bus->methods->open_pipe(p);
	if (r != USBD_NORMAL_COMPLETION) {
		DPRINTFN(-1,("usbd_setup_pipe: endpoint=0x%x failed, error=%d (%s)\n", ep->edesc->bEndpointAddress, r, usbd_error_strs[r]));
		free(p, M_USB);
		return (r);
	}
	/* Clear any stall and make sure DATA0 toggle will be used next. */
	if (UE_GET_ADDR(ep->edesc->bEndpointAddress) != USB_CONTROL_ENDPOINT) {
		r = usbd_clear_endpoint_stall(p);
		/* Some devices reject this command, so ignore a STALL. */
		if (r && r != USBD_STALLED) {
			printf("usbd_setup_pipe: failed to start endpoint, %s\n", usbd_errstr(r));
			return (r);
		}
	}
	*pipe = p;
	return (USBD_NORMAL_COMPLETION);
}

/* Abort the device control pipe. */
void
usbd_kill_pipe(pipe)
	usbd_pipe_handle pipe;
{
	usbd_abort_pipe(pipe);
	pipe->methods->close(pipe);
	pipe->endpoint->refcnt--;
	free(pipe, M_USB);
}

int
usbd_getnewaddr(bus)
	usbd_bus_handle bus;
{
	int addr;

	for (addr = 1; addr < USB_MAX_DEVICES; addr++)
		if (bus->devices[addr] == 0)
			return (addr);
	return (-1);
}


usbd_status
usbd_probe_and_attach(parent, dev, port, addr)
	struct device *parent;
	usbd_device_handle dev;
	int port;
	int addr;
{
	struct usb_attach_arg uaa;
	usb_device_descriptor_t *dd = &dev->ddesc;
	struct device *dv;
	int r, found, i, confi, nifaces;
	usbd_interface_handle ifaces[256]; /* 256 is the absolute max */

	uaa.device = dev;
	uaa.iface = NULL;
	uaa.ifaces = NULL;
	uaa.nifaces = 0;
	uaa.usegeneric = 0;
	uaa.port = port;
	uaa.configno = UHUB_UNK_CONFIGURATION;
	uaa.ifaceno = UHUB_UNK_INTERFACE;
	uaa.vendor = UGETW(dd->idVendor);
	uaa.product = UGETW(dd->idProduct);
	uaa.release = UGETW(dd->bcdDevice);

	/* First try with device specific drivers. */
	DPRINTF(("usbd_probe_and_attach: trying device specific drivers\n"));
	dv = config_found_sm(parent, &uaa, usbd_print, usbd_submatch);
	if (dv) {
		dev->subdevs = malloc(2 * sizeof dv, M_USB, M_NOWAIT);
		if (dev->subdevs == NULL)
			return (USBD_NOMEM);
		dev->subdevs[0] = dv;
		dev->subdevs[1] = 0;
		return (USBD_NORMAL_COMPLETION);
	}

	DPRINTF(("usbd_probe_and_attach: no device specific driver found\n"));

	DPRINTF(("usbd_probe_and_attach: looping over %d configurations\n", dd->bNumConfigurations));

	/* Next try with interface drivers. */
	for (confi = 0; confi < dd->bNumConfigurations; confi++) {
		DPRINTFN(1,("usbd_probe_and_attach: trying config idx=%d\n", confi));
		r = usbd_set_config_index(dev, confi, 1);
		if (r != USBD_NORMAL_COMPLETION) {
#ifdef USB_DEBUG
			DPRINTF(("%s: port %d, set config at addr %d failed, "
				 "error=%d(%s)\n", *parent->dv_xname, port,
				 addr, r, usbd_error_strs[r]));
#else
			printf("%s: port %d, set config at addr %d failed\n", *parent->dv_xname, port, addr);
#endif
 			return (r);
		}
		nifaces = dev->cdesc->bNumInterface;
		uaa.configno = dev->cdesc->bConfigurationValue;
		for (i = 0; i < nifaces; i++)
			ifaces[i] = &dev->ifaces[i];
		uaa.ifaces = ifaces;
		uaa.nifaces = nifaces;
		dev->subdevs = malloc((nifaces+1) * sizeof dv, M_USB,M_NOWAIT);
		found = 0;
		for (found = i = 0; i < nifaces; i++) {
			if (!ifaces[i])
				continue; /* interface already claimed */
			uaa.iface = ifaces[i];
			uaa.ifaceno = ifaces[i]->idesc->bInterfaceNumber;
			dv = config_found_sm(parent, &uaa, usbd_print, usbd_submatch);
			if (dv != NULL) {
				dev->subdevs[found++] = dv;
				dev->subdevs[found] = 0;
				ifaces[i] = 0; /* consumed */
			}
		}
		if (found != 0)
			return (USBD_NORMAL_COMPLETION);
	}
	free(dev->subdevs, M_USB);
	dev->subdevs = 0;

	/* No interfaces were attached in any of the configurations. */
	if (dd->bNumConfigurations > 1)/* don't change if only 1 config */
		usbd_set_config_index(dev, 0, 0);

	DPRINTF(("usbd_probe_and_attach: no interface drivers found\n"));

	/* Finally try the generic driver. */
	uaa.iface = 0;
	uaa.usegeneric = 1;
	uaa.configno = UHUB_UNK_CONFIGURATION;
	uaa.ifaceno = UHUB_UNK_INTERFACE;
	dv = config_found_sm(parent, &uaa, usbd_print, usbd_submatch);
	if (dv != NULL) {
		dev->subdevs = malloc(2 * sizeof dv, M_USB, M_NOWAIT);
		if (dev->subdevs == 0)
			return (USBD_NOMEM);
		dev->subdevs[0] = dv;
		dev->subdevs[1] = 0;
		return (USBD_NORMAL_COMPLETION);
	}

	/* 
	 * The generic attach failed, but leave the device as it is.
	 * We just did not find any drivers, that's all.  The device is
	 * fully operational and not harming anyone.
	 */
	DPRINTF(("usbd_probe_and_attach: generic attach failed\n"));
 	return (USBD_NORMAL_COMPLETION);
}



/*
 * Called when a new device has been put in the powered state,
 * but not yet in the addressed state.
 * Get initial descriptor, set the address, get full descriptor,
 * and attach a driver.
 */
usbd_status
usbd_new_device(parent, bus, depth, lowspeed, port, up)
	struct device *parent;
	usbd_bus_handle bus;
	int depth;
	int lowspeed;
	int port;
	struct usbd_port *up;
{
	struct usbd_device *hub;
	usbd_device_handle dev;
	usb_device_descriptor_t *dd;
	usb_port_status_t ps;
	usbd_status r;
	int addr;
	int i;

	DPRINTF(("usbd_new_device bus=%p depth=%d lowspeed=%d\n", bus, depth, lowspeed));
	addr = usbd_getnewaddr(bus);
	if (addr < 0) {
		printf("%s: No free USB addresses, new device ignored.\n", bus->bdev.dv_xname);
		return (USBD_NO_ADDR);
	}

	dev = malloc(sizeof *dev, M_USB, M_NOWAIT|M_ZERO);
	if (dev == 0)
		return (USBD_NOMEM);

	dev->bus = bus;

	/* Set up default endpoint handle. */
	dev->def_ep.edesc = &dev->def_ep_desc;
	dev->def_ep.state = USBD_ENDPOINT_ACTIVE;

	/* Set up default endpoint descriptor. */
	dev->def_ep_desc.bLength = USB_ENDPOINT_DESCRIPTOR_SIZE;
	dev->def_ep_desc.bDescriptorType = UDESC_ENDPOINT;
	dev->def_ep_desc.bEndpointAddress = USB_CONTROL_ENDPOINT;
	dev->def_ep_desc.bmAttributes = UE_CONTROL;
	USETW(dev->def_ep_desc.wMaxPacketSize, USB_MAX_IPACKET);
	dev->def_ep_desc.bInterval = 0;

	dev->state = USBD_DEVICE_DEFAULT;
	dev->quirks = &usbd_no_quirk;
	dev->address = USB_START_ADDR;
	dev->ddesc.bMaxPacketSize = 0;
	dev->speed = lowspeed;
	dev->depth = depth;
	dev->powersrc = up;
	dev->langid = USBD_NOLANG;
	dev->myhub = up->parent;
	for (hub = up->parent; hub != NULL && hub->speed != USB_SPEED_HIGH; hub =
			hub->myhub)
		;
	dev->myhighhub = hub;
	dev->speed = lowspeed;
	dev->langid = USBD_NOLANG;
	//dev->cookie.cookie = ++usb_cookie_no;

	/* Establish the the default pipe. */
	r = usbd_setup_pipe(dev, 0, &dev->def_ep, USBD_DEFAULT_INTERVAL, &dev->default_pipe);
	if (r != USBD_NORMAL_COMPLETION) {
		usbd_remove_device(dev, up);
		return (r);
	}

	up->device = dev;

	/* Set the address.  Do this early; some devices need that. */
	r = usbd_set_address(dev, addr);
	DPRINTFN(5,("usbd_new_device: setting device address=%d\n", addr));
	if (r) {
		DPRINTFN(-1,("usb_new_device: set address %d failed\n", addr));
		r = USBD_SET_ADDR_FAILED;
		usbd_remove_device(dev, up);
		return (r);
	}

	/* Allow device time to set new address */
	usbd_delay_ms(dev, USB_SET_ADDRESS_SETTLE);
	dev->address = addr;	/* New device address now */
	bus->devices[addr] = dev;

	dd = &dev->ddesc;
	/* Try a few times in case the device is slow (i.e. outside specs.) */
	for (i = 0; i < 10; i++) {
		/* Get the first 8 bytes of the device descriptor. */
		r = usbd_get_desc(dev, UDESC_DEVICE, 0, USB_MAX_IPACKET, dd);
		if (r == USBD_NORMAL_COMPLETION)
			break;
		usbd_delay_ms(dev, 200);
		if ((i & 3) == 3)
			usbd_reset_port(up->parent, port, &ps);
	}
	if (r != USBD_NORMAL_COMPLETION) {
		DPRINTFN(-1, ("usbd_new_device: addr=%d, getting first desc failed\n", addr));
		usbd_remove_device(dev, up);
		return (r);
	}

	if (lowspeed == USB_SPEED_HIGH) {
		/* Max packet size must be 64 (sec 5.5.3). */
		if (dd->bMaxPacketSize != USB_2_MAX_CTRL_PACKET) {
#ifdef DIAGNOSTIC
			printf("usbd_new_device: addr=%d bad max packet size\n", addr);
#endif
			dd->bMaxPacketSize = USB_2_MAX_CTRL_PACKET;
		}
	}

	DPRINTF(("usbd_new_device: adding unit addr=%d, rev=%02x, class=%d, "
		 "subclass=%d, protocol=%d, maxpacket=%d, len=%d, speed=%d\n",
		 addr,UGETW(dd->bcdUSB), dd->bDeviceClass, dd->bDeviceSubClass,
		 dd->bDeviceProtocol, dd->bMaxPacketSize, dd->bLength,
		 dev->speed));

	if (dd->bDescriptorType != UDESC_DEVICE) {
		/* Illegal device descriptor */
		DPRINTFN(-1,("usbd_new_device: illegal descriptor %d\n", dd->bDescriptorType));
		usbd_remove_device(dev, up);
		return (USBD_INVAL);
	}

	if (dd->bLength < USB_DEVICE_DESCRIPTOR_SIZE) {
		DPRINTFN(-1,("usbd_new_device: bad length %d\n", dd->bLength));
		usbd_remove_device(dev, up);
		return (USBD_INVAL);
	}

	USETW(dev->def_ep_desc.wMaxPacketSize, dd->bMaxPacketSize);

	/* Get the full device descriptor. */
	r = usbd_reload_device_desc(dev, dd);
	if (r != USBD_NORMAL_COMPLETION) {
		DPRINTFN(-1, ("usbd_new_device: addr=%d, getting full desc failed\n", addr));
		usbd_remove_device(dev, up);
		return (r);
	}

	/* Figure out what's wrong with this device. */
	dev->quirks = usbd_find_quirk(dd);

	/* Set the address */
	r = usbd_set_address(dev, addr);
	if (r != USBD_NORMAL_COMPLETION) {
		DPRINTFN(-1,("usb_new_device: set address %d failed\n",addr));
		r = USBD_SET_ADDR_FAILED;
		usbd_remove_device(dev, up);
		return (r);
	}

	/* Assume 100mA bus powered for now. Changed when configured. */
	dev->power = USB_MIN_POWER;
	dev->self_powered = 0;

	DPRINTF(("usbd_new_device: new dev (addr %d), dev=%p, parent=%p\n", addr, dev, parent));

	r = usbd_probe_and_attach(parent, dev, port, addr);
	if (r != USBD_NORMAL_COMPLETION) {
		usbd_remove_device(dev, up);
		return (r);
  	}
  
  	return (USBD_NORMAL_COMPLETION);
}

usbd_status
usbd_reload_device_desc(usbd_device_handle dev)
{
	usbd_status err;

	/* Get the full device descriptor. */
	err = usbd_get_device_desc(dev, &dev->ddesc);
	if (err)
		return (err);

	/* Figure out what's wrong with this device. */
	dev->quirks = usbd_find_quirk(&dev->ddesc);

	return (USBD_NORMAL_COMPLETION);
}

void
usbd_remove_device(dev, up)
	usbd_device_handle dev;
	struct usbd_port *up;
{
	DPRINTF(("usbd_remove_device: %p\n", dev));
  
	if (dev->default_pipe)
		usbd_kill_pipe(dev->default_pipe);
	up->device = 0;
	dev->bus->devices[dev->address] = 0;

	free(dev, M_USB);
}

int
usbd_print(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct usb_attach_arg *uaa = aux;
	char devinfo[1024];

	DPRINTFN(15, ("usbd_print dev=%p\n", uaa->device));
	if (pnp) {
		if (!uaa->usegeneric)
			return (QUIET);
		usbd_devinfo(uaa->device, 1, devinfo);
		printf("%s, %s", devinfo, pnp);
	}
	if (uaa->port != 0)
		printf(" port %d", uaa->port);
	if (uaa->configno != UHUB_UNK_CONFIGURATION)
		printf(" configuration %d", uaa->configno);
	if (uaa->ifaceno != UHUB_UNK_INTERFACE)
		printf(" interface %d", uaa->ifaceno);
#if 0
	/*
	 * It gets very crowded with these locators on the attach line.
	 * They are not really needed since they are printed in the clear
	 * by each driver.
	 */
	if (uaa->vendor != UHUB_UNK_VENDOR)
		printf(" vendor 0x%04x", uaa->vendor);
	if (uaa->product != UHUB_UNK_PRODUCT)
		printf(" product 0x%04x", uaa->product);
	if (uaa->release != UHUB_UNK_RELEASE)
		printf(" release 0x%04x", uaa->release);
#endif
	return (UNCONF);
}

int
usbd_submatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct usb_attach_arg *uaa = aux;

	if ((uaa->port != 0 &&
	     cf->cf_loc[UHUBCF_PORT] != UHUB_UNK_PORT &&
	     cf->cf_loc[UHUBCF_PORT] != uaa->port) ||
	    (uaa->configno != UHUB_UNK_CONFIGURATION &&
	     cf->cf_loc[UHUBCF_CONFIGURATION] != UHUB_UNK_CONFIGURATION &&
	     cf->cf_loc[UHUBCF_CONFIGURATION] != uaa->configno) ||
	    (uaa->ifaceno != UHUB_UNK_INTERFACE &&
	     cf->cf_loc[UHUBCF_INTERFACE] != UHUB_UNK_INTERFACE &&
	     cf->cf_loc[UHUBCF_INTERFACE] != uaa->ifaceno) ||
		(uaa->vendor != UHUB_UNK_VENDOR &&
		 cf->cf_loc[UHUBCF_VENDOR] != UHUB_UNK_VENDOR &&
		 cf->cf_loc[UHUBCF_VENDOR] != uaa->vendor) ||
		(uaa->product != UHUB_UNK_PRODUCT &&
		 cf->cf_loc[UHUBCF_PRODUCT] != UHUB_UNK_PRODUCT &&
		 cf->cf_loc[UHUBCF_PRODUCT] != uaa->product) ||
		(uaa->release != UHUB_UNK_RELEASE &&
		 cf->cf_loc[UHUBCF_RELEASE] != UHUB_UNK_RELEASE &&
		 cf->cf_loc[UHUBCF_RELEASE] != uaa->release)
		 )
		return 0;

	return (config_match(parent, cf, aux));
}

void
usbd_fill_deviceinfo(dev, di, usedev)
	usbd_device_handle dev;
	struct usb_device_info *di;
	int usedev;
{
	struct usbd_port *p;
	int i, r, s;

	di->udi_bus = dev->bus->bdev.dv_unit;
	di->udi_addr = dev->address;
	di->udi_cookie  = dev->config;
	usbd_devinfo_vp(dev, di->udi_vendor, di->udi_product, usedev);
	usbd_printBCD(di->udi_release, UGETW(dev->ddesc.bcdDevice));
	di->udi_vendorNo = UGETW(dev->ddesc.idVendor);
	di->udi_productNo = UGETW(dev->ddesc.idProduct);
	di->udi_releaseNo = UGETW(dev->ddesc.bcdDevice);
	di->udi_class = dev->ddesc.bDeviceClass;
	di->udi_subclass = dev->ddesc.bDeviceSubClass;
	di->udi_protocol = dev->ddesc.bDeviceProtocol;
	di->udi_config = dev->config;
	di->udi_power = dev->self_powered ? 0 : dev->power;
	di->udi_speed = dev->speed;

	if (dev->subdevs != NULL) {
		for (i = 0; dev->subdevs[i] && i < USB_MAX_DEVNAMES; i++) {
			strncpy(di->udi_devnames[i], dev->subdevs[i].dv_xname, USB_MAX_DEVNAMELEN);
			di->udi_devnames[i][USB_MAX_DEVNAMELEN - 1] = '\0';
		}
	} else {
		i = 0;
	}
	 for (/*i is set */; i < USB_MAX_DEVNAMES; i++)
		 di->udi_devnames[i][0] = 0;                 /* empty */

	if (dev->hub) {
		for (i = 0; 
		     i < sizeof(di->udi_ports) / sizeof(di->udi_ports[0]) &&
			     i < dev->hub->hubdesc.bNbrPorts;
		     i++) {
			p = &dev->hub->ports[i];
			if (p->device)
				r = p->device->address;
			else {
				s = UGETW(p->status.wPortStatus);
				if (s & UPS_PORT_ENABLED)
					r = USB_PORT_ENABLED;
				else if (s & UPS_SUSPEND)
					r = USB_PORT_SUSPENDED;
				else if (s & UPS_PORT_POWER)
					r = USB_PORT_POWERED;
				else
					r = USB_PORT_DISABLED;
			}
			di->udi_ports[i] = r;
		}
		di->udi_nports = dev->hub->hubdesc.bNbrPorts;
	} else
		di->udi_nports = 0;
}

void
usb_free_device(usbd_device_handle dev)
{
	int ifcidx, nifc;

	if (dev->default_pipe != NULL)
		usbd_kill_pipe(dev->default_pipe);
	if (dev->ifaces != NULL) {
		nifc = dev->cdesc->bNumInterface;
		for (ifcidx = 0; ifcidx < nifc; ifcidx++)
			usbd_free_iface_data(dev, ifcidx);
		free(dev->ifaces, M_USB);
	}
	if (dev->cdesc != NULL)
		free(dev->cdesc, M_USB);
	if (dev->subdevs != NULL)
		free(dev->subdevs, M_USB);
	free(dev, M_USB);
}

/*
 * Called from process context when we discover that a port has
 * been disconnected.
 */
void
usb_disconnect_port(struct usbd_port *up, struct device *parent)
{
	usbd_device_handle dev = up->device;
	char *hubname = USBDEVPTRNAME(parent);
	int i;

	DPRINTFN(3,("uhub_disconnect: up=%p dev=%p port=%d\n", up, dev, up->portno));

#ifdef DIAGNOSTIC
	if (dev == NULL) {
		printf("usb_disconnect_port: no device\n");
		return;
	}
#endif

	if (dev->subdevs != NULL) {
		DPRINTFN(3,("usb_disconnect_port: disconnect subdevs\n"));
		for (i = 0; dev->subdevs[i]; i++) {
			printf("%s: at %s", USBDEVPTRNAME(dev->subdevs[i]),
			       hubname);
			if (up->portno != 0)
				printf(" port %d", up->portno);
			printf(" (addr %d) disconnected\n", dev->address);
			config_detach(dev->subdevs[i], DETACH_FORCE);
			dev->subdevs[i] = 0;
		}
	}

	usbd_add_dev_event(USB_EVENT_DEVICE_DETACH, dev);
	dev->bus->devices[dev->address] = NULL;
	up->device = NULL;
	usb_free_device(dev);
}
