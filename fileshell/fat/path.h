#pragma once

#include "error.h"
#include "types.h"

/**
 * Allocates a file descriptor. Walks the path. Populates file descriptor with the information about the file/dir.
 *
 * @param  path file/dir path
 * @param  mode open mode
 * @return file descriptor
 */
uint8_t fat_find_path(const char *path, uint8_t mode);

uint8_t fat_open_path(const char *path, uint8_t mode);
