#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#define O_CREAT (1 << 0)
#define O_EXCL (1 << 1)

#define FAT_FILE_ATTR_READ_ONLY 1
#define FAT_FILE_ATTR_HIDDEN 2
#define FAT_FILE_ATTR_SYSTEM 4
#define FAT_FILE_ATTR_VOLUME_ID 8
#define FAT_FILE_ATTR_DIRECTORY 16
#define FAT_FILE_ATTR_ARCHIVE 32
#define FAT_FILE_ATTR_LONG_FILENAME 0x0F

struct FatDirEntry {
    char filename[11];
    uint8_t attrs;
    uint8_t _reserved;
    uint8_t create_dsec;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t access_date;
    uint16_t cluster_hi; // 0x0000
    uint16_t modify_time;
    uint16_t modify_date;
    uint16_t cluster_lo; // 0x0004
    uint32_t size;
} __attribute__((packed));

static_assert(sizeof(struct FatDirEntry) == 32, "sizeof FatDirEntry must be 32");
