// SPDX-License-Identifier: GPL-2.0
// Copyright © 2009-2018 ANSSI. All Rights Reserved.
/**
 * @file eventd_input.c
 * CLIP eventd input event handlers.
 * @author Vincent Strubel <clipos@ssi.gouv.fr>
 *
 * Copyright (C) 2010-2012 SGDSN/ANSSI
 * @n
 * All rights reserved.
 */


#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "eventd.h"


#define CMD_SUSPEND 	"/usr/bin/clip-suspend"
#define CMD_KBD_TOGGLE "/usr/local/bin/toggle_kbd.sh"
#define CMD_FNKEYS "/usr/bin/clip-fnkeys-handler"

static int g_sleeping = 0;

/*************************************************************/
/*                       Handlers                            */
/*************************************************************/

static int
_suspend_handler(eventfd *efd, __u16 type, 	
			__u16 code, __s32 value, int switch_p)
{
	if (type == EV_SYN)
		return 0;

	if (type != efd->type)
		return -1;

	if (code != efd->code)
		return -1;

	if (efd->value && value != efd->value)
		return -1;

	if (switch_p) {
		if (g_sleeping) {
			g_sleeping = 0;
			return 0;
		} else {
			g_sleeping = 1;
		}
	}
	LOG("running %s for %s handler", CMD_SUSPEND, efd->path);
	return run_external((void *)CMD_SUSPEND, NULL, NULL);
}

static int
single_suspend_handler(eventfd *efd, __u16 type, __u16 code, __s32 value)
{
	return _suspend_handler(efd, type, code, value, 0);
}

static int
button_suspend_handler(eventfd *efd, __u16 type, __u16 code, __s32 value)
{
	return _suspend_handler(efd, type, code, value, 1);
}

static int
tablet_handler(eventfd *efd, __u16 type, 	
			__u16 code, __s32 value)
{
	char *argv[] = { CMD_KBD_TOGGLE, "", NULL };
	LOG("type = %x, code=%x, value=%x\n", type, code, value);

	if (type != efd->type) {
		ERROR("invalid type for %s handler, 0x%x != 0x%x",
			efd->path, type, efd->type);
		return -1;
	}

	if (code != efd->code) {
		LOG("ignoring code for %s handler, 0x%x != 0x%x",
			efd->path, code, efd->code);
		return -1;
	}

	if (value == 1) 
		argv[1] = "on";
	else if (!value)
		argv[1] = "off";
	else {
		ERROR("unexpected value 0x%x", value);
		return -1;
	}

	LOG("running %s for %s handler (code %x, value %d)", 
		CMD_KBD_TOGGLE, efd->path, code, value);
	return run_external((void *)CMD_KBD_TOGGLE, argv, NULL);
}

static int
fnkeys_handler(eventfd *efd, __u16 type, 	
			__u16 code, __s32 value)
{
	char *argv[] = { CMD_FNKEYS, NULL, NULL, NULL };

	if (type != efd->type)
		return 0;

	if (!value) /* Key up */
		return 0;

	switch (code) {
		case KEY_MUTE:
			argv[1] = "sound";
			argv[2] = "mute";
			break;
		case KEY_VOLUMEDOWN:
			argv[1] = "sound";
			argv[2] = "down";
			break;
		case KEY_VOLUMEUP:
			argv[1] = "sound";
			argv[2] = "up";
			break;
		case KEY_BRIGHTNESSDOWN:
			argv[1] = "brightness";
			argv[2] = "down";
			break;
		case KEY_BRIGHTNESSUP:
			argv[1] = "brightness";
			argv[2] = "up";
			break;
		case KEY_SWITCHVIDEOMODE:
			argv[1] = "video";
			argv[2] = "switch";
			break;
		case KEY_WLAN:
			argv[1] = "wifi";
			argv[2] = "toggle";
			break;
		case KEY_SCREENLOCK:
			argv[1] = "screenlock";
			argv[2] = "doit";
			break;
		case KEY_KBDILLUMDOWN:
			argv[1] = "led";
			argv[2] = "down";
			break;
		case KEY_KBDILLUMUP:
			argv[1] = "led";
			argv[2] = "up";
			break;
		case KEY_EJECTCD:
			argv[1] = "eject";
			argv[2] = "doit";
			break;
		case KEY_SLEEP:
			return single_suspend_handler(efd, type, code, value);
		case KEY_POWER:
			return button_suspend_handler(efd, type, code, value);
		default:
			/* IMPORTANT WARNING : THE FOLLOWING CODE DISCLOSE THE KEY PRESSED WHICH IS NOT DESIRED TO CATCH FN KEYS ON REAL/CONCRETE (I.E. NOT VIRTUAL/SPECIALIZED KEYBOARDS) */
			/*ERROR("unhandled fn key code on %s input device: %d", 
				efd->path, code);*/
			return -1;
	}
	return run_external((void *)CMD_FNKEYS, argv, NULL);
}

static int
debug_handler(eventfd *efd, __u16 type, 	
			__u16 code, __s32 value)
{
	LOG("debug event on %s: type 0x%x, code %d, value 0x%x",
		efd->path, type, code, value);
	return 0;
}

/*************************************************************/
/*                       Interface                           */
/*************************************************************/

inline int
open_input_eventfd(eventfd *efd)
{
	int fd = open(efd->path, O_RDONLY);
	if (fd < 0) {
		ERROR_ERRNO("failed to open %s", efd->path);
		return -1;
	}

	efd->fd = fd;
	return 0;
}

int
read_input_event(eventfd *efd)
{
	struct input_event event;
	ssize_t rret;
	memset(&event, 0, sizeof(event));

	rret = read(efd->fd, &event, sizeof(event));
	if (rret < 0) {
		if (errno == ENODEV) {
			LOG("Eventfd %s returned ENODEV, trying to reopen it", 
								efd->path);
			(void)close(efd->fd);
			if (open_input_eventfd(efd)) {
				ERROR("Failed to reopen event fd %s, "
					"removing it", efd->path);
				destroy_eventfd(efd);
				return -2;
			} else {
				LOG("Eventfd %s reopened successfully", 
								efd->path);
				return 0;
			}
		}
				
		ERROR_ERRNO("failed to read %s", efd->path);
		return -1;
	}
	if (rret != sizeof(event)) {
		ERROR("invalid read, %d != %u", rret, sizeof(event));
		return -1;
	}

	if (efd->input_handler) {
		return efd->input_handler(efd, event.type, event.code, event.value);
	} else
		return 0;
}

static eventfd *
alloc_input_eventfd(const char *path, 
	__u16 type, __u16 code, __s32 value, input_handler_t handler)
{
	eventfd *new = NULL;

	new = calloc(1, sizeof(*new));
	if (!new) {
		ERROR("out of memory");
		return NULL;
	}

	new->path = path;
	new->evtype = InputEvent;
	new->type = type;
	new->code = code;
	new->value = value;
	new->input_handler = handler;

	return new;
}

eventfd *
create_input_eventfd(input_event_t type, const char *path)
{
	eventfd *efd = NULL;

	switch (type) {
		case InputEventLid:
			efd = alloc_input_eventfd(path, 
						EV_SW, 0, 0x1, 
						single_suspend_handler);
			break;
		case InputEventPower:
			efd = alloc_input_eventfd(path, 
						EV_KEY, KEY_POWER, 0x1, 
						button_suspend_handler);
			break;
		case InputEventSleep:
			efd = alloc_input_eventfd(path, 
						EV_KEY, KEY_SLEEP, 0x1, 
						single_suspend_handler);
			break;
		case InputEventTablet:
			efd = alloc_input_eventfd(path, 
						EV_SW, SW_TABLET_MODE, 0, 
						tablet_handler);
			break;
		case InputEventFunctionKey:
			efd = alloc_input_eventfd(path, 
						EV_KEY, 0, 0,
						fnkeys_handler);
			break;
		case InputEventDebug:
			efd = alloc_input_eventfd(path, 
						0, 0, 0,
						debug_handler);
			break;
		default:
			ERROR("Unsupported input event type %d", type);
			return NULL;
	}

	if (!efd)
		ERROR("Failed to create input eventfd of type %d", type);
	return efd;
}
