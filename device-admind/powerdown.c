// SPDX-License-Identifier: GPL-2.0
// Copyright © 2009-2018 ANSSI. All Rights Reserved.
/**
 * @file powerdown.c
 * CLIP device-admind powerdown functions.
 * @author Vincent Strubel <clipos@ssi.gouv.fr>
 *
 * Copyright (C) 2013 ANSSI
 * @n
 * All rights reserved.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "powerdown.h"

#define HALT_COMMAND "/sbin/halt"
#define REBOOT_COMMAND "/sbin/reboot"
#define SUSPEND_COMMAND "/usr/bin/clip-suspend"

int
handle_powerdown_request(char c)
{

	const char *msg = NULL;

	char *argv[] = {
		NULL,
		NULL
	};

	switch (c) {
		case 'h':
			argv[0] = HALT_COMMAND;
			msg = "halting";
			break;
		case 'r':
			argv[0] = REBOOT_COMMAND;
			msg = "rebooting";
			break;
		case 's':
			argv[0] = SUSPEND_COMMAND;
			msg = "suspending";
			break;
		default:
			ERROR("invalid powerdown command: %c", c);
			return -1;
	}

	LOG("%s system as requested", msg);
	if (run_external(argv[0], argv, NULL)) {
		ERROR("%s returned an error", argv[0]);
		return -1;
	}
	return 0;
}
