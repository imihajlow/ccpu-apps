#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <libsys/vga.h>
#include <libsys/ps2keyboard.h>
#include <libsys/fat/fat.h>

#include "engine.h"

void main(void) {
    vga_clear(COLOR(COLOR_GRAY, COLOR_BLACK));
    engine_init();
    if (engine_load("/DATA/LVL1.TXT")) {
        engine_render();
    } else {
        strcpy(VGA_CHAR_SEG, "cannot load");
        fat_print_last_error();
    }
    uint8_t move_flags = 0;
    while (1) {
        for (uint8_t i = 0; i != 6; ++i) {
            uint8_t key = ps2_get_key_event();
            switch (key) {
            case PS2_KEY_UP:
                move_flags |= MOVE_UP;
                break;
            case PS2_KEY_UP | PS2_KEY_RELEASE:
                move_flags &= ~MOVE_UP;
                break;
            case PS2_KEY_DOWN:
                move_flags |= MOVE_DOWN;
                break;
            case PS2_KEY_DOWN | PS2_KEY_RELEASE:
                move_flags &= ~MOVE_DOWN;
                break;
            case PS2_KEY_LEFT:
                move_flags |= MOVE_LEFT;
                break;
            case PS2_KEY_LEFT | PS2_KEY_RELEASE:
                move_flags &= ~MOVE_LEFT;
                break;
            case PS2_KEY_RIGHT:
                move_flags |= MOVE_RIGHT;
                break;
            case PS2_KEY_RIGHT | PS2_KEY_RELEASE:
                move_flags &= ~MOVE_RIGHT;
                break;
            }
        }
        engine_step(move_flags);
    }
}

int getchar(void) { return 0; }
int putchar(int c) { return 0; }
