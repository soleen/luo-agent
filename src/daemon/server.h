/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Copyright (c) 2025, Google LLC.
 * Pasha Tatashin <pasha.tatashin@soleen.com>
 */

#ifndef SERVER_H
#define SERVER_H

#include <linux/liveupdate.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/epoll.h>

#define MAX_EVENTS 64
#define RECV_BUF_SIZE 4096

struct luo_client {
	char session_id[LIVEUPDATE_SESSION_NAME_LENGTH];
	bool is_subscribed;
	int fd;
};

struct luo_server {
	int listen_fd;
	int epoll_fd;
	bool running;
};

extern struct luo_server g_server;

int server_init(void);
void server_run(void);
void server_cleanup(void);

/* The session name used by luod to preserve its own internal registry */
#define LUOD_INTERNAL_SESSION "luod-internal"

#endif /* SERVER_H */
