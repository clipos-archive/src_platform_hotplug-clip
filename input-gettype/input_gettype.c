// SPDX-License-Identifier: GPL-2.0
// Copyright © 2009-2018 ANSSI. All Rights Reserved.
/**
 * @file input_gettype.c
 * CLIP input device classifier.
 * @author Vincent Strubel <clipos@ssi.gouv.fr>
 *
 * Copyright (C) 2011 SGDSN/ANSSI
 * @n
 * All rights reserved.
 *
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>

#define LOG(fmt, args...) \
	printf("%s(%s:%d): "fmt"\n", __FUNCTION__, \
			__FILE__, __LINE__, ##args)

#define ERROR(fmt, args...) \
	fprintf(stderr, "%s(%s:%d): "fmt"\n", __FUNCTION__, \
			__FILE__, __LINE__, ##args)
#define ERROR_ERRNO(fmt, args...) ERROR(fmt": %s", ##args, strerror(errno))

/* End of the 'key' device capabilities string for a regular keyboard */
#define KBD_KEY_CAPS	0xfffffffeUL 


typedef unsigned long cap_t;
#define LSZ 		( sizeof(unsigned long) * 8 )
#define CAP_SZ(max) 	( ( ((max) - 1) / LSZ ) + 1 )
#define CAP_BIT(cap, n)	( ( cap[((n) / LSZ)] >> ((n) % LSZ) ) & 1)
#define CAP_NMEMB(cap) 	(sizeof(cap)/sizeof(*cap))

typedef struct {
	cap_t ev[CAP_SZ(EV_MAX)];
	cap_t abs[CAP_SZ(ABS_MAX)];
	cap_t rel[CAP_SZ(REL_MAX)];
	cap_t key[CAP_SZ(KEY_MAX)];
	cap_t sw[CAP_SZ(SW_MAX)];
} input_caps_t;

static char *
get_sysattr_string(const char *devpath, const char *name)
{
	FILE *f = NULL;
	char *val = NULL;
	char *path = NULL;
	size_t len = 0;

	if (asprintf(&path, "%s/capabilities/%s", devpath, name) < 0) {
		ERROR("Out of memory for path %s/%s", devpath, name);
		return NULL;
	}

	f = fopen(path, "r");
	if (!f) {
		if (errno == ENOENT) /* No warning, OK */
			goto out;
		ERROR_ERRNO("Could not open %s", path);
		goto out;
	}

	if (getline(&val, &len, f) < 0) {
		ERROR("Failed to read a line in %s", path);
		goto out;
	}
	/* Fall through */	
out:
	if (f)
		fclose(f);
	if (path)
		free(path);
	return val;
}

static int
get_caps(const char *path, const char *name, cap_t *cap, size_t nmemb)
{
	char *val = NULL, *ptr;
	int ret = -1;
	unsigned int count = 0;

	val = get_sysattr_string(path, name);
	if (!val) {
		return (errno == ENOENT) ? 0 : -1;
	}

	errno = 0;
	while (count < nmemb) {
		ptr = strrchr(val, ' ');
		if (!ptr)
			break;

		cap[count] = strtoul(ptr + 1, NULL, 16);
		if (errno == ERANGE) {
			ERROR("Out of bound %s capability: %s", name, ptr +1); 
			goto out;
		}
		*ptr = '\0';
		count++;
	}
	if (count < nmemb) { /* First long */
		if (*val == '=')
			ptr = val + 1;
		else 
			ptr = val;
		cap[count] = strtoul(ptr, NULL, 16);
		if (errno == ERANGE) {
			ERROR("Out of bound %s capability: %s", name, ptr); 
			goto out;
		}
	}

	ret = 0;
	/* Fall through */
out:
	if (val)
		free(val);
	return ret;
}

static int
read_all_caps(const char *path, input_caps_t *caps)
{
	if (get_caps(path, "ev", caps->ev, CAP_NMEMB(caps->ev))) {
		ERROR("Failed to get EV capabilities for %s", path);
		return -1;
	}
	
	if (CAP_BIT(caps->ev, EV_KEY) &&
		    get_caps(path, "key", caps->key, CAP_NMEMB(caps->key))) {
		ERROR("Failed to key caps for %s", path);
		return -1;
	}
	if (CAP_BIT(caps->ev, EV_ABS) &&
		    get_caps(path, "abs", caps->abs, CAP_NMEMB(caps->abs))) {
		ERROR("Failed to absolute caps for %s", path);
		return -1;
	}
	if (CAP_BIT(caps->ev, EV_REL) &&
		    get_caps(path, "rel", caps->rel, CAP_NMEMB(caps->rel))) {
		ERROR("Failed to relative caps for %s", path);
		return -1;
	}
	if (CAP_BIT(caps->ev, EV_SW) &&
		    get_caps(path, "sw", caps->sw, CAP_NMEMB(caps->sw))) {
		ERROR("Failed to switch caps for %s", path);
		return -1;
	}

	return 0;
}

static int
print_dev_type(const input_caps_t *caps)
{
	int ret=-1;

	if (CAP_BIT(caps->ev, EV_SW) && CAP_BIT(caps->sw, SW_LID)) {
		puts("lid");
		ret += 1;
	}

	if (!CAP_BIT(caps->ev, EV_KEY)) {
		return -1;
	}

	if ((caps->key[0] & KBD_KEY_CAPS) == KBD_KEY_CAPS) {
		/* Keyboard */
		puts("keyboard");
		ret += 1;
	}

	/* Pointers with absolute position */
	if (CAP_BIT(caps->ev, EV_ABS) 
			&& CAP_BIT(caps->abs, ABS_X) 
			&& CAP_BIT(caps->abs, ABS_Y)) {
		/* This is from udev ... */
		if (CAP_BIT(caps->key, BTN_STYLUS) 
				|| CAP_BIT(caps->key, BTN_TOOL_PEN)) {
			puts("tablet");
			ret += 1;
		} else if (CAP_BIT(caps->key, BTN_MOUSE) 
				&& CAP_BIT(caps->key, BTN_TOOL_FINGER)) { 
			puts("touchpad");
			ret += 1;
		} else if (CAP_BIT(caps->key, BTN_TRIGGER) 
				|| CAP_BIT(caps->key, BTN_A)
				|| CAP_BIT(caps->key, BTN_1)) {
			puts("joystick");
			ret += 1;
		} else if (CAP_BIT(caps->key, BTN_TOUCH)) {
			puts("touchscreen");
			ret += 1;
		} else if (CAP_BIT(caps->key, BTN_MOUSE)) {
			puts("mouse"); 
			ret =  0;
		}
	}

	/* Real mouse with only relative position */
	if (CAP_BIT(caps->ev, EV_REL) && CAP_BIT(caps->key, BTN_MOUSE)
			&& CAP_BIT(caps->rel, REL_X) 
			&& CAP_BIT(caps->rel, REL_Y)) {
		puts("mouse");
		ret =  0;
	}

	if (CAP_BIT(caps->key, KEY_SLEEP)) {
		puts("sleep");
		ret += 1;
	} else if (CAP_BIT(caps->key, KEY_POWER)) {
		/* Power buttons also appear to send */
		puts("power");
		ret += 1;
	}

	if (CAP_BIT(caps->sw, SW_TABLET_MODE)) {
		puts("tabletmode");
		ret += 1;
	}

	return ret;
}

int
main(int argc, char *argv[])
{
	input_caps_t caps;
	const char *devpath = argv[1];

	memset(&caps, 0, sizeof(caps));
	if (argc < 2) {
		ERROR("Missing path");
		return EXIT_FAILURE;
	}

	if (read_all_caps(devpath, &caps))
		return EXIT_FAILURE;

	return (print_dev_type(&caps) < 0) ? EXIT_FAILURE : EXIT_SUCCESS;
}


