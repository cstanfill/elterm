#ifndef BUFFER_H
#define BUFFER_H
#include <sys/types.h>
/* todo: come up with a better name than char_t */
typedef struct {
    char codepoint[4];
    int color;
} char_t;

typedef struct {
    int x;
    int y;
} cursor_t;

typedef struct {
    char_t **contents;
    size_t width;
    size_t height;
} buffer_t;

buffer_t *new_buffer(size_t width, size_t height);
void free_buffer(buffer_t *buffer);

#endif
