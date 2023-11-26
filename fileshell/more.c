#include "more.h"
#include <libsys/vga.h>
#include <libsys/ps2keyboard.h>
#include <string.h>

static uint8_t row;
static uint8_t col;
static uint8_t color;

static uint8_t new_line(void);

void more_init(uint8_t _color) {
    vga_clear(_color);
    color = _color;
    row = 0;
    col = 0;
}

bool more_print(const char *s, uint16_t max_len) {
    for (; max_len != 0; max_len -= 1) {
        char c = *s;
        uint8_t key = more_putchar(c);
        if (key == PS2_KEY_ESCAPE) {
            return false;
        }
        s += 1;
    }
    return true;
}

uint8_t more_putchar(char c) {
    if (c == '\n') {
        return new_line();
    } else {
        VGA_CHAR_SEG[VGA_OFFSET(col, row)] = c;
        col += 1;
        if (col == VGA_COLS) {
            return new_line();
        }
        return 0;
    }
}

static uint8_t new_line(void) {
    col = 0;
    row += 1;
    if (row == VGA_ROWS - 1) {
        strcpy(VGA_CHAR_SEG + VGA_OFFSET(0, VGA_ROWS - 1), "-- MORE --");
        uint8_t highlight_color = color ^ 0x08;
        memset(VGA_COLOR_SEG + VGA_OFFSET(0, VGA_ROWS - 1), highlight_color, 10);
        row = 0;
        uint8_t result = ps2_wait_key_pressed();
        vga_clear(color);
        return result;
    } else {
        return 0;
    }
}
