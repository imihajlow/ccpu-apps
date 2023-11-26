#pragma once

#include "last_error.h"

#include "error.h"
#include "name.h"
#include "types.h"

/**
 Initializes data structures. Does not fail.
 */
void fat_init(void);

/**
 Initializes the card and tries to read FAT headers.

 Returns bool, sets last_error on failure.
 */
bool fat_mount(void);

/**
 Open a directory
 @param dir - dir entry or 0 for root dir.
 Returns dir descriptor or FAT_BAD_DESC on error, sets last_error on failure.
 */
uint8_t fat_open_dir(struct FatDirEntry *dir);

/**
 Returns true on success, false on fail (or when no entries are left), sets last_error on failure.
 */
bool fat_get_next_dir_entry(uint8_t dir_desc, struct FatDirEntry *dst, uint8_t attr_skip_mask);


/**
 Returns file descriptor or FAT_BAD_DESC on error, sets last_error on failure.
 */
uint8_t fat_open_file(struct FatDirEntry *dir, const char *name, uint8_t mode);

/**
 Returns number of bytes actually read, sets last_error on failure.
*/
uint16_t fat_read(uint8_t fd, void *dst, uint16_t len);

/**
 Returns number of bytes actually written, sets last_error on failure.
*/
uint16_t fat_write(uint8_t fd, const void *src, uint16_t len);

/**
 Set file size to the current read/write pointer.
 Returns bool, sets last_error on failure.
 */
bool fat_truncate(uint8_t fd);

/**
 Set file size to the current read/write pointer.
 Returns bool, sets last_error on failure.
 */
bool fat_seek_end(uint8_t fd);

/**
 Returns bool, set last_error on failure. Fails on a stale descriptor when needs to write out the dir entry.
 */
bool fat_close(uint8_t fd);

/**
 Returns 0xffffffff and sets last_error on error.
 */
uint32_t fat_get_size(uint8_t fd);

/**
 Returns the dir entry associated with the file or 0 on error.
 */
struct FatDirEntry *fat_get_dir_entry(uint8_t fd);

/**
 * Look into directory entry for name inside parent and fill dst if it's a dir.
 * @return        true on success, false on fail, sets last_error on failure.
 */
bool fat_change_dir(struct FatDirEntry *parent, const char *name, struct FatDirEntry *dst);
