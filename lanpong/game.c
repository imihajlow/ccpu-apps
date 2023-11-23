#include "game.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <libsys/vga.h>
#include <libsys/eth.h>
#include <libsys/crc.h>
#include <libsys/ps2keyboard.h>
#include "uip.h"
#include "timer.h"
#include "pong.h"
#include "bigdigit.h"

#define UDP_PORT 7657
#define CONN_TIMEOUT (32 * 5) // 5 seconds

#define CHAR_TOP 223

static enum game_state_t {
    STATE_LOBBY = 0,
    STATE_SCORE,
    STATE_GAME,
} game_state;

static uint16_t score_left = 0;
static uint16_t score_right = 0;
static bool left_ready = false;
static bool right_ready = false;

static struct uip_udp_conn *conn;

#define LEN_ANNOUNCE  (5)
#define LEN_JOIN  (5)
#define LEN_READY  (5)
#define LEN_PONG  (5 + sizeof(struct pong_t))
#define LEN_BOARD  (5 + sizeof(struct board_t))
#define LEN_BALLBOARD  (5 + sizeof(struct ball_board_t))
#define LEN_SCORE  (5 + sizeof(struct score_t))

#define LOBBY_SPINNER_OFFSET VGA_OFFSET(VGA_COLS / 2 - 1, VGA_ROWS / 2)

void game_init(void) {
    vga_clear(COLOR(COLOR_WHITE, COLOR_BLACK));
    game_state = STATE_LOBBY;

    uip_ipaddr_t addr;
    addr[0] = 0xffff;
    addr[1] = 0xffff;
    conn = uip_udp_new(&addr, HTONS(UDP_PORT));
}

static void lobby_process(void);
static void score_process(void);
static void pong_process(void);

void game_process(void) {
    switch (game_state) {
    case STATE_LOBBY: lobby_process(); break;
    case STATE_SCORE: score_process(); break;
    case STATE_GAME: pong_process(); break;
    }
}

static void lobby_appcall(void);
static void score_appcall(void);
static void pong_appcall(void);

void game_appcall(void) {
    switch (game_state) {
    case STATE_LOBBY: lobby_appcall(); break;
    case STATE_SCORE: score_appcall(); break;
    case STATE_GAME: pong_appcall(); break;
    }
}

static void show_score(bool set_left_ready, bool set_right_ready);
static void start_round(void);
static void enter_lobby(void);

static struct timer conn_timeout_timer;

static void pong_appcall(void) {
    if (uip_udp_conn->rport == HTONS(UDP_PORT)) {
        if (uip_poll()) {
            struct net_message_t *msg = (struct net_message_t *)uip_appdata;
            msg->cookie = MAGIC_COOKIE;
            msg->tag = BALLBOARD;
            msg->ball_board.ball_x = pong_ball_x;
            msg->ball_board.ball_y = pong_ball_y;
            msg->ball_board.ball_speed_x = pong_ball_speed_x;
            msg->ball_board.ball_speed_y = pong_ball_speed_y;
            msg->ball_board.board_col = board_left_row;
            uip_udp_send(LEN_BALLBOARD);
        } else if (uip_newdata()) {
            timer_set(&conn_timeout_timer, CONN_TIMEOUT);
            if (uip_datalen() >= 5) {
                struct net_message_t *msg = (struct net_message_t *)uip_appdata;
                if (msg->cookie == MAGIC_COOKIE) {
                    switch (msg->tag) {
                    case BOARD:
                        pong_set_right_board(msg->board.board_col);
                        break;
                    case PONG:
                        if (pong_ball_speed_x > 0) {
                            // Ball is flying right
                            pong_ball_bounce_right(msg->pong.ball_x, msg->pong.ball_y, msg->pong.ball_speed_y);
                            msg->tag = BALLBOARD;
                            // msg->ball_board.ball_x = pong_ball_x;
                            // msg->ball_board.ball_y = pong_ball_y;
                            msg->ball_board.ball_speed_x = pong_ball_speed_x;
                            msg->ball_board.ball_speed_y = pong_ball_speed_y;
                            msg->ball_board.board_col = board_left_row;
                            uip_udp_send(LEN_BALLBOARD);
                        } else {
                            // Discard extra message
                        }
                        break;
                    case JOIN:
                        score_left += 1;
                        show_score(false, false);
                        break;
                    default: break;
                    }
                }
            }
        }
    }
}

static void score_appcall(void) {
    if (uip_udp_conn->rport == HTONS(UDP_PORT)) {
        if (uip_poll()) {
            struct net_message_t *msg = (struct net_message_t *)uip_appdata;
            msg->cookie = MAGIC_COOKIE;
            if (!left_ready) {
                msg->tag = SCORE;
                msg->score.score_l = score_left;
                msg->score.score_r = score_right;
                uip_udp_send(LEN_SCORE);
            } else {
                msg->tag = READY;
                uip_udp_send(LEN_READY);
            }
        } else if (uip_newdata()) {
            timer_set(&conn_timeout_timer, CONN_TIMEOUT);
            if (uip_datalen() >= 5) {
                struct net_message_t *msg = (struct net_message_t *)uip_appdata;
                if (msg->cookie == MAGIC_COOKIE) {
                    switch (msg->tag) {
                    case READY:
                        show_score(false, true);
                        break;
                    default:
                        break;
                    }
                }
            }
        }
    }
}

static void lobby_appcall(void) {
    if (uip_udp_conn->rport == HTONS(UDP_PORT)) {
        if (uip_poll()) {
            struct net_message_t *msg = (struct net_message_t *)uip_appdata;
            msg->cookie = MAGIC_COOKIE;
            msg->tag = ANNOUNCE;
            uip_udp_send(LEN_ANNOUNCE);
        } else if (uip_newdata()) {
            timer_set(&conn_timeout_timer, CONN_TIMEOUT);
            if (uip_datalen() >= 5) {
                struct net_message_t *msg = (struct net_message_t *)uip_appdata;
                if (msg->cookie == MAGIC_COOKIE) {
                    // Set remote address of the connection to the source address of this packet
                    struct uip_udpip_hdr *hdr = (struct uip_udpip_hdr *)&uip_buf[UIP_LLH_LEN];
                    uip_udp_conn->ripaddr[0] = hdr->srcipaddr[0];
                    uip_udp_conn->ripaddr[1] = hdr->srcipaddr[1];

                    switch (msg->tag) {
                    case JOIN:
                        show_score(false, false);
                        break;
                    case READY: // missed the join packet
                        show_score(false, true);
                        break;
                    default:
                        uip_log("unexpected message");
                        break;
                    }
                } else {
                    uip_log("bad cookie");
                }
            } else {
                uip_log("short message");
            }
        }
    }
}

static void pong_process(void) {
    u8_t key = ps2_get_key_event();
    switch (key) {
    case PS2_KEY_W:
    case PS2_KEY_UP:
        pong_input(INPUT_UP_PRESSED);
        break;
    case PS2_KEY_W | PS2_KEY_RELEASE:
    case PS2_KEY_UP | PS2_KEY_RELEASE:
        pong_input(INPUT_UP_RELEASED);
        break;
    case PS2_KEY_S:
    case PS2_KEY_DOWN:
        pong_input(INPUT_DOWN_PRESSED);
        break;
    case PS2_KEY_S | PS2_KEY_RELEASE:
    case PS2_KEY_DOWN | PS2_KEY_RELEASE:
        pong_input(INPUT_DOWN_RELEASED);
        break;
    default:
        break;
    }
    enum pong_result_t result = pong_step();
    switch (result) {
    case PONG_OK: break;
    case PONG_LEFT_LOST:
        score_right += 1;
        show_score(false, false);
        break;
    }
    if (timer_expired(&conn_timeout_timer)) {
        enter_lobby();
    }
}

static void lobby_process(void) {
    static uint16_t count = 0;
    ps2_get_key_event();

    VGA_CHAR_SEG[LOBBY_SPINNER_OFFSET + 0] = CHAR_TOP;
    VGA_CHAR_SEG[LOBBY_SPINNER_OFFSET + 1] = CHAR_TOP;

    uint16_t color;
    switch ((count >> 10) & 0x3) {
    case 0: color = (COLOR_LIGHT_RED << 12) | (COLOR_YELLOW << 8) | (COLOR_GREEN << 0) | (COLOR_LIGHT_CYAN << 4); break;
    case 1: color = (COLOR_LIGHT_RED << 8) | (COLOR_YELLOW << 0) | (COLOR_GREEN << 4) | (COLOR_LIGHT_CYAN << 12); break;
    case 2: color = (COLOR_LIGHT_RED << 0) | (COLOR_YELLOW << 4) | (COLOR_GREEN << 12) | (COLOR_LIGHT_CYAN << 8); break;
    case 3: color = (COLOR_LIGHT_RED << 4) | (COLOR_YELLOW << 12) | (COLOR_GREEN << 8) | (COLOR_LIGHT_CYAN << 0); break;
    }
    *(uint16_t*)(VGA_COLOR_SEG + LOBBY_SPINNER_OFFSET) = color;
    count += 3;
}

static void score_process(void) {
    u8_t key = ps2_get_key_event();
    if (key == PS2_KEY_ENTER) {
        show_score(true, false);
    }
    if (timer_expired(&conn_timeout_timer)) {
        enter_lobby();
    }
}

#define READY_ROW 20
#define SCORE_ROW 7
#define READY_LENGTH 6
static void show_score(bool set_left_ready, bool set_right_ready) {
    if (game_state != STATE_SCORE) {
        left_ready = false;
        right_ready = false;
        vga_clear(COLOR(COLOR_WHITE, COLOR_BLACK));
        strcpy(VGA_CHAR_SEG + VGA_OFFSET(VGA_COLS / 4 - READY_LENGTH / 2, READY_ROW), "Ready?");
        strcpy(VGA_CHAR_SEG + VGA_OFFSET(3 * VGA_COLS / 4 - READY_LENGTH / 2, READY_ROW), "Ready?");

        bd_draw_number_centered(VGA_COLS / 4 + 1, SCORE_ROW, score_left);
        bd_draw_number_centered(3 * VGA_COLS / 4 + 1, SCORE_ROW, score_right);
    }
    left_ready |= set_left_ready;
    right_ready |= set_right_ready;

    if (left_ready) {
        VGA_CHAR_SEG[VGA_OFFSET(VGA_COLS / 4 + READY_LENGTH / 2 - 1, READY_ROW)] = '!';
    }
    if (right_ready) {
        VGA_CHAR_SEG[VGA_OFFSET(3 * VGA_COLS / 4 + READY_LENGTH / 2 - 1, READY_ROW)] = '!';
    }

    if (left_ready & right_ready) {
        start_round();
    } else {
        game_state = STATE_SCORE;
    }
}

static void start_round(void) {
    vga_clear(COLOR(COLOR_WHITE, COLOR_BLACK));
    pong_init();
    game_state = STATE_GAME;
}

static void enter_lobby(void) {
    vga_clear(COLOR(COLOR_WHITE, COLOR_BLACK));
    conn->ripaddr[0] = 0xffff;
    conn->ripaddr[1] = 0xffff;
    score_left = 0;
    score_right = 0;
    left_ready = false;
    right_ready = false;
    game_state = STATE_LOBBY;
}
