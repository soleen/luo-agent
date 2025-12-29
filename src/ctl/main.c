// SPDX-License-Identifier: GPL-2.0

/*
 * Copyright (c) 2025, Google LLC.
 * Pasha Tatashin <pasha.tatashin@soleen.com>
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include "luo_defs.h"

int main(int argc, char *argv[])
{
	if (argc > 1)
	printf("Received command: %s\n", argv[1]);
	else
		printf("No command provided.\n");

	return 0;
}
