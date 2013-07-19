#ifndef TILEL_H
#define TILEL_H

#include <poll.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <xcb/xcb_ewmh.h>

#include "windowlist.h"

/***************
 *  Variables  *
 ***************/

const char *script_path;
const char *pipe_path;
const int pipe_flags;
const int pipe_mode;

struct pollfd polls[2];
xcb_connection_t *xcb;
xcb_ewmh_connection_t ewmh;
int screen;

uint32_t screen_x;
uint32_t screen_y;
uint32_t screen_width;
uint32_t screen_height;

struct windowlist all_windows;

/****************
 *  Prototypes  *
 ****************/

void setup();
void setup_connection();
void setup_workarea();
void setup_windows();
void setup_polls();

void run();

void cleanup();
void quit();

#endif /* TILEL_H */
