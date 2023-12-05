#pragma once

#include <stdint.h>

#define PACKED __attribute__((packed))
#define MAGIC_COOKIE 0x474e4f50 // "PONG"

enum net_message_tag_t {
    ANNOUNCE = 0, // broadcast announcement
    JOIN = 1, // join game
    READY = 2, // player is ready
    PONG = 3, // right board has been hit
    BOARD = 4, // board state
    BALLBOARD = 5, // ball and board state
    SCORE = 6,
};

struct pong_t {
    int16_t ball_x;
    int16_t ball_y;
    int16_t ball_speed_y;
} PACKED;

struct ball_board_t {
    int16_t ball_x;
    int16_t ball_y;
    int16_t ball_speed_x;
    int16_t ball_speed_y;
    uint8_t board_col;
} PACKED;

struct board_t {
    uint8_t board_col;
} PACKED;

struct score_t {
    uint16_t score_l;
    uint16_t score_r;
} PACKED;

struct net_message_t {
    uint32_t cookie;
    uint8_t tag;
    union {
        struct pong_t pong;
        struct ball_board_t ball_board;
        struct board_t board;
        struct score_t score;
    };
} PACKED;

void game_init(void);

void game_appcall(void);

void game_process(void);
