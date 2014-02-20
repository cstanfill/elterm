#ifndef CONTROL_H
#define CONTROL_H
#include <stdbool.h>

#define ANSI_RESET      0
#define ANSI_BOLD       1
#define ANSI_UNDERLINED 4
#define ANSI_BLINK      5
#define ANSI_INVERSE    7

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
    CURSOR_REL,
    SAVEPOS,
    RESTORE,
    NEXT_INDEX,
    PREV_INDEX,
    NEXT_LINE,
    CLEAR,
    NOP,
    SCROLL_REG,
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

#define CL_LINE_UP 0x1
#define CL_LINE_DOWN 0x2
#define CL_LINE_LEFT 0x4
#define CL_LINE_RIGHT 0x8
typedef struct {
    control_type type;
    int region_mask;
} clear_regions;

typedef struct {
    control_type type;
    int x;
    int y;
} move_cursor;

typedef struct {
    control_type type;
    int top;
    int bot;
} scroll_region;

typedef union {
     control_type type;
     no_args dummy;
     change_color color;
     move_cursor position;
     clear_regions clear;
     scroll_region scroll;
} control_char_t;

#endif
