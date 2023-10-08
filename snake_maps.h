#pragma once

#include <stdint.h>

extern const uint8_t map_box[];
extern const uint8_t map_corners[];
extern const uint8_t map_obstacles[];
extern const uint8_t map_maze[];
extern const uint8_t map_snake[];

void render_map(const uint8_t *map);
