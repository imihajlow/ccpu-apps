#include "fat.h"
#include "name.h"
#include "private.h"
#include <libsys/card.h>
#include <string.h>
#include <strings.h>

#define INVALID_BLOCK_ADDR 0xffffffff
extern uint8_t cache_block[CARD_BLOCK_SIZE]; // aligned 256
static uint32_t cache_block_addr = INVALID_BLOCK_ADDR;

static struct FatInfo fat_info;
struct FileDescriptor fat_private_file_desc[MAX_FILE_DESC];

static bool cache(uint32_t block_addr);
static void set_fat_error(uint8_t error);
static uint16_t read_fat_entry(uint16_t index);
static uint16_t alloc_cluster(uint16_t last);
static bool seek_advance(struct FileDescriptor *pfd, uint16_t advance);
static bool seek_begin(struct FileDescriptor *pfd);
static uint16_t fat_action(uint8_t fd, char *data, uint16_t len, bool is_write);
static uint16_t find_parent_cluster(uint16_t head, uint16_t cur);
static bool free_cluster_chain(uint16_t from);
static struct FileDescriptor *get_desc(uint8_t fd);

void fat_init(void) {
    for (uint8_t i = 0; i != MAX_FILE_DESC; ++i) {
        fat_private_file_desc[i].is_free = true;
        fat_private_file_desc[i].is_stale = false;
    }
    cache_block_addr = INVALID_BLOCK_ADDR;
}

bool fat_mount(void) {
    cache_block_addr = INVALID_BLOCK_ADDR;
    for (uint8_t i = 0; i != MAX_FILE_DESC; ++i) {
        if (!fat_private_file_desc[i].is_free) {
            fat_private_file_desc[i].is_stale = true;
        }
    }
    uint8_t r = card_init();
    if (r != CARD_SUCCESS) {
        set_fat_error(r);
        return false;
    }

    if (!cache(0)) {
        return false;
    }

    {
        struct Partition *partition = (struct Partition *)(cache_block + 446);
        if (!cache(partition->rel_sector)) {
            return false;
        }
    }

    struct BPB *bpb = (struct BPB *)cache_block;

    if (bpb->bytes_per_sector != 512) {
        last_error = FAT_ERROR_UNSUPPORTED_SECTOR_SIZE;
        return false;
    }

    uint32_t offset = bpb->n_hidden_sectors;
    offset += bpb->n_reserved_sectors;
    fat_info.fat0_offset = offset; // 5000
    offset += bpb->sectors_per_fat;
    if (bpb->n_fats == 2) {
        fat_info.fat1_offset = offset;
        offset += bpb->sectors_per_fat;
    } else if (bpb->n_fats > 2) {
        last_error = FAT_ERROR_TOO_MANY_FATS;
        return false;
    }
    fat_info.root_dir_block = offset; // 23400
    fat_info.sectors_per_fat = bpb->sectors_per_fat;
    fat_info.sectors_per_cluster = bpb->sectors_per_cluster;
    uint8_t log_sectors_per_cluster = 0;
    uint8_t sectors_per_cluster = bpb->sectors_per_cluster;
    while (sectors_per_cluster > 1) {
        log_sectors_per_cluster += 1;
        sectors_per_cluster >>= 1;
    }
    fat_info.log_sectors_per_cluster = log_sectors_per_cluster;
    fat_info.n_dir_entries = bpb->n_dir_entries;
    offset += (fat_info.n_dir_entries + 15) >> 4; // 27400
    fat_info.data_offset = offset;

    uint32_t data_sectors;
    if (bpb->total_sectors == 0) {
        data_sectors = bpb->large_sectors;
    } else {
        data_sectors = bpb->total_sectors;
    }
    data_sectors -= offset;
    uint32_t n_clusters = data_sectors >> log_sectors_per_cluster;
    if (n_clusters < 4085) {
        // FAT12
        last_error = FAT_ERROR_UNSUPPORTED_FAT_TYPE;
        return false;
    } else if (n_clusters < 65525) {
        // FAT16
        fat_info.n_clusters = n_clusters;
    } else {
        // FAT32 or ExFAT
        last_error = FAT_ERROR_UNSUPPORTED_FAT_TYPE;
        return false;
    }

    return true;
}

uint8_t fat_private_get_free_fd(struct FileDescriptor **ppfd) {
    uint8_t fd;
    struct FileDescriptor *pfd = fat_private_file_desc;
    for (fd = 0; fd != MAX_FILE_DESC && !pfd->is_free; fd += 1) {
        pfd += 1;
    }
    if (fd == MAX_FILE_DESC) {
        last_error = FAT_ERROR_TOO_MANY_OPEN_FILES;
        return FAT_BAD_DESC;
    }
    pfd->is_free = false;
    *ppfd = pfd;
    return fd;
}


uint8_t fat_open_dir(struct FatDirEntry *dir) {
    struct FileDescriptor *pfd;
    uint8_t fd = fat_private_get_free_fd(&pfd);
    if (fd == FAT_BAD_DESC) {
        return FAT_BAD_DESC;
    }
    if (!dir || dir->cluster_lo == 0) {
        // root dir
        fat_private_init_root_dir(pfd);
    } else {
        memcpy(&pfd->dir_entry, dir, sizeof(struct FatDirEntry));
        if (!fat_private_init_fd(pfd)) {
            pfd->is_free = true;
            return FAT_BAD_DESC;
        }
    }
    return fd;
}

bool fat_change_dir(struct FatDirEntry *parent, const char *name, struct FatDirEntry *dst) {
    if (!name) {
        dst->cluster_lo = 0;
        dst->attrs = FAT_FILE_ATTR_DIRECTORY;
        return true;
    }
    struct FileDescriptor fd;

    if (fat_private_find_file(parent, name, &fd, 0)) {
        if (fd.dir_entry.attrs & FAT_FILE_ATTR_DIRECTORY) {
            memcpy(dst, &fd.dir_entry, sizeof(struct FatDirEntry));
            return true;
        } else {
            last_error = FAT_ERROR_NOT_A_DIR;
        }
    }
    return false;
}

static void set_fat_error(uint8_t card_error) {
    if (card_error == CARD_ERROR_NO_CARD) {
        last_error = FAT_ERROR_NO_CARD;
        CARD_POWER_OFF();
    } else if (card_error == CARD_ERROR_NOT_INITIALIZED) {
        last_error = FAT_ERROR_CARD_REINSERTED;
        CARD_POWER_OFF();
    } else if (card_error == CARD_ERROR_OUT_OF_RANGE) {
        last_error = FAT_ERROR_FS_BROKEN;
    } else {
        last_error = FAT_ERROR_CARD;
        CARD_POWER_OFF();
    }
}

static bool cache(uint32_t block_addr) {
    if (cache_block_addr == block_addr) {
        return true;
    }
    cache_block_addr = block_addr;
    uint8_t r = card_read_block(block_addr);
    if (r) {
        cache_block_addr = INVALID_BLOCK_ADDR;
        set_fat_error(r);
        return false;
    }
    return true;
}

static bool flush_cache(void) {
    if (cache_block_addr != INVALID_BLOCK_ADDR) {
        uint8_t r = card_write_block(cache_block_addr);
        if (r) {
            set_fat_error(r);
            return false;
        }
    }
    return true;
}

// Find file in dir by name and fill out dir_entry-related fields in file descriptor
bool fat_private_find_file(struct FatDirEntry *dir, const char *name, struct FileDescriptor *pfd, uint8_t mode) {
    uint8_t r;

    bool is_root = !dir || dir->cluster_lo == 0;

    char fat_name[11];
    to_fat_name(fat_name, name);

    uint32_t block_addr;
    uint16_t cluster;
    if (is_root) {
        block_addr = fat_info.root_dir_block;
    } else {
        cluster = dir->cluster_lo;
    }
    uint8_t sector_in_block = 0;
    struct FatDirEntry *slot = (struct FatDirEntry*)0;
    uint32_t slot_block_addr;
    uint8_t slot_entry_index;
    while (is_root || cluster < FAT_MIN_LAST_CLUSTER) {
        if (!is_root) {
            block_addr = fat_info.data_offset + ((uint32_t)(cluster - 2) << fat_info.log_sectors_per_cluster);
            cluster = read_fat_entry(cluster);
            if (cluster == 0) {
                return false;
            }
        }
        for (uint8_t block_in_cluster = 0; block_in_cluster != fat_info.sectors_per_cluster; ++block_in_cluster) {
            if (!cache(block_addr)) {
                return false;
            }
            struct FatDirEntry *pentry = (struct FatDirEntry *)cache_block;
            for (uint8_t i = 0; i != CARD_BLOCK_SIZE / sizeof(struct FatDirEntry); ++i) {
                bool found = false;
                if (pentry->filename[0] == 0) {
                    // last entry
                    if (mode & O_CREAT) {
                        if (!slot) {
                            slot = pentry;
                            slot_entry_index = i;
                        } else {
                            if (block_addr != slot_block_addr) {
                                block_addr = slot_block_addr;
                                if (!cache(block_addr)) {
                                    return false;
                                }
                            }
                            i = slot_entry_index;
                        }
                        bzero(slot, sizeof(*slot));
                        memcpy(slot->filename, fat_name, 11);
                        if (!flush_cache()) {
                            return false;
                        }
                        pentry = slot;
                        found = true;
                    } else {
                        last_error = FAT_ERROR_NOT_FOUND;
                        return false;
                    }
                } else if (pentry->filename[0] != 0xe5) {
                    if (memcmp(pentry->filename, fat_name, 11) == 0) {
                        if ((mode & (O_EXCL | O_CREAT)) == (O_EXCL | O_CREAT)) {
                            last_error = FAT_ERROR_FILE_EXISTS;
                            return false;
                        }
                        found = true;
                    }
                } else if (!slot) {
                    slot = pentry;
                    slot_block_addr = block_addr;
                    slot_entry_index = i;
                }
                if (found) {
                    memcpy(&pfd->dir_entry, pentry, sizeof(struct FatDirEntry));
                    pfd->dir_entry_index_in_block = (uint16_t)i << 5; // log sizeof(struct FatDirEntry)
                    pfd->dir_entry_block_addr = block_addr;
                    return true;
                }
                pentry += 1;
            }
            block_addr += 1;
        }
    }
    if (mode & O_CREAT) {
        last_error = FAT_ERROR_DIR_FULL;
    } else {
        last_error = FAT_ERROR_NOT_FOUND;
    }
    return false;
}

bool fat_get_next_dir_entry(uint8_t dir_desc, struct FatDirEntry *dst, uint8_t attr_skip_mask) {
    struct FileDescriptor *pfd = get_desc(dir_desc);
    if (!pfd) {
        return false;
    }

    last_error = ERROR_OK;
    while (true) {
        if (!cache(pfd->block_addr)) {
            return false;
        }
        memcpy(dst, cache_block + pfd->index_in_block, sizeof(struct FatDirEntry));

        if (!seek_advance(pfd, sizeof(struct FatDirEntry))) {
            return false;
        }

        if (dst->filename[0] == 0) { // last entry
            return false;
        }
        if (dst->filename[0] == 0xe5) { // empty entry
            continue;
        }
        if (dst->attrs == 0x0f) { // long filename
            continue;
        }
        if (dst->attrs & attr_skip_mask) {
            continue;
        }
        return true;
    }
}

// returns 0 on error
static uint16_t read_fat_entry(uint16_t index) {
    uint8_t entry_offset = index;
    uint8_t sector_index = index >> 8;
    uint32_t disk_offset;
    disk_offset = fat_info.fat0_offset;
    disk_offset += sector_index;
    if (!cache(disk_offset)) {
        return 0;
    } else {
        return ((uint16_t*)cache_block)[entry_offset];
    }
}

static bool write_fat_entry(uint16_t index, uint16_t value) {
    uint8_t entry_offset = index;
    uint8_t sector_index = index >> 8;
    uint32_t disk_offset;
    disk_offset = fat_info.fat0_offset;
    disk_offset += sector_index;
    if (!cache(disk_offset)) {
        return false;
    }
    ((uint16_t*)cache_block)[entry_offset] = value;
    if (!flush_cache()) {
        return false;
    }
    disk_offset = fat_info.fat1_offset;
    disk_offset += sector_index;
    if (!cache(disk_offset)) {
        return false;
    }
    ((uint16_t*)cache_block)[entry_offset] = value;
    return flush_cache();
}

// returns 0xffff on error
static uint16_t alloc_cluster(uint16_t last) {
    uint16_t result = 0;
    uint32_t block_addr = fat_info.fat0_offset;
    for (uint16_t fat_sector = 0; fat_sector != fat_info.sectors_per_fat; ++fat_sector) {
        if (!cache(block_addr)) {
            return 0xffff;
        }
        for (uint16_t i = 0; i != (CARD_BLOCK_SIZE >> 1); ++i) {
            uint16_t entry = ((uint16_t*)cache_block)[i];
            if (entry == 0) {
                result += i;
                // write fat0
                ((uint16_t*)cache_block)[i] = 0xffff;
                if (!flush_cache()) {
                    return 0xffff;
                }
                // write fat1
                cache_block_addr = block_addr + fat_info.fat1_offset - fat_info.fat0_offset;
                if (!flush_cache()) {
                    return 0xffff;
                }

                if (last) {
                    // now write its parent, start with fat1
                    block_addr = fat_info.fat1_offset + (last >> 8);
                    if (!cache(block_addr)) {
                        return 0xffff;
                    }
                    ((uint16_t*)cache_block)[(uint8_t)last] = result;
                    if (!flush_cache()) {
                        return 0xffff;
                    }
                    cache_block_addr = block_addr + fat_info.fat0_offset - fat_info.fat1_offset;
                    if (!flush_cache()) {
                        return 0xffff;
                    }
                }
                return result;
            }
        }
        block_addr += 1;
        result += CARD_BLOCK_SIZE >> 1;
    }
    return 0xffff;
}

uint8_t fat_open_file(struct FatDirEntry *dir, const char *name, uint8_t mode) {
    struct FileDescriptor *pfd;
    uint8_t fd = fat_private_get_free_fd(&pfd);
    if (fd == FAT_BAD_DESC) {
        return FAT_BAD_DESC;
    }

    if (!fat_private_find_file(dir, name, pfd, mode)) {
        pfd->is_free = true;
        return FAT_BAD_DESC;
    }

    if (!fat_private_init_fd(pfd)) {
        pfd->is_free = true;
        return FAT_BAD_DESC;
    }
    return fd;
}

// returns bool
bool fat_private_init_fd(struct FileDescriptor *pfd) {
    pfd->is_root = false;
    if (pfd->dir_entry.size == 0) {
        pfd->last_cluster_len = 0;
    } else {
        uint16_t cluster_mask = ~(0xffff << (fat_info.log_sectors_per_cluster + 9));
        pfd->last_cluster_len = (uint16_t)pfd->dir_entry.size & cluster_mask;
        if (pfd->last_cluster_len == 0) {
            pfd->last_cluster_len = cluster_mask + 1; // full cluster
        }
    }

    pfd->dir_entry_modified = false;
    return seek_begin(pfd);
}

void fat_private_init_root_dir(struct FileDescriptor *pfd) {
    pfd->is_root = true;
    pfd->block_addr = fat_info.root_dir_block;
    pfd->block_in_cluster = 0;
    pfd->index_in_block = 0;
    pfd->abs_offset = 0;
    pfd->dir_entry_modified = false;
}

uint16_t fat_read(uint8_t fd, void *dst, uint16_t len) {
    return fat_action(fd, dst, len, false);
}

static bool seek_advance(struct FileDescriptor *pfd, uint16_t advance) {
    pfd->index_in_block += advance;
    pfd->abs_offset += advance;
    if (pfd->index_in_block == CARD_BLOCK_SIZE) {
        pfd->block_in_cluster += 1;
        pfd->index_in_block = 0;
        if (pfd->block_in_cluster == fat_info.sectors_per_cluster) {
            // next cluster
            if (pfd->is_root) {
                pfd->block_addr += 1;
            } else if (pfd->next_cluster < FAT_MIN_LAST_CLUSTER) {
                pfd->block_addr = fat_info.data_offset;
                pfd->block_addr += (uint32_t)(pfd->next_cluster - 2) << fat_info.log_sectors_per_cluster;
                pfd->cur_cluster = pfd->next_cluster;
                pfd->next_cluster = read_fat_entry(pfd->cur_cluster);
                if (pfd->next_cluster == 0) {
                    return false;
                }
                pfd->block_in_cluster = 0;
            }
        } else {
            // next block in cluster
            pfd->block_addr += 1;
        }
    }
    return true;
}

// returns bool
static bool seek_begin(struct FileDescriptor *pfd) {
    pfd->block_addr = fat_info.data_offset;
    pfd->block_addr += (uint32_t)(pfd->dir_entry.cluster_lo - 2) << fat_info.log_sectors_per_cluster;
    pfd->block_in_cluster = 0;
    pfd->index_in_block = 0;
    pfd->cur_cluster = pfd->dir_entry.cluster_lo;
    if (pfd->cur_cluster != 0) {
        pfd->next_cluster = read_fat_entry(pfd->dir_entry.cluster_lo);
    } else {
        pfd->next_cluster = 0xffff;
    }
    pfd->abs_offset = 0;
    return pfd->next_cluster != 0;
}

uint16_t fat_write(uint8_t fd, const void *src, uint16_t len) {
    return fat_action(fd, src, len, true);
}

// return number of bytes processed
static uint16_t fat_action(uint8_t fd, char *data, uint16_t len, bool is_write) {
    struct FileDescriptor *pfd = get_desc(fd);
    if (!pfd) {
        return 0;
    }
    uint16_t result = 0;
    while (len) {
        uint16_t chunk_len = CARD_BLOCK_SIZE - pfd->index_in_block;
        if (is_write && pfd->cur_cluster == 0) {
            // virgin empty file
            uint16_t new_cluster = alloc_cluster(0);
            if (new_cluster == 0xffff) {
                return result;
            }
            pfd->cur_cluster = new_cluster;
            pfd->next_cluster = 0xffff;
            pfd->dir_entry_modified = true;
            pfd->dir_entry.cluster_lo = new_cluster;
            pfd->block_addr = fat_info.data_offset;
            pfd->block_addr += (uint32_t)(new_cluster - 2) << fat_info.log_sectors_per_cluster;
        }
        if (pfd->next_cluster >= FAT_MIN_LAST_CLUSTER) {
            // last cluster
            if (is_write && pfd->block_in_cluster == fat_info.sectors_per_cluster) {
                // last cluster is full, need a new one
                uint16_t new_cluster = alloc_cluster(pfd->cur_cluster);
                if (new_cluster == 0xffff) {
                    return result;
                }
                pfd->cur_cluster = new_cluster;
                pfd->next_cluster = 0xffff;
                pfd->block_addr = fat_info.data_offset;
                pfd->block_addr += (uint32_t)(new_cluster - 2) << fat_info.log_sectors_per_cluster;
                pfd->last_cluster_len = 0;
                pfd->block_in_cluster = 0;
                pfd->last_cluster_len = 0;
            }
            uint16_t remainder = pfd->last_cluster_len - ((uint16_t)pfd->block_in_cluster << 9);
            if (remainder <= CARD_BLOCK_SIZE) {
                // last block
                chunk_len = remainder - pfd->index_in_block;
                if (is_write) {
                    if (len > chunk_len) {
                        // expanding the file past the current size
                        chunk_len = len;
                        if (chunk_len + pfd->index_in_block > CARD_BLOCK_SIZE) {
                            // need to expand over current block
                            chunk_len = CARD_BLOCK_SIZE - pfd->index_in_block;
                        }
                        uint16_t delta_size = pfd->index_in_block + chunk_len - remainder;
                        pfd->last_cluster_len += delta_size;
                        pfd->dir_entry.size += delta_size;
                        pfd->dir_entry_modified = true;
                    }
                }
            }
        }
        if (chunk_len > len) {
            chunk_len = len;
        }
        if (chunk_len == 0) {
            if (pfd->abs_offset == pfd->dir_entry.size) {
                last_error = FAT_EOF;
            }
            return result;
        }
        if (!cache(pfd->block_addr)) {
            return result;
        }
        if (is_write) {
            memcpy(cache_block + pfd->index_in_block, data, chunk_len);
            if (!flush_cache()) {
                return result;
            }
        } else {
            memcpy(data, cache_block + pfd->index_in_block, chunk_len);
        }
        if (!seek_advance(pfd, chunk_len)) {
            return result;
        }
        len -= chunk_len;
        data += chunk_len;
        result += chunk_len;
    }
    return result;
}


bool fat_close(uint8_t fd) {
    bool result = true;
    struct FileDescriptor *pfd = (struct FileDescriptor*)fat_private_file_desc + fd;
    if (pfd->dir_entry_modified) {
        if (pfd->is_stale) {
            last_error = FAT_ERROR_STALE_DESCRIPTOR;
            result = false;
        } else {
            cache(pfd->dir_entry_block_addr);
            memcpy(cache_block + pfd->dir_entry_index_in_block, &pfd->dir_entry, sizeof(struct FatDirEntry));
            flush_cache();
        }
    }
    pfd->is_free = true;
    pfd->is_stale = false;
    return result;
}

static uint16_t find_parent_cluster(uint16_t head, uint16_t cur) {
    while (head < FAT_MIN_LAST_CLUSTER) {
        uint16_t next = read_fat_entry(head);
        if (!next) {
            return 0;
        } else if (next == cur) {
            return head;
        }
        head = next;
    }
    last_error = FAT_ERROR_FS_BROKEN;
    return 0;
}

static bool free_cluster_chain(uint16_t from) {
    while (from < FAT_MIN_LAST_CLUSTER) {
        uint16_t next_cluster = read_fat_entry(from);
        if (next_cluster == 0) {
            return false;
        }
        if (!write_fat_entry(from, 0)) {
            return false;
        }
        from = next_cluster;
    }
    return true;
}

bool fat_truncate(uint8_t fd) {
    struct FileDescriptor *pfd = get_desc(fd);
    if (!pfd) {
        return false;
    }
    if (pfd->abs_offset == pfd->dir_entry.size) {
        return true;
    }
    pfd->dir_entry.size = pfd->abs_offset;
    pfd->dir_entry_modified = true;
    // if (pfd->block_in_cluster == 0 && pfd->index_in_block == 0) {
    if ((pfd->block_in_cluster | pfd->index_in_block) == 0) {
        // cur_cluster is a redundant free cluster, let's find its parent
        if (pfd->cur_cluster == pfd->dir_entry.cluster_lo) {
            // totally empty file
            pfd->dir_entry.cluster_lo = 0;
            bool r = free_cluster_chain(pfd->cur_cluster);
            pfd->cur_cluster = 0;
            pfd->next_cluster = 0xffff;
            return r;
        }
        uint16_t parent = find_parent_cluster(pfd->dir_entry.cluster_lo, pfd->cur_cluster);
        if (parent == 0) {
            return false;
        }
        pfd->next_cluster = pfd->cur_cluster;
        pfd->cur_cluster = parent;
        pfd->block_in_cluster = fat_info.sectors_per_cluster;
    }
    if (pfd->next_cluster < FAT_MIN_LAST_CLUSTER) {
        // there is a chain of clusters to free
        uint16_t cur_cluster = pfd->cur_cluster;
        uint16_t next_cluster = pfd->next_cluster;
        if (!write_fat_entry(cur_cluster, 0xffff)) {
            return false;
        }
        bool r = free_cluster_chain(next_cluster);
        next_cluster += 1;
        if (!r) {
            return false;
        }
        pfd->next_cluster = 0xffff;
    }
    uint16_t cluster_mask = ~(0xffff << (fat_info.log_sectors_per_cluster + 9));
    pfd->last_cluster_len = (uint16_t)pfd->abs_offset & cluster_mask;
    if (pfd->last_cluster_len == 0) {
        if (pfd->block_in_cluster != 0) {
            pfd->last_cluster_len = cluster_mask + 1;
        }
    }
    return true;
}

bool fat_seek_end(uint8_t fd) {
    struct FileDescriptor *pfd = get_desc(fd);
    if (!pfd) {
        return false;
    }

    while (pfd->next_cluster < FAT_MIN_LAST_CLUSTER) {
        pfd->cur_cluster = pfd->next_cluster;
        uint16_t next_cluster = read_fat_entry(pfd->next_cluster);
        pfd->next_cluster = next_cluster;
        if (next_cluster) {
            return false;
        }
    }
    pfd->abs_offset = pfd->dir_entry.size;
    pfd->block_in_cluster = pfd->last_cluster_len >> 9;
    pfd->index_in_block = pfd->last_cluster_len & (CARD_BLOCK_SIZE - 1);
    pfd->block_addr = fat_info.data_offset
        + ((uint32_t)(pfd->cur_cluster - 2) << fat_info.log_sectors_per_cluster)
        + pfd->block_in_cluster;
    return true;
}

uint32_t fat_get_size(uint8_t fd) {
    struct FileDescriptor *pfd = get_desc(fd);
    if (pfd) {
        return pfd->dir_entry.size;
    }
    return 0xFFFFFFFF;
}

struct FatDirEntry *fat_get_dir_entry(uint8_t fd) {
    struct FileDescriptor *pfd = get_desc(fd);
    if (pfd) {
        return &pfd->dir_entry;
    }
    return (struct FatDirEntry *)0;
}

static struct FileDescriptor *get_desc(uint8_t fd) {
    struct FileDescriptor *pfd = (struct FileDescriptor *)fat_private_file_desc + fd;
    if (pfd->is_free) {
        last_error = FAT_ERROR_BAD_DESCRIPTOR;
    } else if (pfd->is_stale) {
        last_error = FAT_ERROR_STALE_DESCRIPTOR;
    } else {
        return pfd;
    }
    return (struct FileDescriptor *)0;
}
