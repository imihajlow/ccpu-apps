#pragma once
#include <stdint.h>

void game_show_menu(void);
const char *game_menu_loop(void);
void game_loop(void);
void game_prepare_level(void);
void game_update_gems_left(uint16_t left);
void game_win(void);
void game_lose(void);
