// SPDX-License-Identifier: GPL-2.0
// Copyright © 2009-2018 ANSSI. All Rights Reserved.
/**
 * @file hotplug.c
 * CLIP hotplug main.
 * @author Vincent Strubel <clipos@ssi.gouv.fr>
 *
 * Copyright (C) 2009 SGDN
 * @n
 * All rights reserved.
 */

#include <stdlib.h>
#include <unistd.h>

#include "hotplug.h"

int g_foreground = 0;

int
main(int argc, char *argv[], char *envp[])
{
	event_handler_t *iter;
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

	iter = &__start_handlers;
	while (iter < &__stop_handlers) {
		ret = (*iter++)(subsys, action, envp);
		if (ret)
			break;
	}

	if (!ret)
		return EXIT_SUCCESS;

	return (ret > 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
