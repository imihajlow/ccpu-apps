#include "ipcfg.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <libsys/fat/fat.h>

/*
    IP.CFG consists of 4 lines with IP addresses in decimal form.
*/

#define MAX_IPCFG_LEN (4 * 4 * 4)

static char hi_buf[MAX_IPCFG_LEN] __attribute__((section("bss_hi")));

static const char *parse_ip(const char *src, uint8_t (*dst)[4]);
static uint8_t parse_byte(const char *src, const char **endptr);

bool ipcfg_load(struct ipcfg_t *cfg) {
    strcpy(hi_buf, "/IP.CFG");
    uint8_t fd = fat_open_path(hi_buf, 0);
    if (fd == FAT_BAD_DESC) {
        return false;
    }

    size_t len = fat_read(fd, hi_buf, MAX_IPCFG_LEN);
    fat_close(fd);
    if (len < 4 * 8) {
        return false;
    }

    const char *p = hi_buf;
    p = parse_ip(p, &cfg->ipaddr8);
    if (!p) {
        return false;
    }
    p = parse_ip(p, &cfg->netmask8);
    if (!p) {
        return false;
    }
    p = parse_ip(p, &cfg->default_router8);
    if (!p) {
        return false;
    }
    p = parse_ip(p, &cfg->dnsaddr8);
    if (!p) {
        return false;
    }
}


static const char *parse_ip(const char *src, uint8_t (*dst)[4]) {
    (*dst)[0] = parse_byte(src, &src);
    if (!src || *src != '.') {
        return 0;
    }
    ++src;
    (*dst)[1] = parse_byte(src, &src);
    if (!src || *src != '.') {
        return 0;
    }
    ++src;
    (*dst)[2] = parse_byte(src, &src);
    if (!src || *src != '.') {
        return 0;
    }
    ++src;
    (*dst)[3] = parse_byte(src, &src);
    if (!src || *src != '\n') {
        return 0;
    }
    ++src;
    return src;
}

static uint8_t parse_byte(const char *src, const char **endptr) {
    uint8_t r = 0;
    while (true) {
        char c = *src;
        if (!isdigit(c)) {
            break;
        }
        r = r * 10 + (c - '0');
        ++src;
    }
    *endptr = src;
    return r;
}
