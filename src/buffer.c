#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include "buffer.h"

bool is_printing(char c) {
    return (c <= '~') && (c >= ' ');
}

inline char_t to_cursor_char_t(buffer_t *buffer, char c) {
    return to_char_t(c, buffer->cursor.fgcolor, buffer->cursor.bgcolor);
}


inline char_t to_char_t(char c, int fgcolor, int bgcolor) {
    return (char_t) {{c, 0, 0, 0}, fgcolor, bgcolor, false};
}

void enqueue(queue_t *queue, char c) {
    char *new_queue = calloc(queue->len + 1, sizeof(char));
    for (int i = 0; i < queue->len; ++i) {
        new_queue[i] = queue->contents[i];
    }
    if (queue->contents != NULL) {
        free(queue->contents);
    }
    new_queue[queue->len++] = c;
    queue->contents = new_queue;
}

char dequeue(queue_t *queue) {
    if (queue->len <= 0) { return -1; }
    char c = queue->contents[0];
    dequeue_n(queue, 1);
    return c;
}

void dequeue_n(queue_t *queue, int count) {
    if (count >= queue->len) { queue->contents = NULL; queue->len = 0;
        return; }
    char *new_queue = calloc(queue->len - count, sizeof(char));
    for (int i = 0; i < queue->len - count; ++i) {
        new_queue[i] = queue->contents[count + i];
    }
    if (queue->contents != NULL) {
        free(queue->contents);
    }
    queue->contents = new_queue;
    queue->len -= count;
}

void flush(queue_t *queue) {
    free(queue->contents);
    queue->contents = NULL;
    queue->len = 0;
}

buffer_t *new_buffer(int width, int height) {
    buffer_t *buffer = calloc(1, sizeof(buffer_t));
    if (buffer == NULL) { return NULL; }
    char_t **contents = calloc(width, sizeof(char_t *));
    if (contents == NULL) {
        free(buffer);
        return NULL;
    }
    for (int i = 0; i < width; ++i) {
        contents[i] = calloc(height, sizeof(char_t));
        if (contents[i] == NULL) {
            for (int j = 0; j < i; ++j) {
                free(contents[j]);
                free(contents);
                free(buffer);
                return NULL;
            }
        }
    }
    queue_t *queue = calloc(1, sizeof(queue_t));
    buffer->unread = queue;
    buffer->contents = contents;
    buffer->width = width;
    buffer->height = height;
    buffer->cursor = (cursor_t) { 0, 0, 7, 0 };
    buffer->scroll_top = 0;
    buffer->scroll_bot = height - 1;
    return buffer;
}

void free_buffer(buffer_t *buffer) {
    for (int i = 0; i < buffer->width; ++i) {
        free(buffer->contents[i]);
    }
    free(buffer->contents);
    free(buffer);
}

void write_char_t(buffer_t *buffer, char_t c) {
    buffer->contents[buffer->cursor.x][buffer->cursor.y] = c;
    buffer->contents[buffer->cursor.x][buffer->cursor.y].fg = buffer->cursor.fgcolor;
    buffer->contents[buffer->cursor.x][buffer->cursor.y].bg = buffer->cursor.bgcolor;
    if (buffer->cursor.x == buffer->width - 1) {
        buffer->contents[buffer->cursor.x][buffer->cursor.y].folded = true;
    }
    move_by(buffer, 1, 0);
}

void write_char(buffer_t *buffer, char c) {
    if (c & 0x80) {
        return;
    }
    if (c == '\r') {
        move_to(buffer, 0, buffer->cursor.y);
        return;
    }
    if (c == '\n') {
        move_by(buffer, 0, 1);
        return;
    }
    if (c == '\b') {
        move_by(buffer, -1, 0);
        return;
    }
    if (c == '\a') {
        return;
    }
    if (c == 15) {
        // shift in
        return;
    }
    if (c == 14) {
        // shift out
        return;
    }
    if (is_printing(c) && buffer->unread->len == 0) {
        write_char_t(buffer, to_cursor_char_t(buffer, c));
    } else {
        queue_t *unread = buffer->unread;
        enqueue(unread, c);

        char_or_control result;
        int len;
        if ((len = parse(unread->contents, unread->len, &result)) > 0) {
            printx(unread->contents);
            dequeue_n(unread, len);
            if (!result.ischar) {
                control_char_t action = result.contents.action;
                if (action.type == SETGMODE) {
                    change_color c = action.color;
                    if (c.attribute == 0) {
                        buffer->cursor.fgcolor = 7;
                        buffer->cursor.bgcolor = 0;
                    }

                    if (c.foreground != -1) {
                        buffer->cursor.fgcolor = c.foreground - ANSI_FG_BLACK;
                    }
                    if (c.background != -1) {
                        buffer->cursor.bgcolor = c.background - ANSI_BG_BLACK;
                    }
                } else if (action.type == CURSOR) {
                    buffer->cursor.x = action.position.x;
                    buffer->cursor.y = action.position.y;
                } else if (action.type == CURSOR_REL) {
                    buffer->cursor.x += action.position.x;
                    buffer->cursor.y += action.position.y;
                } else if (action.type == CLEAR) {
                    if (action.clear.region_mask & CL_LINE_DOWN) {
                        clear_down(buffer);
                    }
                    if (action.clear.region_mask & CL_LINE_UP) {
                        clear_up(buffer);
                    }
                    if (action.clear.region_mask & CL_LINE_LEFT) {
                        clear_left(buffer);
                    }
                    if (action.clear.region_mask & CL_LINE_RIGHT) {
                        clear_right(buffer);
                    }
                } else if (action.type == NEXT_INDEX) {
                    move_by(buffer, 0, 1);
                } else if (action.type == PREV_INDEX) {
                    move_by(buffer, 0, -1);
                } else if (action.type == NEXT_LINE) {
                    move_by(buffer, 0, 1);
                    move_to(buffer, 0, buffer->cursor.y);
                }
            }
        }
    }
}
void write_string(buffer_t *buffer, char *string, int ct) {
    for (int i = 0; i < ct; ++i) {
        write_char(buffer, string[i]);
    }
}

#define REQUIRE(x) if (!(x)) { return i+1; }
int parse(char *buffer, int len, char_or_control *result) {
    result->ischar = false;
    result->contents.action.type = NOP;
    if (len > 1 && buffer[len-1] == 27) {
        return len-1;
    }
    if (len >= 2 && buffer[0] == 27) {
        if (buffer[1] == '[') {
            goto ANSI;
        } else {
            switch (buffer[1]) {
                case '=':
                    return 2;
                case 27:
                    return 1;
                case 'M':
                    result->ischar = false;
                    result->contents.action = (control_char_t) { .dummy = (no_args) { PREV_INDEX } };
                    return 2;
                case 'D':
                    result->ischar = false;
                    result->contents.action = (control_char_t) { .dummy = (no_args) { NEXT_INDEX } };
                    return 2;
                case 'E':
                    result->ischar = false;
                    result->contents.action = (control_char_t) { .dummy = (no_args) { NEXT_LINE } };
                    return 2;
                default:
                    printx(buffer);
                    break;
            }
        }
    }
    goto END;

    int *args;
    change_color color_command;
    clear_regions clear_command;

ANSI:
    for (int i = 2; i < len; ++i) {
        args = calloc(i, sizeof(int));
        int ct;
        int arg;
        int x = 0;
        int y = 0;
        switch (buffer[i]) {
            case 'H':
            case 'f':
                REQUIRE((ct =parse_args(buffer, len, args))<=2);
                REQUIRE(ct != 1);
                result->ischar = false;
                if (ct == 2) {
                    x = args[1] - 1;
                    y = args[0] - 1;
                }
                result->contents.action = (control_char_t) { .position = (move_cursor) { CURSOR, x, y } };
                return i+1;
            case 'A':
                ct = parse_args(buffer, len, args);
                REQUIRE(ct < 2);
                arg = (ct == 0)?1:args[0];
                result->ischar = false;
                result->contents.action = (control_char_t) {
                    .position = (move_cursor) { CURSOR_REL,   0,-arg }
                };
                return i+1;
            case 'B':
                ct = parse_args(buffer, len, args);
                REQUIRE(ct < 2);
                arg = (ct == 0)?1:args[0];
                result->ischar = false;
                result->contents.action = (control_char_t) {
                    .position = (move_cursor) { CURSOR_REL,   0, arg }
                };
                return i+1;
            case 'C':
                ct = parse_args(buffer, len, args);
                REQUIRE(ct < 2);
                arg = (ct == 0)?1:args[0];
                result->ischar = false;
                result->contents.action = (control_char_t) {
                    .position = (move_cursor) { CURSOR_REL, arg,   0 }
                };
                return i+1;
            case 'D':
                ct = parse_args(buffer, len, args);
                REQUIRE(ct < 2);
                arg = (ct == 0)?1:args[0];
                result->ischar = false;
                result->contents.action = (control_char_t) {
                    .position = (move_cursor) { CURSOR_REL,-arg,   0 }
                };
                return i+1;

            case 'm':
                REQUIRE((ct = parse_args(buffer, len, args))<=3);
                color_command.attribute = -1;
                color_command.foreground = -1;
                color_command.background = -1;
                for (int j = 0; j < ct; ++j) {
                    if (0 <= args[j] && args[j] <= 8) {
                        color_command.attribute = args[j];
                    } else if (ANSI_FG_BLACK <= args[j] && args[j] <= ANSI_FG_WHITE) {
                        color_command.foreground = args[j];
                    } else if (ANSI_BG_BLACK <= args[j] && args[j] <= ANSI_BG_WHITE) {
                        color_command.background = args[j];
                    }
                }
                color_command.type = SETGMODE;
                result->ischar = false;
                result->contents.action = (control_char_t) { .color = color_command };
                return i+1;
            case 's':
            case 'u':
            case 'J':
                REQUIRE((ct = parse_args(buffer, len, args))<=1);
                clear_command.type = CLEAR;
                clear_command.region_mask = 0;
                if (ct == 0) {
                    clear_command.region_mask |= CL_LINE_DOWN;
                } else if (ct == 1) {
                    switch (args[0]) {
                        case 0:
                            clear_command.region_mask |= CL_LINE_DOWN;
                            break;
                        case 1:
                            clear_command.region_mask |= CL_LINE_UP;
                            break;
                        case 2:
                            clear_command.region_mask |= CL_LINE_DOWN | CL_LINE_UP;
                            break;
                        default:
                            break;
                    }
                }
                result->ischar = false;
                result->contents.action = (control_char_t) { .clear = clear_command };

                return i+1;
            case 'K':
                REQUIRE((ct = parse_args(buffer, len, args))<=1);
                clear_command.type = CLEAR;
                clear_command.region_mask = 0;
                if (ct == 0) {
                    clear_command.region_mask |= CL_LINE_RIGHT;
                } else if (ct == 1) {
                    switch (args[0]) {
                        case 0:
                            clear_command.region_mask |= CL_LINE_RIGHT;
                            break;
                        case 1:
                            clear_command.region_mask |= CL_LINE_LEFT;
                            break;
                        case 2:
                            clear_command.region_mask |= CL_LINE_RIGHT | CL_LINE_LEFT;
                            break;
                        default:
                            break;
                    }
                }
                result->ischar = false;
                result->contents.action = (control_char_t) { .clear = clear_command };

                return i+1;
            case 'h':
            case 'l':
            case 'p':
                // not implemented
                return i+1;
            case 'r':
                REQUIRE((ct = parse_args(buffer, len, args))==2);
                result->ischar = false;
                result->contents.action = (control_char_t) {
                    .scroll = (scroll_region) {
                        SCROLL_REG, args[0], args[1]
                    }
                };
                return i+1;
            case 27:
                // esc.. we fucked up, probably
                return i;
            default:
                free(args);
        }
    }
END:
    return 0;
}

int parse_args(char *buffer, int len, int *result) {
    int ct = 0;
    char buf[8];
    memset(buf, 0, 8);
    int j = 0;
    for (int i = 2; i < len; ++i) {
        if (buffer[i] == ';') {
            if (j == 0) { break; }
            result[ct++] = atoi(buf);
            j = 0;
            memset(buf, 0, 8);
        } else if ((buffer[i] > '9' || buffer[i] < '0') && buffer[i] != '-') {
            if (j > 0) {
                result[ct++] = atoi(buf);
            }
            break;
        } else {
            buf[j++] = buffer[i];
        }
    }
    return ct;
}

void scroll_down(buffer_t *buffer) {
    for (int y = 0; y < buffer->height - 1; ++y) {
        for (int x = 0; x < buffer->width; ++x) {
            buffer->contents[x][y] = buffer->contents[x][y+1];
        }
    }
    for (int x = 0; x < buffer->width; ++x) {
        buffer->contents[x][buffer->height-1] = to_cursor_char_t(buffer, 0);
    }
}

void scroll_up(buffer_t *buffer) {
    for (int y = buffer->height - 1; y > 0; --y) {
        for (int x = 0; x < buffer->width; ++x) {
            buffer->contents[x][y] = buffer->contents[x][y-1];
        }
    }
    for (int x = 0; x < buffer->width; ++x) {
        buffer->contents[x][0] = to_cursor_char_t(buffer, 0);
    }
}

void clear_down(buffer_t *buffer) {
    for (int y = buffer->cursor.y; y < buffer->height; ++y) {
        for (int x = 0; x < buffer->width; ++x) {
            buffer->contents[x][y] = to_cursor_char_t(buffer, 0);
        }
    }
}

void clear_up(buffer_t *buffer) {
    for (int y = buffer->cursor.y-1; y >= 0; ++y) {
        for (int x = 0; x < buffer->width; ++x) {
            buffer->contents[x][y] = to_cursor_char_t(buffer, 0);
        }
    }
}

void clear_left(buffer_t *buffer) {
    int y = buffer->cursor.y;
    for (int x = buffer->cursor.x; x >= 0; ++x) {
        buffer->contents[x][y] = to_cursor_char_t(buffer, 0);
    }
}

void clear_right(buffer_t *buffer) {
    int y = buffer->cursor.y;
    for (int x = buffer->cursor.x; x < buffer->width; ++x) {
        buffer->contents[x][y] = to_cursor_char_t(buffer, 0);
    }
}

void move_to(buffer_t *buffer, int x, int y) {
    int maxx = buffer->width;
    int maxy = buffer->height;

    buffer->cursor.y = (y<0)?0:((y>=maxy)?maxy-1:y);
    buffer->cursor.x = (x<0)?0:((x>=maxx)?maxx-1:x);
}

void move_by(buffer_t *buffer, int x, int y) {
    move_to(buffer, x + buffer->cursor.x, y + buffer->cursor.y);
}

void printx(const char *string) {
    for (const char *c = string; *c != 0; ++c) {
        if (' ' > *c || '~' < *c) {
            printf("\\%02x", *c);
        } else {
            printf("%c", *c);
        }
    }
    printf("\n");
}
