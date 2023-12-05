#pragma once
#include <stdint.h>
#include <stdbool.h>

struct ipcfg_t {
    union {
        uint16_t ipaddr[2];
        uint8_t ipaddr8[4];
    };
    union {
        uint16_t netmask[2];
        uint8_t netmask8[4];
    };
    union {
        uint16_t default_router[2];
        uint8_t default_router8[4];
    };
    union {
        uint16_t dnsaddr[2];
        uint8_t dnsaddr8[4];
    };
};

bool ipcfg_load(struct ipcfg_t *cfg);
