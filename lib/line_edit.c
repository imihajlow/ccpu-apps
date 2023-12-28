#include "line_edit.h"
#include <string.h>
#include <libsys/vga.h>
#include <libsys/ps2keyboard.h>

static uint8_t cursor;
static uint8_t color;
static uint8_t cursor_color;
static uint8_t screen_row;
static uint8_t min_screen_col;

static uint8_t move_cursor(int8_t d) {
    uint8_t *offset = VGA_COLOR_SEG + VGA_OFFSET(min_screen_col + cursor, screen_row);
    *offset = color;
    cursor += d;
    offset += d;
    *offset = cursor_color;
}

bool line_edit(char *buf, uint8_t max_len, uint8_t min_screen_col_, uint8_t screen_row_, uint8_t color_) {
    uint8_t len = strlen(buf);
    color = color_;
    min_screen_col = min_screen_col_;
    screen_row = screen_row_;
    cursor_color = color ^ 0x80;
    cursor = len;

    strcpy(VGA_CHAR_SEG + VGA_OFFSET(min_screen_col, screen_row), buf);
    VGA_COLOR_SEG[VGA_OFFSET(min_screen_col + cursor, screen_row)] = cursor_color;
    uint8_t *offset;
    while (true) {
        uint16_t key = ps2_get_ascii();
        if (!key) {
            continue;
        }
        if (PS2_IS_ASCII(key)) {
            if (len != max_len) {
                offset = VGA_CHAR_SEG + VGA_OFFSET(min_screen_col + len, screen_row);
                for (uint8_t i = len; i != cursor; --i) {
                    char c = buf[i - 1];
                    buf[i] = c;
                    *offset = c;
                    offset -= 1;
                }
                buf[cursor] = key;
                *offset = key;
                len += 1;
                buf[len] = 0;
                move_cursor(1);
            }
        } else if ((uint8_t)key == PS2_KEY_ENTER || (uint8_t)key == PS2_KEY_NUM_ENTER) {
            VGA_COLOR_SEG[VGA_OFFSET(min_screen_col + cursor, screen_row)] = color;
            return true;
        } else if ((uint8_t)key == PS2_KEY_LEFT) {
            if (cursor) {
                move_cursor(-1);
            }
        } else if ((uint8_t)key == PS2_KEY_RIGHT) {
            if (cursor != len) {
                move_cursor(1);
            }
        } else if ((uint8_t)key == PS2_KEY_BACKSPACE) {
            if (cursor) {
                move_cursor(-1);
                offset = VGA_CHAR_SEG + VGA_OFFSET(min_screen_col + cursor, screen_row);
                for (uint8_t i = cursor; i != len - 1; ++i) {
                    char c = buf[i + 1];
                    *offset = c;
                    buf[i] = c;
                    offset += 1;
                }
                len -= 1;
                *offset = 0;
                buf[len] = 0;
            }
        } else if ((uint8_t)key == PS2_KEY_DELETE) {
            if (cursor != len) {
                offset = VGA_CHAR_SEG + VGA_OFFSET(min_screen_col + cursor, screen_row);
                for (uint8_t i = cursor; i != len - 1; i += 1) {
                    char c = buf[i + 1];
                    *offset = c;
                    buf[i] = c;
                    offset += 1;
                }
                len -= 1;
                *offset = 0;
                buf[len] = 0;
            }
        } else if ((uint8_t)key == PS2_KEY_ESCAPE) {
            VGA_COLOR_SEG[VGA_OFFSET(min_screen_col + cursor, screen_row)] = color;
            return false;
        }
    }
}
