// SPDX-License-Identifier: GPL-2.0
// Copyright © 2009-2018 ANSSI. All Rights Reserved.
/**
 * @file clip-input-notify.c
 * CLIP hotplug utility to notify X11 of new input devices.
 * @author Vincent Strubel <clipos@ssi.gouv.fr>
 *
 * Copyright (C) 2010 ANSSI
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

#define CLIP_INPUT_KBD      	0x01
#define CLIP_INPUT_MOUSE    	0x02
#define CLIP_INPUT_TOUCHPAD 	0x04
#define CLIP_INPUT_TABLET   	0x08
#define CLIP_INPUT_TOUCHSCREEN  0x10

#define CLIP_INPUT_ADD      0x01
#define CLIP_INPUT_REMOVE   0x02

#define WARN(fmt, args...) \
	fprintf(stderr, fmt"\n", ##args)

#define WARN_ERRNO(fmt, args...) \
	WARN(fmt": %s", ##args, strerror(errno))

#define PATH_LEN    256

typedef struct __attribute__((packed)) {
	int version;        /* Should be 0x1 for now */
	int flags;          /* Type */
	int action;         /* CLIP_INPUT_ADD or CLIP_INPUT_REMOVE */
	char devnode[PATH_LEN];  /* Full path, starting with "/dev/" */
	char syspath[PATH_LEN];  /* Full path, starting with "/sys/" */
} clip_device_t;

static int
notify(const char *path, const clip_device_t *device)
{
	int s, ret;
	ssize_t wret;
	struct sockaddr_un addr;

	memset(&addr, 0, sizeof(addr));
	if (strlen(path) > sizeof(addr.sun_path)) {
		WARN("Path %s is too long", path);
		return -1;
	}

	s = socket(PF_UNIX, SOCK_STREAM, 0);
	if (s < 0) {
		WARN_ERRNO("Failed to create socket");
		return -1;
	}

	strcpy(addr.sun_path, path);
	addr.sun_family = AF_UNIX;
	ret = connect(s, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		WARN_ERRNO("Failed to connect to %s", path);
		return -1;
	}

	for (;;) {
		wret = write(s, device, sizeof(*device));
		if (wret < 0) {
			if (errno == EINTR)
				continue;
			WARN_ERRNO("Failed to write on socket");
			return -1;
		}
		if (wret != sizeof(*device)) {
			WARN("Truncated write: %zu != %zu", wret, sizeof(*device));
			return -1;
		}
		break;
	}

	(void)close(s);
	return 0;
}


#define set_if_not_set(var, val, name) do {\
	if (dev->var) { \
		WARN("Too many "name); \
		return EXIT_FAILURE; \
	} \
	dev->var = val; \
} while (0)

#define copy_if_not_set(var, src, name) do {\
	if (*(dev->var)) { \
		WARN("Already have a "name); \
		return -1; \
	} \
	if (strlen(src) > sizeof(dev->var)) { \
		WARN(name" too long: %s", src); \
		return -1; \
	} \
	strcpy(dev->var, src); \
} while (0)


static int
parse_options(int argc, char *argv[], clip_device_t *dev, const char **path)
{
	int c;

	dev->version = 0x01;
	const char *p = NULL;

	while ((c = getopt(argc, argv, "ard:s:p:t:")) != -1) {
		switch (c) {
			case 'a':
				set_if_not_set(action, CLIP_INPUT_ADD, "actions");
				break;
			case 'r':
				set_if_not_set(action, CLIP_INPUT_REMOVE, "actions");
				/* Set default type, only sys path matters 
				 * for removal anyway. */
				dev->flags = CLIP_INPUT_KBD;
				break;
			case 'd':
				copy_if_not_set(devnode, optarg, "device node");
				break;
			case 's':
				copy_if_not_set(syspath, optarg, "sys path");
				break;
			case 'p':
				p = optarg;
				break;
			case 't':
				if (!strcmp(optarg, "keyboard"))
					dev->flags = CLIP_INPUT_KBD;
				else if (!strcmp(optarg, "mouse"))
					dev->flags = CLIP_INPUT_MOUSE;
				else if (!strcmp(optarg, "touchpad"))
					dev->flags = CLIP_INPUT_TOUCHPAD;
				else if (!strcmp(optarg, "tablet"))
					dev->flags = CLIP_INPUT_TABLET;
				else if (!strcmp(optarg, "touchscreen"))
					dev->flags = CLIP_INPUT_TOUCHSCREEN;
				else {
					WARN("Unsupported type: %s", optarg);
					return -1;
				}
				break;
			default:
				WARN("Unsupported option %c", c);
				return -1;
		}
	}

	if (!dev->action || !dev->flags || !*(dev->devnode) 
			|| !*(dev->syspath) || !p) {
		WARN("Missing arguments");
		return -1;
	}

	*path = p;
	return 0;
}

int
main(int argc, char *argv[])
{
	clip_device_t dev;
	const char *path;

	memset(&dev, 0, sizeof(dev));

	if (parse_options(argc, argv, &dev, &path))
		return EXIT_FAILURE;

	if (notify(path, &dev))
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

