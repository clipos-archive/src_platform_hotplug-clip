// SPDX-License-Identifier: GPL-2.0
// Copyright © 2009-2018 ANSSI. All Rights Reserved.
/**
 * @file input.c
 * CLIP hotplug input device handler.
 * @author Vincent Strubel <clipos@ssi.gouv.fr>
 *
 * Copyright (C) 2010 ANSSI
 * @n
 * All rights reserved.
 *
 */

#include <string.h>

#include "hotplug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/input.h>

#define INPUT_SUBSYS	"input"

#define INPUT_CMD	"/lib/hotplug.d/input"


HANDLER(input_handler, subsys, action, envp __U)
{
	char *argv[] = { INPUT_CMD, NULL, NULL };

	if (strcmp(subsys, INPUT_SUBSYS))
		return 0;

	if (!strcmp(action, "add")) {
		argv[1] = "add";
	} else if (!strcmp(action, "remove")) {
		argv[1] = "remove";
	} else {
		LOG("Unknown input device action %s, path %s", 
					action, getenv("DEVPATH"));
		return 0;
	}


	DEBUG("Running input device handler : %s", argv[1]);
	run_external(INPUT_CMD, argv, envp);

	return 1;
}


