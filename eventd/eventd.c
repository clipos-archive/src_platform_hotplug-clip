// SPDX-License-Identifier: GPL-2.0
// Copyright © 2009-2018 ANSSI. All Rights Reserved.
/**
 * @file eventd.c
 * CLIP eventd main.
 * @author Vincent Strubel <clipos@ssi.gouv.fr>
 *
 * Copyright (C) 2010-2012 SGDSN/ANSSI
 * @n
 * All rights reserved.
 */


#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/socket.h>
#include <clip/clip.h>
#include <linux/rtnetlink.h>
#include <linux/if.h>

#include "eventd.h"

#define MAX_EVENTS 	16U

int g_foreground = 1;

static int
open_eventfd(eventfd *efd)
{
	switch (efd->evtype) {
		case InputEvent:
			return open_input_eventfd(efd);
		case NetlinkEvent:
			return open_netlink_eventfd(efd);
		case SocketEvent:
			return open_socket_eventfd(efd);
		default:
			ERROR("Unsupported event descriptor type : %d\n", efd->evtype);
			return -1;
	}
}

void
destroy_eventfd(eventfd *efd)
{
	if (efd->fd)
		close(efd->fd);
	free(efd);
}

static int
read_event(eventfd *efd)
{
	switch (efd->evtype) {
		case InputEvent:
			return read_input_event(efd);
		case NetlinkEvent:
			return read_netlink_event(efd);
		case SocketEvent:
			return read_socket_event(efd);
		default:
			ERROR("Unsupported event descriptor type : %d\n", efd->evtype);
			return -1;
	}
}

static int
select_events(eventfd *events[])
{
	eventfd *efd;
	int nfd = 0, ret;
	unsigned int i;
	fd_set set;

	FD_ZERO(&set);
	for (i = 0; i < MAX_EVENTS; i++) {
		efd = events[i];
		if (!efd)
			continue;
		FD_SET(efd->fd, &set);
		if (efd->fd > nfd)
			nfd = efd->fd;
	}
	nfd++;

	ret = select(nfd, &set, NULL, NULL, NULL);
	if (ret < 0) {
		if (errno == EINTR)
			return 0;
		ERROR_ERRNO("select failed");
		return -1;
	}
	if (!ret) {
		ERROR("select returned with no event ??");
		return -1;
	}

	for (i = 0; i < MAX_EVENTS; i++) {
		efd = events[i];
		if (!efd)
			continue;
		if (!FD_ISSET(efd->fd, &set))
			continue;
		if (read_event(efd) == -2)
			events[i] = NULL; /* Device was removed... */
	}

	return 0;
}

static inline void
print_help(const char *prog)
{
	printf("%s [-h] [-s/l/p/r/i/t <path>]\n", prog);
	puts("Options:");
	puts("\t-h        : print this help and exit");
	puts("\t-N        : listen for network link up/down events");
	puts("\t-l <path> : listen for lid events on <path>");
	puts("\t            (\\_SB_.LID_)");
	puts("\t-p <path> : listen for power events on <path>");
	puts("\t            (\\_SB_.SLPB or \\_SB_.PBTN)");
	puts("\t-s <path> : listen for sleep events on <path>");
	puts("\t            (\\_SB_.SBTN)");
	puts("\t-S <path> : listen for sleep requests on socket <path>");
	puts("\t-t <path> : listen for tablet events on <path>");
	puts("\t-k <path> : listen for \"function key\" events on <path>");
	puts("\t-d <path> : listen for all events on <path>, and log them");
}

#define create_event(type, subtype, path) do {\
	if (curev >= MAX_EVENTS) { \
		ERROR("Too many events\n"); \
		goto err; \
	} \
	if (type == InputEvent) \
		efd = create_input_eventfd(subtype, path); \
	else if (type == NetlinkEvent) \
		efd = create_netlink_eventfd(subtype); \
	else if (type == SocketEvent) \
		efd = create_socket_eventfd(subtype, path); \
	else { \
		ERROR("WTF"); \
		goto err; \
	} \
	if (!efd) \
		goto err; \
	events[curev] = efd; \
	curev++; \
} while (0)

int
main(int argc, char *argv[])
{
	int c, foreground = 0;
	eventfd *efd;
	eventfd *events[MAX_EVENTS];
	unsigned int curev = 0, i;
	const char *prog = basename(argv[0]);

	for (i = 0; i < MAX_EVENTS; i++)
		events[i] = 0;

	while ((c = getopt(argc, argv, "Fhd:k:l:Np:s:S:i:t:")) != -1) {
		switch (c) {
			case 'F':
				foreground = 1;
				break;
			case 'h':
				print_help(prog);
				return EXIT_SUCCESS;
				break;
			case 'l':
				create_event(InputEvent, InputEventLid, optarg);
				break;
			case 'N':
				create_event(NetlinkEvent, NetlinkEventUpdown, NULL);
				break;
			case 'p':
				create_event(InputEvent, InputEventPower, optarg);
				break;
			case 's':
				create_event(InputEvent, InputEventSleep, optarg);
				break;
			case 'S':
				create_event(SocketEvent, SocketEventSleep, optarg);
				break;
			case 't':
				create_event(InputEvent, InputEventTablet, optarg);
				break;
			case 'k':
				create_event(InputEvent, InputEventFunctionKey, optarg);
				break;
			case 'd':
				create_event(InputEvent, InputEventDebug, optarg);
				break;
			default:
				ERROR("Unsupported option: %c", c);
				goto err;
		}
	}

	if (!curev) {
		ERROR("No event descriptor");
		goto err;
	}

	if (!foreground) {
		if (clip_daemonize()) {
			ERROR("Failed to daemonize");
			goto err;
		}
		g_foreground = 0;
		openlog("eventd", LOG_CONS|LOG_NDELAY|LOG_PID, LOG_DAEMON);
	}

	for (i = 0; i < MAX_EVENTS && events[i]; i++) {
		if (open_eventfd(events[i])) {
			ERROR("Failed to open event fd");
			goto err;
		}
	}

	for (;;) {
		(void)select_events(events);
	}

err:
	for (i = 0; i < MAX_EVENTS; i++) {
		if (events[i])
			destroy_eventfd(events[i]);
	}
	return EXIT_FAILURE;
}
