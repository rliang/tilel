#include "script.h"
#include <stdio.h>
#include "tilel.h"
#include "wrappers.h"

FILE *popen(const char *, const char *);
int pclose(FILE *);

static void script_parse(char *buf,
		uint32_t *x, uint32_t *y, uint32_t *width, uint32_t *height)
{
	uint32_t *output[4] = { x, y, width, height };
	for (uint32_t i = 0; i < 4; ++i) {
		char *space = buf;
		*output[i] = strtoul(buf, &space, 10);
		buf = space;
	}
}

static void script_read(FILE *f)
{
	uint32_t x, y, width, height;
	char buf[BUFSIZ];

	for (uint32_t i = 0; i < windows_len; ++i) {
		if (fgets(buf, BUFSIZ, f) == NULL)
			break;
		script_parse(buf, &x, &y, &width, &height);
		wrapper_move_resize(windows[i], x, y, width, height);
	}
}

static FILE *script_open()
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
