#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <libsys/vga.h>
#include <libsys/ps2keyboard.h>
#include "fixpoint.h"

#define CHAR_TOP 223

static const uint8_t color[] = {
    COLOR_BLACK,
    COLOR_BLACK,
    COLOR_DARK_GRAY,
    COLOR_DARK_GRAY,
    COLOR_GRAY,
    COLOR_GRAY,
    COLOR_RED,
    COLOR_RED,
    COLOR_YELLOW,
    COLOR_LIGHT_RED,
    COLOR_GREEN,
    COLOR_BLUE,
    COLOR_LIGHT_GREEN,
    COLOR_CYAN,
    COLOR_LIGHT_BLUE,
    COLOR_LIGHT_CYAN,
    COLOR_WHITE,
    COLOR_BLACK
};

#define CURSOR_COLOR COLOR(COLOR_WHITE, COLOR_WHITE)

#define MAX_STEPS (sizeof(color)-1)

static bool diverged(fp16 c_re, fp16 c_im) {
    return (c_re > (2 << 12)) || (c_re < (-2 << 12)) || (c_im > (2 << 12)) || (c_im < (-2 << 12));
}

static uint8_t iterate(fp16 c_re, fp16 c_im) {
    uint8_t step = 0;
    fp16 z_re = c_re;
    fp16 z_im = c_im;
    while (step < MAX_STEPS && !diverged(z_re, z_im)) {
        // z = z^2 + c
        fp16 z_re_2 = fp16_12_mul(z_re, z_re);
        fp16 z_im_2 = fp16_12_mul(z_im, z_im);
        fp16 z_re_im = fp16_12_mul(z_re, z_im);
        z_re = z_re_2 - z_im_2 + c_re;
        z_im = z_re_im + z_re_im + c_im;
        ++step;
    }
    return step;
}

static fp16 julia_c_im;
static fp16 julia_c_re;

static uint8_t iterate_julia(fp16 z0_re, fp16 z0_im) {
    uint8_t step = 0;
    fp16 z_re = z0_re;
    fp16 z_im = z0_im;
    while (step < MAX_STEPS && !diverged(z_re, z_im)) {
        // z = z^2 + c
        fp16 z_re_2 = fp16_12_mul(z_re, z_re);
        fp16 z_im_2 = fp16_12_mul(z_im, z_im);
        fp16 z_re_im = fp16_12_mul(z_re, z_im);
        z_re = z_re_2 - z_im_2 + julia_c_re;
        z_im = z_re_im + z_re_im + julia_c_im;
        ++step;
    }
    return step;
}

static void plot(fp16 x0, fp16 y0, fp16 step, bool is_julia) {
    char *p_color = VGA_COLOR_SEG;
    fp16 double_step = step + step;
    fp16 im_top = y0;
    fp16 im_bot = y0 - step;
    for (uint8_t r = 0; r != VGA_ROWS; ++r) {
        fp16 re = x0;
        for (uint8_t c = 0; c != VGA_COLS; ++c) {
            uint8_t key = ps2_get_key_event();
            if (key == PS2_KEY_ESCAPE) {
                return;
            }
            uint8_t steps_top, steps_bot;
            if (is_julia) {
                steps_top = iterate_julia(re, im_top);
                steps_bot = iterate_julia(re, im_bot);
            } else {
                steps_top = iterate(re, im_top);
                steps_bot = iterate(re, im_bot);
            }
            re += step;
            *p_color = COLOR(color[steps_top], color[steps_bot]);
            ++p_color;
        }
        im_top -= double_step;
        im_bot -= double_step;
        p_color += 128 - VGA_COLS;
    }
}

#define INIT_X0 (-3 << 12) // -3.0
#define INIT_Y0 (3 << 11) // 1.5
#define INIT_STEP ((1 << 12) / 16)

void main(void) {
    vga_clear(COLOR(COLOR_BLACK, COLOR_BLACK));
    memset(VGA_CHAR_SEG, CHAR_TOP, VGA_ROWS * 128);

    fp16 x0 = INIT_X0;
    fp16 y0 = INIT_Y0;
    fp16 step = INIT_STEP;
    bool is_julia = false;

    while (true) {
        plot(x0, y0, step, is_julia);

        uint8_t cursor_x = VGA_COLS / 2;
        uint8_t cursor_y = VGA_ROWS / 2;
        fp16 new_step = step;
        while (true) {
            char *p = VGA_COLOR_SEG + VGA_OFFSET(cursor_x, cursor_y);
            uint8_t saved_color = *p;
            *p = CURSOR_COLOR;
            while (true) {
                uint8_t key = ps2_get_key_event();
                switch (key) {
                case PS2_KEY_RIGHT:
                    if (cursor_x < VGA_COLS - 1) {
                        cursor_x += 1;
                    }
                    goto move_cursor;
                case PS2_KEY_LEFT:
                    if (cursor_x) {
                        cursor_x -= 1;
                    }
                    goto move_cursor;
                case PS2_KEY_UP:
                    if (cursor_y) {
                        cursor_y -= 1;
                    }
                    goto move_cursor;
                case PS2_KEY_DOWN:
                    if (cursor_y < VGA_ROWS - 1) {
                        cursor_y += 1;
                    }
                    goto move_cursor;
                case PS2_KEY_EQUALS:
                case PS2_KEY_NUM_ADD:
                    if (new_step > 1) {
                        new_step >>= 1;
                        goto apply_zoom;
                    }
                    break;
                case PS2_KEY_DASH:
                case PS2_KEY_NUM_SUB:
                    if (new_step < 0x4000) {
                        new_step <<= 1;
                        goto apply_zoom;
                    }
                    break;
                case PS2_KEY_BACKSPACE:
                    step = INIT_STEP;
                    x0 = INIT_X0;
                    y0 = INIT_Y0;
                    goto redraw;
                case PS2_KEY_J:
                    is_julia = true;
                    julia_c_re = x0 + cursor_x * step;
                    julia_c_im = y0 - cursor_y * step * 2;
                    goto redraw;
                case PS2_KEY_M:
                    is_julia = false;
                    goto redraw;
                }
            }
move_cursor:
            *p = saved_color;
        }
apply_zoom: ;
        fp16 center_x = x0 + cursor_x * step;
        fp16 center_y = y0 - cursor_y * step * 2;
        step = new_step;
        x0 = center_x - VGA_COLS / 2 * step;
        y0 = center_y + VGA_ROWS * step;
redraw: ;
    }
}

int getchar(void) { return 0; }
int putchar(int c) { return 0; }
