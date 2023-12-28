#include "line_edit.h"
#include "more.h"
#include <libsys/vga.h>
#include <libsys/fat/fat.h>
#include <libsys/fat/name.h>
#include <libsys/syscall.h>
#include <libsys/ps2keyboard.h>
#include <string.h>
#include <assert.h>

#define TYPING_COLOR COLOR(COLOR_WHITE, COLOR_BLUE)
#define CURSOR_COLOR COLOR(COLOR_BLUE, COLOR_WHITE)
#define STATUS_COLOR COLOR(COLOR_BLACK, COLOR_CYAN)
#define PROMPT_COLOR COLOR(COLOR_WHITE, COLOR_RED)

#define MAX_LINES 300

#define NO_NEWLINE 128

#define LINE_WIDTH 59

struct Line {
    struct Line *prev;
    struct Line *next;
    uint8_t len; // 0-59 - has newline, 128 - no newline, full
    uint8_t data[LINE_WIDTH];
};

static_assert(sizeof(struct Line) == 64, "sizeof(struct Line) must be 64");

static struct Line line_pool[MAX_LINES];

// where each screen before current starts:
static struct Line *screen_ptrs[MAX_LINES / VGA_ROWS + 1];

// current screen index
static uint8_t screen_idx;

// lines list head
static struct Line *head;

// pool head and tail for allocating new lines
static struct Line *pool_head;
static struct Line *pool_tail;

static char buf[128] __attribute__((section("bss_hi")));

static char filename_buf[40]  __attribute__((section("bss_hi")));

// actual file name
static char *filename;

static bool dirty;

static uint8_t cursor_col;
static uint8_t cursor_row;
static struct Line *cursor_line;

static struct Line *next_screen;

static void show_dirty(void);
static void init_layout(void);
static struct Line *show_lines(struct Line *start);

static bool load(const char *path) {
    uint8_t fd = fat_open_path(path, O_CREAT);
    if (fd == FAT_BAD_DESC) {
        // log_string("Can't open");
        // log_u8(last_error);
        return false;
    }
    screen_idx = 0;
    struct Line *p = line_pool;
    for (uint16_t i = 0; i != MAX_LINES - 1; i += 1) {
        p->next = p + 1;
        p = p->next;
    }
    p->next = 0;
    pool_tail = p;

    head = (struct Line*)line_pool;
    struct Line *cur_line = (struct Line*)line_pool;

    cur_line->len = 0;
    cur_line->prev = 0;
    while (true) {
        uint16_t r16 = fat_read(fd, buf, sizeof(buf));
        uint8_t r8 = r16;
        for (uint8_t i8 = 0; i8 != r8; i8 += 1) {
            cur_line->data[cur_line->len] = buf[i8];
            cur_line->len += 1;
            if (buf[i8] == '\n' || cur_line->len == (uint8_t)sizeof(cur_line->data)) {
                if (buf[i8] == '\n') {
                    cur_line->len -= 1;
                } else {
                    cur_line->len = NO_NEWLINE;
                }
                p = cur_line;
                cur_line = cur_line->next;
                if (!cur_line) {
                    // log_string("out of lines");
                    fat_close(fd);
                    return false;
                }
                cur_line->prev = p;
                cur_line->len = 0;
            }
        }

        if (r16 != sizeof(buf)) {
            if (fat_get_last_error() != FAT_EOF) {
                // log_string("Can't read");
                // log_u8(last_error);
                fat_close(fd);
                return false;
            }
            break;
        }
    }
    pool_head = cur_line->next;
    cur_line->next = 0;
    fat_close(fd);
    return true;
}

static bool save(const char *path) {
    uint8_t fd = fat_open_path(path, O_CREAT); // file must already exist
    if (fd == FAT_BAD_DESC) {
        // log_string("Can't open");
        // log_u8(last_error);
        return false;
    }

    struct Line *l = head;
    uint8_t nl = '\n';
    while (l) {
        uint16_t to_write = l->len;
        if (l->len == NO_NEWLINE) {
            to_write = sizeof(l->data);
        }
        memcpy(buf, l->data, to_write);
        if (l->len != NO_NEWLINE) {
            buf[l->len] = '\n';
            to_write += 1;
        }
        uint16_t written = fat_write(fd, buf, to_write);
        if (written != to_write) {
            // log_string("Write error");
            // log_u8(last_error);
            fat_close(fd);
            return false;
        }
        l = l->next;
    }
    if (!fat_truncate(fd)) {
        // log_string("Can't truncate");
        // log_u8(last_error);
        fat_close(fd);
        return false;
    }
    fat_close(fd);
    dirty = false;
    return true;
}

static bool save_as(void) {
    if (filename != filename_buf) {
        strcpy(filename_buf, filename);
        filename = filename_buf;
    }
    memset(VGA_CHAR_SEG + VGA_OFFSET(0, VGA_ROWS - 1), 0, VGA_COLS);
    memset(VGA_COLOR_SEG + VGA_OFFSET(0, VGA_ROWS - 1), PROMPT_COLOR, VGA_COLS);
    bool r = line_edit(filename_buf, sizeof(filename_buf) - 1, 0, VGA_ROWS - 1, PROMPT_COLOR);
    if (r) {
        r = save(filename);
    } else {
        r = false;
    }
    init_layout();
    next_screen = show_lines(screen_ptrs[screen_idx]);
    return r;
}

static void show_filename(void) {
    char *name = filename;
    for (uint8_t row = 10; *name && row != VGA_ROWS; ++row) {
        uint8_t col = LINE_WIDTH + 2;
        char *p = VGA_CHAR_SEG + VGA_OFFSET(col, row);
        while (true) {
            char c = *name;
            if (!c)
                break;
            *p = c;
            ++name;
            ++col;
            if (c == '/') {
                if (col + 11 > VGA_COLS)
                    break;
            }
            ++p;
        }
    }
}

static void init_layout(void) {
    memset(VGA_CHAR_SEG, 0, VGA_ROWS * 128);
    for (char *pcolor = VGA_COLOR_SEG; pcolor != VGA_COLOR_SEG + VGA_OFFSET(0, VGA_ROWS); pcolor += VGA_OFFSET(0, 1)) {
        memset(pcolor, TYPING_COLOR, LINE_WIDTH + 1);
        memset(pcolor + (LINE_WIDTH + 1), STATUS_COLOR, VGA_COLS - LINE_WIDTH - 1);
    }
    strcpy(VGA_CHAR_SEG + VGA_OFFSET(LINE_WIDTH + 2, 2), "Esc - exit");
    strcpy(VGA_CHAR_SEG + VGA_OFFSET(LINE_WIDTH + 2, 3), "F2  - save");
    strcpy(VGA_CHAR_SEG + VGA_OFFSET(LINE_WIDTH + 2, 4), "F3  - save as");
    show_filename();
    show_dirty();
}

static void show_dirty(void) {
    uint8_t c = LINE_WIDTH + 1;
    uint8_t r = 3;
    if (dirty) {
        VGA_CHAR_SEG[VGA_OFFSET(c, r)] = '*';
    } else {
        VGA_CHAR_SEG[VGA_OFFSET(c, r)] = 0;
    }
}

static struct Line *show_lines(struct Line *start) {
    uint8_t row = 0;
    while (row != VGA_ROWS && start) {
        uint8_t *p = VGA_CHAR_SEG + VGA_OFFSET(0, row);
        uint8_t i;
        for (i = 0; i != start->len && i != LINE_WIDTH; i += 1) {
            uint8_t c = start->data[i];
            *p = c;
            p += 1;
        }
        for (; i != LINE_WIDTH; i += 1) {
            *p = 0;
            p += 1;
        }
        row += 1;
        start = start->next;
    }
    if (row != VGA_ROWS) {
        start = 0;
    }
    for (; row != VGA_ROWS; row += 1) {
        memset(VGA_CHAR_SEG + VGA_OFFSET(0, row), 0, LINE_WIDTH + 1);
    }
    return start;
}

static struct Line *alloc_line(void) {
    struct Line *result = pool_head;
    pool_head = pool_head->next;
    return result;
}

static void free_line(struct Line *l) {
    if (!pool_head) {
        pool_head = l;
        pool_tail = l;
        l->next = 0;
    } else {
        pool_tail->next = l;
        pool_tail = l;
    }
}

bool insert(uint8_t c) {
    if (!pool_head) {
        return false; // can't insert, no lines left
    }
    struct Line *l = cursor_line;
    uint8_t next;
    uint8_t col = cursor_col;
    uint8_t row = cursor_row;
    bool cursor_set = false;
    while (true) {
        if (l->len < sizeof(l->data)) {
            // no line overflow
            for (uint8_t i = l->len; i != col; i -= 1) {
                l->data[i] = l->data[i - 1];
            }
            l->len += 1;
            l->data[col] = c;
            if (row < VGA_ROWS) {
                memcpy(VGA_CHAR_SEG + VGA_OFFSET(col, row), l->data + col, l->len - col);
            }
            if (!cursor_set) {
                cursor_col = col + 1;
            }
            break;
        } else if (l->len == sizeof(l->data)) {
            // full line with a newline
            // insert a new line
            struct Line *new_line = alloc_line();
            new_line->len = 1;
            new_line->next = l->next;
            new_line->prev = l;
            if (l->next) {
                l->next->prev = new_line;
            }
            l->next = new_line;
            if (col != sizeof(l->data)) {
                next = l->data[sizeof(l->data) - 1];
                for (uint8_t i = sizeof(l->data) - 1; i != col; i -= 1) {
                    l->data[i] = l->data[i - 1];
                }
                l->data[col] = c;
                if (row < VGA_ROWS) {
                    memcpy(VGA_CHAR_SEG + VGA_OFFSET(col, row), l->data + col, sizeof(l->data) - col);
                }
                if (!cursor_set) {
                    cursor_col = col + 1;
                }
            } else {
                next = c;
                if (!cursor_set) {
                    cursor_col = 1;
                    cursor_row += 1;
                    cursor_line = new_line;
                }
            }
            l->len = NO_NEWLINE;
            new_line->data[0] = next;
            if (row == VGA_ROWS - 1) {
                next_screen = next_screen->prev;
            } else {
                next_screen = show_lines(screen_ptrs[screen_idx]);
            }
            break;
        } else {
            // full line without a newline
            // carry over to the next line
            if (col != sizeof(l->data)) {
                next = l->data[sizeof(l->data) - 1];
                for (uint8_t i = sizeof(l->data) - 1; i != col; i -= 1) {
                    l->data[i] = l->data[i - 1];
                }
                l->data[col] = c;
                if (row < VGA_ROWS) {
                    memcpy(VGA_CHAR_SEG + VGA_OFFSET(col, row), l->data + col, sizeof(l->data) - col);
                }
                if (!cursor_set) {
                    cursor_set = true;
                    cursor_col = col + 1;
                }
            } else {
                next = c;
                if (!cursor_set) {
                    cursor_set = true;
                    cursor_col = 1;
                    cursor_row += 1;
                    cursor_line = cursor_line->next;
                }
            }
            c = next;
            col = 0;
            l = l->next;
            row += 1;
        }
    }
    if (cursor_row == VGA_ROWS) {
        screen_idx += 1;
        screen_ptrs[screen_idx] = next_screen;
        next_screen = show_lines(next_screen);
        cursor_row = 0;
    }
    dirty = true;
    return true;
}

bool break_line(void) {
    if (!pool_head) {
        return false; // can't insert, no lines left
    }
    struct Line *new_line;
    if (cursor_line->len != NO_NEWLINE) {
        new_line = alloc_line();
        new_line->len = cursor_line->len - cursor_col;
        memcpy(new_line->data, cursor_line->data + cursor_col, new_line->len);
        cursor_line->len = cursor_col;
        if (cursor_line->next) {
            cursor_line->next->prev = new_line;
        }
        new_line->next = cursor_line->next;
        new_line->prev = cursor_line;
        cursor_line->next = new_line;
        cursor_line = new_line;
        cursor_row += 1;
        cursor_col = 0;
    } else {
        if (cursor_col == sizeof(cursor_line->data)) {
            cursor_line->len = sizeof(cursor_line->data);
            cursor_line = cursor_line->next;
            cursor_row += 1;
            cursor_col = 0;
        } else if (cursor_col == 0) {
            if (!cursor_line->prev) {
                new_line = alloc_line();
                head = new_line;
                new_line->prev = 0;
                new_line->next = cursor_line;
                new_line->len = 0;
                cursor_line->prev = new_line;
                screen_ptrs[0] = new_line;
                cursor_row += 1;
            } else if (cursor_line->prev->len == NO_NEWLINE) {
                cursor_line->prev->len = sizeof(cursor_line->data);
            } else {
                new_line = alloc_line();
                new_line->prev = cursor_line->prev;
                new_line->next = cursor_line;
                cursor_line->prev->next = new_line;
                cursor_line->prev = new_line;
                if (cursor_row == 0) {
                    screen_ptrs[screen_idx] = new_line;
                }
                cursor_row += 1;
            }
            cursor_col = 0;
        } else {
            uint8_t rest = sizeof(cursor_line->data) - cursor_col; // how many bytes to carry over to the next line
            // Cursor line is a part of a list of long lines.
            // Find the last line in the list.
            struct Line *tail = cursor_line;
            while (tail->len == NO_NEWLINE) { // tail can't be 0
                tail = tail->next;
            }
            if (tail->len >= cursor_col) {
                // the last line doesn't fit the carry-over part and the original contents
                new_line = alloc_line();
                new_line->prev = tail;
                new_line->next = tail->next;
                tail->next = new_line;
                if (new_line->next) {
                    new_line->next->prev = new_line;
                }
                new_line->len = tail->len - cursor_col;
                memcpy(new_line->data, tail->data + cursor_col, new_line->len);
                tail->len = NO_NEWLINE;
            } else {
                tail->len += rest;
            }

            while (tail != cursor_line) {
                for (int8_t i = cursor_col - 1; i >= 0; i -= 1) {
                    tail->data[i + rest] = tail->data[i];
                }
                memcpy(tail->data, tail->prev->data + cursor_col, rest);
                tail = tail->prev;
            }
            cursor_line->len = cursor_col;
            cursor_line = cursor_line->next;
            cursor_row += 1;
            cursor_col = 0;
        }
    }
    if (cursor_row == VGA_ROWS) {
        screen_idx += 1;
        screen_ptrs[screen_idx] = new_line;
        cursor_row = 0;
    }
    next_screen = show_lines(screen_ptrs[screen_idx]);
    dirty = true;
    return true;
}

bool delete(struct Line *l, uint8_t col) {
    uint8_t signle_line = true;
    while (true) {
        if (l->len == 0) {
            if (!l->prev && !l->next) {
                break; // the only line
            } else if (!l->next) {
                // last line
                if (l == cursor_line) {
                    cursor_line = cursor_line->prev;
                    if (cursor_row == 0) {
                        screen_idx -= 1;
                        cursor_row = VGA_ROWS - 1;
                    } else {
                        cursor_row -= 1;
                    }
                }
                l = l->prev;
                free_line(l->next);
                l->next = 0;
            } else if (!l->prev) {
                // first line
                head = head->next;
                head->prev = 0;
                screen_ptrs[0] = head;
                if (cursor_line == l) {
                    cursor_line = head;
                }
                free_line(l);
            } else {
                // line with both neighbors
                l->prev->next = l->next;
                l->next->prev = l->prev;
                if (cursor_line == l) {
                    cursor_line = l->next;
                }
                free_line(l);
            }
            signle_line = false;
            break;
        } else if (l->len != NO_NEWLINE) {
            l->len -= 1;
            for (uint8_t i = col; i != l->len; i += 1) {
                l->data[i] = l->data[i + 1];
            }
            break;
        } else {
            signle_line = false;
            for (uint8_t i = col; i != sizeof(l->data) - 1; i += 1) {
                l->data[i] = l->data[i + 1];
            }
            if (l->next->len != 0) {
                l->data[sizeof(l->data) - 1] = l->next->data[0];
            } else {
                l->len = sizeof(l->data) - 1;
            }
            l = l->next;
            col = 0;
        }
    }
    return signle_line;
}

// Merge l and l->next
// l has newline
static void merge_lines(struct Line *l) {
    struct Line *next = l->next;
    if (!next) {
        return;
    }
    uint8_t tail = sizeof(l->data) - l->len;
    while (true) {
        if (next->len <= tail) {
            memcpy(l->data + l->len, next->data, next->len);
            l->len += next->len;
            l->next = next->next;
            if (l->next) {
                l->next->prev = l;
            }
            free_line(next);
            break;
        } else {
            memcpy(l->data + l->len, next->data, tail);
            l->len = NO_NEWLINE;
            if (next->len != NO_NEWLINE) {
                next->len -= tail;
                memcpy(next->data, next->data + tail, next->len);
                break;
            } else {
                next->len = sizeof(next->data) - tail;
                memcpy(next->data, next->data + tail, next->len);
                l = next;
                next = l->next;
            }
        }
    }
}

static void delete_right(void) {
    if (cursor_col != sizeof(cursor_line->data) && cursor_col != cursor_line->len) {
        bool single = delete(cursor_line, cursor_col);
        if (single) {
            uint8_t last = cursor_line->len;
            if (last == NO_NEWLINE) {
                last = sizeof(cursor_line->data);
            }
            memcpy(VGA_CHAR_SEG + VGA_OFFSET(cursor_col, cursor_row), cursor_line->data + cursor_col, last - cursor_col);
            VGA_CHAR_SEG[VGA_OFFSET(last, cursor_row)] = 0;
        } else {
            next_screen = show_lines(screen_ptrs[screen_idx]);
        }
    } else {
        if (cursor_line->len == NO_NEWLINE) {
            delete(cursor_line->next, 0);
        } else {
            merge_lines(cursor_line);
        }
        next_screen = show_lines(screen_ptrs[screen_idx]);
    }
    dirty = true;
}

static void delete_left(void) {
    if (cursor_col) {
        cursor_col -= 1;
        bool single = delete(cursor_line, cursor_col);
        if (single) {
            uint8_t last = cursor_line->len;
            if (last == NO_NEWLINE) {
                last = sizeof(cursor_line->data);
            }
            memcpy(VGA_CHAR_SEG + VGA_OFFSET(cursor_col, cursor_row), cursor_line->data + cursor_col, last - cursor_col);
            VGA_CHAR_SEG[VGA_OFFSET(last, cursor_row)] = 0;
        } else {
            next_screen = show_lines(screen_ptrs[screen_idx]);
        }
    } else {
        struct Line *prev_line = cursor_line->prev;
        if (prev_line) {
            cursor_line = prev_line;
            if (cursor_row == 0) {
                cursor_row = VGA_ROWS - 1;
                screen_idx -= 1;
            } else {
                cursor_row -= 1;
            }

            if (prev_line->len == NO_NEWLINE) {
                cursor_col = sizeof(prev_line->data) - 1;
            } else if (prev_line->len != 0) {
                cursor_col = prev_line->len;
            } else {
                cursor_col = 0;
            }

            if (prev_line->len == NO_NEWLINE) {
                delete(prev_line, cursor_col);
            } else {
                merge_lines(prev_line);
            }
        }
        next_screen = show_lines(screen_ptrs[screen_idx]);
    }
    dirty = true;
}

void main(uint8_t argc, ...) {
    vga_clear(TYPING_COLOR);
    if (argc == 0) {
        filename_buf[0] = 0;
        strcpy(VGA_CHAR_SEG + VGA_OFFSET(0, VGA_ROWS - 1), "Filename: ");
        memset(VGA_COLOR_SEG + VGA_OFFSET(0, VGA_ROWS - 1), PROMPT_COLOR, VGA_COLS);
        bool r = line_edit(filename_buf, sizeof(filename_buf) - 1, 10, VGA_ROWS - 1, PROMPT_COLOR);
        if (!r) {
            return;
        }
        vga_clear(TYPING_COLOR);
        filename = filename_buf;
    } else {
        va_list va;
        va_start(va, argc);
        filename = va_arg(va, char *);
        va_end(va);
    }
    if (!load(filename)) {
        return;
    }
    dirty = false;
    cursor_col = 0;
    cursor_row = 0;
    init_layout();
    next_screen = show_lines(head);
    VGA_COLOR_SEG[VGA_OFFSET(0, 0)] = CURSOR_COLOR;
    screen_ptrs[0] = head;
    cursor_line = head;
    while (true) {
        uint16_t k = ps2_get_ascii();
        if (k == 0) {
            continue;
        }
        VGA_COLOR_SEG[VGA_OFFSET(cursor_col, cursor_row)] = TYPING_COLOR;
        if (PS2_IS_ASCII(k)) {
            insert(k);
        } else {
            k &= ~PS2_ASCII_SCANCODE_MASK;
            if (k == PS2_KEY_ENTER || k == PS2_KEY_NUM_ENTER) {
                break_line();
            } else if (k == PS2_KEY_BACKSPACE) {
                delete_left();
            } else if (k == PS2_KEY_DELETE || k == PS2_KEY_F1) {
                delete_right();
            } else if (k == PS2_KEY_UP) {
                if (cursor_row == 0) {
                    if (screen_idx != 0) {
                        screen_idx -= 1;
                        next_screen = show_lines(screen_ptrs[screen_idx]);
                        cursor_row = VGA_ROWS - 1;
                    }
                } else {
                    cursor_row -= 1;
                }
                if (cursor_line->prev) {
                    cursor_line = cursor_line->prev;
                    if (cursor_line->len < cursor_col) {
                        cursor_col = cursor_line->len;
                    }
                }
            } else if (k == PS2_KEY_DOWN) {
                if (cursor_line->next) {
                    if (cursor_row == VGA_ROWS - 1) {
                        if (next_screen) {
                            screen_idx += 1;
                            screen_ptrs[screen_idx] = next_screen;
                            next_screen = show_lines(next_screen);
                            cursor_row = 0;
                        }
                    } else {
                        cursor_row += 1;
                    }
                    cursor_line = cursor_line->next;
                    if (cursor_line->len < cursor_col) {
                        cursor_col = cursor_line->len;
                    }
                }
            } else if (k == PS2_KEY_RIGHT) {
                if (cursor_col == cursor_line->len || cursor_col == (uint8_t)sizeof(next_screen->data)) {
                    // TODO next line
                } else {
                    cursor_col += 1;
                }
            } else if (k == PS2_KEY_LEFT) {
                if (cursor_col == 0) {
                    // TODO prev line
                } else {
                    cursor_col -= 1;
                }
            } else if (k == PS2_KEY_HOME) {
                cursor_col = 0;
            } else if (k == PS2_KEY_END) {
                if (cursor_line->len == NO_NEWLINE) {
                    cursor_col = sizeof(cursor_line->data);
                } else {
                    cursor_col = cursor_line->len;
                }
            } else if (k == PS2_KEY_PAGEUP) {
                if (screen_idx) {
                    screen_idx -= 1;
                    next_screen = show_lines(screen_ptrs[screen_idx]);
                }
                cursor_col = 0;
                cursor_row = 0;
                cursor_line = screen_ptrs[screen_idx];
            } else if (k == PS2_KEY_PAGEDOWN) {
                if (next_screen) {
                    screen_idx += 1;
                    screen_ptrs[screen_idx] = next_screen;
                    next_screen = show_lines(next_screen);
                    cursor_row = 0;
                    cursor_col = 0;
                    cursor_line = screen_ptrs[screen_idx];
                }
            } else if (k == PS2_KEY_ESCAPE) {
                break;
            } else if (k == PS2_KEY_F2) {
                save(filename);
            } else if (k == PS2_KEY_F3) {
                save_as();
            }
        }
        VGA_COLOR_SEG[VGA_OFFSET(cursor_col, cursor_row)] = CURSOR_COLOR;
        show_dirty();
    }
}

int getchar(void) { return -1; }
int putchar(int c) { return -1; }
