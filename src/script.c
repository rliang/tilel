/*
 * Copyright (C) 2013 Ricardo Liang <ricardoliang@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "script.h"
#include <stdio.h>
#include "tilel.h"
#include "wrappers.h"

FILE *popen(const char *, const char *);
int pclose(FILE *);

static void script_parse(char *buf, uint32_t xywh[4])
{
	for (uint32_t i = 0; i < 4; ++i) {
		char *space = buf;
		xywh[i] = strtoul(buf, &space, 10);
		buf = space;
	}
}

static void script_read(FILE *f)
{
	uint32_t xywh[4];
	char buf[BUFSIZ];

	for (uint32_t i = 0; i < all_windows.len; ++i) {
		if (fgets(buf, BUFSIZ, f) == NULL)
			break;
		script_parse(buf, xywh);
		wrapper_move_resize(all_windows.wins[i], xywh);
	}
}

static FILE *script_open()
{
	char cmd[BUFSIZ];
	snprintf(cmd, BUFSIZ, "%s/%s %u %u %u", getenv("HOME"), script_path,
			screen_xywh[2], screen_xywh[3], all_windows.len);
	return popen(cmd, "r");
}

void script()
{
	if (all_windows.len < 1)
		return;

	FILE *file = script_open();
	if (file == NULL)
		quit();

	script_read(file);
	pclose(file);
}
