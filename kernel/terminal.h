#ifndef TERMINAL_H
#define TERMINAL_H

#include <stddef.h>

enum VGAColor
{
    COLOR_BLACK = 0,
    COLOR_BLUE = 1,
    COLOR_GREEN = 2,
    COLOR_CYAN = 3,
    COLOR_RED = 4,
    COLOR_MAGENTA = 5,
    COLOR_BROWN = 6,
    COLOR_LIGHT_GREY = 7,
    COLOR_DARK_GREY = 8,
    COLOR_LIGHT_BLUE = 9,
    COLOR_LIGHT_GREEN = 10,
    COLOR_LIGHT_CYAN = 11,
    COLOR_LIGHT_RED = 12,
    COLOR_LIGHT_MAGENTA = 13,
    COLOR_LIGHT_BROWN = 14,
    COLOR_WHITE = 15
};

void terminal_initialize();

void terminal_clear();

void terminal_set_color(
    VGAColor foreground,
    VGAColor background
);

void terminal_putchar(char c);

void terminal_write(const char* str);

void terminal_writeline(const char* str);

void terminal_write_number(unsigned int number);

void terminal_write_hex(unsigned int number);

#endif