#include <stdlib.h>
#include <sys/types.h>
#include "buffer.h"

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
    buffer->contents = contents;
    buffer->width = width;
    buffer->height = height;
    buffer->cursor = (cursor_t) { 0, 0};
    return buffer;
}

void free_buffer(buffer_t *buffer) {
    for (int i = 0; i < buffer->width; ++i) {
        free(buffer->contents[i]);
    }
    free(buffer->contents);
    free(buffer);
}

void write_char(buffer_t *buffer, char c) {
    buffer->contents[buffer->cursor.x][buffer->cursor.y].codepoint[0] = c;
    buffer->contents[buffer->cursor.x][buffer->cursor.y].codepoint[1] = 0;
    buffer->contents[buffer->cursor.x][buffer->cursor.y].codepoint[2] = 0;
    buffer->contents[buffer->cursor.x][buffer->cursor.y].codepoint[3] = 0;
    if (++(buffer->cursor.x) >= buffer->width) {
        buffer->cursor.x = 0;
        if (++(buffer->cursor.y) >= buffer->height) {
            buffer->cursor.y = 0;
        }
    }
}
void write_string(buffer_t *buffer, char *string, int ct) {
    for (int i = 0; i < ct; ++i) {
        write_char(buffer, string[i]);
    }
}
