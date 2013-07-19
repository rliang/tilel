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

static int windowlist_search_exclusive(struct windowlist *list,
		struct windowlist *ex)
{
	for (uint32_t i = 0; i < ex->len; ++i)
		if (windowlist_search(list, ex->wins[i]) < 0)
			return i;
	return -1;
}

static void windowlist_insert_into(struct windowlist *list,
		struct windowlist *from)
{
	int i = windowlist_search_exclusive(from, list);
	if (i < 0) 
		return;

	struct windowlist new;
	new.len = list->len + 1;
	new.wins = malloc(new.len * sizeof(xcb_window_t));

	memcpy(&new.wins[1], list->wins, list->len * sizeof(xcb_window_t));
	new.wins[0] = from->wins[i];

	windowlist_replace(list, &new);
	windowlist_insert_into(list, from);
}

static void windowlist_remove_from(struct windowlist *list,
		struct windowlist *not_in)
{
	int i = windowlist_search_exclusive(list, not_in);
	if (i < 0) 
		return;

	struct windowlist new;
	new.len = list->len - 1;
	new.wins = malloc(new.len * sizeof(xcb_window_t));

	memcpy(new.wins, list->wins, i * sizeof(xcb_window_t));
	memcpy(&new.wins[i], &list->wins[i + 1], (list->len - i) * sizeof(xcb_window_t));

	windowlist_replace(list, &new);
	windowlist_remove_from(list, not_in);
}

void windowlist_intersect(struct windowlist *list, struct windowlist *ex)
{
	windowlist_remove_from(list, ex);
	windowlist_insert_into(list, ex);
	free(ex);
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
	uint32_t new_len = windowlist_zero_disallowed(list, allowed);
	xcb_window_t *new = malloc(new_len * sizeof(xcb_window_t));

	uint32_t new_i = 0;
	for (uint32_t i = 0; i < list->len; ++i) 
		if (list->wins[i] != 0)
			new[new_i++] = list->wins[i];

	free(list->wins);
	list->wins = new;
	list->len = new_len;
}
