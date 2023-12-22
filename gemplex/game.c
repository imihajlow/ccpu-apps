#include "game.h"
#include "engine.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <libsys/vga.h>
#include <libsys/fat/fat.h>
#include <libsys/fat/name.h>
#include <libsys/ps2keyboard.h>
#include <libsys/syscall.h>

#define GAME_DATA_DIR "/GEMPLEX"

#define MENU_FIRST_ROW 3
#define MENU_COLUMN_WIDTH 15
#define MENU_COLUMNS 4
#define MENU_FIRST_COLUMN ((VGA_COLS - MENU_COLUMN_WIDTH * MENU_COLUMNS) / 2)
#define CAPTION_ROW 3
#define GEMS_ROW 5
#define FACE_ROW 8
#define MAX_LEVELS 40
#define MENU_MAIN_COLOR COLOR(COLOR_WHITE, COLOR_GRAY)
#define MENU_CURSOR_COLOR COLOR(COLOR_WHITE, COLOR_BLUE)

static struct {
    bool loaded;
    uint8_t n_levels;
    char filenames[MAX_LEVELS][16];
    uint8_t cursor_idx;
} game_state;

static char fat_buf[64] __attribute__((section("bss_hi")));

static bool load_dir(void);
static void show_table(void);
static void set_cursor_color(uint8_t color);
static const char *get_selected_level_filename(void);
static void draw_face(const char *data, uint8_t color);

void game_show_menu(void) {
    game_state.loaded = false;
    vga_clear(MENU_MAIN_COLOR);
    strcpy(VGA_CHAR_SEG + VGA_OFFSET(37, 1), "GEMPLEX");
    if (!load_dir()) {
        return;
    }

    show_table();
    game_state.cursor_idx = 0;
    set_cursor_color(MENU_CURSOR_COLOR);
}

const char *game_menu_loop(void) {
    if (game_state.loaded) {
        while(1) {
            uint8_t key = ps2_get_key_event();
            int8_t new_cursor_idx = game_state.cursor_idx;
            switch (key) {
            case PS2_KEY_UP:
                new_cursor_idx -= MENU_COLUMNS;
                break;
            case PS2_KEY_DOWN:
                new_cursor_idx += MENU_COLUMNS;
                break;
            case PS2_KEY_LEFT:
                new_cursor_idx -= 1;
                break;
            case PS2_KEY_RIGHT:
                new_cursor_idx += 1;
                break;
            case PS2_KEY_ENTER:
                return get_selected_level_filename();
            case PS2_KEY_ESCAPE:
                reboot();
            }
            if (new_cursor_idx != game_state.cursor_idx && new_cursor_idx >= 0 && new_cursor_idx < game_state.n_levels) {
                set_cursor_color(MENU_MAIN_COLOR);
                game_state.cursor_idx = new_cursor_idx;
                set_cursor_color(MENU_CURSOR_COLOR);
            }
        }
    } else {
        while(1) {
            uint8_t key = ps2_get_key_event();
            if (key == PS2_KEY_ESCAPE) {
                reboot();
            }
        }
    }
}

void game_prepare_level(void) {
    vga_clear(COLOR(COLOR_RED, COLOR_GRAY));
    strcpy(VGA_CHAR_SEG + VGA_OFFSET(64 + 4, CAPTION_ROW), "GEMPLEX");
    VGA_COLOR_SEG[VGA_OFFSET(64 + 3, GEMS_ROW)] = COLOR(COLOR_LIGHT_CYAN, COLOR_GRAY);
    VGA_COLOR_SEG[VGA_OFFSET(64 + 4, GEMS_ROW)] = COLOR(COLOR_LIGHT_CYAN, COLOR_GRAY);
    memset(VGA_COLOR_SEG + VGA_OFFSET(64 + 6, GEMS_ROW), COLOR(COLOR_WHITE, COLOR_BLACK), 5);
    VGA_CHAR_SEG[VGA_OFFSET(64 + 3, GEMS_ROW)] = 158;
    VGA_CHAR_SEG[VGA_OFFSET(64 + 4, GEMS_ROW)] = 159;

}

void game_update_gems_left(uint16_t left) {
    char buf[6];
    char *end = __uitoa(left, buf, 10);
    size_t len = end - (char*)buf;
    VGA_CHAR_SEG[VGA_OFFSET(64 + 7, GEMS_ROW)] = '0';
    VGA_CHAR_SEG[VGA_OFFSET(64 + 8, GEMS_ROW)] = '0';
    VGA_CHAR_SEG[VGA_OFFSET(64 + 9, GEMS_ROW)] = '0';
    strcpy(VGA_CHAR_SEG + VGA_OFFSET(64 + 10 - len, GEMS_ROW), buf);
}

void game_loop(void) {
    uint8_t move_flags = 0;
    while (true) {
        for (uint8_t i = 0; i != 50; ++i) {
            uint8_t key = ps2_get_key_event();
            switch (key) {
            case PS2_KEY_UP:
                move_flags |= MOVE_UP;
                goto step;
            case PS2_KEY_UP | PS2_KEY_RELEASE:
                move_flags &= ~MOVE_UP;
                goto step;
            case PS2_KEY_DOWN:
                move_flags |= MOVE_DOWN;
                goto step;
            case PS2_KEY_DOWN | PS2_KEY_RELEASE:
                move_flags &= ~MOVE_DOWN;
                goto step;
            case PS2_KEY_LEFT:
                move_flags |= MOVE_LEFT;
                goto step;
            case PS2_KEY_LEFT | PS2_KEY_RELEASE:
                move_flags &= ~MOVE_LEFT;
                goto step;
            case PS2_KEY_RIGHT:
                move_flags |= MOVE_RIGHT;
                goto step;
            case PS2_KEY_RIGHT | PS2_KEY_RELEASE:
                move_flags &= ~MOVE_RIGHT;
                goto step;
            case PS2_KEY_ESCAPE:
                return;
            }
        }
step:
        engine_step(move_flags, ps2_modifiers_mask & PS2_MASK_SHIFT);
    }
}

#define TOP 223
#define BOT 220
#define FUL 219
#define NUL 0
/*
.....######.....
...##########...
..##........##..
.##..........##.
.##..........##.
##..##....##..##
##..##....##..##
##............##
##............##
##..#......#..##
##..##....##..##
.##..######..##.
.##...####...##.
..##........##..
...##########...
.....######.....
*/
static const char happy_face[] = {
    NUL,NUL,NUL,BOT,BOT,FUL,FUL,FUL,FUL,FUL,FUL,BOT,BOT,NUL,NUL,NUL,
    NUL,BOT,FUL,TOP,NUL,NUL,NUL,NUL,NUL,NUL,NUL,NUL,TOP,FUL,BOT,NUL,
    BOT,FUL,TOP,NUL,BOT,BOT,NUL,NUL,NUL,NUL,BOT,BOT,NUL,TOP,FUL,BOT,
    FUL,FUL,NUL,NUL,TOP,TOP,NUL,NUL,NUL,NUL,TOP,TOP,NUL,NUL,FUL,FUL,
    FUL,FUL,NUL,NUL,BOT,NUL,NUL,NUL,NUL,NUL,NUL,BOT,NUL,NUL,FUL,FUL,
    TOP,FUL,BOT,NUL,TOP,FUL,BOT,BOT,BOT,BOT,FUL,TOP,NUL,BOT,FUL,TOP,
    NUL,TOP,FUL,BOT,NUL,NUL,TOP,TOP,TOP,TOP,NUL,NUL,BOT,FUL,TOP,NUL,
    NUL,NUL,NUL,TOP,TOP,FUL,FUL,FUL,FUL,FUL,FUL,TOP,TOP,NUL,NUL,NUL,
};
void game_win(void) {
    draw_face(happy_face, COLOR(COLOR_GREEN, COLOR_GRAY));
}

/*
.....######.....
...##########...
..##........##..
.##..........##.
.##..........##.
##..##....##..##
##..##....##..##
##............##
##............##
##....####....##
##...######...##
.##..#....#..##.
.##..........##.
..##........##..
...##########...
.....######.....
*/
static const char sad_face[] = {
    NUL,NUL,NUL,BOT,BOT,FUL,FUL,FUL,FUL,FUL,FUL,BOT,BOT,NUL,NUL,NUL,
    NUL,BOT,FUL,TOP,NUL,NUL,NUL,NUL,NUL,NUL,NUL,NUL,TOP,FUL,BOT,NUL,
    BOT,FUL,TOP,NUL,BOT,BOT,NUL,NUL,NUL,NUL,BOT,BOT,NUL,TOP,FUL,BOT,
    FUL,FUL,NUL,NUL,TOP,TOP,NUL,NUL,NUL,NUL,TOP,TOP,NUL,NUL,FUL,FUL,
    FUL,FUL,NUL,NUL,NUL,NUL,BOT,BOT,BOT,BOT,NUL,NUL,NUL,NUL,FUL,FUL,
    TOP,FUL,BOT,NUL,NUL,FUL,TOP,TOP,TOP,TOP,FUL,NUL,NUL,BOT,FUL,TOP,
    NUL,TOP,FUL,BOT,NUL,NUL,NUL,NUL,NUL,NUL,NUL,NUL,BOT,FUL,TOP,NUL,
    NUL,NUL,NUL,TOP,TOP,FUL,FUL,FUL,FUL,FUL,FUL,TOP,TOP,NUL,NUL,NUL,
};
void game_lose(void) {
    draw_face(sad_face, COLOR(COLOR_RED, COLOR_GRAY));
}

static void draw_face(const char *data, uint8_t color) {
    char *dst_char = VGA_CHAR_SEG + VGA_OFFSET(64, FACE_ROW);
    char *dst_color = VGA_COLOR_SEG + VGA_OFFSET(64, FACE_ROW);
    for (uint8_t r = 0; r != 8; ++r) {
        memcpy(dst_char, data, 16);
        memset(dst_color, color, 16);
        dst_char += VGA_OFFSET(0,1);
        dst_color += VGA_OFFSET(0,1);
        data += 16;
    }
}

static bool load_dir(void) {
    strcpy(fat_buf, GAME_DATA_DIR);
    uint8_t fd = fat_open_path(fat_buf, 0);
    if (fd == FAT_BAD_DESC) {
        game_state.loaded = false;
        strcpy(VGA_CHAR_SEG + VGA_OFFSET(30, 15), "Cannot open " GAME_DATA_DIR);
        return false;
    }

    game_state.n_levels = 0;
    while (true) {
        struct FatDirEntry *de = (struct FatDirEntry *)fat_buf;
        bool r = fat_get_next_dir_entry(fd, de, FAT_FILE_ATTR_DIRECTORY);
        if (!r) {
            if (fat_get_last_error() == FAT_ERROR_OK) {
                break;
            } else {
                strcpy(VGA_CHAR_SEG + VGA_OFFSET(30, 15), "Cannot read directory");
                fat_close(fd);
                return false;
            }
        }
        memcpy(game_state.filenames[game_state.n_levels], de->filename, 11);
        game_state.n_levels += 1;
    }

    fat_close(fd);
    game_state.loaded = true;
    return true;
}

static void show_table(void) {
    if (game_state.n_levels == 0) {
        strcpy(VGA_CHAR_SEG + VGA_OFFSET(36, 15), "NO FILES");
        return;
    }
    uint8_t i = 0;
    for (uint8_t r = MENU_FIRST_ROW; ; r += 2) {
        for (uint8_t c = MENU_FIRST_COLUMN; c != MENU_FIRST_COLUMN + MENU_COLUMNS * MENU_COLUMN_WIDTH; c += MENU_COLUMN_WIDTH) {
            memcpy(VGA_CHAR_SEG + VGA_OFFSET(c + 2, r), game_state.filenames[i], 11);
            ++i;
            if (i == game_state.n_levels) {
                return;
            }
        }
    }
}

static void set_cursor_color(uint8_t color) {
    uint8_t cursor_row = game_state.cursor_idx / MENU_COLUMNS * 2 + MENU_FIRST_ROW;
    uint8_t cursor_col = game_state.cursor_idx % MENU_COLUMNS * MENU_COLUMN_WIDTH + MENU_FIRST_COLUMN;
    memset(VGA_COLOR_SEG + VGA_OFFSET(cursor_col, cursor_row), color, MENU_COLUMN_WIDTH);
}

static const char *get_selected_level_filename(void) {
    char fname[12];
    from_fat_name(fname, game_state.filenames[game_state.cursor_idx]);
    strcpy(fat_buf, GAME_DATA_DIR);
    strcat(fat_buf, "/");
    strcat(fat_buf, fname);
    return (const char *)fat_buf;
}
