// SPDX-License-Identifier: GPL-2.0
// Copyright © 2009-2018 ANSSI. All Rights Reserved.
/**
 * @file eventd.h.
 * CLIP eventd header.
 * @author Vincent Strubel <clipos@ssi.gouv.fr>
 *
 * Copyright (C) 2010 ANSSI
 * @n
 * All rights reserved.
 */

#ifndef _CLIP_EVENTD_H
#define _CLIP_EVENTD_H

#include <linux/types.h>
#include "common.h"

extern int g_foreground;


/* Input event handler */
struct _event;
typedef int (*input_handler_t)(struct _event *, 
		__u16 type, __u16 code, __s32 value);
struct nlmsghdr;
typedef int (*netlink_handler_t)(struct _event *, 
		struct nlmsghdr *hdr);

typedef int (*socket_handler_t)(struct _event *);

typedef enum {
	InputEvent,
	NetlinkEvent,
	SocketEvent,
} EventType;

/* Event device */
typedef struct _event {
	int fd;
	EventType evtype;
	union {
		input_handler_t input_handler;
		netlink_handler_t netlink_handler;
		socket_handler_t socket_handler;
	};
	__u16 type;
	__u16 code;
	__s32 value;
	const char *path;
} eventfd;

extern void destroy_eventfd(eventfd *efd);

/*
 * Input events.
 */
typedef enum {
	InputEventLid = 0,
	InputEventPower,
	InputEventSleep,
	InputEventTablet,
	InputEventFunctionKey,
	InputEventDebug,
	InputEventMax
} input_event_t;

extern int open_input_eventfd(eventfd *);
extern int read_input_event(eventfd *);
extern eventfd *create_input_eventfd(input_event_t type, const char *path); 


/*
 * Netlink events.
 */
typedef enum {
	NetlinkEventUpdown = 0,
	NetlinkEventMax
} netlink_event_t;

extern int open_netlink_eventfd(eventfd *);
extern int read_netlink_event(eventfd *);
extern eventfd *create_netlink_eventfd(netlink_event_t type);

/*
 * Socket events.
 */
typedef enum {
	SocketEventSleep = 0,
	SocketEventMax
} socket_event_t;

extern int open_socket_eventfd(eventfd *);
extern int read_socket_event(eventfd *);
extern eventfd *create_socket_eventfd(socket_event_t type, const char *path);



#endif /* _CLIP_EVENTD_H */
