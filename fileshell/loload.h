#pragma once
#include <stdint.h>

extern uint8_t loload_buf[256];

/**
    Copy 256 bytes from loload_buf to (addr_hi << 8) in lo RAM.
 */
void copy_to_loram(uint8_t addr_hi);
