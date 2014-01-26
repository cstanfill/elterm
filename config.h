#ifndef CONFIG_H
#define CONFIG_H
typedef struct config_t {
    char *font_pattern;
    char *shell;
} config_t;

extern config_t config;

config_t default_config;
#endif
