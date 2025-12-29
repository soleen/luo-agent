// SPDX-License-Identifier: GPL-2.0

/*
 * Copyright (c) 2025, Google LLC.
 * Pasha Tatashin <pasha.tatashin@soleen.com>
 */
#define _GNU_SOURCE

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include "luo_defs.h"
#include "server.h"

struct luo_server g_server;

static int set_nonblocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);

	if (flags == -1)
		return -1;

	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static void handle_client_disconnect(struct luo_client *client)
{
	printf("[INFO] Client fd=%d disconnected\n", client->fd);
	epoll_ctl(g_server.epoll_fd, EPOLL_CTL_DEL, client->fd, NULL);
	close(client->fd);
	free(client);
}

static void handle_client_data(struct luo_client *client)
{
	char buf[RECV_BUF_SIZE];
	struct json_object *jobj;
	ssize_t n;

	n = recv(client->fd, buf, sizeof(buf) - 1, 0);
	if (n <= 0) {
		if (n < 0 && errno != EAGAIN)
			warn("recv failed on fd=%d", client->fd);
		handle_client_disconnect(client);
		return;
	}

	buf[n] = '\0';

	/* Minimal JSON parsing check */
	jobj = json_tokener_parse(buf);
	if (!jobj) {
		warnx("Invalid JSON from fd=%d: %s", client->fd, buf);
		return;
	}

	printf("[DEBUG] Received from fd=%d: %s\n", client->fd,
	       json_object_to_json_string(jobj));

	/* TODO: Dispatch based on "cmd" field here */

	json_object_put(jobj);
}

static void handle_new_connection(void)
{
	struct sockaddr_un client_addr;
	struct epoll_event ev = {0};
	struct luo_client *client;
	socklen_t len = sizeof(client_addr);
	int conn_fd;

	conn_fd = accept(g_server.listen_fd, (struct sockaddr *)&client_addr, &len);
	if (conn_fd < 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK)
			warn("accept");
		return;
	}

	if (set_nonblocking(conn_fd))
		warn("set_nonblocking failed");

	client = calloc(1, sizeof(struct luo_client));
	if (!client) {
		warn("calloc failed for new client");
		close(conn_fd);
		return;
	}
	client->fd = conn_fd;

	ev.events = EPOLLIN | EPOLLET;
	ev.data.ptr = client;

	if (epoll_ctl(g_server.epoll_fd, EPOLL_CTL_ADD, conn_fd, &ev) < 0) {
		warn("epoll_ctl failed for conn_fd");
		free(client);
		close(conn_fd);
		return;
	}

	printf("[INFO] New connection accepted (fd=%d)\n", conn_fd);
}

int server_init(void)
{
	struct sockaddr_un addr = { .sun_family = AF_UNIX };
	struct epoll_event ev = {0};

	g_server.listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (g_server.listen_fd < 0) {
		warn("socket creation failed");
		return -1;
	}

	strncpy(addr.sun_path, LUOD_SOCKET_PATH, sizeof(addr.sun_path) - 1);

	mkdir(LUOD_RUN_DIR, 0755);
	unlink(LUOD_SOCKET_PATH);

	if (bind(g_server.listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		warn("bind failed on %s", LUOD_SOCKET_PATH);

		return -1;
	}

	if (listen(g_server.listen_fd, 64) < 0) {
		warn("listen failed");

		return -1;
	}

	if (set_nonblocking(g_server.listen_fd)) {
		warn("set_nonblocking failed");

		return -1;
	}

	g_server.epoll_fd = epoll_create1(0);
	if (g_server.epoll_fd < 0) {
		warn("epoll_create1 failed");

		return -1;
	}

	ev.events = EPOLLIN; /* Level triggered for listen socket */
	ev.data.fd = g_server.listen_fd;

	if (epoll_ctl(g_server.epoll_fd, EPOLL_CTL_ADD, g_server.listen_fd, &ev) < 0) {
		warn("epoll_ctl failed for listen_fd");

		return -1;
	}

	g_server.running = true;
	printf("[INFO] Listening on %s\n", LUOD_SOCKET_PATH);

	return 0;
}

void server_run(void)
{
	struct epoll_event events[MAX_EVENTS];
	int n, i;

	while (g_server.running) {
		n = epoll_wait(g_server.epoll_fd, events, MAX_EVENTS, -1);
		if (n < 0) {
			if (errno == EINTR)
				continue;
			warn("epoll_wait");
			break;
		}

		for (i = 0; i < n; i++) {
			if (events[i].data.fd == g_server.listen_fd)
				handle_new_connection();
			else
				handle_client_data((struct luo_client *)events[i].data.ptr);
		}
	}
}

void server_cleanup(void)
{
	if (g_server.listen_fd >= 0)
		close(g_server.listen_fd);

	if (g_server.epoll_fd >= 0)
		close(g_server.epoll_fd);

	unlink(LUOD_SOCKET_PATH);
}
