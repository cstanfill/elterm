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
    int width;
    int height;
    cursor_t cursor;
} buffer_t;

buffer_t *new_buffer(int width, int height);
void free_buffer(buffer_t *buffer);
void write_char(buffer_t *buffer, char c);
// TODO: write different encodings
void write_string(buffer_t *buffer, char *string, int ct);

#endif
