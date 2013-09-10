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

#include "events.h"
#include "manager.h"
#include "tilel.h"

static void event_interpret(xcb_atom_t a)
{
	if (a == ewmh._NET_CLIENT_LIST || a == ewmh._NET_CURRENT_DESKTOP)
		manager_update();
	/*
	 *else if (a == ewmh._NET_WORKAREA)
	 *    setup_workarea();
	 */
	else
		return;
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

/* vim: set ts=4 sw=4 noet: */
