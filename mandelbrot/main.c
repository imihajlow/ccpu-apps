#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <libsys/vga.h>
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

#define MAX_STEPS (sizeof(color)-1)

static bool diverged(fp c_re, fp c_im) {
    return (c_re > (2l << 28)) || (c_re < (-2l << 28)) || (c_im > (2l << 28)) || (c_im < (-2l << 28));
}

static uint8_t iterate(fp c_re, fp c_im) {
    uint8_t step = 0;
    fp z_re = c_re;
    fp z_im = c_im;
    while (step < MAX_STEPS && !diverged(z_re, z_im)) {
        // z = z^2 + c
        fp z_re_2 = fp_mul(z_re, z_re);
        fp z_im_2 = fp_mul(z_im, z_im);
        fp z_re_im = fp_mul(z_re, z_im);
        z_re = z_re_2 - z_im_2 + c_re;
        z_im = z_re_im + z_re_im + c_im;
        ++step;
    }
    return step;
}

void main(void) {
    vga_clear(COLOR(COLOR_BLACK, COLOR_BLACK));
    memset(VGA_CHAR_SEG, CHAR_TOP, VGA_ROWS * 128);

    char *p_color = VGA_COLOR_SEG;
    fp im_top = 3l << 27; // 1.5
    fp im_bot = im_top - (1l << 28) / 20; // 0.05
    for (uint8_t r = 0; r != VGA_ROWS; ++r) {
        fp re = -3l << 28; // -3.0
        for (uint8_t c = 0; c != VGA_COLS; ++c) {
            uint8_t steps_top = iterate(re, im_top);
            uint8_t steps_bot = iterate(re, im_bot);
            re += (1l << 28) / 20; // 0.05
            *p_color = COLOR(color[steps_top], color[steps_bot]);
            ++p_color;
        }
        im_top -= (1l << 28) / 10; // 0.1
        im_bot -= (1l << 28) / 10; // 0.1
        p_color += 128 - VGA_COLS;
    }
    while(1);
}

int getchar(void) { return 0; }
int putchar(int c) { return 0; }
