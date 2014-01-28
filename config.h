#ifndef CONFIG_H
#define CONFIG_H
#include "buffer.h"
typedef struct config_t {
    char *font_pattern;
    char *shell;
    cursor_t term_size;
} config_t;

extern config_t config;

config_t default_config;
#endif
