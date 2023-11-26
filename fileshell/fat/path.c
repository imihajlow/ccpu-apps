#include "path.h"
#include "private.h"
#include <string.h>
#include <stdbool.h>
#include "last_error.h"

static const char *copy_first_entry(char *dst, const char *path) {
    while (true) {
        char c = *path;
        if (c == 0 || c == '/') {
            *dst = 0;
            return path;
        }
        path += 1;
        *dst = c;
        dst += 1;
    }
}

uint8_t fat_find_path(const char *path, uint8_t mode) {
    if (*path != '/') {
        last_error = FAT_ERROR_RELATIVE_PATH;
        return FAT_BAD_DESC;
    }
    struct FileDescriptor *pfd;
    uint8_t fd = fat_private_get_free_fd(&pfd);
    if (fd == FAT_BAD_DESC) {
        return FAT_BAD_DESC;
    }

    pfd->dir_entry.cluster_lo = 0; // start with root dir
    pfd->dir_entry.attrs = FAT_FILE_ATTR_DIRECTORY;
    uint8_t filename[8 + 3 + 1 + 1];
    while (*path != 0) {
        if (*path == '/') {
            path += 1;
        } else {
            path = copy_first_entry(filename, path);
            uint8_t cur_mode;
            if (*path == 0) {
                cur_mode = mode;
            } else {
                cur_mode = 0;
            }
            if (!fat_private_find_file(&pfd->dir_entry, filename, pfd, cur_mode)) {
                pfd->is_free = true;
                return FAT_BAD_DESC;
            }
        }
    }
    return fd;
}

uint8_t fat_open_path(const char *path, uint8_t mode) {
    uint8_t fd = fat_find_path(path, mode);
    if (fd == FAT_BAD_DESC) {
        return FAT_BAD_DESC;
    }
    struct FileDescriptor *pfd = fat_private_file_desc + fd;

    if (pfd->dir_entry.attrs & FAT_FILE_ATTR_DIRECTORY) {
        last_error = FAT_ERROR_IS_DIR;
        pfd->is_free = true;
        return FAT_BAD_DESC;
    }

    if ((pfd->dir_entry.attrs & FAT_FILE_ATTR_DIRECTORY) && pfd->dir_entry.cluster_lo == 0) {
        // root dir
        fat_private_init_root_dir(pfd);
    } else {
        if (!fat_private_init_fd(pfd)) {
            pfd->is_free = true;
            return FAT_BAD_DESC;
        }
    }
    return fd;
}

