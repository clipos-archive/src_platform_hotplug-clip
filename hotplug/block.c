// SPDX-License-Identifier: GPL-2.0
// Copyright © 2009-2018 ANSSI. All Rights Reserved.
/**
 * @file block.c
 * CLIP hotplug block connect / disconnect handler.
 * @author Vincent Strubel <clipos@ssi.gouv.fr>
 *
 * Copyright (C) 2011 SGDSN/ANSSI
 * @n
 * All rights reserved.
 *
 */

#include <string.h>

#include "hotplug.h"
#include <stdio.h>

#define BLOCK_CMD	"/lib/hotplug.d/block"
#define BLOCK_SUBSYS	"block"

#define BLOCK_ADD_ACTION 	"add"
#define BLOCK_DEL_ACTION 	"remove"

HANDLER(block_handler, subsys, action, envp)
{
	int ret = 0;
	char *argv[] = { BLOCK_CMD, NULL, NULL };

	if (strcmp(subsys, BLOCK_SUBSYS))
		return 0;
	if (!strcmp(action, BLOCK_ADD_ACTION)) {
		argv[1] = "add";
	} else if (!strcmp(action, BLOCK_DEL_ACTION)) {
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


