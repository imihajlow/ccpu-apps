#include "pong.h"

#include <libsys/vga.h>
#include <stdbool.h>
#include <stdlib.h>

#define BOARD_CYCLES 10
#define BOARD_ACCELERATION 1

#define CHAR_TOP 223
#define CHAR_BOT 220
#define CHAR_BOARD 219

#define TOP (1 * 512)
#define BOTTOM (VGA_ROWS * 512)
#define LEFT (1 * 256)
#define RIGHT ((VGA_COLS - 1) * 256)

#define BOARD_SIZE 3

static uint8_t board_right_row = VGA_ROWS >> 1;

uint8_t board_left_row = VGA_ROWS >> 1;
static uint16_t board_left_y; // screen_y * 256

static int16_t board_left_speed;

int16_t pong_ball_x; // screen_x * 256
int16_t pong_ball_y; // screen_y * 512

int16_t pong_ball_speed_x;
int16_t pong_ball_speed_y;

// quasipixel coordinates
static uint8_t last_ball_row;
static uint8_t last_ball_col;

static bool up_pressed = false;
static bool down_pressed = false;


static void show_board(uint8_t col, uint8_t pos) {
    char *p = VGA_CHAR_SEG + VGA_OFFSET(col, TOP >> 9);
    for (uint8_t r = TOP >> 9; r != VGA_ROWS; r += 1) {
        if (r >= pos - BOARD_SIZE && r <= pos + BOARD_SIZE) {
            *p = CHAR_BOARD;
        } else {
            *p = 0;
        }
        p += 128;
    }
}

void pong_init(void) {
    if (pong_ball_speed_x > 0) {
        pong_ball_speed_x = 1;
    } else {
        pong_ball_speed_x = -1;
    }
    pong_ball_speed_y = rand() % 10 - 5;
    pong_ball_x = VGA_COLS << 7;
    pong_ball_y = VGA_ROWS << 8;
    board_left_speed = 0;
    board_left_row = VGA_ROWS >> 1;
    board_left_y = board_left_row << 8;
    board_right_row = VGA_ROWS >> 1;
    up_pressed = false;
    down_pressed = false;
    show_board(0, board_left_row);
    show_board(VGA_COLS - 1, board_right_row);
}

void pong_input(enum pong_input_t input) {
    switch (input) {
    case INPUT_UP_PRESSED:
        if (!up_pressed) {
            board_left_y -= 256;
        }
        up_pressed = true;
        break;
    case INPUT_UP_RELEASED:
        up_pressed = false;
        board_left_speed = 0;
        break;
    case INPUT_DOWN_PRESSED:
        if (!down_pressed) {
            board_left_y += 256;
        }
        down_pressed = true;
        break;
    case INPUT_DOWN_RELEASED:
        down_pressed = false;
        board_left_speed = 0;
        break;
    }
}

static void board_step(void) {
    static uint8_t board_cycle = 0;
    board_cycle += 1;
    if (board_cycle == BOARD_CYCLES) {
        board_cycle = 0;
        if (up_pressed) {
            board_left_speed -= BOARD_ACCELERATION;
        }
        if (down_pressed) {
            board_left_speed += BOARD_ACCELERATION;
        }

        board_left_y += board_left_speed;

        if (board_left_y < (BOARD_SIZE << 8) + (TOP >> 1)) {
            board_left_y = (BOARD_SIZE << 8) + (TOP >> 1);
        } else if (board_left_y >= ((BOTTOM >> 1) - (BOARD_SIZE << 8) - 1)) {
            board_left_y = ((BOTTOM >> 1) - (BOARD_SIZE << 8) - 1); // 27 * 256 - 1
        }

        uint8_t new_board_left_row = board_left_y >> 8; // 26
        if (new_board_left_row != board_left_row) {
            show_board(0, new_board_left_row);
            board_left_row = new_board_left_row;
        }
    }
}

enum pong_result_t pong_step(void) {
    pong_ball_x += pong_ball_speed_x;
    pong_ball_y += pong_ball_speed_y;

    if (pong_ball_y < TOP) {
        pong_ball_speed_y = -pong_ball_speed_y;
        pong_ball_y = 2 * TOP - pong_ball_y;
    } else if (pong_ball_y >= BOTTOM) {
        pong_ball_speed_y = -pong_ball_speed_y;
        pong_ball_y = 2 * BOTTOM - pong_ball_y;
    }
    uint8_t ball_row = pong_ball_y >> 8;
    uint8_t ball_screen_row = ball_row >> 1;
    if (pong_ball_x < LEFT) {
        if (ball_screen_row >= board_left_row - BOARD_SIZE && ball_screen_row <= board_left_row + BOARD_SIZE) {
            pong_ball_speed_x = -pong_ball_speed_x + 1;
            pong_ball_x = 2 * LEFT - pong_ball_x;
            pong_ball_speed_y = (int8_t)ball_screen_row - (int8_t)board_left_row;
        } else {
            return PONG_LEFT_LOST;
        }
    }

    uint8_t ball_col = pong_ball_x >> 8;

    if (last_ball_col != ball_col || last_ball_row != ball_row) {
        uint16_t last_offset = VGA_OFFSET(last_ball_col, last_ball_row >> 1);
        uint16_t new_offset = VGA_OFFSET(ball_col, ball_screen_row);
        if (last_offset != new_offset && last_ball_col != 0 && last_ball_col != VGA_COLS - 1) {
            VGA_CHAR_SEG[last_offset] = 0;
        }

        uint8_t c;
        if (ball_row & 1) {
            c = CHAR_BOT;
        } else {
            c = CHAR_TOP;
        }
        if (ball_col != 0 && ball_col != VGA_COLS - 1) {
            VGA_CHAR_SEG[new_offset] = c;
        }
        last_ball_row = ball_row;
        last_ball_col = ball_col;
    }

    board_step();

    return PONG_OK;
}

void pong_ball_bounce_right(int16_t ball_x, int16_t ball_y, int16_t ball_speed_y) {
    pong_ball_x = ball_x;
    pong_ball_y = ball_y;
    pong_ball_speed_y = ball_speed_y;
    pong_ball_speed_x = -pong_ball_speed_x - 1;
}

void pong_set_right_board(uint8_t row) {
    board_right_row = row;
    show_board(VGA_COLS - 1, row);
}
