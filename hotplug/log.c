// SPDX-License-Identifier: GPL-2.0
// Copyright © 2009-2018 ANSSI. All Rights Reserved.
/**
 * @file log.c
 * CLIP hotplug generic event logger.
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

HANDLER(log_handler, subsys, action, envp)
{
	char **ptr;
	DEBUG("subsys: %s, action: %s", subsys, action);
	for (ptr = envp; ptr && *ptr; ptr++) {
		DEBUG("env: %s", *ptr);
	}
	return 0;
}


