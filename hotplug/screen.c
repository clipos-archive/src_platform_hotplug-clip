// SPDX-License-Identifier: GPL-2.0
// Copyright © 2009-2018 ANSSI. All Rights Reserved.
/**
 * @file screen.c
 * CLIP screen hotplug handler.
 * @author Vincent Strubel <clipos@ssi.gouv.fr>
 *
 * Copyright (C) 2013 ANSSI
 * @n
 * All rights reserved.
 *
 */

#include <string.h>

#include "hotplug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCREEN_SUBSYS	"drm"
#define SCREEN_ACTION	"change"
#define SCREEN_CMD	"/lib/hotplug.d/screen"


HANDLER(screen_handler, subsys, action, envp __U)
{
	char *argv[] = { SCREEN_CMD, NULL};

	if (strcmp(subsys, SCREEN_SUBSYS))
		return 0;

	if (strcmp(action, SCREEN_ACTION))
		return 0;

	DEBUG("Running screen handler");
	run_external(SCREEN_CMD, argv, envp);

	return 1;
}


