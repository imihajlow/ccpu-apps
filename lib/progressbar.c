#include "progressbar.h"
#include <libsys/vga.h>
#include <string.h>

#define CHAR_EMPTY 177
#define CHAR_FULL 219

static uint16_t last_x;
static uint16_t last_y;
static char *offset;
static uint8_t max_value;

void progressbar_init(uint8_t max_value_) {
    offset = VGA_CHAR_SEG + VGA_OFFSET(0, VGA_ROWS - 1);
    memset(offset, CHAR_EMPTY, VGA_COLS);
    max_value = max_value_;
    last_x = 0;
    last_y = 0;
}

void progressbar_progress(uint8_t n) {
    for (uint8_t i = 0; i != n; ++i) {
        last_x += VGA_COLS;
        while (last_y < last_x) {
            *offset = CHAR_FULL;
            offset += 1;
            last_y += max_value;
        }
    }
}
