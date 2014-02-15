#ifndef DISPLAY_H
#define DISPLAY_H
#define COLOR_CT 5

#include <stdbool.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xdbe.h>
#include <X11/Xft/Xft.h>
#include "buffer.h"

typedef struct screen_t {
    Display *display;
    Window window;
    XdbeBackBuffer backBuffer;
    int screen;
    XftDraw *textarea;
    XftColor *colors;
    XftFont *font;
    int pty;
    buffer_t *buffer;
} screen_t;

typedef struct screens_t {
    screen_t *screens;
    size_t length;
} screens_t;

extern XRenderColor default_colors[COLOR_CT];
extern screens_t all_screens;
extern int x11fd;

Visual *get_visual(Display *display);

int display_init();
int add_screen(screens_t *screens, screen_t screen);
int remove_screen(screens_t *screens, size_t index);

screen_t new_screen(int pty);

void handle_x11evs();
void handle_x11ev(XEvent ev);
void handle_windowev(screen_t window, XEvent ev);

void wipe_screen(screen_t screen);
void render_buffer(screen_t screen, buffer_t *buffer, int startx, int starty);
#endif
