// SPDX-License-Identifier: GPL-2.0
// Copyright © 2009-2018 ANSSI. All Rights Reserved.
/**
 * @file clip-device-request.c
 * CLIP utility to request device actions from device-admind.
 * @author Vincent Strubel <clipos@ssi.gouv.fr>
 *
 * Copyright (C) 2013-2015 ANSSI
 * @n
 * All rights reserved.
 *
 */

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "soundcard.h"
#include "powerdown.h"

#define WARN(fmt, args...) \
	fprintf(stderr, fmt"\n", ##args)

#define WARN_ERRNO(fmt, args...) \
	WARN(fmt": %s", ##args, strerror(errno))

#ifndef DEVICED_SOCKPATH
#define DEVICED_SOCKPATH "/var/run/deviced"
#endif

#define _TO_STR(var) #var
#define TO_STR(var) _TO_STR(var)

static inline void
print_help(const char *prog)
{
	printf("%s -[hv] <command>\n", prog);
	puts("Commands");
	puts("\t-p h/r/s: powerdown system (halt / reboot / suspend)");
	puts("\t-s h/b/n: switch soundcard to context (rm_h / rm_b / none)");
}

static inline void
print_version(const char *prog)
{
	printf("%s - Version %s\n", prog, TO_STR(VERSION));
}

static int
do_connect(void)
{
	struct sockaddr_un addr;
	int s, ret;

	memset(&addr, 0, sizeof(addr));
	if (strlen(DEVICED_SOCKPATH) > sizeof(addr.sun_path)) {
		WARN("Path %s is too long", DEVICED_SOCKPATH);
		return -1;
	}

	s = socket(PF_UNIX, SOCK_STREAM, 0);
	if (s < 0) {
		WARN_ERRNO("Failed to create socket");
		return -1;
	}

	strcpy(addr.sun_path, DEVICED_SOCKPATH);
	addr.sun_family = AF_UNIX;
	ret = connect(s, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		WARN_ERRNO("Failed to connect to eventd on %s", 
							DEVICED_SOCKPATH);
		return -1;
	}

	return s;
}

static int
check_result(int s)
{
	ssize_t rret;
	char c;

	rret = read(s, &c, 1);
	if (rret < 0) {
		WARN_ERRNO("Failed to read on socket");
		return -1;
	} else if (!rret) {
		WARN("EOF on socket ?");
		return -1;
	}

	if (c == 'Y') {
		puts("device request successful");
		return 0;
	} else {
		fputs("device request failed", stderr);
		return -1;
	}
}

static int
do_soundcard_request(char *arg)
{
	char cmd[2];
	ssize_t wret;
	int s;

	switch (arg[0]) {
		case 'h':
		case 'b':
		case 'n':
			break;
		default:
			WARN("Invalid level : %s", arg);
			return -1;
	}

	cmd[0] = SOUNDCARD_CMD_PREFIX;
	cmd[1] = arg[0];

	s = do_connect();
	if (s < 0)
		return s;

	wret = write(s, cmd, sizeof(cmd));
	if (wret < 0) {
		WARN_ERRNO("Failed to write on socket");
		return -1;
	} else if (wret != sizeof(cmd)) {
		WARN("Short write : %zd", wret);
		return -1;
	}

	return check_result(s);
}

static int
do_powerdown_request(char *arg)
{
	char cmd[2];
	ssize_t wret;
	int s;

	switch (arg[0]) {
		case 'h':
		case 'r':
		case 's':
			break;
		default:
			WARN("Invalid action : %s", arg);
			return -1;
	}

	cmd[0] = POWERDOWN_CMD_PREFIX;
	cmd[1] = arg[0];

	s = do_connect();
	if (s < 0)
		return s;

	wret = write(s, cmd, sizeof(cmd));
	if (wret < 0) {
		WARN_ERRNO("Failed to write on socket");
		return -1;
	} else if (wret != sizeof(cmd)) {
		WARN("Short write : %zd", wret);
		return -1;
	}

	return check_result(s);
}


int
main(int argc __attribute__((unused)), char *argv[] __attribute__((unused)))
{
	int c;
	char *exe;

	if (argv[0]) {
		exe = basename(argv[0]);
	} else {
		exe = "device-admind";
	}

	while ((c = getopt(argc, argv, "hvp:s:")) != -1) {
		switch (c) {
			case 'h':
				print_help(exe);
				return EXIT_SUCCESS;
				break;
			case 'v':
				print_version(exe);
				return EXIT_SUCCESS;
				break;
			case 'p':
				return do_powerdown_request(optarg);
				break;
			case 's':
				return do_soundcard_request(optarg);
				break;
		}
	}

	WARN("missing command line argument");
	print_help(exe);
	return EXIT_FAILURE;
}
