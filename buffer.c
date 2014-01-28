#include <stdlib.h>
#include <sys/types.h>
#include "buffer.h"

buffer_t *new_buffer(size_t width, size_t height) {
    buffer_t *buffer = malloc(sizeof(buffer_t));
    if (buffer == NULL) { return NULL; }
    char_t **contents = malloc(sizeof(char_t *) * width);
    if (contents == NULL) {
        free(buffer);
        return NULL;
    }
    for (size_t i = 0; i < width; ++i) {
        contents[i] = malloc(sizeof(char_t) * height);
        if (contents[i] == NULL) {
            for (size_t j = 0; j < i; ++j) {
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
    return buffer;
}

void free_buffer(buffer_t *buffer) {
    for (size_t i = 0; i < buffer->width; ++i) {
        free(buffer->contents[i]);
    }
    free(buffer->contents);
    free(buffer);
}
