#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include "buffer.h"

bool is_printing(char c) {
    return (c <= '~') && (c >= ' ');
}

char_t to_char_t(char c) {
    return (char_t) {{c, 0, 0, 0}, 0};
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
    if (count >= queue->len) { queue->contents = NULL; queue->len = 0; return; }
    char *new_queue = calloc(queue->len - count, sizeof(char));
    for (int i = 0; i < queue->len - count; ++i) {
        new_queue[i] = queue->contents[i];
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
    buffer->cursor = (cursor_t) { 0, 0, 0 };
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
    buffer->contents[buffer->cursor.x][buffer->cursor.y].color = buffer->cursor.color;
    if (++(buffer->cursor.x) >= buffer->width) {
        buffer->cursor.x = 0;
        if (++(buffer->cursor.y) >= buffer->height) {
            buffer->cursor.y = 0;
        }
    }
}

void write_char(buffer_t *buffer, char c) {
    if (c == '\r') { buffer->cursor.x = 0; return; }
    if (c == '\n') { ++buffer->cursor.y; return; }
    if (c == '\b') {
        --buffer->cursor.x;
        if (buffer->cursor.x < 0) {
            buffer->cursor.x = buffer->width - 1;
            --buffer->cursor.y;
        }
        buffer->contents[buffer->cursor.x][buffer->cursor.y] = to_char_t(0);
        return;
    }
    if (c == '\a') { 
        return;
    }
    if (is_printing(c) && buffer->unread->len == 0) {
        write_char_t(buffer, to_char_t(c));
    } else {
        queue_t *unread = buffer->unread;
        enqueue(unread, c);

        char_or_control result;
        int len;
        if ((len = parse(unread->contents, unread->len, &result)) > 0) {
            dequeue_n(unread, len);
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
    if (len >= 2 && buffer[0] == 27) {
        if (buffer[1] == '[') {
            goto ANSI;
        } else {
            switch (buffer[1]) {
                case '=':
                    return 2;
                case 27:
                    return 1;
                default:
                    break;
            }
        }
    }
    goto END;

ANSI:
    ; // stupid
    int *args;
    for (char *s = buffer; *s != 0; ++s) {
    }
    for (int i = 2; i < len; ++i) {
        args = calloc(i, sizeof(int));
        switch (buffer[i]) {
            case 'H':
            case 'f':
                REQUIRE((parse_args(buffer, len, args)==2));
                result->ischar = false;
                result->contents.action = (control_char_t) { .position = (move_cursor) { CURSOR, args[0], args[1] } };
                return i+1;
            case 'A':
            case 'B':
            case 'C':
            case 'D':
            case 's':
            case 'u':
            case 'J':
            case 'K':
            case 'm':
                ;
                int ct;
                REQUIRE((ct = parse_args(buffer, len, args))<=3);
                change_color command;
                for (int j = 0; j < ct; ++j) {
                    if (0 <= args[j] && args[j] <= 8) {
                        command.attribute = args[j];
                    } else if (ANSI_FG_WHITE <= args[j] && args[j] <= ANSI_FG_BLACK) {
                        command.foreground = args[j];
                    } else if (ANSI_BG_WHITE <= args[j] && args[j] <= ANSI_BG_BLACK) {
                        command.background = args[j];
                    }
                }
                command.type = SETGMODE;
                result->ischar = false;
                result->contents.action = (control_char_t) { .color = command };
                return i+1;
            case 'h':
            case 'l':
            case 'p':
                // not implemented
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
    for (int i = 3; i < len; ++i) {
        if (buffer[i] == ';') {
            if (j == 0) { break; }
            result[ct++] = atoi(buf);
        } else if ((buffer[i] > '9' || buffer[i] < '0') && buffer[i] != '-') {
            break;
        }

        buf[j++] = buffer[i];
    }
    return ct;
}
