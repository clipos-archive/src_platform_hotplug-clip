// SPDX-License-Identifier: GPL-2.0
// Copyright © 2009-2018 ANSSI. All Rights Reserved.
/**
 * @file net.c
 * CLIP hotplug net interface connect / disconnect handler.
 * @author Vincent Strubel <clipos@ssi.gouv.fr>
 *
 * Copyright (C) 2010-2011 SGDSN/ANSSI
 * @n
 * All rights reserved.
 */

#include <stdlib.h>
#include <string.h>

#include "hotplug.h"
#include <stdio.h>

#define NET_SUBSYS	"net"
#define NET_CMD		"/lib/hotplug.d/net"

#define NET_ADD_ACTION 	"add"

#define NET_DEL_ACTION "remove"

HANDLER(net_handler, subsys, action, envp)
{
	char *argv[] = { NET_CMD, NULL, NULL, NULL };
	int ret;
	char *interface = NULL;
	
	if (strcmp(subsys, NET_SUBSYS))
		return 0;
	
	interface = getenv("INTERFACE");
	if(!interface) {
		ERROR("net event, no interface");
		return 0;
	}
	
	DEBUG("net event : (subsys = %s, interface = %s, action = %s)", 
			subsys, interface, action);
	
	if (!strcmp(action, NET_ADD_ACTION)) {
		argv[1] = "add";
	} else if (!strcmp(action, NET_DEL_ACTION)) {
		argv[1] = "remove";
	}
	
	if (!argv[1])
		return 0;

	argv[2] = interface;

	DEBUG("Running %s hotplug handler", argv[0]);
	ret = run_external(argv[0], argv, envp);
	if (ret) {
		ERROR("Hotplug handler %s failed (%d)", argv[0], ret);
		return -1;
	}
	
	return 1;
}
