// SPDX-License-Identifier: GPL-2.0
// Copyright © 2009-2018 ANSSI. All Rights Reserved.
/**
 * @file soundcard.c
 * CLIP device-admind soundcard functions.
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
#include "soundcard.h"

#define SOUNDCARD_COMMAND "/sbin/switch-sound-context"

int
handle_soundcard_request(char c)
{

	char *argv[] = {
		SOUNDCARD_COMMAND,
		NULL,
		NULL
	};

	switch (c) {
		case 'h':
			argv[1] = "rm_h";
			break;
		case 'b':
			argv[1] = "rm_b";
			break;
		case 'n':
			argv[1] = "none";
			break;
		default:
			ERROR("invalid soundcard command: %c", c);
			return -1;
	}

	LOG("switching sound to jail : %s", argv[1]);
	if (run_external(SOUNDCARD_COMMAND, argv, NULL)) {
		ERROR("%s returned an error", SOUNDCARD_COMMAND);
		return -1;
	}
	return 0;
}
