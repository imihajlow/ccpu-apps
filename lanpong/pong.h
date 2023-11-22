#pragma once
#include <stdint.h>

enum pong_input_t {
    INPUT_UP_PRESSED,
    INPUT_UP_RELEASED,
    INPUT_DOWN_PRESSED,
    INPUT_DOWN_RELEASED,
};

enum pong_result_t {
    PONG_OK = 0,
    PONG_LEFT_LOST,
};


extern uint8_t board_left_row;
extern int16_t pong_ball_x;
extern int16_t pong_ball_y;
extern int16_t pong_ball_speed_x;
extern int16_t pong_ball_speed_y;

void pong_init(void);
void pong_input(enum pong_input_t input);
enum pong_result_t pong_step(void);
void pong_set_right_board(uint8_t row);
void pong_ball_bounce_right(int16_t ball_x, int16_t ball_y, int16_t ball_speed_y);
