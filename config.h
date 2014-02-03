#ifndef CONFIG_H
#define CONFIG_H
#include "buffer.h"
typedef struct config_t {
    char *font_pattern;
    char *shell;
    int term_width;
    int term_height;
} config_t;

extern config_t config;

config_t default_config;
#endif
