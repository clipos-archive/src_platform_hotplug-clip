// SPDX-License-Identifier: GPL-2.0
// Copyright © 2009-2018 ANSSI. All Rights Reserved.
/**
 * @file common.h.
 * CLIP hotplug / eventd common functions.
 * @author Vincent Strubel <clipos@ssi.gouv.fr>
 *
 * Copyright (C) 2009 SGDN
 * Copyright (C) 2010 ANSSI
 * @n
 * All rights reserved.
 */

#ifndef HOTPLUG_COMMON_H
#define HOTPLUG_COMMON_H

#include <errno.h>
#include <string.h>

#define __U __attribute__((unused))

extern int g_foreground;

extern void do_log(int lvl, const char *fmt, ...);
extern int run_external(char *cmd, char **argv __U, char **envp);

#define _LOG(lvl, fmt, args...) \
	do_log(lvl, "%s(%s:%d): "fmt"\n", __FUNCTION__, \
			__FILE__, __LINE__, ##args)

#define DEBUG(fmt, args...) _LOG(2, fmt, ##args)
#define LOG(fmt, args...) _LOG(1, fmt, ##args)
#define ERROR(fmt, args...) _LOG(0, fmt, ##args)
#define ERROR_ERRNO(fmt, args...) ERROR(fmt": %s", ##args, strerror(errno))

#endif /* HOTPLUG_COMMON_H */
