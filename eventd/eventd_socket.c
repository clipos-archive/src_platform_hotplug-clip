// SPDX-License-Identifier: GPL-2.0
// Copyright © 2009-2018 ANSSI. All Rights Reserved.
/**
 * @file eventd_socket.c
 * CLIP eventd socket handlers.
 * @author Vincent Strubel <clipos@ssi.gouv.fr>
 *
 * Copyright (C) 2013 SGDSN/ANSSI
 * @n
 * All rights reserved.
 */


#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "eventd.h"

#define CMD_SUSPEND 	"/usr/bin/clip-suspend"

/*************************************************************/
/*                       Handlers                            */
/*************************************************************/


static int
socket_sleep_handler(eventfd *efd)
{
	LOG("running %s for socket handler %s", CMD_SUSPEND, efd->path);
	return run_external((void *)CMD_SUSPEND, NULL, NULL);
}

/*************************************************************/
/*                       Interface                           */
/*************************************************************/

int
open_socket_eventfd(eventfd *efd)
{
	int fd;
	size_t slen;
	struct sockaddr_un addr;
	mode_t mask;

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	slen = snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", efd->path);
	if (slen >= sizeof(addr.sun_path)) {
		ERROR("Socket path is too long: %s", efd->path);
		return -1;
	}

	fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if (fd < 0) {
		ERROR_ERRNO("failed to create unix socket");
		return -1;
	}

	(void)unlink(efd->path);

	mask = umask(0);
	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr))) {
		ERROR_ERRNO("failed to bind unix socket");
		(void)umask(mask);
		goto err;
	}
	(void)umask(mask);

	if (listen(fd, 0)) {
		ERROR_ERRNO("failed to listen on unix socket");
		goto err;
	}

	efd->fd = fd;
	return 0;

err:
	(void)close(fd);
	return -1;
}

int
read_socket_event(eventfd *efd)
{
	int sock, ret = -1;
	char c = 'Y';

	sock = accept(efd->fd, NULL, NULL);
	if (sock < 0) {
		ERROR_ERRNO("failed to accept connection on socket %s", efd->path);
		return -1;
	}

	/* We notify the client immediately so as not to block until resume */
	if (write(sock, &c, 1) != 1) {
		ERROR_ERRNO("failed to write answer %c on socket %s", c, efd->path);
		goto out;
	}
	
	ret = efd->socket_handler(efd);
	
out:
	if (close(sock)) {
		ERROR_ERRNO("failed to close client socket on socket %s", efd->path);
		ret = -1;
	}

	return ret;
}

static eventfd *
alloc_socket_eventfd(const char *path, socket_handler_t handler)
{
	eventfd *new = NULL;

	new = calloc(1, sizeof(*new));
	if (!new) {
		ERROR("out of memory");
		return NULL;
	}

	new->path = path;
	new->evtype = SocketEvent;
	new->socket_handler = handler;

	return new;
}

eventfd *
create_socket_eventfd(socket_event_t type, const char *path)
{
	eventfd *efd = NULL;

	switch (type) {
		case SocketEventSleep:
			efd = alloc_socket_eventfd(path, socket_sleep_handler);
			break;
		default:
			ERROR("Unsupported socket event type %d", type);
			return NULL;
	}

	if (!efd)
		ERROR("Failed to create socket eventfd of type %d", type);
	return efd;
}
