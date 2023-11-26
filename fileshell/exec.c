#include "fat/fat.h"
#include "fat/path.h"
#include "fat/last_error.h"
#include "progressbar.h"
#include "loload.h"
#include "lorun.h"
#include "exec.h"
#include <string.h>

extern uint16_t __seg_empty_end;

bool exec(const char *filename) {
    uint8_t fd = fat_open_path(filename, 0);
    if (fd == FAT_BAD_DESC) {
        return false;
    }
    return exec_fd(fd);
}

bool exec_fd(uint8_t fd) {
    struct FatDirEntry *de = fat_get_dir_entry(fd);
    if (!de) {
        fat_close(fd);
        return false;
    }
    char *ext = de->filename + 8;
    if (memcmp(ext, "APP", 3) != 0) {
        fat_close(fd);
        last_error = EXEC_ERROR_WRONG_EXTENSION;
        return false;
    }
    uint32_t size32 = de->size;
    uint16_t max_size = __seg_empty_end;
    if (size32 > max_size) {
        fat_close(fd);
        last_error = EXEC_ERROR_FILE_TOO_LARGE;
        return false;
    }
    uint16_t size = size32;
    progressbar_init(size >> 8);
    uint8_t dst_hi = 0;
    // load lo RAM part
    while (size != 0 && dst_hi != 0x80) {
        uint16_t r = fat_read(fd, loload_buf, sizeof(loload_buf));
        if (r != sizeof(loload_buf)) {
            if (last_error != FAT_EOF) {
                fat_close(fd);
                return false;
            }
        }
        copy_to_loram(dst_hi);
        size -= r;
        dst_hi += 1;
        progressbar_progress(1);
    }
    // load hi RAM part
    if (size != 0) {
        uint16_t r = fat_read(fd, (uint8_t*)0x8000, size);
        if (r != size) {
            fat_close(fd);
            return false;
        }
        progressbar_progress(r >> 8);
    }
    fat_close(fd);
    lorun();
    return true;
}
