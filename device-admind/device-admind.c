// SPDX-License-Identifier: GPL-2.0
// Copyright © 2009-2018 ANSSI. All Rights Reserved.
/**
 * @file device-admind.c
 * CLIP device-admind main.
 * @author Vincent Strubel <clipos@ssi.gouv.fr>
 *
 * Copyright (C) 2013 ANSSI
 * @n
 * All rights reserved.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <syslog.h>

#include <clip/clip.h>

#include "common.h"
#include "soundcard.h"
#include "powerdown.h"

#define _TO_STR(var) #var
#define TO_STR(var) _TO_STR(var)

static char *socket_path;
int g_foreground = 1;

static inline void
print_help(const char *prog)
{
	printf("%s [-Fhv] -s <socket path>\n", prog);
	puts("Options:");
	puts("\t-F: run in foreground (do not detach)");
	puts("\t-h: print this help and exit");
	puts("\t-v: print the version number and exit");
}

static inline void
print_version(const char *prog)
{
	printf("%s - Version %s\n", prog, TO_STR(VERSION));
}

static void 
main_loop(int daemonize)
{
	int s, s_com, ret;
	socklen_t len;
	struct sockaddr_un sau;
	ssize_t wret;
	char cmd[2], c;

	if (daemonize) { 
		if (clip_daemonize()) {
			ERROR_ERRNO("clip_fork");
			return;
		}
		g_foreground = 0;
	}

	openlog("device-admind", LOG_PID, LOG_DAEMON);
	
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
		ERROR_ERRNO("signal");
		return;
	}

        LOG("starting to listen on %s ...", socket_path);
	
	s = clip_sock_listen(socket_path, &sau, 0);

	if (s < 0) {
		ERROR_ERRNO("failed to create socket at %s", socket_path);
		return;
	}

	for (;;) {
		memset(cmd, 0, sizeof(cmd));

		len = sizeof(struct sockaddr_un);
		s_com = accept(s, (struct sockaddr *)&sau, &len);
		if (s_com < 0) {
			ERROR_ERRNO("accept");
			close(s);
			return;
		}

		/* Get the command */
		if (read(s_com, cmd, sizeof(cmd)) != sizeof(cmd))
		{
			ERROR_ERRNO("read command");
			close(s_com);
			continue;
		}

		ret = -1;

		switch (cmd[0]) {
			case SOUNDCARD_CMD_PREFIX:
				ret = handle_soundcard_request(cmd[1]);
				break;
			case POWERDOWN_CMD_PREFIX:
				ret = handle_powerdown_request(cmd[1]);
				break;
			default:
				ERROR("invalid command: %.*s", 
							sizeof(cmd), cmd);
				break;
		}
		c = (ret == 0) ? 'Y' : 'N';
		wret = write(s_com, &c, 1);
		if (wret < 0) 
			ERROR_ERRNO("failed to write status");
		close(s_com);
	}

	ERROR("WTF am I doing here ?");
}

int 
main(int argc, char *argv[])
{
	char *exe;
	int c, daemonize = 1;

	if (argv[0]) {
		exe = basename(argv[0]);
	} else {
		exe = "device-admind";
	}

	while ((c = getopt(argc, argv, "Fhvs:u:")) != -1) {
		switch (c) {
			case 'F':
				daemonize = 0;
				break;
			case 'h':
				print_help(exe);
				return EXIT_SUCCESS;
				break;
			case 'v':
				print_version(exe);
				return EXIT_SUCCESS;
				break;
			case 's':
				if (socket_path) {
					fputs("Dual socket path option\n", stderr);
					return EXIT_FAILURE;
				}
				socket_path = optarg;
				break;
			default:
				fputs("Invalid option\n", stderr);
				return EXIT_FAILURE;
				break;
		}
	}
				
	argv += optind;
	argc -= optind;
		
	if (!socket_path) {
		fputs("No socket path specified on command line", stderr);
		return EXIT_FAILURE;
	}

	main_loop(daemonize);
	/* Not reached */
	return EXIT_FAILURE;
}
