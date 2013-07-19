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

#include "tilel.h"
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/time.h>
#include <sys/types.h>
#include "windowlist.h"
#include "wrappers.h"
#include "script.h"

const char *script_path = "~/.tilel";
const char *pipe_path = "/tmp/tilel.fifo";
const int pipe_flags = S_IRUSR | S_IWUSR;
const int pipe_mode = O_RDONLY | O_NONBLOCK;

void input_parse();
void input_interpret(char cmd, int target);

void event_parse();
void event_interpret(xcb_atom_t a);

bool window_allowed(xcb_window_t w);
bool window_allowed_by_desktop(xcb_window_t w);
bool window_allowed_by_type(xcb_window_t w);

void windows_update();
int windows_search(xcb_window_t key);
int windows_target_absolute(int count);
int windows_target_relative(int count, int relativeto);

/***************
 *  Functions  *
 ***************/

bool window_allowed_by_desktop(xcb_window_t w)
{
	return wrapper_wm_desktop(w) == wrapper_current_desktop();
}

bool window_allowed_by_type(xcb_window_t w)
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

bool window_allowed(xcb_window_t w)
{
	return window_allowed_by_desktop(w)
		&& window_allowed_by_type(w);
}

int windows_search(xcb_window_t key)
{
	return windowlist_search(&all_windows, key);
}

void windows_update()
{
	struct windowlist fetch;
	fetch.len = wrapper_client_list(&fetch.wins);
	windowlist_filter(&fetch, window_allowed);

	if (all_windows.len == 0 || fetch.len == 0)
		windowlist_replace(&all_windows, &fetch);
	else
		windowlist_intersect(&all_windows, &fetch);
}

int windows_target_relative(int count, int relativeto)
{
	count += relativeto;
	while (count >= (int)all_windows.len)
		count -= all_windows.len;
	while (count < 0)
		count += all_windows.len;
	return count;
}

int windows_target_absolute(int count)
{
	if (count < 0)
		count = 0;
	else if (count >= (int)all_windows.len)
		count = all_windows.len - 1;
	return count;
}

void script_parse(char *buf,
		uint32_t *x, uint32_t *y, uint32_t *width, uint32_t *height)
{
	uint32_t *output[4] = { x, y, width, height };
	for (uint32_t i = 0; i < 4; ++i) {
		char *space = buf;
		*output[i] = strtoul(buf, &space, 10);
		buf = space;
	}
}

void setup_polls()
{
	unlink(pipe_path);
	errno = 0;
	if (mkfifo(pipe_path, pipe_mode) < 0)
		quit();

	polls[0] = (struct pollfd){
		.fd = open(pipe_path, pipe_flags, pipe_mode),
		.events = POLLIN
	};
	polls[1] = (struct pollfd){
		.fd = xcb_get_file_descriptor(xcb),
		.events = POLLIN
	};
}

void setup_windows()
{
	all_windows.len = 0;
	all_windows.wins = malloc(0);
	windows_update();
}

void setup_workarea()
{
	xcb_get_property_cookie_t c = xcb_ewmh_get_workarea(&ewmh, screen);
	xcb_ewmh_get_workarea_reply_t r;
	if (!xcb_ewmh_get_workarea_reply(&ewmh, c, &r, NULL))
		quit();

	screen_x = r.workarea->x;
	screen_y = r.workarea->y;
	screen_width = r.workarea->width;
	screen_height = r.workarea->height;

	xcb_ewmh_get_workarea_reply_wipe(&r);
}

void setup_connection()
{
	xcb = xcb_connect(NULL, &screen);
	if (xcb_connection_has_error(xcb))
		quit();

	xcb_intern_atom_cookie_t *c = xcb_ewmh_init_atoms(xcb, &ewmh);
	if (!xcb_ewmh_init_atoms_replies(&ewmh, c, NULL))
		quit();

	xcb_screen_t *s = xcb_setup_roots_iterator(xcb_get_setup(xcb)).data;
	xcb_change_window_attributes(xcb, s->root, XCB_CW_EVENT_MASK,
			(uint32_t []){ XCB_EVENT_MASK_PROPERTY_CHANGE });
	xcb_flush(xcb);
}

void setup()
{
	setup_connection();
	setup_workarea();
	setup_windows();
	setup_polls();
	script();
}

void event_interpret(xcb_atom_t a)
{
	if (a == ewmh._NET_CLIENT_LIST || a == ewmh._NET_CURRENT_DESKTOP)
		windows_update();
	else if (a == ewmh._NET_WORKAREA)
		setup_workarea();
	else
		return;

	script();
}

void event_parse()
{
	if (xcb_connection_has_error(xcb))
		quit();

	xcb_generic_event_t *xe = xcb_poll_for_event(xcb);

	if ((xe->response_type & ~0x80) == XCB_PROPERTY_NOTIFY)
		event_interpret(((xcb_property_notify_event_t *)xe)->atom);

	free(xe);
}

void input_interpret(char cmd, int target)
{
	if (all_windows.len < 1)
		return;

	int active = windows_search(wrapper_active_window());
	target = isupper(cmd) ?
		windows_target_absolute(target) :
		windows_target_relative(target, active);

	bool needs_refresh = true;
	cmd = tolower(cmd);

	if (cmd == 'a')  {
		wrapper_change_active(all_windows.wins[target]);
		needs_refresh = false;
	} else if (cmd == 'm') {
		xcb_window_t tmp = all_windows.wins[active];
		all_windows.wins[active] = all_windows.wins[target];
		all_windows.wins[target] = tmp;
	}

	if (needs_refresh)
		script();
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

void run()
{
	for (;;) {
		if (!poll(polls, 2, -1))
			continue;
		if (polls[1].revents & POLLIN)
			event_parse();

		if (polls[0].revents & POLLIN)
			input_parse();
		if (polls[0].revents & POLLHUP) {
			close(polls[0].fd);
			polls[0].fd = open(pipe_path, pipe_flags, pipe_mode);
		}

		xcb_flush(xcb);
	}
}

void cleanup()
{
	close(polls[0].fd);
	unlink(pipe_path);
	free(all_windows.wins);
	xcb_ewmh_connection_wipe(&ewmh);
	xcb_flush(xcb);
	xcb_disconnect(xcb);
}

void quit()
{
	perror(NULL);
	exit(errno);
}

int main(void)
{
	signal(SIGTERM, quit);
	atexit(cleanup);
	setup();
	run();
	return EXIT_SUCCESS;
}
