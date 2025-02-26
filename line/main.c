#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <libsys/vga.h>
#include <libsys/ps2keyboard.h>
#include "line.h"

#define CHAR_BASE 240

void main(void) {
    vga_clear(COLOR(COLOR_WHITE, COLOR_BLACK));
    uint8_t x0 = 0;
    uint8_t y0 = 0;
    uint8_t x1 = 40;
    uint8_t y1 = 30;
    while (true) {
        memset(VGA_CHAR_SEG, CHAR_BASE, VGA_ROWS * 128);
        line(x0, y0, x1, y1);

        uint8_t key = ps2_wait_key_pressed();
        switch (key) {
        case PS2_KEY_DOWN: if (y1 < 2 * VGA_ROWS) y1 += 1; break;
        case PS2_KEY_UP: if (y1 != 0) y1 -= 1; break;
        case PS2_KEY_RIGHT: if (x1 < VGA_COLS) x1 += 1; break;
        case PS2_KEY_LEFT: if (x1 != 0) x1 -= 1; break;

        case PS2_KEY_S: if (y0 < 2 * VGA_ROWS) y0 += 1; break;
        case PS2_KEY_W: if (y0 != 0) y0 -= 1; break;
        case PS2_KEY_D: if (x0 < VGA_COLS) x0 += 1; break;
        case PS2_KEY_A: if (x0 != 0) x0 -= 1; break;
        }
    }
}


int getchar(void) { return -1; }
void putchar(int x) {}
