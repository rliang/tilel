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
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include "windowlist.h"
#include "wrappers.h"
#include "manager.h"
#include "script.h"
#include "events.h"
#include "input.h"

const char *script_path = SCRIPT_PATH;
const char *pipe_path = PIPE_PATH;
const int pipe_flags = PIPE_FLAGS;
const int pipe_mode = PIPE_MODE;

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
	manager_update();
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
