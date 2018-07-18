// SPDX-License-Identifier: GPL-2.0
// Copyright © 2009-2018 ANSSI. All Rights Reserved.
/**
 * @file powerdown.h
 * CLIP device-admind powerdown header.
 * @author Vincent Strubel <clipos@ssi.gouv.fr>
 *
 * Copyright (C) 2015 ANSSI
 * @n
 * All rights reserved.
 */

#ifndef _POWERDOWN_H
#define _POWERDOWN_H_

#define POWERDOWN_CMD_PREFIX 'h'

extern int handle_powerdown_request(char c);

#endif
