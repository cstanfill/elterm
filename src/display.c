#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <X11/Xft/Xft.h>
#include <X11/extensions/Xdbe.h>

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

    default_colors[5].red = 0xFFFF;
    default_colors[5].blue = 0xFFFF;
    default_colors[5].green = 0;
    default_colors[5].alpha = 0xFFFF;

    default_colors[6].red = 0;
    default_colors[6].blue = 0xFFFF;
    default_colors[6].green = 0xFFFF;
    default_colors[6].alpha = 0xFFFF;

    default_colors[7].red = 0xFFFF;
    default_colors[7].blue = 0xFFFF;
    default_colors[7].green = 0xFFFF;
    default_colors[7].alpha = 0xFFFF;

    printf("Initialized display\n");
    return 0;
}

// I have no idea how this works
Visual *get_visual(Display *display) {
    int screen_ct = 1;
    Drawable screens[] = { DefaultRootWindow(display) };
    XdbeScreenVisualInfo *info = XdbeGetVisualInfo(display, screens, &screen_ct);
    if (info == NULL || screen_ct < 1 || info->count < 1) {
        printf("No visuals support Xdbe.\n");
        exit(1);
    }

    XVisualInfo xvis_temp;
    xvis_temp.visualid = info->visinfo[0].visual;
    xvis_temp.screen = 0;
    xvis_temp.depth = info->visinfo[0].depth;

    int matches;
    XVisualInfo *match = XGetVisualInfo(display, VisualIDMask
                                               | VisualScreenMask
                                               | VisualDepthMask,
                                               &xvis_temp, &matches);
    if (match == NULL || matches < 1) {
        printf("Couldn't find a visual for double buffering.\n");
        exit(1);
    }

    return match->visual;
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
            handle_windowev(&all_screens.screens[i], ev);
        }
    }
}

void handle_windowev(screen_t *window, XEvent ev) {
    if (ev.type == KeyPress) {
        char buffer[9];
        memset(buffer, 0, 9);
        KeySym symbol;
        XComposeStatus status;
        int ct = XLookupString(&(ev.xkey), buffer, 8, &symbol, &status);
        if (ct == 0) { // special keys
            int keysymct;
            KeySym *syms = XGetKeyboardMapping(window->display, ev.xkey.keycode,
                    1, &keysymct);
            for (int i = 0; i < keysymct; ++i) {
                ct = keysym_to_input(syms[i], buffer);
                write(window->pty, buffer, ct);
            }
            XFree(syms);
        } else {
            if (ev.xkey.state & Mod1Mask) {
                char modbuffer[1] = { 27} ; // ESC
                write(window->pty, modbuffer, 1);
            }
            write(window->pty, buffer, ct);
        }
    } else if (ev.type == ResizeRequest) {
        resize_screen(window, ev.xresizerequest.width, ev.xresizerequest.height);
    } else if (ev.type == ConfigureNotify) {
        if (ev.xconfigure.width != window->width ||
            ev.xconfigure.height != window->height) {
            resize_screen(window, ev.xconfigure.width, ev.xconfigure.height);
        }
    } else if (ev.type == Expose) {
        refresh(window);
    } else if (ev.type == MapNotify) {
        refresh(window);
    } else{
        printf("Event %d\n", ev.type);
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
    Visual *vis = get_visual(display);
    XSetWindowAttributes attr;
    attr.background_pixel = BlackPixel(display, screen);
    Window window = XCreateWindow(display, RootWindow(display, screen),
            0, 0, config.term_width * 8, config.term_height * 12, 1,
            CopyFromParent, CopyFromParent, vis, CWBackPixel, &attr);
    XdbeBackBuffer back = XdbeAllocateBackBufferName(display, window, XdbeBackground);
    GC gc = XCreateGC(display, back, 0, NULL);

    XSelectInput(display, window, ExposureMask
                                | KeyPressMask
                                | StructureNotifyMask
                                );
    XMapWindow(display, window);
    XftDraw *d = XftDrawCreate(display, back, vis, XDefaultColormap(display, screen));

    XftColor *colors = calloc(COLOR_CT, sizeof(XftColor));
    for (int i = 0; i < COLOR_CT; ++i) {
        XftColorAllocValue(display, XDefaultVisual(display, screen),
                XDefaultColormap(display, screen), default_colors + i, colors + i);
    }

    XftFont *f = XftFontOpenName(display, screen, "DejaVu Sans Mono:pixelsize=12:antialias=true:autohint=true");

    screen_t res = { display, window, back, gc, screen, d,
        (XftColor*) colors, f, pty, new_buffer(config.term_width, config.term_height),
        800, 800};
    printf("Initialized new window\n");
    XFlush(display);
    return res;
}

void resize_screen(screen_t *screen, int width, int height) {
    int new_cols = width / 8;
    int new_rows = height / 12;
    if (new_cols == screen->buffer->width && new_rows == screen->buffer->height) {
        return;
    }
    buffer_t *resized = new_buffer(new_cols, new_rows);
    for (int x = 0; x < resized->width && x < screen->buffer->width; ++x) {
        for (int y = 0; y < resized->height && y < screen->buffer->height; ++y) {
            resized->contents[x][y] = screen->buffer->contents[x][y];
        }
    }
    resized->unread = screen->buffer->unread;
    resized->cursor = screen->buffer->cursor;
    free_buffer(screen->buffer);
    screen->buffer = resized;
    screen->width = width;
    screen->height = height;
    XftDrawChange(screen->textarea, screen->backBuffer);

    struct winsize newsz;
    newsz.ws_row = new_rows;
    newsz.ws_col = new_cols;

    ioctl(screen->pty, TIOCSWINSZ, &newsz);
}

void render_buffer(screen_t screen) {
    buffer_t *buffer = screen.buffer;
    for (int y = 0; y < buffer->height; ++y) {
        for (int x = 0; x < buffer->width; ++x) {
            char_t entry = buffer->contents[x][y];
            XftChar8 *data = (XftChar8 *)(entry.codepoint);
            if (x == buffer->cursor.x && y == buffer->cursor.y) {
                XSetForeground(screen.display, screen.gc, 0x0000FF00);
                XFillRectangle(screen.display, screen.backBuffer, screen.gc,
                        x * 8, y * 12 + 2, 8, 12);
            } else {
                XRenderColor bg = default_colors[entry.bg];
                XSetForeground(screen.display, screen.gc,
                        (((bg.green>>2)<<8)  & 0x0000FF00) +
                        (((bg.red>>2)<<16)   & 0x00FF0000) +
                        (((bg.blue>>2)<<0)   & 0x000000FF) +
                        (((bg.alpha>>2)<<24) & 0xFF000000));
                XFillRectangle(screen.display, screen.backBuffer, screen.gc,
                        x * 8, y * 12 + 2, 8, 12);
            }
            if (*data != 0) {
                XftDrawString8(screen.textarea, screen.colors + entry.fg, screen.font,
                        x * 8, y * 12 + 12, data, 1);
            }
        }
    }
}

void wipe_screen(screen_t screen) {
    XSetForeground(screen.display, screen.gc, 0xFF000000);
    XFillRectangle(screen.display, screen.backBuffer, screen.gc, 0, 0, screen.width, screen.height);
}

void swap_buffers(screen_t screen) {
    XdbeSwapInfo swap = { .swap_window = screen.window,
                          .swap_action = XdbeUndefined};

    XdbeSwapBuffers(screen.display, &swap, 1);
}

void refresh(screen_t *screen) {
    XdbeBeginIdiom(screen->display);
    wipe_screen(*screen);
    render_buffer(*screen);
    swap_buffers(*screen);
    XdbeEndIdiom(screen->display);
    XFlush(screen->display);
}

int keysym_to_input(KeySym k, char *buffer) {
    // TODO: buffer overflow
    int ct = 0;
    switch (k) {
        case XK_Up:
            buffer[0] = 27;
            buffer[1] = 'O';
            buffer[2] = 'A';
            ct = 3;
            break;
        case XK_Down:
            buffer[0] = 27;
            buffer[1] = 'O';
            buffer[2] = 'B';
            ct = 3;
            break;
        case XK_Right:
            buffer[0] = 27;
            buffer[1] = 'O';
            buffer[2] = 'C';
            ct = 3;
            break;
        case XK_Left:
            buffer[0] = 27;
            buffer[1] = 'O';
            buffer[2] = 'D';
            ct = 3;
            break;
        default:
            ct = 0;
            break;
    }
    return ct;
}
