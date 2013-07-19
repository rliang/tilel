#ifndef WINDOWLIST_H
#define WINDOWLIST_H

#include <stdbool.h>
#include <xcb/xcb_ewmh.h>

struct windowlist {
	xcb_window_t *wins;
	uint32_t len;
};

int windowlist_search(struct windowlist *list, xcb_window_t key);
void windowlist_replace(struct windowlist *to, struct windowlist *from);
void windowlist_intersect(struct windowlist *list, struct windowlist *ex);
void windowlist_filter(struct windowlist *list, bool (*allowed)(xcb_window_t));

#endif /* WINDOWLIST_H */
