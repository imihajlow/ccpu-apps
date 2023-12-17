#include "engine.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <strings.h>

#include <libsys/vga.h>
#include <libsys/fat/fat.h>

#include "props.h"

#define MAP_W 32
#define MAP_H 30
#define MAX_CHANGES (MAP_W * MAP_H)

#define IS_NORMAL_OBJECT(obj) (!((obj) & (FLAG_EXPLOSION | FLAG_MOVED)))

uint8_t map[MAP_W * MAP_H] __attribute__((aligned(256)));

uint16_t change[MAX_CHANGES];
uint16_t change_count;

static uint8_t player_x;
static uint8_t player_y;

static void explosion(uint16_t idx);
static void apply_flag(uint16_t idx, uint8_t flag);
static void check_fall(uint16_t idx, uint8_t c, uint8_t r);
static void check_roll(uint16_t idx, uint8_t c, uint8_t r);
static void render_one(uint16_t idx, uint8_t obj);

void engine_init(void) {
    bzero(map, sizeof(map));
    change_count = 0;
    props_init();
}

void trap(void) {}

void engine_step(void) {
    uint16_t *pchange = change;
    for (uint16_t change_idx = 0; change_idx != change_count; ++change_idx, ++pchange) {
        uint16_t idx = *pchange;
        uint8_t changed_obj = map[idx];
        uint8_t changed_obj_prop = object_props[changed_obj];
        uint8_t changed_r = idx / MAP_W;
        uint8_t changed_c = idx % MAP_W;

        if (changed_obj_prop & PROP_EMPTY) {
            uint16_t n_idx = idx - MAP_W;
            uint16_t w_idx = idx - 1;
            uint16_t e_idx = idx + 1;
            uint16_t s_idx = idx + MAP_W;
            uint8_t n_obj = map[n_idx];
            uint8_t s_obj = map[s_idx];
            uint8_t w_obj = map[w_idx];
            uint8_t e_obj = map[e_idx];
            // check if something falls from above
            uint8_t n_obj_prop = object_props[n_obj];
            if (n_obj_prop & PROP_ROLL_FALL) {
                map[n_idx] = OBJ_EMPTY | FLAG_NEW | FLAG_MOVED;
                map[idx] = FLAG_NEW | FLAG_MOVED | n_obj;
                render_one(n_idx, OBJ_EMPTY);
                render_one(idx, n_obj);
                if (n_obj_prop & PROP_EXPLODE) {
                    explosion(idx);
                } else if (IS_NORMAL_OBJECT(s_obj)) {
                    uint8_t s_obj_prop = object_props[s_obj];
                    if (s_obj_prop & PROP_EXPLODE) {
                        explosion(s_idx);
                    }
                }
                map[s_idx] |= FLAG_NEW;
                map[w_idx] |= FLAG_NEW;
                map[e_idx] |= FLAG_NEW;
                map[n_idx - MAP_W] |= FLAG_NEW;
                continue;
            }
            // check if something can roll
            uint8_t s_obj_prop = object_props[s_obj];
            if (s_obj_prop & PROP_EMPTY) {
                uint8_t e_obj_prop = object_props[e_obj];
                if (e_obj_prop & PROP_ROLL_FALL) {
                    uint16_t se_idx = idx + (MAP_W + 1);
                    uint8_t se_obj = map[se_idx];
                    uint8_t se_obj_prop = object_props[se_obj];
                    if (se_obj_prop & PROP_ROUND) {
                        uint16_t ne_idx = n_idx + 1;
                        map[e_idx] = OBJ_EMPTY | FLAG_NEW | FLAG_MOVED;
                        map[idx] = FLAG_NEW | FLAG_MOVED | e_obj;
                        map[s_idx] |= FLAG_NEW;
                        map[ne_idx] |= FLAG_NEW;
                        render_one(e_idx, OBJ_EMPTY);
                        render_one(idx, e_obj);
                        continue;
                    }
                }
                uint8_t w_obj_prop = object_props[w_obj];
                if (w_obj_prop & PROP_ROLL_FALL) {
                    uint16_t sw_idx = idx + (MAP_W - 1);
                    uint8_t sw_obj = map[sw_idx];
                    uint8_t sw_obj_prop = object_props[sw_obj];
                    if (sw_obj_prop & PROP_ROUND) {
                        uint16_t nw_idx = n_idx - 1;
                        map[w_idx] = OBJ_EMPTY | FLAG_NEW | FLAG_MOVED;
                        map[idx] = FLAG_NEW | FLAG_MOVED | w_obj;
                        map[s_idx] |= FLAG_NEW;
                        map[nw_idx] |= FLAG_NEW;
                        render_one(w_idx, OBJ_EMPTY);
                        render_one(idx, w_obj);
                        continue;
                    }
                }
            }
        } else if ((changed_obj & (FLAG_EXPLOSION | FLAG_MOVED)) == FLAG_EXPLOSION) {
            uint8_t affected_obj = changed_obj & ~FLAG_EXPLOSION;
            uint8_t affected_obj_prop = object_props[affected_obj];
            if (affected_obj_prop & PROP_DESTROY) {
                map[idx] = FLAG_NEW | FLAG_MOVED | OBJ_EMPTY;
                render_one(idx, OBJ_EMPTY);
            } else if (affected_obj_prop & PROP_EXPLODE) {
                explosion(idx);
            } else {
                map[idx] = FLAG_NEW | FLAG_MOVED | affected_obj;
                render_one(idx, affected_obj);
            }
        }
    }
}

/*
void engine_collect_changed(void) {
    uint16_t i = 0;
    uint16_t change_idx = 0;
    for (uint8_t *pobject = map; pobject != map + MAP_W * MAP_H; ++pobject, ++i) {
        uint8_t obj = *pobject;
        if ((obj & FLAG_NEW) && (obj & ~(FLAG_NEW | FLAG_MOVED)) == OBJ_EMPTY) {
            change[change_idx] = i;
            ++change_idx;
        }
        *pobject = obj & ~(FLAG_NEW | FLAG_MOVED);
    }
    change_count = change_idx;
}*/

static char fat_buf[64] __attribute__((section("bss_hi")));

bool engine_load(const char *filename) {
    strcpy(fat_buf, filename);
    uint8_t fd = fat_open_path(fat_buf, 0);
    if (fd == FAT_BAD_DESC) {
        return false;
    }

    bool result = false;

    uint16_t bytes_read;
    uint8_t *p = map;
    uint16_t count = 0;
    do {
        bytes_read = fat_read(fd, fat_buf, sizeof(fat_buf));
        for (uint8_t i = 0; i != (uint8_t)bytes_read; ++i) {
            uint8_t obj = 0xff;
            switch (fat_buf[i]) {
            case ' ': obj = OBJ_EMPTY; break;
            case 'p': obj = OBJ_PLAYER; break;
            case 'E': obj = OBJ_EXIT; break;
            case '=': obj = OBJ_WALL; break;
            case '#': obj = OBJ_GROUND; break;
            case 'o': obj = OBJ_ROCK; break;
            case '*': obj = OBJ_GEM; break;
            case 'n': obj = OBJ_BOX; break;
            case 'b': obj = OBJ_BOMB; break;
            case '^': obj = OBJ_ENEMY_N; break;
            case '<': obj = OBJ_ENEMY_E; break;
            case 'v': obj = OBJ_ENEMY_S; break;
            case '>': obj = OBJ_ENEMY_W; break;
            }
            if (obj != 0xff) {
                if (obj == OBJ_PLAYER) {
                    player_x = count % MAP_W;
                    player_y = count / MAP_W;
                }
                *p = obj;
                ++p;
                ++count;
                if (count == MAP_W * MAP_H) {
                    result = true;
                    goto done;
                }
            }
        }
    } while (bytes_read == sizeof(fat_buf));
done:
    fat_close(fd);
    return result;
}

void engine_up(void) {
    uint16_t player_idx = player_x + player_y * MAP_W;
    uint16_t top_idx = player_idx - MAP_W;
    uint8_t top_obj = map[top_idx];
    uint8_t top_obj_prop = object_props[top_obj];
    if (top_obj_prop & PROP_EAT) {
        map[top_idx] = OBJ_PLAYER | FLAG_NEW;
        map[player_idx] = OBJ_EMPTY | FLAG_NEW;
        player_y -= 1;
        apply_flag(top_idx, FLAG_NEW);
        render_one(top_idx, OBJ_PLAYER);
        render_one(player_idx, OBJ_EMPTY);
    }
}

void engine_down(void) {
    uint16_t player_idx = player_x + player_y * MAP_W;
    uint16_t bot_idx = player_idx + MAP_W;
    uint8_t bot_obj = map[bot_idx];
    uint8_t bot_obj_prop = object_props[bot_obj];
    if (bot_obj_prop & PROP_EAT) {
        map[bot_idx] = OBJ_PLAYER | FLAG_NEW;
        map[player_idx] = OBJ_EMPTY | FLAG_NEW;
        player_y += 1;
        apply_flag(player_idx, FLAG_NEW);
        render_one(bot_idx, OBJ_PLAYER);
        render_one(player_idx, OBJ_EMPTY);
    }
}

void engine_left(void) {
    uint16_t player_idx = player_x + player_y * MAP_W; // 13 + 10 * 32 = 333
    uint16_t left_idx = player_idx - 1;
    uint8_t left_obj = map[left_idx];
    uint8_t left_obj_prop = object_props[left_obj];
    if (left_obj_prop & PROP_EAT) {
        map[left_idx] = OBJ_PLAYER | FLAG_NEW;
        map[player_idx] = OBJ_EMPTY | FLAG_NEW;
        render_one(left_idx, OBJ_PLAYER);
        render_one(player_idx, OBJ_EMPTY);
        player_x -= 1;
        apply_flag(player_idx, FLAG_NEW);
    } else if (left_obj_prop & PROP_PUSH) {
        uint16_t left_left_idx = left_idx - 1;
        uint8_t left_left_obj = map[left_left_idx];
        uint8_t left_left_obj_prop = object_props[left_left_obj];
        if (left_left_obj_prop & PROP_EMPTY) {
            map[left_left_idx] = left_obj | FLAG_NEW;
            map[left_idx] = OBJ_PLAYER | FLAG_NEW;
            map[player_idx] = OBJ_EMPTY | FLAG_NEW;
            player_x -= 1;
            apply_flag(player_idx, FLAG_NEW);
            apply_flag(left_left_idx, FLAG_NEW);
            render_one(left_left_idx, left_obj);
            render_one(left_idx, OBJ_PLAYER);
            render_one(player_idx, OBJ_EMPTY);
        }
    }
}

void engine_right(void) {
    uint16_t player_idx = player_x + player_y * MAP_W;
    uint16_t right_idx = player_idx + 1;
    uint8_t right_obj = map[right_idx];
    uint8_t right_obj_prop = object_props[right_obj];
    if (!right_obj || (right_obj_prop & PROP_EAT)) {
        map[right_idx] = OBJ_PLAYER | FLAG_NEW;
        map[player_idx] = OBJ_EMPTY | FLAG_NEW;
        render_one(right_idx, OBJ_PLAYER);
        render_one(player_idx, OBJ_EMPTY);
        player_x += 1;
        apply_flag(player_idx, FLAG_NEW);
    } else if (right_obj_prop & PROP_PUSH) {
        uint16_t right_right_idx = right_idx + 1;
        uint8_t right_right_obj = map[right_right_idx];
        uint8_t right_right_obj_prop = object_props[right_right_obj];
        if (right_right_obj_prop & PROP_EMPTY) {
            map[right_right_idx] = right_obj | FLAG_NEW;
            map[right_idx] = OBJ_PLAYER | FLAG_NEW;
            map[player_idx] = OBJ_EMPTY | FLAG_NEW;
            player_x += 1;
            apply_flag(player_idx, FLAG_NEW);
            apply_flag(right_right_idx, FLAG_NEW);
            render_one(right_right_idx, right_obj);
            render_one(right_idx, OBJ_PLAYER);
            render_one(player_idx, OBJ_EMPTY);
        }
    }
}

static void explosion(uint16_t idx) {
    map[idx] = OBJ_EMPTY;
    apply_flag(idx, FLAG_NEW | FLAG_EXPLOSION | FLAG_MOVED);
    render_one(idx, FLAG_EXPLOSION);
    render_one(idx + 1, FLAG_EXPLOSION);
    render_one(idx - 1, FLAG_EXPLOSION);
    idx += MAP_W;
    render_one(idx, FLAG_EXPLOSION);
    render_one(idx + 1, FLAG_EXPLOSION);
    render_one(idx - 1, FLAG_EXPLOSION);
    idx -= 2 * MAP_W;
    render_one(idx, FLAG_EXPLOSION);
    render_one(idx + 1, FLAG_EXPLOSION);
    render_one(idx - 1, FLAG_EXPLOSION);
}

static void apply_flag(uint16_t idx, uint8_t flag) {
    map[idx] |= flag;
    map[idx - 1] |= flag;
    map[idx + 1] |= flag;
    idx += MAP_W;
    map[idx] |= flag;
    map[idx - 1] |= flag;
    map[idx + 1] |= flag;
    idx -= 2 * MAP_W;
    map[idx] |= flag;
    map[idx - 1] |= flag;
    map[idx + 1] |= flag;
}

static_assert(MAP_W == 32, "render_one requires MAP_W to be 32");
static void render_one(uint16_t idx, uint8_t obj) {
    uint16_t r = idx & ~(MAP_W - 1);
    uint16_t c = idx & (MAP_W - 1);
    uint16_t offset = r * 4 + c * 2;
    uint8_t color = object_color[obj];
    VGA_COLOR_SEG[offset] = color;
    VGA_CHAR_SEG[offset] = object_char_l[obj];
    offset += 1;
    VGA_COLOR_SEG[offset] = color;
    VGA_CHAR_SEG[offset] = object_char_r[obj];
}

