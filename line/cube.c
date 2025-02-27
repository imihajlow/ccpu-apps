#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <libsys/vga.h>
#include <libsys/ps2keyboard.h>
#include "line.h"
#include "fpmath/fixpoint.h"

#define CHAR_BASE 240

struct Point3 {
    fp16 x, y, z;
};

struct ScreenPoint {
    int8_t x, y;
};

void trap(fp16 x, fp16 y) {}

static void project_point(const struct Point3 *pin, struct ScreenPoint *pout) {
    // Projection plane is at z = 1.5
    // scale = 1.5 / pin->z
    fp16 scale = fp16_12_rc(pin->z);
    scale += scale >> 1;
    fp16 x = fp16_12_mul(pin->x, scale);
    fp16 y = fp16_12_mul(pin->y, scale);
    trap(x,y);

    int8_t xs = (x << 2) >> 8;
    int8_t ys = (y << 2) >> 8;

    if (xs > VGA_COLS / 2) {
        pout->x = 127;
    } else {
        pout->x = xs + VGA_COLS / 2;
    }

    if (ys > VGA_ROWS) {
        pout->y = 127;
    } else {
        pout->y = ys + VGA_ROWS;
    }
}

static struct Point3 cube_points[8];
static struct ScreenPoint projected_points[8];
static const uint8_t cube_edges[] = {
    0, 1, 1, 3, 3, 2, 2, 0,
    4, 5, 5, 7, 7, 6, 6, 4,
    0, 4, 1, 5, 3, 7, 2, 6,
};

static void line_checked(const struct ScreenPoint *a, const struct ScreenPoint *b) {
    if (a->x < 0) return;
    if (a->y < 0) return;
    if (b->x < 0) return;
    if (b->y < 0) return;
    if (a->x > VGA_COLS) return;
    if (b->x > VGA_COLS) return;
    if (a->y > VGA_ROWS * 2) return;
    if (b->y > VGA_ROWS * 2) return;

    line(a->x, a->y, b->x, b->y);
}

static const fp16 z_offset = 3 << 12;

static void init_cube(void) {
    /*
      2---3
     /|  /|
    0-+-1 |
    | 6-+-7
    |/  |/
    4---5
    */
    fp16 size = 1 << 12;

    cube_points[0].y = size >> 1;
    cube_points[1].y = size >> 1;
    cube_points[2].y = size >> 1;
    cube_points[3].y = size >> 1;
    cube_points[4].y = -(size >> 1);
    cube_points[5].y = -(size >> 1);
    cube_points[6].y = -(size >> 1);
    cube_points[7].y = -(size >> 1);

    cube_points[0].x = -(size >> 1);
    cube_points[2].x = -(size >> 1);
    cube_points[4].x = -(size >> 1);
    cube_points[6].x = -(size >> 1);
    cube_points[1].x = size >> 1;
    cube_points[3].x = size >> 1;
    cube_points[5].x = size >> 1;
    cube_points[7].x = size >> 1;

    cube_points[2].z = z_offset + (size >> 1);
    cube_points[3].z = z_offset + (size >> 1);
    cube_points[6].z = z_offset + (size >> 1);
    cube_points[7].z = z_offset + (size >> 1);
    cube_points[0].z = z_offset + -(size >> 1);
    cube_points[1].z = z_offset + -(size >> 1);
    cube_points[4].z = z_offset + -(size >> 1);
    cube_points[5].z = z_offset + -(size >> 1);
}

static void rotate_point(int8_t angle, const struct Point3 *pin, struct Point3 *pout) {
    fp16 sin = fp16_12_sin(angle);
    fp16 cos = fp16_12_cos(angle);

    pout->x = fp16_12_mul(cos, pin->x) + fp16_12_mul(sin, pin->z - z_offset);
    pout->y = pin->y;
    pout->z = -fp16_12_mul(sin, pin->x) + fp16_12_mul(cos, pin->z - z_offset) + z_offset;
}

static void draw_cube(int8_t angle) {
    for (uint8_t i = 0; i != 8; ++i) {
        struct Point3 rotated;
        rotate_point(angle, cube_points + i, &rotated);
        project_point(&rotated, projected_points + i);
    }

    for (uint8_t i = 0; i != sizeof(cube_edges); i += 2) {
        uint8_t f = cube_edges[i];
        uint8_t t = cube_edges[i + 1];
        line_checked(projected_points + f, projected_points + t);
    }
}

void main(void) {
    vga_clear(COLOR(COLOR_WHITE, COLOR_BLACK));
    init_cube();
    int8_t angle = 0;
    while (true) {
        memset(VGA_CHAR_SEG, CHAR_BASE, VGA_ROWS * 128);
        draw_cube(angle);

        uint8_t key = ps2_wait_key_pressed();
        switch (key) {
        case PS2_KEY_DOWN: angle += 1; break;
        case PS2_KEY_UP: angle -= 1; break;
        // case PS2_KEY_RIGHT: if (x1 < VGA_COLS) x1 += 1; break;
        // case PS2_KEY_LEFT: if (x1 != 0) x1 -= 1; break;

        // case PS2_KEY_S: if (y0 < 2 * VGA_ROWS) y0 += 1; break;
        // case PS2_KEY_W: if (y0 != 0) y0 -= 1; break;
        // case PS2_KEY_D: if (x0 < VGA_COLS) x0 += 1; break;
        // case PS2_KEY_A: if (x0 != 0) x0 -= 1; break;
        }
    }
}


int getchar(void) { return -1; }
void putchar(int x) {}
