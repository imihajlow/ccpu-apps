#include <stdio.h>
#include <string.h>
#include <libsys/ps2.h>
#include <libsys/ps2keyboard.h>
#include <libsys/vga.h>

static void print_code(uint8_t code);
static void print_raw(uint8_t raw);
static void scroll_up(uint8_t c);

void main(void) {
    ps2_set_rate(PS2_RATE_20_CPTS | PS2_DELAY_1000_MS);
    vga_clear(COLOR(COLOR_GRAY, COLOR_BLACK));
    for (uint8_t r = 0; r != VGA_ROWS; ++r) {
        memset(VGA_COLOR_SEG + VGA_OFFSET(VGA_COLS / 2, r), COLOR(COLOR_BLACK, COLOR_GRAY), VGA_COLS / 2);
    }

    uint8_t r_code = 0;
    uint8_t c_code = VGA_COLS / 2;
    while (1) {
        uint8_t raw = ps2_read();
        if (raw != PS2_READ_EMPTY) {
            print_raw(raw);
            uint8_t code = ps2_get_key_event_with_code(raw);
            if (code != PS2_KEY_NONE) {
                print_code(code);
            }
        }
    }
}

static void print_raw(uint8_t raw) {
    static uint8_t r = 0;
    static uint8_t c = 0;
    sprintf(VGA_CHAR_SEG + VGA_OFFSET(c, r), "%02hX", raw & 0xff);
    c += 3;
    if (c + 2 > VGA_COLS / 2) {
        c = 0;
        if (r < VGA_ROWS - 1) {
            r += 1;
        } else {
            scroll_up(0);
        }
    }
}

static void print_code(uint8_t code) {
    static uint8_t r = 0;
    static uint8_t c = VGA_COLS / 2;
    sprintf(VGA_CHAR_SEG + VGA_OFFSET(c, r), "%02hX-%02hX", code & 0xff, ps2_modifiers_mask & 0xff);
    c += 6;
    if (c + 5 > VGA_COLS) {
        c = VGA_COLS / 2;
        if (r < VGA_ROWS - 1) {
            r += 1;
        } else {
            scroll_up(VGA_COLS / 2);
        }
    }
}

static void scroll_up(uint8_t c) {
    for (uint8_t r = 0; r != VGA_ROWS - 1; ++r) {
        memcpy(VGA_CHAR_SEG + VGA_OFFSET(c, r), VGA_CHAR_SEG + VGA_OFFSET(c, r + 1), VGA_COLS / 2);
    }
    memset(VGA_CHAR_SEG + VGA_OFFSET(c, VGA_ROWS - 1), 0, VGA_COLS / 2);
}

int getchar(void) { return 0; }
int putchar(int c) { return 0; }
