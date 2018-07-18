// SPDX-License-Identifier: GPL-2.0
// Copyright © 2009-2018 ANSSI. All Rights Reserved.
/**
 * @file hotplug_initrd.c
 * CLIP initrd hotplug main.
 * @author Vincent Strubel <clipos@ssi.gouv.fr>
 *
 * Copyright (C) 2010 ANSSI
 * @n
 * All rights reserved.
 */

#include <stdlib.h>
#include <unistd.h>

#include "hotplug.h"

int g_foreground = 1;

extern int firmware_try_load(const char *, const char *, char **);

int
main(int argc, char *argv[], char *envp[])
{
	int ret = 0;
	const char *action, *subsys;

	if (argc < 2) {
		ERROR("Not enough arguments");
		return EXIT_FAILURE;
	}

	subsys = argv[1];
	if (!subsys) {
		ERROR("Missing subsystem ?");
		return EXIT_FAILURE;
	}

	action = getenv("ACTION");
	if (!action) {
		ERROR("Missing action");
		return EXIT_FAILURE;
	}

	ret = firmware_try_load(subsys, action, envp);

	if (!ret)
		return EXIT_SUCCESS;

	return (ret > 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
