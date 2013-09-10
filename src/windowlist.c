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

#include "windowlist.h"
#include <string.h>

int windowlist_search(struct windowlist *list, xcb_window_t key)
{
	for (uint32_t i = 0; i < list->len; ++i) 
		if (list->wins[i] == key)
			return i;
	return -1;
}

void windowlist_replace(struct windowlist *to, struct windowlist *from)
{
	free(to->wins);
	to->len = from->len;
	to->wins = from->wins;
}

static uint32_t windowlist_zero_disallowed(struct windowlist *list,
		bool (*allowed)(xcb_window_t))
{
	uint32_t new_len = list->len;

	for (uint32_t i = 0; i < list->len; ++i) {
		if (allowed(list->wins[i])) 
			continue;
		list->wins[i] = 0;
		new_len -= 1;
	}

	return new_len;
}

void windowlist_filter(struct windowlist *list, bool (*allowed)(xcb_window_t))
{
	struct windowlist new;
	new.len = windowlist_zero_disallowed(list, allowed);
	new.wins = malloc(new.len * sizeof(xcb_window_t));

	uint32_t new_i = 0;
	for (uint32_t i = 0; i < list->len; ++i) 
		if (list->wins[i] != 0)
			new.wins[new_i++] = list->wins[i];

	windowlist_replace(list, &new);
}

static uint32_t windowlist_subtract(struct windowlist *list,
		struct windowlist *other, bool *difference)
{
	for (uint32_t i = 0; i < list->len; ++i)
		difference[i] = false;

	uint32_t list_ex_len = 0;

	for (uint32_t i = 0; i < list->len; ++i)
		if (windowlist_search(other, list->wins[i]) < 0) {
			difference[i] = true;
			list_ex_len += 1;
		}

	return list_ex_len;
}

void windowlist_stable_replace(struct windowlist *list,
		struct windowlist *replaced)
{
	bool list_ex[list->len];
	bool replaced_ex[replaced->len];
	uint32_t list_ex_len = windowlist_subtract(list, replaced, list_ex);
	uint32_t replaced_ex_len = windowlist_subtract(replaced, list, replaced_ex);

	struct windowlist new;
	new.len = (list->len - list_ex_len) + replaced_ex_len;
	new.wins = malloc(new.len * sizeof(xcb_window_t));

	uint32_t new_i = 0;
	for (uint32_t i = 0; i < list->len; ++i)
		if (!list_ex[i])
			new.wins[new_i++] = list->wins[i];
	for (uint32_t i = 0; i < replaced->len; ++i)
		if (replaced_ex[i])
			new.wins[new_i++] = replaced->wins[i];

	windowlist_replace(list, &new);
	free(replaced->wins);
}

/* vim: set ts=4 sw=4 noet: */
