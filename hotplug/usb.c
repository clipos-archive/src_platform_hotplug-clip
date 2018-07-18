// SPDX-License-Identifier: GPL-2.0
// Copyright © 2009-2018 ANSSI. All Rights Reserved.
/**
 * @file usb.c
 * CLIP hotplug usb connect / disconnect handler.
 * @author Vincent Strubel <clipos@ssi.gouv.fr>
 * @author Benjamin Morin <clipos@ssi.gouv.fr>
 *
 * Copyright (C) 2010-2011 SGDSN/ANSSI
 * @n
 * All rights reserved.
 */

#include <stdlib.h>
#include <string.h>

#include "hotplug.h"
#include <stdio.h>

#define USB_MAJOR	"189"

#define USB_SUBSYS	"usb"
#define USB_DEVTYPE	"usb_device"
#define USB_CMD		"/lib/hotplug.d/usb"
#define ZEROCD_CMD	"/sbin/zerocdoff.sh"

#define USB_ADD_ACTION 	"add"

#define USB_DEL_ACTION "remove"

HANDLER(usb_handler, subsys, action, envp)
{
	char *argv[] = { USB_CMD, NULL, NULL, NULL };
	int ret;
	const char *major = NULL;
	const char *devtype = NULL;
	
	if (strcmp(subsys, USB_SUBSYS))
		return 0;
	
	major = getenv("MAJOR");
	if (!major) {
		return 0;
	}
	
	devtype = getenv("DEVTYPE");
	if(!devtype) {
		return 0;
	}
	
	if (strcmp(major, USB_MAJOR))
		return 0;
	if (strcmp(devtype, USB_DEVTYPE))
		return 0;
	
	DEBUG("usb event : (subsys = %s, action = %s)", subsys, action);
	
	if (!strcmp(action, USB_ADD_ACTION)) {
		argv[1] = "add";
	} else if (!strcmp(action, USB_DEL_ACTION)) {
		argv[1] = "remove";
	}
	
	if (!argv[1])
		return 0;

	DEBUG("Running %s hotplug handler", argv[0]);
	ret = run_external(argv[0], argv, envp);
	if (ret) {
		ERROR("Hotplug handler %s failed (%d)", argv[0], ret);
		return -1;
	}
	
	return 1;
}
