#include "game.h"
#include <string.h>
#include <stdlib.h>
#include <libsys/vga.h>

#define CAPTION_ROW 3
#define GEMS_ROW 5

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

/*
################
####GEMPLEX#####
################
###**# 123 #####
################
*/
