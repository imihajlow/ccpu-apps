#pragma once
#include "types.h"
#include <assert.h>

#define FAT_MIN_LAST_CLUSTER 0xFFF8
#define MAX_FILE_DESC 4

struct Partition {
    uint8_t boot; // 0x00
    uint8_t start_h; // 0x01
    uint8_t start_s; // 0x08
    uint8_t start_c; // 0x00
    uint8_t sys_id; // 0x06
    uint8_t end_h;  // 0x07
    uint8_t end_s; // 0x60
    uint8_t end_c; // 0xe4
    uint32_t rel_sector; // 0x00000027
    uint32_t n_sectors; // 0x0001e4d9
} __attribute__((packed));

static_assert(sizeof(struct Partition) == 16, "sizeof Partition must be 16");

struct BPB {
        uint8_t jump[3];
        uint8_t oem_id[8];
        uint16_t bytes_per_sector; // 0x0200
    uint8_t sectors_per_cluster; // 0x04
    uint16_t n_reserved_sectors; // 0x0001
    uint8_t n_fats; // 0x02
    uint16_t n_dir_entries; // 0x0200
    uint16_t total_sectors; // 0x0000
        uint8_t media_descriptor_type; // 0xf8
    uint16_t sectors_per_fat; // 0x0079
        uint16_t sectors_per_track; // 0x0020
        uint16_t n_heads; // 0x0010
    uint32_t n_hidden_sectors; // 0x00000027
    uint32_t large_sectors; // 0x0001e4d9
} __attribute__((packed));

struct FatInfo {
    uint32_t fat0_offset;
    uint32_t fat1_offset;
    uint16_t sectors_per_fat;
    uint32_t root_dir_block;
    uint8_t log_sectors_per_cluster;
    uint8_t sectors_per_cluster;
    uint16_t n_dir_entries;
    uint32_t data_offset;
    uint16_t n_clusters;
};

struct FileDescriptor {
    bool is_free;
    bool is_stale; // an old open file descriptor becomes stale when the card is mounted
    uint16_t last_cluster_len; // number of bytes in the last cluster in the chain
    uint32_t block_addr; // current block
    uint8_t block_in_cluster; // block index in current cluster
    uint16_t index_in_block; // read/write pointer in current block
    uint16_t next_cluster; // value from the FAT
    uint16_t cur_cluster; // index in the FAT
    struct FatDirEntry dir_entry;
    uint32_t dir_entry_block_addr; // block index on disc
    uint16_t dir_entry_index_in_block; // offset inside the block
    bool is_root; // for directory descriptors: if this is a root directory (a special case)
    bool dir_entry_modified; // need to write out the dir entry on file close
    uint32_t abs_offset; // absolute read/write pointer
};

extern struct FileDescriptor fat_private_file_desc[MAX_FILE_DESC];


// Find file in dir by name and fill out dir_entry-related fields in file descriptor
bool fat_private_find_file(struct FatDirEntry *dir, const char *name, struct FileDescriptor *pfd, uint8_t mode);

// Finds a free slot, clears free flag
// returns fd or FAT_BAD_DESC on error
uint8_t fat_private_get_free_fd(struct FileDescriptor **ppfd);

bool fat_private_init_fd(struct FileDescriptor *pfd);

void fat_private_init_root_dir(struct FileDescriptor *pfd);

