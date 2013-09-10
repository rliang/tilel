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

#include "input.h"
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include "tilel.h"
#include "manager.h"

static void input_interpret(char cmd, int target)
{
	if (all_windows.len < 1)
		return;

	bool relative = !isupper(cmd);
	cmd = tolower(cmd);

	if (cmd == 'a')
		manager_activate(target, relative);
	else if (cmd == 'm')
		manager_move(target, relative);
}

void input_parse()
{
	char buf[BUFSIZ];

	int len = read(polls[0].fd, buf, BUFSIZ);
	if (len < 0) 
		quit();

	char cmd = buf[0];
	int target = atoi(&buf[1]);
	input_interpret(cmd, target);
}

/* vim: set ts=4 sw=4 noet: */
