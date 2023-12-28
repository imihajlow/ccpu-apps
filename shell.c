#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <libsys/vga.h>
#include <libsys/ps2keyboard.h>
#include <libsys/card.h>
#include <libsys/fat/fat.h>
#include <libsys/fat/name.h>
#include "lib/more.h"
// #include <line_edit.h>
// #include <log.h>

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

static char current_dir_name[256] __attribute__((section ("bss_hi")));
static uint8_t buf[256] __attribute__((section ("bss_hi")));

static void message(const char *msg) {
    memset(VGA_COLOR_SEG + VGA_OFFSET(0, VGA_ROWS - 1), STATUS_COLOR, VGA_COLS);
    strcpy(VGA_CHAR_SEG + VGA_OFFSET(0, VGA_ROWS - 1), msg);
}

static bool load_files(void) {
    uint8_t fd = fat_open_path(current_dir_name, 0);
    if (fd == FAT_BAD_DESC) {
        message("fat_open_path failed");
        ps2_wait_key_pressed();
        return false;
    }
    static struct FatDirEntry ent __attribute__((section ("bss_hi")));
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
    bool r = fat_get_last_error() == FAT_ERROR_OK;
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
        set_cursor(col, row, CURSOR_COLOR);
    }
    while (true) {
        uint8_t k = ps2_get_key_event();
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

void print_file(const char *rel_name) {

    strcpy(buf, current_dir_name);
    strcat(buf, "/");
    strcat(buf, rel_name);

    more_init(COLOR(COLOR_WHITE, COLOR_GREEN));

    uint8_t f = fat_open_path(buf, 0);
    if (f == FAT_BAD_DESC) {
        strcpy(VGA_CHAR_SEG, "Error");
        ps2_wait_key_pressed();
        return;
    }
    uint8_t k = 1;
    while (k) {
        uint16_t len = fat_read(f, buf, 256);
        k = more_print(buf, len);
        if (len < 256) {
            if (fat_get_last_error() != FAT_EOF) {
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

static void run_app(const char *rel_name) {
    strcpy(buf, current_dir_name);
    strcat(buf, "/");
    strcat(buf, rel_name);

    if (!fat_exec(buf, 0)) {
        strcpy(VGA_CHAR_SEG + VGA_OFFSET(0, VGA_ROWS - 1), "ERROR");
        ps2_wait_key_pressed();
    }
}

static void run_editor(const char *rel_name) {
    char *shared_mem = get_shared_mem_ptr();
    strcpy(shared_mem, current_dir_name);
    strcat(shared_mem, "/");
    strcat(shared_mem, rel_name);
    strcpy(buf, "/EDIT.APP");
    if (!fat_exec(buf, 1, shared_mem)) {
        strcpy(VGA_CHAR_SEG + VGA_OFFSET(0, VGA_ROWS - 1), "ERROR");
        ps2_wait_key_pressed();
    }
}

static void append_path(const char *name) {
    if (strcmp(name, ".") == 0) {
        return;
    }
    if (strcmp(name, "..") == 0) {
        char *slash = strrchr(current_dir_name, '/');
        if (slash) {
            if (slash == current_dir_name) {
                // can't go above root
                slash[1] = 0;
            } else {
                *slash = 0;
            }
        }
        return;
    }
    int len = strlen(current_dir_name);
    if (current_dir_name[len - 1] == '/') {
        // ends with a slash, must be root
        strcpy(current_dir_name + len, name);
    } else {
        // doesn't end with a slash
        current_dir_name[len] = '/';
        strcpy(current_dir_name + len + 1, name);
    }
}

void main(void) {
    bool card_present = false;
    bool fat_success = false;
    CARD_POWER_OFF();

    while (true) {
        vga_clear(FILE_COLOR);
        memset(VGA_COLOR_SEG + VGA_OFFSET(0, VGA_ROWS - 1), STATUS_COLOR, VGA_COLS);
        if (!card_present) {
            message("Waiting for card");
            while (!card_present) {
                card_present = CARD_IS_PRESENT();
            }
            fat_success = fat_mount();
            if (fat_success) {
                strcpy(current_dir_name, "/");
            }
        } else if (!fat_success) {
            message("FAT error, remove card");
            while (card_present) {
                card_present = CARD_IS_PRESENT();
            }
        } else {
            bool init = true;
            message("F3 - view, F4 - edit, Enter - run");
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
                        if (!(filenames[cursor_file_index].attrs & FAT_FILE_ATTR_DIRECTORY)) {
                            run_editor(filenames[cursor_file_index].name);
                            break;
                        }
                    } else if (k == PS2_KEY_ENTER) {
                        if (filenames[cursor_file_index].attrs & FAT_FILE_ATTR_DIRECTORY) {
                            char *name = filenames[cursor_file_index].name;
                            append_path(name);
                            break;
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

int getchar(void) { return EOF; }
int putchar(int x) { return 0; }
