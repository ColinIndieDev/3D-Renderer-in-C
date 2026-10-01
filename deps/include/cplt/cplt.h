#pragma once

#include <unistd.h>

#define CLEAR_SCREEN "\x1b[2J"
#define RESET_CURSOR_POS "\x1b[H"
#define HIDE_CURSOR "\x1b[?25l"
#define SHOW_CURSOR "\x1b[?25h"

typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
} color_t;
#define RGB(r, g, b) (color_t){r, g, b}

typedef struct {
    char c;
    color_t f;
    color_t b;
} pixel_t;

int cplt_begin();
void cplt_end();
void cplt_clear();
void cplt_refresh();
void cplt_putc(int x, int y, char c, color_t f, color_t b);
void cplt_puts(int x, int y, const char *str, color_t f, color_t b);
void cplt_get_key_pressed(unsigned char buffer[3], ssize_t *n);
int cplt_get_screen_width();
int cplt_get_screen_height();
