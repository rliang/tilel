/*
 * Copyright (C) 2013 shian5 at github.com
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

#include "manager.h"
#include <xcb/xcb.h>
#include "windowlist.h"
#include "wrappers.h"
#include "script.h"
#include "tilel.h"

bool manager_allows_type(xcb_window_t w)
{
	xcb_atom_t *atoms;
	uint32_t atoms_len = wrapper_wm_window_type(w, &atoms);

	if (atoms_len == 0)
		return true;

	bool allowed = false;
	for (uint32_t i = 0; i < atoms_len; ++i)
		if (atoms[i] == ewmh._NET_WM_WINDOW_TYPE_NORMAL) {
			allowed = true;
			break;
		}

	free(atoms);
	return allowed;
}

bool manager_allows_desktop(xcb_window_t w)
{
	return wrapper_wm_desktop(w) == wrapper_current_desktop();
}

bool manager_allows(xcb_window_t w)
{
	return manager_allows_desktop(w)
		&& manager_allows_type(w);
}

void manager_update()
{
	struct windowlist fetch;
	fetch.len = wrapper_client_list(&fetch.wins);
	windowlist_filter(&fetch, manager_allows);

	if (all_windows.len == 0 || fetch.len == 0)
		windowlist_replace(&all_windows, &fetch);
	else
		windowlist_intersect(&all_windows, &fetch);

	script();
}

static int manager_search(xcb_window_t key)
{
	return windowlist_search(&all_windows, key);
}

static int manager_target_absolute(int index)
{
	if (index < 0)
		index = 0;
	else if (index >= (int)all_windows.len)
		index = all_windows.len - 1;
	return index;
}

static int manager_target_relative(int index, int to)
{
	index += to;
	while (index >= (int)all_windows.len)
		index -= all_windows.len;
	while (index < 0)
		index += all_windows.len;
	return index;
}

static int manager_target(int index, bool relative, int to)
{
	return relative ?
		manager_target_relative(index, to) :
		manager_target_absolute(index);
}

void manager_move(int index, bool relative)
{
	int active = manager_search(wrapper_active_window());
	int target = manager_target(index, relative, active);

	xcb_window_t tmp = all_windows.wins[active];
	all_windows.wins[active] = all_windows.wins[target];
	all_windows.wins[target] = tmp;

	script();
}

void manager_activate(int index, bool relative)
{
	int active = manager_search(wrapper_active_window());
	int target = manager_target(index, relative, active);

	wrapper_change_active(all_windows.wins[target]);
}
