/*	$NetBSD: usbdivar.h,v 1.17.2.1 1999/05/06 19:32:00 perry Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (augustss@carlstedt.se) at
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

#ifndef _USBDIVAR_H_
#define _USBDIVAR_H_

/* From usb_mem.h */
struct usb_dma_block;
struct usb_dma {
	struct usb_dma_block			*block;
	u_int			 				offs;
#define USB_DMA_COHERENT			(1 << 0)
};

struct usbd_request;
struct usbd_pipe;

struct usbd_endpoint {
	usb_endpoint_descriptor_t 		*edesc;
	usbd_endpoint_state				state;
	int								refcnt;
	int								toggle;	/* XXX */
};

typedef void (*usbd_xfercb)(usbd_request_handle req);

struct usbd_methods {
	usbd_status	      				(*transfer)(usbd_request_handle reqh);
	usbd_status	      				(*start)(usbd_request_handle reqh);
	void		      				(*abort)(usbd_request_handle reqh);
	void		      				(*close)(usbd_pipe_handle pipe);
	usbd_status	      				(*isobuf)(usbd_pipe_handle pipe, u_int32_t bufsize,u_int32_t nbuf);
};

struct usbd_port {
	usb_port_status_t				status;
	u_int16_t						power;	/* mA of current on port */
	u_int8_t						portno;
	u_int8_t						restartcnt;
#define USBD_RESTART_MAX 5
	struct usbd_device  			*device;
	struct usbd_device  			*parent;	/* The ports hub */
};

struct usbd_hub {
	usbd_status	     				(*explore)(usbd_device_handle hub);
	void		      	 			*hubsoftc;
	usb_hub_descriptor_t			hubdesc;
	struct usbd_port        		ports[1];
};

struct usb_softc;

/*****/

struct usbd_bus {
	/* Filled by HC driver */
	struct device					bdev; /* base device, host adapter */
	usbd_status	      				(*open_pipe)(struct usbd_pipe *pipe);
	u_int32_t						pipe_size; /* size of a pipe struct */
	void		      				(*do_poll)(struct usbd_bus *);
	int								has_console; /* console input on this bus */
	/* Filled by usb driver */
	struct usbd_device     			*root_hub;
	usbd_device_handle				devices[USB_MAX_DEVICES];
	char							needs_explore;/* a hub a signalled a change */
	char							use_polling;
	char							dying;
	struct usb_softc       			*usbctl;
	struct usb_device_stats			stats;
	int 							intr_context;
	u_int							no_intrs;
	int								usbrev;	/* USB revision */
#define USBREV_UNKNOWN				0
#define USBREV_PRE_1_0				1
#define USBREV_1_0					2
#define USBREV_1_1					3
#define USBREV_2_0					4
#define USBREV_3_0					5
#define USBREV_STR { "unknown", "pre 1.0", "1.0", "1.1", "2.0", "3.0" }
	void		       				*soft; /* soft interrupt cookie */
	bus_dma_tag_t					dmatag;	/* DMA tag */
};

struct usbd_device {
	struct usbd_bus	       			*bus;
	usbd_device_state				state;
	struct usbd_pipe       			*default_pipe;
	u_int8_t						address;
	u_int8_t						depth;
	u_int8_t						lowspeed;
	u_int16_t						power;
	u_int8_t						self_powered;
	int								config;
	int								langid;			/* language to use for strings */
#define USBD_NOLANG (-1)
	struct usbd_port       			*powersrc;
	struct usbd_endpoint			def_ep;			/* for pipe 0 */
	usb_endpoint_descriptor_t 		def_ep_desc; 	/* for pipe 0 */
	struct usbd_interface  			*ifaces;
	usb_device_descriptor_t 		ddesc;
	usb_config_descriptor_t 		*cdesc;			/* full config descr */
	struct usbd_quirks     			*quirks;
	struct usbd_hub	       			*hub; 			/* only if this is a hub */
	void		       				*softc;			/* device softc if attached */
};

struct usbd_interface {
	struct usbd_device     			*device;
	usbd_interface_state			state;
	usb_interface_descriptor_t 		*idesc;
	int								index;
	int								altindex;
	struct usbd_endpoint   			*endpoints;
	void		       				*priv;
	LIST_HEAD(, usbd_pipe)			pipes;
};

struct usbd_pipe {
	struct usbd_interface  			*iface;
	struct usbd_device     			*device;
	struct usbd_endpoint   			*endpoint;
	usbd_pipe_state					state;
	int32_t							refcnt;
	char							running;
	SIMPLEQ_HEAD(, usbd_request) 	queue;
	LIST_ENTRY(usbd_pipe)			next;

	void		     				(*disco) (void *);
	void		       				*discoarg;

	usbd_request_handle     		intrreqh; /* used for repeating requests */

	/* Filled by HC driver. */
	struct usbd_methods    			*methods;
};

struct usbd_request {
	struct usbd_pipe    			*pipe;
	void		       				*priv;
	void		       				*buffer;
	u_int32_t						length;
	u_int32_t						actlen;
	u_int16_t						flags;
	u_int32_t						timeout;
	usbd_status						status;
	usbd_callback					callback;
	usbd_xfercb						xfercb;
	u_int32_t						retries;
	char							done;

	usb_device_request_t			request;
	u_int8_t						isreq;

	SIMPLEQ_ENTRY(usbd_request) 	next;

	void		       				*hcpriv; /* XXX private use by the HC driver */
};

void 		usbd_init (void);

/* Routines from usb_subr.c */
int			usbctlprint (void *, const char *);
void		usb_delay_ms (usbd_bus_handle, u_int);
void		usbd_devinfo_vp (usbd_device_handle, char *, char *);
usbd_status	usbd_reset_port (usbd_device_handle dev, int port, usb_port_status_t *ps);
usbd_status	usbd_setup_pipe (usbd_device_handle dev, usbd_interface_handle iface, struct usbd_endpoint *, usbd_pipe_handle *pipe);
usbd_status	usbd_new_device (struct device *parent, usbd_bus_handle bus, int depth, int lowspeed, int port, struct usbd_port *);
void		usbd_remove_device (usbd_device_handle, struct usbd_port *);
int			usbd_printBCD (char *cp, int bcd);
usbd_status	usb_insert_transfer (usbd_request_handle reqh);
void		usb_start_next (usbd_pipe_handle pipe);
usbd_status	usbd_fill_iface_data (usbd_device_handle dev, int i, int a);

/* Routines from usb.c */
int			usb_bus_count (void);
void		usb_needs_explore (usbd_bus_handle);
#if 0
usbd_status	usb_get_bus_handle (int, usbd_bus_handle *);
#endif

/* Locator stuff. */

#define USBTAP_DIR_OUT					0
#define USBTAP_DIR_IN					1

#define UHUBCF_PORT_DEFAULT 			-1
#define UHUBCF_CONFIGURATION_DEFAULT 	-1
#define UHUBCF_INTERFACE_DEFAULT 		-1
#define	UHUBCF_PORT						1
#define	UHUBCF_CONFIGURATION			1
#define	UHUBCF_INTERFACE				1
#define	UHUB_UNK_PORT					UHUBCF_PORT_DEFAULT 			/* wildcarded 'port' */
#define	UHUB_UNK_CONFIGURATION			UHUBCF_CONFIGURATION_DEFAULT 	/* wildcarded 'configuration' */
#define	UHUB_UNK_INTERFACE				UHUBCF_INTERFACE_DEFAULT 		/* wildcarded 'interface' */
