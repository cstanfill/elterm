#ifndef BUFFER_H
#define BUFFER_H
#include "control.h"
#include <stdbool.h>
#include <sys/types.h>

/* todo: come up with a better name than char_t */
typedef struct {
    char codepoint[4];
    int color;
    bool folded;
} char_t;

typedef struct {
    int x;
    int y;
    int color;
} cursor_t;

typedef struct {
    bool ischar;
    union {
        char_t glyph;
        control_char_t action;
    } contents;
} char_or_control;

typedef struct {
    char *contents;
    int len;
} queue_t;

typedef struct {
    char_t **contents;
    int width;
    int height;
    queue_t *unread;
    cursor_t cursor;
} buffer_t;

bool is_printable(char c);
bool is_control(char c);

char_t to_char_t(char c, int color);
void enqueue(queue_t *queue, char c);
void dequeue_n(queue_t *queue, int count);
char dequeue(queue_t *queue);
void flush(queue_t *queue);


int parse(char *buffer, int len, char_or_control *result);
int parse_args(char *buffer, int len, int *result);

buffer_t *new_buffer(int width, int height);
void free_buffer(buffer_t *buffer);

void move_to(buffer_t *buffer, int x, int y);
void move_by(buffer_t *buffer, int x, int y);
void scroll_up(buffer_t *buffer);
void scroll_down(buffer_t *buffer);
void clear_down(buffer_t *buffer);
void clear_up(buffer_t *buffer);
void clear_left(buffer_t *buffer);
void clear_right(buffer_t *buffer);

void write_char(buffer_t *buffer, char c);
// TODO: write different encodings
void write_string(buffer_t *buffer, char *string, int ct);

void printx(const char *string);

#endif
