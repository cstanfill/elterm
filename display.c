#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xft/Xft.h>

#include "buffer.h"
#include "config.h"
#include "display.h"
#include "control.h"

static Display *display;

screens_t all_screens = { NULL, 0 };
int x11fd;
XRenderColor default_colors[COLOR_CT];

int display_init() {
    display = XOpenDisplay(NULL);
    if (display == NULL) { return -1; }
    x11fd = ConnectionNumber(display);
    default_colors[0].red = 0;
    default_colors[0].blue = 0;
    default_colors[0].green = 0;
    default_colors[0].alpha = 0xFFFF;

    default_colors[1].red = 0xFFFF;
    default_colors[1].blue = 0;
    default_colors[1].green = 0;
    default_colors[1].alpha = 0xFFFF;

    default_colors[2].red = 0;
    default_colors[2].blue = 0;
    default_colors[2].green = 0xFFFF;
    default_colors[2].alpha = 0xFFFF;

    default_colors[3].red = 0xFFFF;
    default_colors[3].blue = 0;
    default_colors[3].green = 0xFFFF;
    default_colors[3].alpha = 0xFFFF;

    default_colors[4].red = 0;
    default_colors[4].blue = 0xFFFF;
    default_colors[4].green = 0;
    default_colors[4].alpha = 0xFFFF;

    printf("Initialized display\n");
    return 0;
}

void handle_x11evs() {
    XEvent ev;
    while (XPending(display)) {
        XNextEvent(display, &ev);
        handle_x11ev(ev);
    }
}

void handle_x11ev(XEvent ev) {
    Window w = ev.xany.window;
    if (all_screens.screens == NULL) { return; }
    for (size_t i = 0; i < all_screens.length; ++i) {
        if (all_screens.screens[i].window == w) {
            handle_windowev(all_screens.screens[i], ev);
        }
    }
}

void handle_windowev(screen_t window, XEvent ev) {
    if (ev.type == KeyPress) {
        char buffer[8];
        KeySym symbol;
        XComposeStatus status;
        int ct = XLookupString(&(ev.xkey), buffer, 8, &symbol, &status);
        write(window.pty, buffer, ct);
    }
}

int add_screen(screens_t *screens, screen_t screen) {
    if (screens == NULL) { return -1; }
    if (screens->screens == NULL && screens->length > 0) { return -2; }
    screen_t *new_screens = malloc(sizeof(screen_t) * (screens->length + 1));
    for (size_t i = 0; i < screens->length; ++i) {
        new_screens[i] = screens->screens[i];
    }
    new_screens[screens->length] = screen;
    screens->screens = new_screens;
    ++screens->length;
    return 0;
}

int remove_screen(screens_t *screens, size_t index) {
    if (screens == NULL) { return -1; }
    if (screens->screens == NULL) { return -1; } 
    if (screens->length <= index) { return -2; }
    screen_t *new_screens = malloc(sizeof(screen_t) * (screens->length - 1));
    for (size_t i = 0; i < screens->length - 1; ++i) {
        new_screens[i] = screens->screens[(i > index)?(i+1):i];
    }

    screens->screens = new_screens;
    --screens->length;
    return 0;
}

screen_t new_screen(int pty) {
    int screen = DefaultScreen(display);
    Window window = XCreateSimpleWindow(display, RootWindow(display, screen), 
            10, 10, 800, 800, 1, BlackPixel(display, screen),
                                 WhitePixel(display, screen));

    XSelectInput(display, window, ExposureMask 
                                | KeyPressMask
                                | StructureNotifyMask
                                );
    XMapWindow(display, window);
    XftDraw *d = XftDrawCreate(display, window, XDefaultVisual(display, screen),
                                                XDefaultColormap(display, screen));

    XftColor *colors = calloc(COLOR_CT, sizeof(XftColor));
    for (int i = 0; i < COLOR_CT; ++i) {
        XftColorAllocValue(display, XDefaultVisual(display, screen),
                XDefaultColormap(display, screen), default_colors + i, colors + i);
    }

    XftFont *f = XftFontOpenName(display, screen, "DejaVu Sans Mono:pixelsize=12:antialias=true:autohint=true");
    screen_t res = { display, window, screen, d,
        (XftColor*) colors, f, pty, new_buffer(config.term_width, config.term_height) };
    printf("Initialized new window\n");
    XFlush(display);
    return res;
}

void render_buffer(screen_t screen, buffer_t *buffer, int startx, int starty) {
    for (int y = 0; y < buffer->height; ++y) {
        for (int x = 0; x < buffer->width; ++x) {
            char_t entry = buffer->contents[startx+x][starty+y];
            XftChar8 *data = (XftChar8 *)(entry.codepoint);
            if (*data != 0) {
                XftDrawString8(screen.textarea, screen.colors + entry.color, screen.font,
                        x * 8, y * 12 + 12, data, 1);
            }
        }
    }
    XFlush(display);
}

void wipe_screen(screen_t screen) {
    XClearArea(screen.display, screen.window, 0, 0, 800, 800, false);
}
