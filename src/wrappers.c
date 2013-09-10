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

#include "wrappers.h"
#include <string.h>
#include "tilel.h"

xcb_window_t wrapper_active_window()
{
	xcb_get_property_cookie_t c =
		xcb_ewmh_get_active_window_unchecked(&ewmh, screen);
	xcb_window_t f = 0;
	xcb_ewmh_get_active_window_reply(&ewmh, c, &f, NULL);
	return f;
}

uint32_t wrapper_current_desktop()
{
	xcb_get_property_cookie_t c =
		xcb_ewmh_get_current_desktop_unchecked(&ewmh, screen);
	uint32_t desktop = 0;
	xcb_ewmh_get_current_desktop_reply(&ewmh, c, &desktop, NULL);
	return desktop;
}

uint32_t wrapper_client_list(xcb_window_t **output)
{
	xcb_get_property_cookie_t c =
		xcb_ewmh_get_client_list_unchecked(&ewmh, screen);
	xcb_ewmh_get_windows_reply_t r;
	if (!xcb_ewmh_get_client_list_reply(&ewmh, c, &r, NULL))
		return 0;

	uint32_t len = r.windows_len;
	uint32_t bytes = len * sizeof(xcb_window_t);

	*output = malloc(bytes);
	memcpy(*output, r.windows, bytes);
	xcb_ewmh_get_windows_reply_wipe(&r);

	return len;
}

uint32_t wrapper_wm_desktop(xcb_window_t w)
{
	xcb_get_property_cookie_t c = xcb_ewmh_get_wm_desktop_unchecked(&ewmh, w);
	uint32_t desktop = 0;
	xcb_ewmh_get_wm_desktop_reply(&ewmh, c, &desktop, NULL);
	return desktop;
}

uint32_t wrapper_wm_window_type(xcb_window_t w, xcb_atom_t **output)
{
	xcb_get_property_cookie_t c =
		xcb_ewmh_get_wm_window_type_unchecked(&ewmh, w);
	xcb_ewmh_get_atoms_reply_t r;
	if (!xcb_ewmh_get_wm_window_type_reply(&ewmh, c, &r, NULL))
		return 0;

	uint32_t len = r.atoms_len;
	uint32_t bytes = len * sizeof(xcb_atom_t);

	*output = malloc(bytes);
	memcpy(*output, r.atoms, bytes);
	xcb_ewmh_get_atoms_reply_wipe(&r);

	return len;
}

void wrapper_change_active(xcb_window_t w)
{
	xcb_window_t f = wrapper_active_window();
	xcb_ewmh_request_change_active_window(&ewmh, screen, w,
			XCB_EWMH_CLIENT_SOURCE_TYPE_OTHER, XCB_CURRENT_TIME, f);
}

static void wrapper_frame_extents(xcb_window_t w, uint32_t output[4])
{
	xcb_get_property_cookie_t c = 
		xcb_ewmh_get_frame_extents_unchecked(&ewmh, w);
	xcb_ewmh_get_extents_reply_t r;
	xcb_ewmh_get_frame_extents_reply(&ewmh, c, &r, NULL);
	output[0] = r.left;
	output[1] = r.right;
	output[2] = r.top;
	output[3] = r.bottom;
}

void wrapper_move_resize(xcb_window_t w, uint32_t extents[4])
{
	uint32_t frame[4];
	wrapper_frame_extents(w, frame);

	extents[0] += screen_xywh[0];
	extents[1] += screen_xywh[1];
	extents[2] -= (frame[0] + frame[1]);
	extents[3] -= (frame[2] + frame[3]);

	xcb_ewmh_request_moveresize_window(&ewmh, screen, w,
			XCB_GRAVITY_STATIC,
			XCB_EWMH_CLIENT_SOURCE_TYPE_OTHER,
			XCB_EWMH_MOVERESIZE_WINDOW_X |
			XCB_EWMH_MOVERESIZE_WINDOW_Y |
			XCB_EWMH_MOVERESIZE_WINDOW_WIDTH |
			XCB_EWMH_MOVERESIZE_WINDOW_HEIGHT,
			extents[0], extents[1], extents[2], extents[3]);
}

/* vim: set ts=4 sw=4 noet: */
