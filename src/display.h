#ifndef DISPLAY_H
#define DISPLAY_H
#define COLOR_CT 8

#include <stdbool.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xdbe.h>
#include <X11/Xft/Xft.h>
#include "buffer.h"

typedef struct screen_t {
    Display *display;
    Window window;
    XdbeBackBuffer backBuffer;
    GC gc;
    int screen;
    XftDraw *textarea;
    XftColor *colors;
    XftFont *font;
    int pty;
    buffer_t *buffer;
    int width;
    int height;
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
void handle_windowev(screen_t *window, XEvent ev);
int keysym_to_input(KeySym k, char *buffer);

void resize_screen(screen_t *screen, int width, int height);

void refresh(screen_t *screen);
void wipe_screen(screen_t screen);
void render_buffer(screen_t screen);
void swap_buffers(screen_t screen);
#endif
