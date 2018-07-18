// SPDX-License-Identifier: GPL-2.0
// Copyright © 2009-2018 ANSSI. All Rights Reserved.
/**
 * @file printer.c
 * CLIP hotplug printer connect / disconnect handler.
 * @author Benjamin Morin <clipos@ssi.gouv.fr>
 *
 * Copyright (C) 2009 SGDN
 * @n
 * All rights reserved.
 *
 * Inspired by the usb handler by Vincent Strubel.
 */

#include <stdlib.h>
#include <string.h>

#include "hotplug.h"
#include <stdio.h>

#define USBPRINTER_CMD		"/lib/hotplug.d/printer"
#define USBPRINTER_SUBSYS	"usbmisc"

#define USBPRINTER_MAJOR	"180"
#define USBPRINTER_ADD_ACTION 	"add"
#define USBPRINTER_DEL_ACTION 	"remove"

HANDLER(usbprinter_handler, subsys, action, envp)
{
  int ret = 0; 

  char *argv[] = { USBPRINTER_CMD, NULL, NULL };
  const char *major = getenv("MAJOR");
  if (!major) {
	return 0;
  }

  if (strcmp(major, USBPRINTER_MAJOR))
  	return 0;


  if (!strcmp(subsys, USBPRINTER_SUBSYS) 
      && !strcmp(action, USBPRINTER_ADD_ACTION)) {
    argv[1] = "add";
  } else if (!strcmp(subsys, USBPRINTER_SUBSYS) 
	     && !strcmp(action, USBPRINTER_DEL_ACTION)) {
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
