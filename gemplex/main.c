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
    while (1) {
        uint8_t key = ps2_get_key_event();
        bool change = true;
        switch (key) {
        case PS2_KEY_UP: engine_up(); break;
        case PS2_KEY_DOWN: engine_down(); break;
        case PS2_KEY_LEFT: engine_left(); break;
        case PS2_KEY_RIGHT: engine_right(); break;
        default: change = false; break;
        }
        engine_collect_changed();
        // engine_render();
        engine_step();
    }
}

int getchar(void) { return 0; }
int putchar(int c) { return 0; }
