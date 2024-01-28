#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <libsys/vga.h>
#include <libsys/ps2keyboard.h>
#include <libsys/ps2.h>
#include <libsys/fat/fat.h>

#include "engine.h"
#include "game.h"

void main(void) {
    while (true) {
        game_show_menu();
        const char *fname = game_menu_loop();
        engine_init();
        game_prepare_level();
        if (engine_load(fname)) {
            engine_render();
        } else {
            strcpy(VGA_CHAR_SEG, "cannot load");
            fat_print_last_error();
        }
        game_loop();
    }
}

int getchar(void) { return 0; }
int putchar(int c) { return 0; }
