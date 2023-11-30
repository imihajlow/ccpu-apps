#include <assert.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include <libsys/ps2keyboard.h>
#include <libsys/vga.h>

#include "lib/quasipixel.h"

#include "snake_maps.h"

#define MAX_SNAKE 255

static_assert(MAX_SNAKE <= UINT8_MAX, "snake index must fit in a byte");

struct Coord {
    uint8_t x, y;
};

static struct Coord snake[MAX_SNAKE];

static void init_snake(void) {
    snake[0].x = QP_WIDTH / 2 - 1;
    snake[1].x = QP_WIDTH / 2;
    snake[2].x = QP_WIDTH / 2 + 1;
    snake[0].y = QP_HEIGHT / 2;
    snake[1].y = QP_HEIGHT / 2;
    snake[2].y = QP_HEIGHT / 2;

    for (uint8_t i = 0; i != 3; ++i) {
        qp_set(snake[i].x, snake[i].y, true);
    }
    qp_render();
}

static void loose(void) {
    qp_set_color(COLOR_RED, COLOR_BLACK);
    while (ps2_get_key_event() != PS2_KEY_ENTER) ;
}

static void pause(void) {
    while (ps2_get_key_event() != PS2_KEY_P) ;
}

static int delay = 160;
static bool run_snake(const uint8_t *map) {
    qp_init(COLOR_GREEN, COLOR_BLACK);

    render_map(map);

    init_snake();

    uint8_t head = 2;
    uint8_t tail = 0;
    int8_t dx = 1;
    int8_t dy = 0;
    int8_t head_x = snake[head].x;
    int8_t head_y = snake[head].y;
    uint8_t apple_x = head_x + 1;
    uint8_t apple_y = head_y;

    qp_set_and_render(apple_x, apple_y, true);

    bool run = true;
    while (run) {
        head_x += dx;
        head_y += dy;
        if (head_x == QP_WIDTH) {
            head_x = 0;
        } else if (head_x < 0) {
            head_x = QP_WIDTH - 1;
        }
        if (head_y == QP_HEIGHT) {
            head_y = 0;
        } else if (head_y < 0) {
            head_y = QP_HEIGHT - 1;
        }
        qp_set_and_render(snake[tail].x, snake[tail].y, false);
        if (head_x == apple_x && head_y == apple_y) {
            while (qp_get(apple_x, apple_y)) {
                apple_x = rand() % QP_WIDTH;
                apple_y = rand() % QP_HEIGHT;
            }
            qp_set_and_render(apple_x, apple_y, true);
        } else if (qp_get(head_x, head_y)) {
            loose();
            return true;
        } else {
            qp_set_and_render(head_x, head_y, true);
            tail += 1;
            if (tail >= MAX_SNAKE) {
                tail = 0;
            }
        }
        head += 1;
        if (head >= MAX_SNAKE) {
            head = 0;
        }
        snake[head].x = head_x;
        snake[head].y = head_y;

        for (int i = 0; i != delay; ++i) {
            uint8_t key = ps2_get_key_event();
            if (key) {
                switch (key) {
                case PS2_KEY_ESCAPE:
                    run = false;
                    break;
                case PS2_KEY_EQUALS:
                case PS2_KEY_NUM_ADD:
                    if (delay > 1) {
                        --delay;
                    }
                    break;
                case PS2_KEY_DASH:
                case PS2_KEY_NUM_SUB:
                    ++delay;
                    break;
                case PS2_KEY_P:
                    pause();
                    break;
                default:
                    break;
                }
                if (dy == 0) {
                    switch (key) {
                    case PS2_KEY_UP:
                        dy = -1;
                        dx = 0;
                        break;
                    case PS2_KEY_DOWN:
                        dy = 1;
                        dx = 0;
                        break;
                    default: break;
                    }
                } else if (dx == 0) {
                    switch (key) {
                    case PS2_KEY_LEFT:
                        dy = 0;
                        dx = -1;
                        break;
                    case PS2_KEY_RIGHT:
                        dy = 0;
                        dx = 1;
                        break;
                    default: break;
                    }
                }
                break;
            }
        }
    }
    return run;
}

void main(void) {
    const uint8_t *maps[5];
    maps[0] = map_corners;
    maps[1] = map_box;
    maps[2] = map_obstacles;
    maps[3] = map_maze;
    maps[4] = map_snake;
    uint16_t map_index = 0;
    while (run_snake(maps[map_index])) {
        map_index += 1;
        if (map_index == sizeof(maps) / sizeof(maps[0])) {
            map_index = 0;
        }
    }
}

int getchar(void) {}
int putchar(int c) {}
