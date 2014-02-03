#ifndef CONTROL_H
#define CONTROL_H
#include <stdbool.h>

#define ANSI_FG_BLACK   30
#define ANSI_FG_RED     31
#define ANSI_FG_GREEN   32
#define ANSI_FG_YELLOW  33
#define ANSI_FG_BLUE    34
#define ANSI_FG_MAGENTA 35
#define ANSI_FG_CYAN    36
#define ANSI_FG_WHITE   37

#define ANSI_BG_BLACK   40
#define ANSI_BG_RED     41
#define ANSI_BG_GREEN   42
#define ANSI_BG_YELLOW  43
#define ANSI_BG_BLUE    44
#define ANSI_BG_MAGENTA 45
#define ANSI_BG_CYAN    46
#define ANSI_BG_WHITE   47

typedef enum {
    SETGMODE,
    SETSMODE,
    CURSOR,
    WIPE,
    SAVEPOS,
    RESTORE,
    ERASE,
    CLEARLINE,
} control_type;

typedef struct {
    control_type type;
    int foreground;
    int background;
    int attribute;
} change_color;

typedef struct {
    control_type type;
} no_args;

typedef struct {
    control_type type;
    int x;
    int y;
} move_cursor;

typedef union {
     control_type type;
     union {
         no_args dummy;
         change_color color;
         move_cursor position;
     }
} control_char_t;

#endif
