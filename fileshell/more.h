#pragma once
#include <stdint.h>
#include <stdbool.h>

void more_init(uint8_t color);

// Returns true if everything was printed, false if user interrupted the output (by pressing escape on more).
bool more_print(const char *s, uint16_t max_len);

// Returns PS2 key pressed on MORE or 0 if text fits into screen.
uint8_t more_putchar(char c);
