#include "bigdigit.h"
#include <string.h>
#include <libsys/vga.h>

#define DIGIT_WIDTH 8
#define DIGIT_HEIGHT 6

#define CHAR_TOP 223
#define CHAR_BOT 220
#define CHAR_FUL 219
#define CHAR_NUL 0

/*
..###...
.##.##..
##...##.
##...##.
##.#.##.
##.#.##.
##.#.##.
##.#.##.
##...##.
##...##.
.##.##..
..###...
*/
static const uint8_t digit_0[] = {
    CHAR_NUL, CHAR_BOT, CHAR_FUL, CHAR_TOP, CHAR_FUL, CHAR_BOT, CHAR_NUL, CHAR_NUL,
    CHAR_FUL, CHAR_FUL, CHAR_NUL, CHAR_NUL, CHAR_NUL, CHAR_FUL, CHAR_FUL, CHAR_NUL,
    CHAR_FUL, CHAR_FUL, CHAR_NUL, CHAR_FUL, CHAR_NUL, CHAR_FUL, CHAR_FUL, CHAR_NUL,
    CHAR_FUL, CHAR_FUL, CHAR_NUL, CHAR_FUL, CHAR_NUL, CHAR_FUL, CHAR_FUL, CHAR_NUL,
    CHAR_FUL, CHAR_FUL, CHAR_NUL, CHAR_NUL, CHAR_NUL, CHAR_FUL, CHAR_FUL, CHAR_NUL,
    CHAR_NUL, CHAR_TOP, CHAR_FUL, CHAR_BOT, CHAR_FUL, CHAR_TOP, CHAR_NUL, CHAR_NUL,
};
/*
...##...
..###...
.####...
...##...
...##...
...##...
...##...
...##...
...##...
...##...
...##...
.######.
*/
static const uint8_t digit_1[] = {
    CHAR_NUL, CHAR_NUL, CHAR_BOT, CHAR_FUL, CHAR_FUL, CHAR_NUL, CHAR_NUL, CHAR_NUL,
    CHAR_NUL, CHAR_TOP, CHAR_TOP, CHAR_FUL, CHAR_FUL, CHAR_NUL, CHAR_NUL, CHAR_NUL,
    CHAR_NUL, CHAR_NUL, CHAR_NUL, CHAR_FUL, CHAR_FUL, CHAR_NUL, CHAR_NUL, CHAR_NUL,
    CHAR_NUL, CHAR_NUL, CHAR_NUL, CHAR_FUL, CHAR_FUL, CHAR_NUL, CHAR_NUL, CHAR_NUL,
    CHAR_NUL, CHAR_NUL, CHAR_NUL, CHAR_FUL, CHAR_FUL, CHAR_NUL, CHAR_NUL, CHAR_NUL,
    CHAR_NUL, CHAR_BOT, CHAR_BOT, CHAR_FUL, CHAR_FUL, CHAR_BOT, CHAR_BOT, CHAR_NUL,
};
/*
..###...
.#####..
##...##.
.....##.
.....##.
....##..
...##...
..##....
.##.....
##...##.
#######.
#######.
*/
static const uint8_t digit_2[] = {
    CHAR_NUL, CHAR_BOT, CHAR_FUL, CHAR_FUL, CHAR_FUL, CHAR_BOT, CHAR_NUL, CHAR_NUL,
    CHAR_TOP, CHAR_TOP, CHAR_NUL, CHAR_NUL, CHAR_NUL, CHAR_FUL, CHAR_FUL, CHAR_NUL,
    CHAR_NUL, CHAR_NUL, CHAR_NUL, CHAR_NUL, CHAR_BOT, CHAR_FUL, CHAR_TOP, CHAR_NUL,
    CHAR_NUL, CHAR_NUL, CHAR_BOT, CHAR_FUL, CHAR_TOP, CHAR_NUL, CHAR_NUL, CHAR_NUL,
    CHAR_BOT, CHAR_FUL, CHAR_TOP, CHAR_NUL, CHAR_NUL, CHAR_BOT, CHAR_BOT, CHAR_NUL,
    CHAR_FUL, CHAR_FUL, CHAR_FUL, CHAR_FUL, CHAR_FUL, CHAR_FUL, CHAR_FUL, CHAR_NUL,
};
/*
..###...
.#####..
##...##.
.....##.
.....##.
...###..
...###..
.....##.
.....##.
##...##.
.#####..
..###...
*/
static const uint8_t digit_3[] = {
    CHAR_NUL, CHAR_BOT, CHAR_FUL, CHAR_FUL, CHAR_FUL, CHAR_BOT, CHAR_NUL, CHAR_NUL,
    CHAR_TOP, CHAR_TOP, CHAR_NUL, CHAR_NUL, CHAR_NUL, CHAR_FUL, CHAR_FUL, CHAR_NUL,
    CHAR_NUL, CHAR_NUL, CHAR_NUL, CHAR_BOT, CHAR_BOT, CHAR_FUL, CHAR_TOP, CHAR_NUL,
    CHAR_NUL, CHAR_NUL, CHAR_NUL, CHAR_TOP, CHAR_TOP, CHAR_FUL, CHAR_BOT, CHAR_NUL,
    CHAR_BOT, CHAR_BOT, CHAR_NUL, CHAR_NUL, CHAR_NUL, CHAR_FUL, CHAR_FUL, CHAR_NUL,
    CHAR_NUL, CHAR_TOP, CHAR_FUL, CHAR_FUL, CHAR_FUL, CHAR_TOP, CHAR_NUL, CHAR_NUL,
};
/*
....##..
...###..
..####..
.##.##..
##..##..
#######.
#######.
....##..
....##..
....##..
....##..
...####.
*/
static const uint8_t digit_4[] = {
    CHAR_NUL, CHAR_NUL, CHAR_NUL, CHAR_BOT, CHAR_FUL, CHAR_FUL, CHAR_NUL, CHAR_NUL,
    CHAR_NUL, CHAR_BOT, CHAR_FUL, CHAR_TOP, CHAR_FUL, CHAR_FUL, CHAR_NUL, CHAR_NUL,
    CHAR_FUL, CHAR_FUL, CHAR_BOT, CHAR_BOT, CHAR_FUL, CHAR_FUL, CHAR_BOT, CHAR_NUL,
    CHAR_TOP, CHAR_TOP, CHAR_TOP, CHAR_TOP, CHAR_FUL, CHAR_FUL, CHAR_TOP, CHAR_NUL,
    CHAR_NUL, CHAR_NUL, CHAR_NUL, CHAR_NUL, CHAR_FUL, CHAR_FUL, CHAR_NUL, CHAR_NUL,
    CHAR_NUL, CHAR_NUL, CHAR_NUL, CHAR_BOT, CHAR_FUL, CHAR_FUL, CHAR_BOT, CHAR_NUL,
};
/*
#######.
#######.
##......
##......
######..
#######.
.....##.
.....##.
.....##.
##...##.
.#####..
..###...
*/
static const uint8_t digit_5[] = {
    CHAR_FUL, CHAR_FUL, CHAR_FUL, CHAR_FUL, CHAR_FUL, CHAR_FUL, CHAR_FUL, CHAR_NUL,
    CHAR_FUL, CHAR_FUL, CHAR_NUL, CHAR_NUL, CHAR_NUL, CHAR_NUL, CHAR_NUL, CHAR_NUL,
    CHAR_FUL, CHAR_FUL, CHAR_FUL, CHAR_FUL, CHAR_FUL, CHAR_FUL, CHAR_BOT, CHAR_NUL,
    CHAR_NUL, CHAR_NUL, CHAR_NUL, CHAR_NUL, CHAR_NUL, CHAR_FUL, CHAR_FUL, CHAR_NUL,
    CHAR_BOT, CHAR_BOT, CHAR_NUL, CHAR_NUL, CHAR_NUL, CHAR_FUL, CHAR_FUL, CHAR_NUL,
    CHAR_NUL, CHAR_TOP, CHAR_FUL, CHAR_FUL, CHAR_FUL, CHAR_TOP, CHAR_NUL, CHAR_NUL,
};
/*
..###...
.####...
##......
##......
##......
######..
#######.
##...##.
##...##.
##...##.
.#####..
..###...
*/
static const uint8_t digit_6[] = {
    CHAR_NUL, CHAR_BOT, CHAR_FUL, CHAR_FUL, CHAR_FUL, CHAR_NUL, CHAR_NUL, CHAR_NUL,
    CHAR_FUL, CHAR_FUL, CHAR_NUL, CHAR_NUL, CHAR_NUL, CHAR_NUL, CHAR_NUL, CHAR_NUL,
    CHAR_FUL, CHAR_FUL, CHAR_BOT, CHAR_BOT, CHAR_BOT, CHAR_BOT, CHAR_NUL, CHAR_NUL,
    CHAR_FUL, CHAR_FUL, CHAR_TOP, CHAR_TOP, CHAR_TOP, CHAR_FUL, CHAR_FUL, CHAR_NUL,
    CHAR_FUL, CHAR_FUL, CHAR_NUL, CHAR_NUL, CHAR_NUL, CHAR_FUL, CHAR_FUL, CHAR_NUL,
    CHAR_NUL, CHAR_TOP, CHAR_FUL, CHAR_FUL, CHAR_FUL, CHAR_TOP, CHAR_NUL, CHAR_NUL,
};
/*
#######.
#######.
#....##.
.....##.
....##..
...##...
..##....
..##....
..##....
..##....
..##....
..##....
*/
static const uint8_t digit_7[] = {
    CHAR_FUL, CHAR_FUL, CHAR_FUL, CHAR_FUL, CHAR_FUL, CHAR_FUL, CHAR_FUL, CHAR_NUL,
    CHAR_TOP, CHAR_NUL, CHAR_NUL, CHAR_NUL, CHAR_NUL, CHAR_FUL, CHAR_FUL, CHAR_NUL,
    CHAR_NUL, CHAR_NUL, CHAR_NUL, CHAR_BOT, CHAR_FUL, CHAR_TOP, CHAR_NUL, CHAR_NUL,
    CHAR_NUL, CHAR_NUL, CHAR_FUL, CHAR_FUL, CHAR_NUL, CHAR_NUL, CHAR_NUL, CHAR_NUL,
    CHAR_NUL, CHAR_NUL, CHAR_FUL, CHAR_FUL, CHAR_NUL, CHAR_NUL, CHAR_NUL, CHAR_NUL,
    CHAR_NUL, CHAR_NUL, CHAR_FUL, CHAR_FUL, CHAR_NUL, CHAR_NUL, CHAR_NUL, CHAR_NUL,
};
/*
..###...
.##.##..
##...##.
##...##.
##...##.
.#####..
.#####..
##...##.
##...##.
##...##.
.##.##..
..###...
*/
static const uint8_t digit_8[] = {
    CHAR_NUL, CHAR_BOT, CHAR_FUL, CHAR_TOP, CHAR_FUL, CHAR_BOT, CHAR_NUL, CHAR_NUL,
    CHAR_FUL, CHAR_FUL, CHAR_NUL, CHAR_NUL, CHAR_NUL, CHAR_FUL, CHAR_FUL, CHAR_NUL,
    CHAR_TOP, CHAR_FUL, CHAR_BOT, CHAR_BOT, CHAR_BOT, CHAR_FUL, CHAR_TOP, CHAR_NUL,
    CHAR_BOT, CHAR_FUL, CHAR_TOP, CHAR_TOP, CHAR_TOP, CHAR_FUL, CHAR_BOT, CHAR_NUL,
    CHAR_FUL, CHAR_FUL, CHAR_NUL, CHAR_NUL, CHAR_NUL, CHAR_FUL, CHAR_FUL, CHAR_NUL,
    CHAR_NUL, CHAR_TOP, CHAR_FUL, CHAR_BOT, CHAR_FUL, CHAR_TOP, CHAR_NUL, CHAR_NUL,
};
/*
..###...
.#####..
##...##.
##...##.
##...##.
.######.
..#####.
.....##.
.....##.
.....##.
...###..
..###...
*/
static const uint8_t digit_9[] = {
    CHAR_NUL, CHAR_BOT, CHAR_FUL, CHAR_FUL, CHAR_FUL, CHAR_BOT, CHAR_NUL, CHAR_NUL,
    CHAR_FUL, CHAR_FUL, CHAR_NUL, CHAR_NUL, CHAR_NUL, CHAR_FUL, CHAR_FUL, CHAR_NUL,
    CHAR_TOP, CHAR_FUL, CHAR_BOT, CHAR_BOT, CHAR_BOT, CHAR_FUL, CHAR_FUL, CHAR_NUL,
    CHAR_NUL, CHAR_NUL, CHAR_TOP, CHAR_TOP, CHAR_TOP, CHAR_FUL, CHAR_FUL, CHAR_NUL,
    CHAR_NUL, CHAR_NUL, CHAR_NUL, CHAR_NUL, CHAR_NUL, CHAR_FUL, CHAR_FUL, CHAR_NUL,
    CHAR_NUL, CHAR_NUL, CHAR_BOT, CHAR_FUL, CHAR_FUL, CHAR_TOP, CHAR_NUL, CHAR_NUL,
};

static void draw_digit(uint8_t col, uint8_t row, uint8_t x) {
    const uint8_t *data;
    switch (x) {
    case 0: data = digit_0; break;
    case 1: data = digit_1; break;
    case 2: data = digit_2; break;
    case 3: data = digit_3; break;
    case 4: data = digit_4; break;
    case 5: data = digit_5; break;
    case 6: data = digit_6; break;
    case 7: data = digit_7; break;
    case 8: data = digit_8; break;
    case 9: data = digit_9; break;
    }

    uint8_t *dst = VGA_CHAR_SEG + VGA_OFFSET(col, row);
    for (uint8_t i = 0; i != DIGIT_HEIGHT; ++i, ++row) {
        memcpy(dst, data, DIGIT_WIDTH);
        dst += 128;
        data += DIGIT_WIDTH;
    }
}

void bd_draw_number_centered(uint8_t col, uint8_t row, uint8_t x) {
    if (x < 10) {
        draw_digit(col - DIGIT_WIDTH / 2, row, x);
    } else {
        draw_digit(col - DIGIT_WIDTH, row, x / 10);
        draw_digit(col, row, x % 10);
    }
}
