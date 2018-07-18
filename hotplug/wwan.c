// SPDX-License-Identifier: GPL-2.0
// Copyright © 2009-2018 ANSSI. All Rights Reserved.
/**
 * @file 3g.c
 * CLIP hotplug 3G modules connect / disconnect handler.
 * @author Vincent Strubel <clipos@ssi.gouv.fr>
 *
 * Copyright (C) 2011 SGDSN/ANSSI
 * @n
 * All rights reserved.
 */

#include <stdlib.h>
#include <string.h>

#include "hotplug.h"
#include <stdio.h>

#define WWAN_CMD		"/lib/hotplug.d/wwan"
#define WWAN_SUBSYS		"tty"

#define WWAN_OPTION_DEV		"ttyHS"
#define WWAN_HUAWEI_DEV		"ttyUSB"
#define WWAN_ACM_DEV		"ttyACM"
#define WWAN_ADD_ACTION 	"add"
#define WWAN_DEL_ACTION 	"remove"

HANDLER(wwan_handler, subsys, action, envp)
{
	int ret = 0; 
	char *argv[] = { WWAN_CMD, NULL, NULL };

	const char *devname = getenv("DEVNAME");
	if (!devname)
		return 0;

	if (strcmp(subsys, WWAN_SUBSYS))
		return 0;

	if ((strncmp(devname, WWAN_OPTION_DEV, sizeof(WWAN_OPTION_DEV) - 1)) &&
			(strncmp(devname, WWAN_ACM_DEV, sizeof(WWAN_ACM_DEV) - 1)) &&
			(strncmp(devname, WWAN_HUAWEI_DEV, sizeof(WWAN_HUAWEI_DEV) - 1))) {
		DEBUG("Unsupported tty %s : %s", action, devname);
		return 0;
	}

	if (!strcmp(action, WWAN_ADD_ACTION)) {
		argv[1] = "add";
	} else if (!strcmp(action, WWAN_DEL_ACTION)) {
		argv[1] = "remove";
	}

	if (!argv[1])
		return 0;
	
	DEBUG("Running %s hotplug handler", argv[0]);
	ret = run_external(argv[0], argv, envp);
	if (ret)
		ERROR("Hotplug handler %s failed (%d)", argv[0], ret);
	return (ret) ? -1 : 1;
}
