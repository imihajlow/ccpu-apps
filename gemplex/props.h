#pragma once
#include <stdint.h>

#define PROP_ARRAY_LEN (32 * 8)

extern uint8_t object_props[PROP_ARRAY_LEN];
extern char object_char_l[PROP_ARRAY_LEN] __attribute__((aligned(256)));
extern char object_char_r[PROP_ARRAY_LEN] __attribute__((aligned(256)));
extern uint8_t object_color[PROP_ARRAY_LEN] __attribute__((aligned(256)));

void props_init(void);
