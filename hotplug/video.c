// SPDX-License-Identifier: GPL-2.0
// Copyright © 2009-2018 ANSSI. All Rights Reserved.
/**
 * @file video.c
 * CLIP hotplug video camera connect / disconnect handler.
 * @author Yves-Alexis Perez <clipos@ssi.gouv.fr>
 *
 * Copyright © 2012 SGDSN/ANSSI
 * @n
 * All rights reserved.
 *
 * Inspired by the printer handler by Benjamin Morin.
 */

#include <stdlib.h>
#include <string.h>

#include "hotplug.h"
#include <stdio.h>

#define VIDEO_CMD		"/lib/hotplug.d/video"
#define VIDEO_SUBSYS	"video4linux"

#define VIDEO_MAJOR	"81"
#define VIDEO_ADD_ACTION 	"add"
#define VIDEO_DEL_ACTION 	"remove"

HANDLER(usbvideo_handler, subsys, action, envp)
{
  int ret = 0; 

  char *argv[] = { VIDEO_CMD, NULL, NULL };
  const char *major = getenv("MAJOR");
  if (!major) {
	return 0;
  }

  if (strcmp(major, VIDEO_MAJOR))
  	return 0;

	DEBUG("usb event : (subsys = %s, action = %s)", subsys, action);

  if (!strcmp(subsys, VIDEO_SUBSYS) 
      && !strcmp(action, VIDEO_ADD_ACTION)) {
    argv[1] = "add";
  } else if (!strcmp(subsys, VIDEO_SUBSYS) 
	     && !strcmp(action, VIDEO_DEL_ACTION)) {
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
