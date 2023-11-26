#include <string.h>
#include <libsys/vga.h>
#include <libsys/ps2keyboard.h>
#include <libsys/card.h>
#include "fat/fat.h"
#include "more.h"
// #include <line_edit.h>
// #include <log.h>
#include "exec.h"

#define FILE_COLOR COLOR(COLOR_LIGHT_BLUE, COLOR_BLUE)
#define CURSOR_COLOR COLOR(COLOR_BLUE, COLOR_CYAN)
#define STATUS_COLOR COLOR(COLOR_BLACK, COLOR_CYAN)
#define TYPING_COLOR COLOR(COLOR_GREEN, COLOR_BLACK)

#define MAX_FILES (5 * (VGA_ROWS - 1))

struct Filename {
    char name[13];
    uint8_t attrs;
};

static struct Filename filenames[MAX_FILES];

static struct FatDirEntry current_dir;

static char current_dir_name[256];

static bool load_files(void) {
    uint8_t fd = fat_open_dir(&current_dir);
    if (fd == FAT_BAD_DESC) {
        strcpy(VGA_CHAR_SEG + VGA_OFFSET(0, VGA_ROWS - 1), "fat_open_dir failed");
        ps2_wait_key_pressed();
        return false;
    }
    struct FatDirEntry ent;
    struct Filename *name = (struct Filename*)filenames;
    for (uint16_t i = 0; i != MAX_FILES; i += 1) {
        name->name[0] = 0;
        if (!fat_get_next_dir_entry(fd, &ent, FAT_FILE_ATTR_HIDDEN | FAT_FILE_ATTR_SYSTEM | FAT_FILE_ATTR_VOLUME_ID)) {
            break;
        }
        from_fat_name(name->name, ent.filename);
        name->attrs = ent.attrs;
        name += 1;
    }
    bool r = last_error == ERROR_OK;
    fat_close(fd);
    return r;
}

uint16_t display_table(uint16_t offset) {
    struct Filename *f = (struct Filename*)filenames + offset;
    uint8_t row = 0;
    uint8_t col = 1;
    while (offset != MAX_FILES) {
        if (f->name[0] == 0) {
            break;
        }
        strcpy(VGA_CHAR_SEG + VGA_OFFSET(col, row), f->name);
        if (f->attrs & FAT_FILE_ATTR_DIRECTORY) {
            VGA_CHAR_SEG[VGA_OFFSET(col + 13, row)] = '/';
        }
        f += 1;
        offset += 1;
        row += 1;
        if (row == VGA_ROWS - 1) {
            col += 16;
            row = 0;
            if (col >= VGA_COLS) {
                break;
            }
        }
    }
    return offset;
}

void set_cursor(uint8_t col, uint8_t row, uint8_t color) {
    memset(VGA_COLOR_SEG + VGA_OFFSET(col, row), color, 16);
}

static uint16_t cursor_file_index;

uint8_t table_cursor_loop(uint8_t max_index, bool reset_cursor) {
    // max 5 x 29 = 145 entries on the screen
    static uint8_t row = 0;
    static uint8_t col = 0;
    static uint8_t index = 0;
    uint8_t new_index;
    if (reset_cursor) {
        row = 0;
        col = 0;
        index = 0;
        new_index = 0;
        cursor_file_index = 0;
    }
    set_cursor(col, row, CURSOR_COLOR);
    while (true) {
        uint8_t k = ps2_wait_key_pressed();
        switch (k) {
        case PS2_KEY_DOWN:
            new_index = index + 1;
            break;
        case PS2_KEY_UP:
            new_index = index - 1;
            break;
        case PS2_KEY_RIGHT:
            new_index = index + (VGA_ROWS - 1);
            break;
        case PS2_KEY_LEFT:
            new_index = index - (VGA_ROWS - 1);
            break;
        default:
            set_cursor(col, row, FILE_COLOR);
            return k;
        }
        if (new_index != index) {
            if (new_index < max_index) {
                index = new_index;
                set_cursor(col, row, FILE_COLOR);
                row = index % (VGA_ROWS - 1);
                col = (index / (VGA_ROWS - 1)) << 4;
                set_cursor(col, row, CURSOR_COLOR);
                cursor_file_index = index;
            } else {
                new_index = index;
            }
        }
    }
}

void print_file(const char *name) {
    more_init(COLOR(COLOR_WHITE, COLOR_GREEN));
    uint8_t f = fat_open_file(&current_dir, name, 0);
    if (f == FAT_BAD_DESC) {
        strcpy(VGA_CHAR_SEG, "Error");
        ps2_wait_key_pressed();
        return;
    }
    uint8_t buf[256];
    uint8_t k = 1;
    while (k) {
        uint16_t len = fat_read(f, buf, 256);
        k = more_print(buf, len);
        if (len < 256) {
            if (last_error != FAT_EOF) {
                strcpy(VGA_CHAR_SEG + VGA_OFFSET(0, VGA_ROWS - 1), "ERROR");
                ps2_wait_key_pressed();
            }
            break;
        }
    }
    if (k) {
        strcpy(VGA_CHAR_SEG + VGA_OFFSET(0, VGA_ROWS - 1), "-- END --");
        ps2_wait_key_pressed();
    }
    fat_close(f);
}

static void run_app(const char *name) {
    uint8_t fd = fat_open_file(&current_dir, name, 0);
    if (fd == FAT_BAD_DESC) {
        // log_string("Cannot open");
        // log_u8(last_error);
        return;
    }
    if (!exec_fd(fd)) {
        // log_string("Cannot exec");
        // log_u8(last_error);
        return;
    }
}

static void run_editor(const char *target) {
    /*u8 *slash = append_path(target);
    u8 r = exec("/EDIT.APP", (u8*)current_dir_name, (u8*)0, (u8*)0, (u8*)0, (u8*)0, (u8*)0, (u8*)0);
    if (!r) {
        log_string("Cannot exec");
        log_u8(last_error);
    }
    *slash = 0u8;*/
}

static char *append_path(const char *name) {
    char *p = current_dir_name;
    while (*p) {
        p += 1;
    }
    char *r = p;
    if (current_dir_name != p && *(p - 1) != '/') {
        *p = '/';
        p += 1;
    }
    while (*name) {
        *p = *name;
        p += 1;
        name += 1;
    }
    *p = 0;
    return r;
}

void main(void) {
    fat_init();

    bool card_present = false;
    bool fat_success = false;

    while (true) {
        vga_clear(FILE_COLOR);
        memset(VGA_COLOR_SEG + VGA_OFFSET(0, VGA_ROWS - 1), STATUS_COLOR, VGA_COLS);
        if (!card_present) {
            strcpy(VGA_CHAR_SEG + VGA_OFFSET(0, VGA_ROWS - 1), "Waiting for card");
            while (!card_present) {
                card_present = CARD_IS_PRESENT();
            }
            fat_success = fat_mount();
            if (fat_success) {
                bool r = fat_change_dir(0, 0, &current_dir);
                current_dir_name[0] = '/';
                current_dir_name[1] = 0;
                if (!r) {
                    strcpy(VGA_CHAR_SEG + VGA_OFFSET(0, VGA_ROWS - 1), "fat_change_dir failed");
                    ps2_wait_key_pressed();
                }
            }
        } else if (!fat_success) {
            strcpy(VGA_CHAR_SEG + VGA_OFFSET(0, VGA_ROWS - 1), "FAT error, remove card");
            while (card_present) {
                card_present = CARD_IS_PRESENT();
            }
        } else {
            bool init = true;
            strcpy(VGA_CHAR_SEG + VGA_OFFSET(0, VGA_ROWS - 1), "F3 - view, F4 - edit, Enter - run");
            if (load_files()) {
                uint16_t max_index = display_table(0);
                while (true) {
                    uint8_t k = table_cursor_loop(max_index, init);
                    init = false;
                    card_present = CARD_IS_PRESENT();
                    if (!card_present) {
                        CARD_POWER_OFF();
                        break;
                    }
                    if (k == PS2_KEY_F3) {
                        if (!(filenames[cursor_file_index].attrs & FAT_FILE_ATTR_DIRECTORY)) {
                            print_file(filenames[cursor_file_index].name);
                            break;
                        }
                    } else if (k == PS2_KEY_F4) {
                        // if (!(filenames[cursor_file_index].attrs & FAT_FILE_ATTR_DIRECTORY)) {
                        //     run_editor(filenames[cursor_file_index].name);
                        //     break;
                        // }
                    } else if (k == PS2_KEY_ENTER) {
                        if (filenames[cursor_file_index].attrs & FAT_FILE_ATTR_DIRECTORY) {
                            char *name = filenames[cursor_file_index].name;
                            if (fat_change_dir(&current_dir, name, &current_dir)) {
                                append_path(name);
                                break;
                            }
                        } else {
                            char fat_name[11];
                            to_fat_name(fat_name, filenames[cursor_file_index].name);
                            if (memcmp(fat_name + 8, "APP", 3) == 0) {
                                run_app(filenames[cursor_file_index].name);
                            }
                            break;
                        }
                    }
                }
            }
            else
            {
                card_present = false;
                CARD_POWER_OFF();
            }
        }
    }
}

int getchar(void) {}
int putchar(int x) {}
