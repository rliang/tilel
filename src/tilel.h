#ifndef TILEL_H
#define TILEL_H

#include <poll.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <xcb/xcb_ewmh.h>

#include "windowlist.h"

#define SCRIPT_PATH "~/.tilel"
#define PIPE_PATH "/tmp/tilel.fifo"
#define PIPE_FLAGS O_RDONLY | O_NONBLOCK
#define PIPE_MODE S_IRWXU

const char *script_path;
const char *pipe_path;
const int pipe_flags;
const int pipe_mode;

struct pollfd polls[2];
xcb_connection_t *xcb;
xcb_ewmh_connection_t ewmh;
int screen;

uint32_t screen_xywh[4];
struct windowlist all_windows;

void quit();

#endif /* TILEL_H */
