// SPDX-License-Identifier: GPL-2.0

/*
 * Copyright (c) 2025, Google LLC.
 * Pasha Tatashin <pasha.tatashin@soleen.com>
 */

#include <stdio.h>

void luo_log(const char *msg) {
	fprintf(stderr, "[LUO] %s\n", msg);
}
