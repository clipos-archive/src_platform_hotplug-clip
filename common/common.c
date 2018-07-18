// SPDX-License-Identifier: GPL-2.0
// Copyright © 2009-2018 ANSSI. All Rights Reserved.
/**
 * @file common.c
 * CLIP hotplug common functions.
 * @author Vincent Strubel <clipos@ssi.gouv.fr>
 *
 * Copyright (C) 2009 SGDN/DCSSI
 * Copyright (C) 2010-2014 SGDSN/ANSSI
 * @n
 * All rights reserved.
 */

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "common.h"

static void 
do_log_syslog(int lvl, const char *fmt, va_list args)
{
	int prio = LOG_DAEMON;

	switch (lvl) {
		case 0:
			prio |= LOG_ERR;
			break;
		case 1:
			prio |= LOG_INFO;
			break;
		default:
			prio |= LOG_DEBUG;
			break;
	}

	vsyslog(prio, fmt, args);
}

static void
do_log_std(int lvl, const char *fmt, va_list args)
{
	if (!lvl)
		vfprintf(stderr, fmt, args);
	else
		vfprintf(stdout, fmt, args);
}

void
do_log(int lvl, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	if (g_foreground)
		do_log_std(lvl, fmt, args);
	else 
		do_log_syslog(lvl, fmt, args);

	va_end(args);
}

static inline void __attribute__((noreturn))
run_external_child(char *cmd, char **argv, char **envp)
{
	int fd;
	char *const args[] = { cmd, NULL };

	fd = open("/dev/null", O_RDWR|O_NOFOLLOW);
	if (fd < 0) {
		ERROR_ERRNO("failed to open /dev/null");
		exit(-1);
	}
	if (dup2(fd, STDIN_FILENO) < 0) {
		ERROR_ERRNO("failed to set stdin for script");
		exit(-1);
	}
	if (dup2(fd, STDOUT_FILENO) < 0) {
		ERROR_ERRNO("failed to set stdout for script");
		exit(-1);
	}
	if (dup2(fd, STDERR_FILENO) < 0) {
		ERROR_ERRNO("failed to set stderr for script");
		exit(-1);
	}
	if (fd > STDERR_FILENO) /* fd could already be stderr */
		(void)close(fd);

	if (argv)
		execve(cmd, argv, envp);
	else 
		execve(cmd, args, envp);
	ERROR_ERRNO("execve %s failed", cmd);
	exit(-1);
}

int
run_external(char *cmd, char **argv, char **envp)
{
	pid_t pid, wret;
	int status;

	pid = fork();

	switch (pid) {
		case -1:
			ERROR_ERRNO("fork %s failed", cmd);
			return -1;
		case 0:
			run_external_child(cmd, argv, envp);
			/* Not reached */
			break;
		default:
			break;
	}

	wret = waitpid(pid, &status, 0);
	if (wret < 0) {
		ERROR_ERRNO("Waitpid %s error", cmd);
		return -1;
	}

	if (wret != pid) {
		ERROR("Weird waitpid() return: %d != %d", wret, pid);
		return -1;
	}

	if (WIFEXITED(status) && !WEXITSTATUS(status))
		return 0;

	if (WIFSIGNALED(status))
		ERROR("Child %s (%d) killed by signal %d", 
			cmd, pid, WTERMSIG(status));

	return -1;
}
