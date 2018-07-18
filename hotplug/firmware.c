// SPDX-License-Identifier: GPL-2.0
// Copyright © 2009-2018 ANSSI. All Rights Reserved.
/**
 * @file firmware.c
 * CLIP hotplug firmware loading functions.
 * @author Vincent Strubel <clipos@ssi.gouv.fr>
 *
 * Copyright (C) 2009 SGDN
 * Copyright (C) 2010 ANSSI
 * @n
 * All rights reserved.
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "hotplug.h"

#define FW_PATH 	"/lib/firmware"
#define FW_SUBSYS	"firmware"
#define FW_ACTION	"add"

static int 
semaphore_init(void)
{
	key_t key;
	int semid;

	key = ftok("/sbin/hotplug", 42);
	if (key < 0) {
		ERROR_ERRNO("Failed to generate ipc key");
		return -1;
	}

	semid = semget(key, 1, 0600 | IPC_CREAT);
	if (semid < 0) {
		ERROR_ERRNO("Failed to create semaphore");
		return -1;
	}
	return semid;
}

static int
semaphore_lock(int semid)
{
	struct sembuf sem[2];
	sem[0].sem_num = 0;
	sem[0].sem_flg = SEM_UNDO;
	sem[0].sem_op = 0; /* Wait for unlocked */
	sem[1].sem_num = 0;
	sem[1].sem_flg = SEM_UNDO;
	sem[1].sem_op = 1; /* Lock */

	if (semop(semid, sem, 2) < 0) {
		ERROR_ERRNO("Failed to lock semaphore %d", semid);
		return -1;
	}
	return 0;
}

static int
semaphore_unlock(int semid)
{
	struct sembuf sem;
	sem.sem_num = 0;
	sem.sem_flg = SEM_UNDO;
	sem.sem_op = -1; /* Unlock */

	if (semop(semid, &sem, 1) < 0) {
		ERROR_ERRNO("Failed to unlock semaphore %d", semid);
		return -1;
	}

	return 0;
}

static inline int
write_status(const char *path, int status)
{
	char buf[4];
	int len, fd;
	ssize_t wlen;

	len = snprintf(buf, 4, "%d", status);
	if (len < 0 || len >= 4) {
		ERROR("Truncated status %d", status);
		return -1;
	}

	fd = open(path, O_WRONLY|O_NOATIME|O_SYNC);
	if (fd == -1) {
		ERROR_ERRNO("Failed to open %s", path);
		return -1;
	}

retry:
	wlen = write(fd, buf, len);
	if (wlen < 0) {
		if (errno == EINTR)
			goto retry;
		ERROR_ERRNO("Failed to write status to %s", path);
		(void)close(fd);
		return -1;
	}
	(void)fsync(fd);
	(void)fdatasync(fd);
	(void)close(fd);
	if (wlen != len) {
		ERROR("Truncated write to %s : %d != %d", path, wlen, len);
		return -1;
	}

	return 0;
}

static inline int
write_content(const char *path, const char *content, size_t len)
{
	ssize_t wlen;
	const char *ptr;
	int fd, ret = -1;

	fd = open(path, O_WRONLY|O_NOATIME|O_SYNC);
	if (fd == -1) {
		ERROR_ERRNO("Failed to open %s", path);
		return -1;
	}

	ptr = content;

	while (len) {
		wlen = write(fd, ptr, len);
		if (wlen < 0) {
			if (errno == EINTR)
				continue;
			ERROR_ERRNO("Failed to write contents to %s", path);
			goto out;
		}

		len -= wlen;
		ptr += wlen;
	}
	
	ret = 0;
	/* Fall through */
out:
	(void)fsync(fd);
	(void)fdatasync(fd);
	(void)close(fd);
	return ret;
}

static inline int
write_error(const char *devpath)
{
	char *spath = NULL;
	int ret = -1;

	if (asprintf(&spath, "/sys%s/loading", devpath) == -1) {
		ERROR("Out of memory allocating sysfs path for %s", devpath);
		return ret;
	}

	ret = write_status(spath, -1);

	if (spath)
		free(spath);
	return ret;
}

static inline int
do_install(const char *devpath, const char *content, size_t len)
{
	char *dpath = NULL, *spath = NULL;
	int ret = -1;

	if (asprintf(&dpath, "/sys%s/data", devpath) == -1 
			|| asprintf(&spath, "/sys/%s/loading", devpath) == -1) {
		ERROR("Out of memory allocating sysfs path for %s", devpath);
		goto out;
	}

	if (write_status(spath, 1)) 
		goto out;

	if (write_content(dpath, content, len)) {
		(void)write_status(spath, -1);
		goto out;
	}
	ret = write_status(spath, 0);
	/* Fall through */
out:
	if (dpath)
		free(dpath);
	if (spath)
		free(spath);
	return ret;
}

static inline int
install_firmware(const char *fw, const char *devpath)
{
	char *cpath = NULL, *content = NULL;
	int fd = -1, ret = -1;
	struct stat st;
	size_t clen;

	if (asprintf(&cpath, FW_PATH"/%s", fw) == -1) {
		ERROR("Out of memory allocating firmware name");
		goto out;
	}

	fd = open(cpath, O_RDONLY|O_NOATIME);
	if (fd == -1) {
		ERROR_ERRNO("Could not open firmware at %s", cpath);
		goto out;
	}

	if (fstat(fd, &st)) {
		ERROR_ERRNO("Could not stat firmware at %s", cpath);
		goto out;
	}

	clen = st.st_size;
	if (!clen) {
		ERROR("Empty firmware at %s", cpath);
		goto out;
	}

	content = mmap(NULL, clen, PROT_READ, MAP_SHARED|MAP_POPULATE, fd, 0);
	if (content == MAP_FAILED) {
		ERROR_ERRNO("Could not map the firmware at %s", cpath);
		goto out;
	}

	ret = do_install(devpath, content, clen);
	/* Fall through */
out:
	if (ret != 0)
		write_error(devpath);

	if (cpath)
		free(cpath);
	if (content)
		(void)munmap(content, clen);

	if (fd != -1)
		(void)close(fd);

	return ret;
}
		
HANDLER(firmware_handler, subsys, action, envp __U)
{
	const char *fw, *devpath;
	int ret, semid;

	/* We only deal with firmware loads */
	if (strcmp(subsys, FW_SUBSYS) || strcmp(action, FW_ACTION))
		return 0;

	fw = getenv("FIRMWARE");
	if (!fw) {
		ERROR("Firmware loading requested without a firmware name");
		return -1;
	}
	
	devpath = getenv("DEVPATH");
	if (!devpath) {
		ERROR("Firmware %s loading requested without DEVPATH", fw);
		return -1;
	}

	DEBUG("Firmware loading requested : %s", fw);

	semid = semaphore_init();
	if (semid < 0)
		return -1;
	if (semaphore_lock(semid))
		return -1;

	ret = install_firmware(fw, devpath);

	if (semaphore_unlock(semid))
		return -1;

	return (ret) ? -1 : 1;
}

/* 
 * Public version for use by initrd hotplug.
 */
int firmware_try_load(const char *, const char *, char **);

int 
firmware_try_load(const char *subsys, const char *action, char **envp)
{
	return firmware_handler(subsys, action, envp);
}
