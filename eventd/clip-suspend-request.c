// SPDX-License-Identifier: GPL-2.0
// Copyright © 2009-2018 ANSSI. All Rights Reserved.
/**
 * @file clip-suspend-request.c
 * CLIP hotplug utility request suspend from eventd.
 * @author Vincent Strubel <clipos@ssi.gouv.fr>
 *
 * Copyright (C) 2013 ANSSI
 * @n
 * All rights reserved.
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define WARN(fmt, args...) \
	fprintf(stderr, fmt"\n", ##args)

#define WARN_ERRNO(fmt, args...) \
	WARN(fmt": %s", ##args, strerror(errno))

#ifndef SUSPEND_SOCKPATH
#define SUSPEND_SOCKPATH "/var/run/suspend"
#endif

int
main(int argc __attribute__((unused)), char *argv[] __attribute__((unused)))
{
	int s, ret;
	char c;
	ssize_t rret;
	struct sockaddr_un addr;

	memset(&addr, 0, sizeof(addr));
	if (strlen(SUSPEND_SOCKPATH) > sizeof(addr.sun_path)) {
		WARN("Path %s is too long", SUSPEND_SOCKPATH);
		return -1;
	}

	s = socket(PF_UNIX, SOCK_STREAM, 0);
	if (s < 0) {
		WARN_ERRNO("Failed to create socket");
		return -1;
	}

	strcpy(addr.sun_path, SUSPEND_SOCKPATH);
	addr.sun_family = AF_UNIX;
	ret = connect(s, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		WARN_ERRNO("Failed to connect to eventd on %s", 
							SUSPEND_SOCKPATH);
		return -1;
	}

	rret = read(s, &c, 1);
	if (rret < 0) {
		WARN_ERRNO("Failed to read on socket");
		return -1;
	} else if (!rret) {
		WARN("EOF on socket ?");
		return -1;
	}

	if (c == 'Y') {
		puts("Suspend request successful");
		return 0;
	} else {
		fputs("Suspend request failed", stderr);
		return -1;
	}
}
