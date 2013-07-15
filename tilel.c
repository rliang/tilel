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

#include <poll.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>

#define SCRIPT_PATH "~/.xct"
#define PIPE_PATH "/tmp/xct.fifo"
#define PIPE_MODE S_IRUSR | S_IWUSR
#define PIPE_FLAGS O_RDONLY | O_NONBLOCK 

/***************
 *  Variables  *
 ***************/

static const char *script_path = SCRIPT_PATH;
static const char *pipe_path = PIPE_PATH;
static const int pipe_flags = PIPE_FLAGS;
static const int pipe_mode = PIPE_MODE;

static struct pollfd polls[2];
static xcb_connection_t *xcb;
static xcb_ewmh_connection_t ewmh;
static int screen;

static uint32_t screen_x;
static uint32_t screen_y;
static uint32_t screen_width;
static uint32_t screen_height;

static xcb_window_t *windows;
static uint32_t windows_len;

/****************
 *  Prototypes  *
 ****************/

FILE *popen(const char *command, const char *type);
int pclose(FILE *stream);

void setup();
void setup_connection();
void setup_workarea();
void setup_windows();
void setup_polls();

void run();
void cleanup();
void quit();

void input_parse();
void input_interpret(char cmd, int target);

void event_parse();
void event_interpret(xcb_atom_t a);

void script();
FILE *script_open();
void script_read(FILE *f);
void script_parse(char *buf,
		uint32_t *x, uint32_t *y, uint32_t *width, uint32_t *height);

xcb_window_t get_active_window();
uint32_t get_current_desktop();
void get_client_list(xcb_window_t **loc, uint32_t *len_loc);
uint32_t get_wm_desktop(xcb_window_t w);
void get_wm_window_type(xcb_window_t w,
		xcb_atom_t **loc, uint32_t *len_loc);
void get_frame_extents(xcb_window_t w,
		uint32_t *left, uint32_t *right, uint32_t *top, uint32_t *bottom);

bool window_is_allowed(xcb_window_t w);
bool window_allowed_by_desktop(xcb_window_t w);
bool window_allowed_by_type(xcb_window_t w);

void request_change_active(xcb_window_t w);
void request_move_resize(xcb_window_t w,
		uint32_t x, uint32_t y, uint32_t width, uint32_t height);

int windowlist_search(xcb_window_t key, xcb_window_t *list, uint32_t len);
void windowlist_filter(xcb_window_t **loc, uint32_t *loc_len);
int windowlist_search_exclusive(xcb_window_t *ex, uint32_t ex_len,
		xcb_window_t *list, uint32_t len);

void windows_update();
int windows_search(xcb_window_t key);
void windows_replace(xcb_window_t *list);
void windows_insert_from(xcb_window_t *list, uint32_t len);
void windows_remove_not_in(xcb_window_t *list, uint32_t len);
int windows_target_absolute(int count);
int windows_target_relative(int count, int relativeto);

/***************
 *  Functions  *
 ***************/

xcb_window_t get_active_window()
{
	xcb_get_property_cookie_t c =
		xcb_ewmh_get_active_window_unchecked(&ewmh, screen);
	xcb_window_t f = 0;
	xcb_ewmh_get_active_window_reply(&ewmh, c, &f, NULL);
	return f;
}

uint32_t get_current_desktop()
{
	xcb_get_property_cookie_t c =
		xcb_ewmh_get_current_desktop_unchecked(&ewmh, screen);
	uint32_t desktop = 0;
	xcb_ewmh_get_current_desktop_reply(&ewmh, c, &desktop, NULL);
	return desktop;
}

uint32_t get_wm_desktop(xcb_window_t w)
{
	xcb_get_property_cookie_t c = xcb_ewmh_get_wm_desktop_unchecked(&ewmh, w);
	uint32_t desktop = 0;
	xcb_ewmh_get_wm_desktop_reply(&ewmh, c, &desktop, NULL);
	return desktop;
}

void get_client_list(xcb_window_t **loc, uint32_t *len_loc)
{
	*loc = NULL;
	*len_loc = 0;

	xcb_get_property_cookie_t c =
		xcb_ewmh_get_client_list_unchecked(&ewmh, screen);
	xcb_ewmh_get_windows_reply_t r;
	if (!xcb_ewmh_get_client_list_reply(&ewmh, c, &r, NULL))
		return;

	uint32_t bytes = r.windows_len * sizeof(xcb_window_t);
	*loc = malloc(bytes);
	*len_loc = r.windows_len;
	memcpy(*loc, r.windows, bytes);

	xcb_ewmh_get_windows_reply_wipe(&r);
}

void get_wm_window_type(xcb_window_t w, xcb_atom_t **loc, uint32_t *len_loc)
{
	*loc = NULL;
	*len_loc = 0;

	xcb_get_property_cookie_t c =
		xcb_ewmh_get_wm_window_type_unchecked(&ewmh, w);
	xcb_ewmh_get_atoms_reply_t r;
	if (!xcb_ewmh_get_wm_window_type_reply(&ewmh, c, &r, NULL))
		return;

	uint32_t bytes = r.atoms_len * sizeof(xcb_atom_t);
	*loc = malloc(bytes);
	*len_loc = r.atoms_len;
	memcpy(*loc, r.atoms, bytes);

	xcb_ewmh_get_atoms_reply_wipe(&r);
}

void get_frame_extents(xcb_window_t w,
		uint32_t *left, uint32_t *right, uint32_t *top, uint32_t *bottom)
{
	xcb_get_property_cookie_t c = 
		xcb_ewmh_get_frame_extents_unchecked(&ewmh, w);
	xcb_ewmh_get_extents_reply_t r;
	xcb_ewmh_get_frame_extents_reply(&ewmh, c, &r, NULL);
	*left = r.left;
	*right = r.right;
	*top = r.top;
	*bottom = r.bottom;
}

void request_change_active(xcb_window_t w)
{
	xcb_window_t f = get_active_window();
	xcb_ewmh_request_change_active_window(&ewmh, screen, w,
			XCB_EWMH_CLIENT_SOURCE_TYPE_OTHER, XCB_CURRENT_TIME, f);
}

void request_move_resize(xcb_window_t w, 
		uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	uint32_t f_left, f_right, f_top, f_bottom;
	get_frame_extents(w, &f_left, &f_right, &f_top, &f_bottom);

	x += screen_x;
	y += screen_y;
	width -= (f_left + f_right);
	height -= (f_top + f_bottom);

	xcb_ewmh_request_moveresize_window(&ewmh, screen, w, 0,
			XCB_EWMH_CLIENT_SOURCE_TYPE_NORMAL,
			XCB_EWMH_MOVERESIZE_WINDOW_X |
			XCB_EWMH_MOVERESIZE_WINDOW_Y |
			XCB_EWMH_MOVERESIZE_WINDOW_WIDTH |
			XCB_EWMH_MOVERESIZE_WINDOW_HEIGHT,
			x, y, width, height);
}

bool window_allowed_by_desktop(xcb_window_t w)
{
	return get_wm_desktop(w) == get_current_desktop();
}

bool window_allowed_by_type(xcb_window_t w)
{
	xcb_atom_t *atoms;
	uint32_t atoms_len;
	get_wm_window_type(w, &atoms, &atoms_len);

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

int windowlist_search(xcb_window_t key,
		xcb_window_t *w, uint32_t len)
{
	for (uint32_t i = 0; i < len; ++i) 
		if (w[i] == key)
			return i;
	return -1;
}

int windowlist_search_exclusive(xcb_window_t *ex, uint32_t ex_len,
		xcb_window_t *list, uint32_t len)
{
	int index = -1;
	for (uint32_t i = 0; i < ex_len; ++i)
		if (windowlist_search(ex[i], list, len) < 0) {
			index = i;
			break;
		} 
	return index;
}

void windowlist_filter(xcb_window_t **loc, uint32_t *len_loc)
{
	uint32_t new_len = *len_loc;

	for (uint32_t i = 0; i < *len_loc; ++i) {
		if (window_allowed((*loc)[i])) 
			continue;
		(*loc)[i] = 0;
		new_len -= 1;
	}

	xcb_window_t *new = malloc(new_len * sizeof(xcb_window_t));

	uint32_t new_i = 0;
	for (uint32_t i = 0; i < *len_loc; ++i) 
		if ((*loc)[i] != 0)
			new[new_i++] = (*loc)[i];

	free(*loc);
	*loc = new;
	*len_loc = new_len;
}

void windows_replace(xcb_window_t *new)
{
	free(windows);
	windows = new;
}

int windows_search(xcb_window_t key)
{
	return windowlist_search(key, windows, windows_len);
}

void windows_remove_not_in(xcb_window_t *loc, uint32_t len)
{
	int i = windowlist_search_exclusive(windows, windows_len, loc, len);
	if (i < 0) 
		return;

	windows_len -= 1;

	xcb_window_t *new = malloc(windows_len * sizeof(xcb_window_t));
	memcpy(new, windows,
			i * sizeof(xcb_window_t));
	memcpy(&new[i], &windows[i + 1],
			(windows_len - i) * sizeof(xcb_window_t));

	windows_replace(new);
	windows_remove_not_in(loc, len);
}

void windows_insert_from(xcb_window_t *loc, uint32_t len)
{
	int i = windowlist_search_exclusive(loc, len, windows, windows_len);
	if (i < 0) 
		return;

	windows_len += 1;

	xcb_window_t *new = malloc(windows_len * sizeof(xcb_window_t));
	memcpy(&new[1], windows,
			(windows_len - 1) * sizeof(xcb_window_t));
	new[0] = loc[i];

	windows_replace(new);
	windows_insert_from(loc, len);
}

void windows_update()
{
	xcb_window_t *fetch;
	uint32_t fetch_len;
	get_client_list(&fetch, &fetch_len);
	windowlist_filter(&fetch, &fetch_len);

	if (windows_len == 0 || fetch_len == 0) {
		windows_len = fetch_len;
		windows_replace(fetch);
	} else {
		windows_remove_not_in(fetch, fetch_len);
		windows_insert_from(fetch, fetch_len);
		free(fetch);
	}
}

int windows_target_relative(int count, int relativeto)
{
	count += relativeto;
	while (count >= (int)windows_len)
		count -= windows_len;
	while (count < 0)
		count += windows_len;
	return count;
}

int windows_target_absolute(int count)
{
	if (count < 0)
		count = 0;
	else if (count >= (int)windows_len)
		count = windows_len - 1;
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

void script_read(FILE *f)
{
	uint32_t x, y, width, height;
	char buf[BUFSIZ];

	for (uint32_t i = 0; i < windows_len; ++i) {
		if (fgets(buf, BUFSIZ, f) == NULL)
			break;
		script_parse(buf, &x, &y, &width, &height);
		request_move_resize(windows[i], x, y, width, height);
	}
}

FILE *script_open()
{
	char cmd[BUFSIZ];
	snprintf(cmd, BUFSIZ, "%s %u %u %u", script_path,
			screen_width, screen_height, windows_len);
	return popen(cmd, "r");
}

void script()
{
	if (windows_len < 1)
		return;

	FILE *file = script_open();
	if (file == NULL)
		quit();

	script_read(file);
	pclose(file);
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
	windows_len = 0;
	windows = malloc(0);
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
	bool update = true;

	if (a == ewmh._NET_CLIENT_LIST || a == ewmh._NET_CURRENT_DESKTOP)
		windows_update();
	else if (a == ewmh._NET_WORKAREA)
		setup_workarea();
	else
		update = false;

	if (update)
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
	if (windows_len < 1)
		return;

	int active = windows_search(get_active_window());
	target = isupper(cmd) ?
		windows_target_absolute(target) :
		windows_target_relative(target, active);

	cmd = tolower(cmd);
	if (cmd == 'a')  {
		request_change_active(windows[target]);
	} else if (cmd == 'm') {
		xcb_window_t tmp = windows[active];
		windows[active] = windows[target];
		windows[target] = tmp;
		script();
	}
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
	free(windows);
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
