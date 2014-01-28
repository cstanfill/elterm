#include <stdlib.h>
#include <sys/types.h>
#include "buffer.h"

int new_buffer(buffer_t *buffer, size_t width, size_t height) {
    char_t **contents = malloc(sizeof(char_t *) * width);
    if (contents == NULL) {
        return -1;
    }
    for (size_t i = 0; i < width; ++i) {
        contents[i] = malloc(sizeof(char_t) * height);
        if (contents[i] == NULL) {
            for (size_t j = 0; j < i; ++j) {
                free(contents[j]);
                free(contents);
                return -1;
            }
        }
    }
    buffer->contents = contents;
    buffer->width = width;
    buffer->height = height;
    return 0;
}

void free_buffer(buffer_t *buffer) {
    for (size_t i = 0; i < buffer->width; ++i) {
        free(buffer->contents[i]);
    }
    free(buffer->contents);
    buffer->contents = NULL;
    buffer->width = 0;
    buffer->height = 0;
}
