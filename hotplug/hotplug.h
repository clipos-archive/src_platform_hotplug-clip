// SPDX-License-Identifier: GPL-2.0
// Copyright © 2009-2018 ANSSI. All Rights Reserved.
/**
 * @file hotplug.h.
 * CLIP hotplug main header.
 * @author Vincent Strubel <clipos@ssi.gouv.fr>
 *
 * Copyright (C) 2009 SGDN
 * Copyright (C) 2010 ANSSI
 * @n
 * All rights reserved.
 */

#ifndef HOTPLUG_H
#define HOTPLUG_H

#include "common.h"

/** 
 * Event handler type.
 * @return 1 if event was handled, 0 if it didn't match this 
 * event handler type, -1 if it matched but an error was encountered.
 */
typedef int (*event_handler_t)(const char *subsys, 
			const char *action, char **envp);

/**
 * Define a static handler called @a name (with args @a subsys, @a action
 * and @a envp), and register a function pointer to that handler in the
 * @a handlers ELF section. Those function pointers get called one after
 * another (in an undefined order) when hotplug is executed. The first 
 * handler that matches (by returning either 1 or an error, -1) stops 
 * the iteration.
 */
#define HANDLER(name, subsys, action, envp) \
	static int name(const char *, const char *, char **); \
	int (*const fn_##name)(const char *, const char *, char **) \
		__attribute__((unused, section("handlers"), visibility("hidden"))) = &name; \
	static int name(const char *subsys, const char *action, char **envp)
		
/**
 * Pointer to first hotplug handler.
 */
extern event_handler_t __start_handlers;
/**
 * Pointer to last hotplug handler.
 */
extern event_handler_t __stop_handlers;

#endif /* HOTPLUG_H */
