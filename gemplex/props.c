#include "props.h"
#include "engine.h"
#include <libsys/vga.h>

#define EXPLOSION_CHAR 176
#define EXPLOSION_COLOR COLOR(COLOR_RED, COLOR_YELLOW)

uint8_t object_props[PROP_ARRAY_LEN] = {
    [OBJ_EMPTY      ] = PROP_EMPTY | PROP_EAT,
    [OBJ_PLAYER     ] = PROP_DESTROY | PROP_EXPLODE,
    [OBJ_WALL       ] = 0,
    [OBJ_EXIT       ] = PROP_DESTROY,
    [OBJ_EXIT_OPEN  ] = PROP_DESTROY | PROP_EAT,
    [OBJ_GROUND     ] = PROP_DESTROY | PROP_EAT,
    [OBJ_ROCK       ] = PROP_DESTROY | PROP_PUSH | PROP_ROUND | PROP_ROLL_FALL,
    [OBJ_GEM        ] = PROP_DESTROY | PROP_EAT | PROP_ROUND | PROP_ROLL_FALL,
    [OBJ_BOX        ] = PROP_DESTROY | PROP_ROUND,
    [OBJ_BOMB       ] = PROP_ROLL_FALL | PROP_PUSH | PROP_EXPLODE,
    [OBJ_ENEMY_N    ] = PROP_LETHAL | PROP_EXPLODE,
    [OBJ_ENEMY_E    ] = PROP_LETHAL | PROP_EXPLODE,
    [OBJ_ENEMY_S    ] = PROP_LETHAL | PROP_EXPLODE,
    [OBJ_ENEMY_W    ] = PROP_LETHAL | PROP_EXPLODE,
    [OBJ_ENEMY_N_FWD] = PROP_LETHAL | PROP_EXPLODE,
    [OBJ_ENEMY_E_FWD] = PROP_LETHAL | PROP_EXPLODE,
    [OBJ_ENEMY_S_FWD] = PROP_LETHAL | PROP_EXPLODE,
    [OBJ_ENEMY_W_FWD] = PROP_LETHAL | PROP_EXPLODE,
};

char object_char_l[PROP_ARRAY_LEN] __attribute__((aligned(256))) = {
    [OBJ_EMPTY] = 0,
    [OBJ_PLAYER] = 160,
    [OBJ_WALL] = 219,
    [OBJ_GROUND] = 177,
    [OBJ_ROCK] = 144,
    [OBJ_GEM] = 158,
    [OBJ_BOX] = 146,
    [OBJ_BOMB] = 162,
    [OBJ_ENEMY_N] = 168,
    [OBJ_ENEMY_E] = 144,
    [OBJ_ENEMY_S] = 166,
    [OBJ_ENEMY_W] = 164,
    [OBJ_ENEMY_N_FWD] = 168,
    [OBJ_ENEMY_E_FWD] = 144,
    [OBJ_ENEMY_S_FWD] = 166,
    [OBJ_ENEMY_W_FWD] = 164,
    [OBJ_EXIT] = 216,
    [OBJ_EXIT_OPEN] = 216,
};

char object_char_r[PROP_ARRAY_LEN] __attribute__((aligned(256))) = {
    [OBJ_EMPTY] = 0,
    [OBJ_PLAYER] = 161,
    [OBJ_WALL] = 219,
    [OBJ_GROUND] = 177,
    [OBJ_ROCK] = 145,
    [OBJ_GEM] = 159,
    [OBJ_BOX] = 147,
    [OBJ_BOMB] = 163,
    [OBJ_ENEMY_N] = 169,
    [OBJ_ENEMY_E] = 165,
    [OBJ_ENEMY_S] = 167,
    [OBJ_ENEMY_W] = 145,
    [OBJ_ENEMY_N_FWD] = 169,
    [OBJ_ENEMY_E_FWD] = 165,
    [OBJ_ENEMY_S_FWD] = 167,
    [OBJ_ENEMY_W_FWD] = 145,
    [OBJ_EXIT] = 216,
    [OBJ_EXIT_OPEN] = 216,
};

uint8_t object_color[PROP_ARRAY_LEN] __attribute__((aligned(256))) = {
    [OBJ_EMPTY] = 0,
    [OBJ_PLAYER] = COLOR(COLOR_YELLOW, COLOR_BLACK),
    [OBJ_WALL] = COLOR(COLOR_GRAY, COLOR_BLACK),
    [OBJ_GROUND] = COLOR(COLOR_LIGHT_GREEN, COLOR_BLACK),
    [OBJ_ROCK] = COLOR(COLOR_DARK_GRAY, COLOR_BLACK),
    [OBJ_GEM] = COLOR(COLOR_LIGHT_CYAN, COLOR_BLACK),
    [OBJ_BOX] = COLOR(COLOR_YELLOW, COLOR_BLACK),
    [OBJ_BOMB] = COLOR(COLOR_RED, COLOR_BLACK),
    [OBJ_ENEMY_N] = COLOR(COLOR_MAGENTA, COLOR_BLACK),
    [OBJ_ENEMY_E] = COLOR(COLOR_MAGENTA, COLOR_BLACK),
    [OBJ_ENEMY_S] = COLOR(COLOR_MAGENTA, COLOR_BLACK),
    [OBJ_ENEMY_W] = COLOR(COLOR_MAGENTA, COLOR_BLACK),
    [OBJ_ENEMY_N_FWD] = COLOR(COLOR_MAGENTA, COLOR_BLACK),
    [OBJ_ENEMY_E_FWD] = COLOR(COLOR_MAGENTA, COLOR_BLACK),
    [OBJ_ENEMY_S_FWD] = COLOR(COLOR_MAGENTA, COLOR_BLACK),
    [OBJ_ENEMY_W_FWD] = COLOR(COLOR_MAGENTA, COLOR_BLACK),
    [OBJ_EXIT] = COLOR(COLOR_LIGHT_RED, COLOR_BLACK),
    [OBJ_EXIT_OPEN] = COLOR(COLOR_LIGHT_GREEN, COLOR_BLACK),
};

void props_init(void) {
    for (uint8_t i = 0; i != OBJ_TOTAL; ++i) {
        object_char_l[i | FLAG_NEW] = object_char_l[i];
        object_char_l[i | FLAG_MOVED] = object_char_l[i];
        object_char_l[i | FLAG_NEW | FLAG_MOVED] = object_char_l[i];
        object_char_l[i | FLAG_EXPLOSION] = EXPLOSION_CHAR;
        object_char_l[i | FLAG_EXPLOSION | FLAG_NEW] = EXPLOSION_CHAR;
        object_char_l[i | FLAG_EXPLOSION | FLAG_MOVED] = EXPLOSION_CHAR;
        object_char_l[i | FLAG_EXPLOSION | FLAG_NEW | FLAG_MOVED] = EXPLOSION_CHAR;

        object_char_r[i | FLAG_NEW] = object_char_r[i];
        object_char_r[i | FLAG_MOVED] = object_char_r[i];
        object_char_r[i | FLAG_NEW | FLAG_MOVED] = object_char_r[i];
        object_char_r[i | FLAG_EXPLOSION] = EXPLOSION_CHAR;
        object_char_r[i | FLAG_EXPLOSION | FLAG_NEW] = EXPLOSION_CHAR;
        object_char_r[i | FLAG_EXPLOSION | FLAG_MOVED] = EXPLOSION_CHAR;
        object_char_r[i | FLAG_EXPLOSION | FLAG_NEW | FLAG_MOVED] = EXPLOSION_CHAR;

        object_color[i | FLAG_NEW] = object_color[i];
        object_color[i | FLAG_MOVED] = object_color[i];
        object_color[i | FLAG_NEW | FLAG_MOVED] = object_color[i];
        object_color[i | FLAG_EXPLOSION] = EXPLOSION_COLOR;
        object_color[i | FLAG_EXPLOSION | FLAG_NEW] = EXPLOSION_COLOR;
        object_color[i | FLAG_EXPLOSION | FLAG_MOVED] = EXPLOSION_COLOR;
        object_color[i | FLAG_EXPLOSION | FLAG_NEW | FLAG_MOVED] = EXPLOSION_COLOR;

        object_props[i | FLAG_NEW] = object_props[i];
        object_props[i | FLAG_EXPLOSION] = PROP_LETHAL;
        object_props[i | FLAG_EXPLOSION | FLAG_NEW] = PROP_LETHAL;
        object_props[i | FLAG_EXPLOSION | FLAG_MOVED] = PROP_LETHAL;
        object_props[i | FLAG_EXPLOSION | FLAG_NEW | FLAG_MOVED] = PROP_LETHAL;
    }
}

