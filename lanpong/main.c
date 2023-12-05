#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <libsys/vga.h>
#include <libsys/eth.h>
#include <libsys/crc.h>
#include <libsys/ps2keyboard.h>
#include <libsys/syscall.h>

#include "../lib/ipcfg.h"

#include "uip.h"
#include "uip_arp.h"
#include "clock-arch.h"
#include "game.h"

static void configure_ip(void);

static uint8_t hex_digit(uint8_t x) {
    if (x < 10) {
        return x + '0';
    } else {
        return x - 10 + 'A';
    }
}

static void print_hex(char *dst, uint8_t x) {
    dst[0] = hex_digit(x >> 4);
    dst[1] = hex_digit(x & 0xf);
}

void print_mss(uint8_t i, uint16_t mss) {
    char *p = VGA_CHAR_SEG + VGA_OFFSET(0, VGA_ROWS - 4 - i);
    print_hex(p, i);
    print_hex(p + 3, mss >> 8);
    print_hex(p + 5, mss);
}

void uip_log(char *s) {
    static uint16_t counter = 0;
    strcpy(VGA_CHAR_SEG + VGA_OFFSET(VGA_COLS / 2 + 5, 0), s);
    print_hex(VGA_CHAR_SEG + VGA_OFFSET(VGA_COLS / 2 + 0, 0), counter >> 8);
    print_hex(VGA_CHAR_SEG + VGA_OFFSET(VGA_COLS / 2 + 2, 0), counter);
    counter += 1;
}

void iptest_appcall_udp(void) {
    game_appcall();
}

#define BUF ((struct uip_eth_hdr *)&uip_buf[0])

static void network_device_send(void) {
    eth_transmit(uip_buf, uip_len);
}

static uint16_t network_device_read(void) {
    if (!(ETH_SR & ETH_SR_RX_FULL)) {
        return 0;
    }
    uint16_t len = ETH_RX_LEN + 1;
    memcpy(uip_buf, ETH_BUF, len);
    if (len > UIP_BUFSIZE + 5) { // FCS 4 bytes, maybe 1 extra byte
        ETH_RX_ARM();
        return 0;
    } else if (!crc_check_relaxed(uip_buf, uip_buf + len)) {
        ETH_RX_ARM();
        return 0;
    } else {
        ETH_RX_ARM();
        return len - 4;
    }
}

#define TIMER_PERIOD 160 // 1/32 sec
#define PERIODIC_PERIOD 4 // 1/4 sec
#define ARP_PERIOD 1280 // each 10 seconds

static_assert(UIP_CONNS < 256, "UIP_CONNS must fit into byte");
static_assert(UIP_UDP_CONNS < 256, "UIP_UDP_CONNS must fit into byte");

static uint16_t time = 0;

clock_time_t clock_time(void) {
    return time;
}

void main(void)
{
    vga_clear(COLOR(COLOR_GRAY, COLOR_BLACK));
    eth_init();
    uint8_t loop_count = 0;
    uint16_t arp_count = 0;
    uint8_t periodic_count = 0;
    uip_init();
    uip_arp_init();

    configure_ip();
    game_init();

    while (true) {
        uip_len = network_device_read();
        if (uip_len > 0) {
            loop_count += 4;
            if (BUF->type == HTONS(UIP_ETHTYPE_IP)) {
                uip_arp_ipin();
                uip_input();
                /* If the above function invocation resulted in data that
                     should be sent out on the network, the global variable
                     uip_len is set to a value > 0. */
                if (uip_len > 0) {
                    uip_arp_out();
                    network_device_send();
                }
            } else if (BUF->type == HTONS(UIP_ETHTYPE_ARP)) {
                uip_arp_arpin();
                /* If the above function invocation resulted in data that
                     should be sent out on the network, the global variable
                     uip_len is set to a value > 0. */
                if(uip_len > 0) {
                    network_device_send();
                }
            }
        } else if (periodic_count == PERIODIC_PERIOD) {
            loop_count += 4;
            periodic_count = 0;
            arp_count += 1;
#if UIP_TCP
            for (uint8_t i = 0; i < UIP_CONNS; i++) {
                uip_periodic(i);
                /* If the above function invocation resulted in data that
                     should be sent out on the network, the global variable
                     uip_len is set to a value > 0. */
                if(uip_len > 0) {
                    uip_arp_out();
                    network_device_send();
                }
            }
#endif
#if UIP_UDP
            for(uint8_t i = 0; i < UIP_UDP_CONNS; i++) {
                uip_udp_periodic(i);
                /* If the above function invocation resulted in data that
                     should be sent out on the network, the global variable
                     uip_len is set to a value > 0. */
                if(uip_len > 0) {
                    uip_arp_out();
                    network_device_send();
                }
            }
#endif /* UIP_UDP */

            /* Call the ARP timer function every 10 seconds. */
            if (arp_count == ARP_PERIOD) {
                arp_count = 0;
                uip_arp_timer();
            }
        } else {
            ++loop_count;
        }
        if (loop_count >= TIMER_PERIOD) {
            time += 1;
            periodic_count += 1;
            loop_count = 0;
        }
        game_process();
    }
}

static void configure_ip(void) {
    struct ipcfg_t cfg;
    if (!ipcfg_load(&cfg)) {
        strcpy(VGA_CHAR_SEG, "Cannot load IP.CFG");
        ps2_wait_key_pressed();
        reboot();
    }
    uip_sethostaddr(cfg.ipaddr);
    uip_setnetmask(cfg.netmask);
    uip_setdraddr(cfg.default_router);
}

int getchar(void) { return 0; }
int putchar(int c) { return 0; }
