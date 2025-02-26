#include "line.h"
#include <libsys/vga.h>

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
