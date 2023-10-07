#include "quasipixel.h"
#include <string.h>
#include <libsys/vga.h>

#define CHAR_NONE 0
#define CHAR_TOP 223
#define CHAR_BOTTOM 220
#define CHAR_BOTH 219

extern uint8_t qp_fb[QP_SIZE]; // same memory organization as in video buffer
extern uint8_t qp_colors[4]; // used in assembly functions

static bool cursor_enabled = false;
uint8_t qp_cursor_x;
uint8_t qp_cursor_y;
static uint8_t fg_color;
static uint8_t bg_color;

void qp_render_fast(void);
void qp_vga_clear(void);

void qp_init(uint8_t fg, uint8_t bg) {
    cursor_enabled = false;
    qp_cursor_x = 0;
    qp_cursor_y = 0;
    memset(qp_fb, 0, QP_SIZE);
    qp_set_color(fg, bg);
}

void qp_set_color(uint8_t fg, uint8_t bg) {
    fg_color = fg;
    bg_color = bg;

    qp_colors[0] = COLOR(bg_color, bg_color);
    qp_colors[1] = COLOR(bg_color, fg_color);
    qp_colors[2] = COLOR(fg_color, bg_color);
    qp_colors[3] = COLOR(fg_color, fg_color);

    qp_vga_clear();
    qp_render_fast();
}

static uint8_t render_one_char(uint8_t c, uint8_t r, uint16_t offset) {
    uint8_t top_pixel = qp_fb[offset];
    uint8_t bot_pixel = top_pixel >> 1;
    top_pixel &= 1;

    uint8_t cursor_on_col = cursor_enabled && qp_cursor_x == c;
    uint8_t cursor_on_top = cursor_on_col && qp_cursor_y == (r << 1);
    uint8_t cursor_on_bot = cursor_on_col && qp_cursor_y == (r << 1) + 1;

    uint8_t top_color;
    uint8_t bot_color;

    if (top_pixel) {
        top_color = fg_color;
    } else {
        top_color = bg_color;
    }

    if (bot_pixel) {
        bot_color = fg_color;
    } else {
        bot_color = bg_color;
    }

    if (cursor_on_top) {
        top_color = top_color ^ 8;
    }
    if (cursor_on_bot) {
        bot_color = bot_color ^ 8;
    }
    VGA_COLOR_SEG[offset] = COLOR(bot_color, top_color);
}

void qp_render(void) {
    qp_render_fast();

    if (cursor_enabled) {
        qp_render_one(qp_cursor_x, qp_cursor_y);
    }
}

void qp_set(uint8_t x, uint8_t y, bool v) {
    uint16_t index = VGA_OFFSET(x, y >> 1);
    uint8_t value = qp_fb[index];
    uint8_t mask = 1;
    if (y & 1) {
        mask <<= 1;
    }
    if (v) {
        value |= mask;
    } else {
        value &= ~mask;
    }
    qp_fb[index] = value;
}

bool qp_get(uint8_t x, uint8_t y) {
    uint16_t index = VGA_OFFSET(x, y >> 1);
    uint8_t value = qp_fb[index];
    if (y & 1) {
        value >>= 1u;
    }
    return value & 1;
}

void qp_render_one(uint8_t x, uint8_t y) {
    y >>= 1;
    uint16_t offset = VGA_OFFSET(x, y);
    render_one_char(x, y, offset);
}

void qp_set_and_render(uint8_t x, uint8_t y, bool v) {
    qp_set(x, y, v);
    qp_render_one(x, y);
}

void qp_set_cursor_enabled(bool enabled) {
    cursor_enabled = enabled;
    qp_render_one(qp_cursor_x, qp_cursor_y);
}

void qp_set_cursor_pos(uint8_t x, uint8_t y) {
    if (cursor_enabled) {
        bool old_enabled = cursor_enabled;
        cursor_enabled = false;
        qp_render_one(qp_cursor_x, qp_cursor_y);
        qp_cursor_x = x;
        qp_cursor_y = y;
        cursor_enabled = old_enabled;
        qp_render_one(qp_cursor_x, qp_cursor_y);
    } else {
        qp_cursor_x = x;
        qp_cursor_y = y;
    }
}

void qp_move_cursor(int8_t dx, int8_t dy) {
    int8_t new_x = (int8_t)qp_cursor_x + dx;
    int8_t new_y = (int8_t)qp_cursor_y + dy;
    if (new_x < 0) {
        new_x = 0;
    }
    if (new_y < 0) {
        new_y = 0;
    }
    if (new_x >= (int8_t)QP_WIDTH) {
        new_x = (int8_t)QP_WIDTH - 1;
    }
    if (new_y >= (int8_t)QP_HEIGHT) {
        new_y = (int8_t)QP_HEIGHT - 1;
    }
    qp_set_cursor_pos((uint8_t)new_x, (uint8_t)new_y);
}

void qp_render_rect(uint8_t col_from, uint8_t row_from, uint8_t col_to, uint8_t row_to) {
    for (; row_from != row_to; ++row_from) {
        uint16_t index = VGA_OFFSET(col_from, row_from);
        for (uint8_t col = col_from; col != col_to; ++col) {
            render_one_char(col, row_from, index);
            index += 1;
        }
    }
}
