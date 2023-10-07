#pragma once
#include <stdint.h>
#include <stdbool.h>

#define QP_WIDTH 80
#define QP_HEIGHT 60
#define QP_SIZE (30 * 128)

extern uint8_t qp_fb[QP_SIZE];

void qp_init(uint8_t fg_color, uint8_t bg_color);

void qp_set_color(uint8_t fg_color, uint8_t bg_color);

/*
    Sets the pixel to the value v, but doesn't render.
 */
void qp_set(uint8_t x, uint8_t y, bool v);

bool qp_get(uint8_t x, uint8_t y);

/*
    Sets the pixel to the value v and renders it immediately.
 */
void qp_set_and_render(uint8_t x, uint8_t y, bool v);

void qp_render(void);

/*
    Renders only part of the screen. Coordinates are character coordinates.
 */
void qp_render_rect(uint8_t col_from, uint8_t row_from, uint8_t col_to, uint8_t row_to);
void qp_render_one(uint8_t x, uint8_t y);

void qp_set_cursor_enabled(bool enabled);
void qp_set_cursor_pos(uint8_t x, uint8_t y);
void qp_move_cursor(int8_t dx, int8_t dy);

extern uint8_t qp_cursor_x;
extern uint8_t qp_cursor_y;
