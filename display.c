#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysymdef.h>
#include "display.h"
static Display *display;

screens_t all_screens = { NULL, 0 };

int display_init() {
    display = XOpenDisplay(NULL);
    if (display == NULL) { return -1; }
    return 0;
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
            10, 10, 200, 200, 1, BlackPixel(display, screen), 
                                 WhitePixel(display, screen));

    XSelectInput(display, window, ExposureMask 
                                | KeyPressMask
                                | StructureNotifyMask
                                );
    XMapWindow(display, window);

    screen_t res = { display, window, pty };
    return res;
}
