// SPDX-License-Identifier: GPL-2.0

/*
 * Copyright (c) 2025, Google LLC.
 * Pasha Tatashin <pasha.tatashin@soleen.com>
 */
#define _GNU_SOURCE

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "luo_defs.h"
#include "server.h"

static void signal_handler(int sig)
{
	if (sig == SIGINT || sig == SIGTERM) {
		printf("\nStopping daemon...\n");
		g_server.running = false;
	}
}

static void retrieve_preserved_state(void)
{
	struct liveupdate_ioctl_retrieve_session args = {
		.size = sizeof(struct liveupdate_ioctl_retrieve_session),
		.name = LUOD_INTERNAL_SESSION,
		.fd = -1,
	};
	int ret;
	int fd;

	printf("[INFO] Probing %s...\n", LUO_DEVICE_PATH);

	fd = open(LUO_DEVICE_PATH, O_RDWR | O_CLOEXEC);
	if (fd < 0) {
		err(EXIT_FAILURE, "Failed to open %s", LUO_DEVICE_PATH);
	}

	ret = ioctl(fd, LIVEUPDATE_IOCTL_RETRIEVE_SESSION, &args);
	if (ret == 0) {
		printf("[INFO] Found preserved session 'luod-internal'. Post-kexec restoration detected.\n");

		/*
		 * In a full implementation, we would deserialize state from args.fd here.
		 * For now, we just acknowledge existence and close it.
		 */
		close(args.fd);
	} else {
		if (errno == ENOENT)
			printf("[INFO] No preserved state found. Starting fresh.\n");
		else
			warn("Probe for 'luod-internal' failed");
	}

	/* We do not hold the device open in the Idle phase. */
	close(fd);
}

int main(void)
{
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	printf("luod starting...\n");

	retrieve_preserved_state();

	if (server_init() < 0)
		errx(EXIT_FAILURE, "Server initialization failed");
	server_run();
	server_cleanup();

	return 0;
}
