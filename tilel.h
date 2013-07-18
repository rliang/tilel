#ifndef TILEL_H
#define TILEL_H

#include <poll.h>
#include <fcntl.h>
#include <xcb/xcb_ewmh.h>

/***************
 *  Variables  *
 ***************/

const char *script_path = "~/.tilel";
const char *pipe_path = "/tmp/tilel.fifo";
const int pipe_flags = S_IRUSR | S_IWUSR;
const int pipe_mode = O_RDONLY | O_NONBLOCK;

struct pollfd polls[2];
xcb_connection_t *xcb;
xcb_ewmh_connection_t ewmh;
int screen;

uint32_t screen_x;
uint32_t screen_y;
uint32_t screen_width;
uint32_t screen_height;

xcb_window_t *windows;
uint32_t windows_len;

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
