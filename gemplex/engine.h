#pragma once
#include <stdbool.h>
#include <assert.h>

#define FLAG_NEW 0x20 // this cell has to be checked on next turn
#define FLAG_MOVED 0x40 // this cell has been filled on this turn
#define FLAG_EXPLOSION 0x80 // explosion in this cell

#define PROP_EMPTY (1 << 0) // empty space
#define PROP_ROLL_FALL (1 << 1) // falls if nothing is below it, rolls when sits on top of something with FLAG_ROUND
#define PROP_EAT (1 << 2) // can be eaten
#define PROP_EXPLODE (1 << 3) // explodes when falls or if struck above
#define PROP_DESTROY (1 << 4) // can be destroyed by explosions
#define PROP_ROUND (1 << 5) // rolling things roll if placed on top of this
#define PROP_PUSH (1 << 6) // can be pushed
#define PROP_LETHAL (1 << 7) // letal to touch

typedef enum Object {
    OBJ_EMPTY = 0,
    OBJ_PLAYER,
    OBJ_EXIT,
    OBJ_WALL,
    OBJ_GROUND,
    OBJ_ROCK,
    OBJ_GEM,
    OBJ_BOX,
    OBJ_BOMB,
    OBJ_ENEMY_N,
    OBJ_ENEMY_E,
    OBJ_ENEMY_S,
    OBJ_ENEMY_W,
    OBJ_ENEMY_N_FWD,
    OBJ_ENEMY_E_FWD,
    OBJ_ENEMY_S_FWD,
    OBJ_ENEMY_W_FWD,
    OBJ_TOTAL,
} Object;

static_assert(OBJ_TOTAL <= 32, "No more than 32 objects!");

void engine_init(void);
void engine_step(void);
void engine_render(void);
bool engine_load(const char *filename);
void engine_collect_changed(void);

void engine_up(void);
void engine_down(void);
void engine_left(void);
void engine_right(void);
