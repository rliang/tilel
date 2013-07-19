#ifndef WRAPPERS_H
#define WRAPPERS_H

#include <xcb/xcb.h>

xcb_window_t wrapper_active_window();
uint32_t wrapper_current_desktop();
uint32_t wrapper_client_list(xcb_window_t **output);
uint32_t wrapper_wm_desktop(xcb_window_t w);
uint32_t wrapper_wm_window_type(xcb_window_t w, xcb_atom_t **output);
void wrapper_change_active(xcb_window_t w);
void wrapper_move_resize(xcb_window_t w, uint32_t extents[4]);

#endif /* WRAPPERS_H */
