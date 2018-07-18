// SPDX-License-Identifier: GPL-2.0
// Copyright © 2009-2018 ANSSI. All Rights Reserved.
/**
 * @file eventd_netlink.c
 * CLIP eventd netlink event handlers.
 * @author Vincent Strubel <clipos@ssi.gouv.fr>
 *
 * Copyright (C) 2010-2012 SGDSN/ANSSI
 * @n
 * All rights reserved.
 */


#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "eventd.h"

#define CMD_IFACE_UPDOWN "/usr/bin/clip-device-notify"

/*************************************************************/
/*                       Handlers                            */
/*************************************************************/

#define IFACE_NONE	-1
#define IFACE_UP	 1
#define IFACE_DOWN	 2
#define IFACE_REMOVED	 3

static int g_last_iface_index = 0;
static int g_last_iface_status = IFACE_NONE;

static void
iface_updown_notifier(int signum, siginfo_t *info, 
				void *ctx __attribute__((unused)))
{
	char index[8];
	char *argv[] = { CMD_IFACE_UPDOWN, "", 
				"net-interface", index, "down", NULL };

	if (signum != SIGALRM || info->si_code != SI_KERNEL)
		return;

	memset(index, 0, sizeof(index));
	snprintf(index, sizeof(index), "%d", g_last_iface_index);

	switch (g_last_iface_status) {
		case IFACE_UP:
			argv[1] = "add";
			argv[4] = "up";
			break;
		case IFACE_DOWN:
			argv[1] = "add";
			argv[4] = "down";
			break;
		case IFACE_REMOVED:
			argv[1] = "remove";
			break;
		default:
			return;
	}

	LOG("running %s for netlink handler (iface %s - %s, link %s)",
		CMD_IFACE_UPDOWN, argv[3], argv[1], argv[4]);
	run_external((void *)CMD_IFACE_UPDOWN, argv, NULL);
}

static int 
iface_updown_handler(eventfd *efd __attribute__((unused)), 
					struct nlmsghdr *hdr)
{
	struct ifinfomsg *info = NLMSG_DATA(hdr);
	unsigned int flags;
	int index, status = IFACE_NONE;
	sigset_t set;

	if (hdr->nlmsg_len < sizeof(*hdr) + sizeof(*info)) {
		ERROR("truncated NEWLINK message ? %zu vs %zu",
			hdr->nlmsg_len, sizeof(*hdr) + sizeof(*info));
		return -1;
	}

	flags = info->ifi_flags;
	index = info->ifi_index;

	switch (hdr->nlmsg_type) {
		case RTM_NEWLINK:
			if (flags & IFF_UP) 
				status = (flags & IFF_RUNNING) ? 
						IFACE_UP : IFACE_DOWN;
			break;
		case RTM_DELLINK:
			status = IFACE_REMOVED;
			break;
		default:
			ERROR("Unexpected netlink message type: %d", 
							hdr->nlmsg_type);
			return -1;
	}

	if (g_last_iface_status == IFACE_NONE) {
		/* First run, register sighandler */
		struct sigaction action;
		memset(&action, 0, sizeof(action));
		action.sa_sigaction = iface_updown_notifier,
		action.sa_flags = SA_SIGINFO,
		sigemptyset(&(action.sa_mask));

		if (sigaction(SIGALRM, &action, NULL)) {
			ERROR_ERRNO("Failed to register SIGALRM handler");
			return -1;
		}
	}

	(void)sigemptyset(&set);
	(void)sigaddset(&set, SIGALRM);
	(void)sigprocmask(SIG_BLOCK, &set, NULL);

	g_last_iface_status = status;
	g_last_iface_index = index;

	(void)sigprocmask(SIG_UNBLOCK, &set, NULL);

	(void)alarm(1);
	return 0;
}

/*************************************************************/
/*                       Interface                           */
/*************************************************************/

int
open_netlink_eventfd(eventfd *efd)
{
	int fd;
	struct sockaddr_nl addr;

	fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (fd < 0) {
		ERROR_ERRNO("failed to create netlink socket");
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.nl_family = AF_NETLINK;
	addr.nl_pid = getpid();
	addr.nl_groups = RTNLGRP_LINK;

	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr))) {
		ERROR_ERRNO("failed to bind netlink socket");
		(void)close(fd);
		return -1;
	}

	efd->fd = fd;
	return 0;
}

int
read_netlink_event(eventfd *efd)
{
	struct sockaddr_nl addr;
	struct iovec iov;
	struct msghdr msg; 
	struct nlmsghdr hdr;
	int ret = 0;
	char buf[1024];
	char *buf_ptr;
	ssize_t rlen;
	size_t len;

	memset(&msg, 0, sizeof(msg));

	iov.iov_base = buf;
	msg.msg_name = &addr;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	for (;;) { /* Read all messages... */
		memset(&addr, 0, sizeof(addr));
		iov.iov_len = sizeof(buf);
		msg.msg_namelen = sizeof(addr);

		rlen = recvmsg(efd->fd, &msg, MSG_DONTWAIT);
		if (rlen < 0) {
			if (errno == EINTR)
				continue;
			if (errno == EAGAIN) /* No more messages */
				goto out;
			ERROR_ERRNO("recvmsg failed on netlink socket");
			ret = -1;
			goto out;
		}
		if (!rlen) /* EOF */
			goto out;

		if (msg.msg_namelen != sizeof(addr)) {
			ERROR("WTF? Wrong msg namelen on netlink socket: "
				"%zu vs %zu", msg.msg_namelen, sizeof(addr));
			ret = -1;
			continue;
		}
		
		len = rlen;
		buf_ptr = buf;
		while (len >= sizeof(hdr)) {
			memcpy(&hdr, buf_ptr, sizeof(hdr)); 
			if (hdr.nlmsg_len > len 
					|| hdr.nlmsg_len < sizeof(hdr)) {
				ERROR("truncated netlink message");
				ret = -1;
				break;
			}
			if (efd->netlink_handler(efd, &hdr) < 0)
				ret = -1; /* We still handle the next headers */

			len -= NLMSG_ALIGN(hdr.nlmsg_len);
			buf_ptr += NLMSG_ALIGN(hdr.nlmsg_len);
		}
	}
	/* Not reached */

out:
	return ret;
}

static eventfd *
alloc_netlink_eventfd(netlink_handler_t handler)
{
	eventfd *new = NULL;

	new = calloc(1, sizeof(*new));
	if (!new) {
		ERROR("out of memory");
		return NULL;
	}

	new->evtype = NetlinkEvent;
	new->netlink_handler = handler;

	return new;
}

eventfd *
create_netlink_eventfd(netlink_event_t type)
{
	eventfd *efd = NULL;

	switch (type) {
		case NetlinkEventUpdown:
			efd = alloc_netlink_eventfd(iface_updown_handler);
			break;
		default:
			ERROR("Unsupported netlink event type %d", type);
			return NULL;
	}

	if (!efd)
		ERROR("Failed to create netlink eventfd of type %d", type);
	return efd;
}
