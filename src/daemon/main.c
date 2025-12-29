// SPDX-License-Identifier: GPL-2.0

/*
 * Copyright (c) 2025, Google LLC.
 * Pasha Tatashin <pasha.tatashin@soleen.com>
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/liveupdate.h>
#include "luo_defs.h"

int main(void)
{
	printf("Max Session Name Length: %d\n", LIVEUPDATE_SESSION_NAME_LENGTH);

	return 0;
}
