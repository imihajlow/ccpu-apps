#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <libsys/vga.h>
#include <libsys/ps2keyboard.h>

// static void line_norm(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
// }

#define CHAR_BASE 240

// x1 >= x0, y1 >= y0, dx >= dy
static void line_1(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    uint8_t dx = x1 - x0;
    uint8_t dy = y1 - y0;
    uint8_t y = y0;
    uint8_t x = x0;
    uint16_t cx = dy / 2;
    uint16_t cy = dx / 2;
    while (x <= x1) {
        if (y & 1) {
            VGA_CHAR_SEG[VGA_OFFSET(x, y / 2)] |= 2;
        } else {
            VGA_CHAR_SEG[VGA_OFFSET(x, y / 2)] |= 1;
        }
        x += 1;
        cx += dy;
        if (cx > cy) {
            y += 1;
            cy += dx;
        }
    }
}

// x1 >= x0, y1 >= y0, dx < dy
static void line_2(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    uint8_t dx = x1 - x0;
    uint8_t dy = y1 - y0;
    uint8_t y = y0;
    uint8_t x = x0;
    uint16_t cx = dy / 2;
    uint16_t cy = dx / 2;
    while (y <= y1) {
        if (y & 1) {
            VGA_CHAR_SEG[VGA_OFFSET(x, y / 2)] |= 2;
        } else {
            VGA_CHAR_SEG[VGA_OFFSET(x, y / 2)] |= 1;
        }
        y += 1;
        cy += dx;
        if (cy > cx) {
            x += 1;
            cx += dy;
        }
    }
}

// x1 >= x0, y1 < y0, dx >= |dy|
static void line_3(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    uint8_t dx = x1 - x0;
    uint8_t dy = y0 - y1;
    uint8_t y = y0;
    uint8_t x = x0;
    uint16_t cx = dy / 2;
    uint16_t cy = dx / 2;
    while (x <= x1) {
        if (y & 1) {
            VGA_CHAR_SEG[VGA_OFFSET(x, y / 2)] |= 2;
        } else {
            VGA_CHAR_SEG[VGA_OFFSET(x, y / 2)] |= 1;
        }
        x += 1;
        cx += dy;
        if (cx > cy) {
            y -= 1;
            cy += dx;
        }
    }
}

// x1 >= x0, y1 < y0, dx < |dy|
static void line_4(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    uint8_t dx = x1 - x0;
    uint8_t dy = y0 - y1;
    uint8_t y = y1;
    uint8_t x = x1;
    uint16_t cx = dy / 2;
    uint16_t cy = dx / 2;
    while (y <= y0) {
        if (y & 1) {
            VGA_CHAR_SEG[VGA_OFFSET(x, y / 2)] |= 2;
        } else {
            VGA_CHAR_SEG[VGA_OFFSET(x, y / 2)] |= 1;
        }
        y += 1;
        cy += dx;
        if (cy > cx) {
            x -= 1;
            cx += dy;
        }
    }
}

// x1 >= x0
static void line_norm(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    uint8_t dx = x1 - x0;
    if (y1 >= y0) {
        uint8_t dy = y1 - y0;
        if (dx >= dy) {
            line_1(x0, y0, x1, y1);
        } else {
            line_2(x0, y0, x1, y1);
        }
    } else {
        uint8_t dy = y0 - y1;
        if (dx >= dy) {
            line_3(x0, y0, x1, y1);
        } else {
            line_4(x0, y0, x1, y1);
        }
    }
}

void line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    if (x1 < x0) {
        line_norm(x1, y1, x0, y0);
    } else {
        line_norm(x0, y0, x1, y1);
    }
}

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
