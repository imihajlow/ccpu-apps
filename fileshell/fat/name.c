#include "name.h"
#include <string.h>
#include <stdint.h>

void to_fat_name(char *dst, const char *src) {
    memset(dst, ' ', 11); // name + extension, all spaces
    if (*src == '.') {
        // just copy if it starts with a dot
        for (uint8_t i = 0; i != 11; ++i) {
            char c = *src;
            if (!c) {
                return;
            }
            *dst = c;
            ++src;
            ++dst;
        }
        return;
    }
    for (uint8_t i = 0; i != 8; ++i) {
        char c = *src;
        if (!c || c == '.') {
            break;
        }
        dst[i] = c;
        src += 1;
    }
    char c = *src;
    if (c == '.') {
        src += 1;
        c = *src;
    } else if (c) {
        while (c && c != '.') {
            src += 1;
            c = *src;
        }
    }
    for (uint8_t i = 8; c && i != 11; i += 1) {
        dst[i] = c;
        src += 1;
        c = *src;
    }
}

void from_fat_name(char *dst, const char *src) {
    for (uint8_t i = 0; i != 8; ++i) {
        char c = src[i];
        if (c == ' ') {
            break;
        }
        *dst = c;
        dst += 1;
    }
    if (src[8] != ' ') {
        *dst = '.';
        dst += 1;
        for (uint8_t i = 8; i != 11; ++i) {
            char c = src[i];
            if (c == ' ') {
                break;
            }
            *dst = c;
            dst += 1;
        }
    }
    *dst = 0;
}
