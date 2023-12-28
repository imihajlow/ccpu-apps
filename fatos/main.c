#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <libsys/vga.h>
#include <libsys/ps2keyboard.h>
#include <libsys/card.h>

#define FAT_IMPL
#include <libsys/fat/fat.h>

enum State {
    STATE_WAIT_CARD,
    STATE_CARD_PRESENT,
    STATE_EXEC,
    STATE_GIVE_UP,
};

static void console_init(void);

void main(void) {
    enum State state = STATE_WAIT_CARD;
    uint8_t attempts_left;

    console_init();
    fat_init();

    puts("FAT OS");
    puts("Waiting for card...");

    while (true) {
        switch (state) {
        case STATE_WAIT_CARD:
            if (CARD_IS_PRESENT()) {
                puts("Card is detected");
                state = STATE_CARD_PRESENT;
            } else {
                attempts_left = 5;
            }
            break;
        case STATE_CARD_PRESENT:
            puts("Mounting card...");
            if (fat_mount(0)) {
                puts("Mounted successfully");
                state = STATE_EXEC;
            } else {
                fat_print_last_error(0);
                puts("Trying to reinialize card...");
                CARD_POWER_OFF();
                attempts_left -= 1;
                if (attempts_left != 0) {
                    state = STATE_WAIT_CARD;
                } else {
                    puts("No attempts left");
                    state = STATE_GIVE_UP;
                }
            }
            break;
        case STATE_GIVE_UP:
            if (!CARD_IS_PRESENT()) {
                puts("Card was removed");
                state = STATE_WAIT_CARD;
            }
            break;
        case STATE_EXEC:
            puts("Starting /SHELL.APP");
            if (fat_exec(0, "/SHELL.APP", 0)) {
            } else {
                fat_print_last_error(0);
                state = STATE_GIVE_UP;
            }
            break;
        }
    }
}

static uint8_t console_col;
static uint8_t console_row;

static void console_init(void) {
    vga_clear(COLOR(COLOR_GRAY, COLOR_BLACK));
    console_col = 0;
    console_row = 0;
}

static void scroll_up(void) {
    char *dst = VGA_CHAR_SEG;
    char *src = VGA_CHAR_SEG + VGA_OFFSET(0, 1);
    for (uint8_t row = 0; row != VGA_ROWS - 1; ++row) {
        memcpy(dst, src, VGA_COLS);
        dst = src;
        src += VGA_OFFSET(0, 1);
    }
    bzero(VGA_CHAR_SEG + VGA_OFFSET(0, VGA_ROWS - 1), VGA_COLS);
}

int getchar(void) { return EOF; }
int putchar(int x) {
    char c = x;
    if (c == '\n') {
        console_col = 0;
        if (console_row == VGA_ROWS - 1) {
            scroll_up();
        } else {
            console_row += 1;
        }
    } else {
        VGA_CHAR_SEG[VGA_OFFSET(console_col, console_row)] = c;
        console_col += 1;
        if (console_col == VGA_COLS) {
            console_col = 0;
            if (console_row == VGA_ROWS - 1) {
                scroll_up();
            } else {
                console_row += 1;
            }
        }
    }
    return 0;
}

int syscall_putchar(char _syscall_nr, int x) {
    return putchar(x);
}

extern char __seg_shared_begin;
extern char __seg_shared_end;

void *get_shared_mem_ptr(char _syscall_nr) {
    return &__seg_shared_begin;
}

size_t get_shared_mem_size(char _syscall_nr) {
    // compiler TODO workaround
    static uint16_t begin;
    begin = (uint16_t)&__seg_shared_begin;
    static uint16_t end;
    end = (uint16_t)&__seg_shared_end;
    return end - begin;
}
