#include "script.h"
#include <stdio.h>
#include "tilel.h"
#include "wrappers.h"

FILE *popen(const char *, const char *);
int pclose(FILE *);

static void script_parse(char *buf, uint32_t xywh[4])
{
	for (uint32_t i = 0; i < 4; ++i) {
		char *space = buf;
		xywh[i] = strtoul(buf, &space, 10);
		buf = space;
	}
}

static void script_read(FILE *f)
{
	uint32_t xywh[4];
	char buf[BUFSIZ];

	for (uint32_t i = 0; i < all_windows.len; ++i) {
		if (fgets(buf, BUFSIZ, f) == NULL)
			break;
		script_parse(buf, xywh);
		wrapper_move_resize(all_windows.wins[i], xywh);
	}
}

static FILE *script_open()
{
	char cmd[BUFSIZ];
	snprintf(cmd, BUFSIZ, "%s %u %u %u", script_path,
			screen_width, screen_height, all_windows.len);
	return popen(cmd, "r");
}

void script()
{
	if (all_windows.len < 1)
		return;

	FILE *file = script_open();
	if (file == NULL)
		quit();

	script_read(file);
	pclose(file);
}
