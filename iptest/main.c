#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <libsys/vga.h>
#include <libsys/eth.h>
#include <libsys/crc.h>
#include <libsys/ps2keyboard.h>

#include "uip.h"
#include "uip_arp.h"
#include "clock-arch.h"

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
    strcpy(VGA_CHAR_SEG + VGA_OFFSET(5, VGA_ROWS - 2), s);
    print_hex(VGA_CHAR_SEG + VGA_OFFSET(0, VGA_ROWS - 2), counter >> 8);
    print_hex(VGA_CHAR_SEG + VGA_OFFSET(2, VGA_ROWS - 2), counter);
    counter += 1;
}

void iptest_appinit(void) {
    // uip_listen(HTONS(1234));
}

void iptest_appcall(void) {
    // if (uip_newdata() || uip_rexmit()) {
    //     uip_send("ok\n", 3);
    // }
}

void iptest_appcall_udp(void) {
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

static void print_header(void) {
    print_hex(VGA_CHAR_SEG + VGA_OFFSET(0, VGA_ROWS - 1), BUF->type);
    print_hex(VGA_CHAR_SEG + VGA_OFFSET(2, VGA_ROWS - 1), BUF->type >> 8);
}

#define TIMER_PERIOD 160 // 1/32 sec
#define PERIODIC_PERIOD 32 // each second
#define ARP_PERIOD 320 // each 10 seconds

static_assert(UIP_CONNS < 256, "UIP_CONNS must fit into byte");
static_assert(UIP_UDP_CONNS < 256, "UIP_UDP_CONNS must fit into byte");


void dump_packet(void) {
    char *p = VGA_CHAR_SEG + VGA_OFFSET(0, VGA_ROWS - 3);
    for (uint8_t i = 0; i != 34; ++i, p += 2) {
        print_hex(p, uip_buf[i]);
    }
}

void set_color(void) {
    char *p = VGA_COLOR_SEG + VGA_OFFSET(14 * 2, VGA_ROWS - 3);
    for (uint8_t i = 0; i != 20; ++i, p += 2) {
        uint8_t color = i & 1 ? COLOR(COLOR_WHITE, COLOR_RED) : COLOR(COLOR_WHITE, COLOR_GREEN);
        p[0] = color;
        p[1] = color;
    }
}

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

    // uip_ipaddr_t ipaddr;
    // uip_ipaddr(ipaddr, 192,168,178,239);
    // uip_sethostaddr(ipaddr);
    // uip_ipaddr(ipaddr, 255,255,255,0);
    // uip_setnetmask(ipaddr);

    iptest_appinit();
    dhcpc_init(eth_mac, 6);

    uint8_t r = 0;
    uint8_t c = 0;

    while (true) {
        char indicator = 0;
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
                indicator = 'I';
            } else if (BUF->type == HTONS(UIP_ETHTYPE_ARP)) {
                uip_arp_arpin();
                /* If the above function invocation resulted in data that
                     should be sent out on the network, the global variable
                     uip_len is set to a value > 0. */
                if(uip_len > 0) {
                    network_device_send();
                }
                indicator = 'A';
            } else {
                indicator = '+';
            }
        } else if (periodic_count == PERIODIC_PERIOD) {
            loop_count += 4;
            periodic_count = 0;
            arp_count += 1;
            indicator = 'P';
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
                indicator = 'R';
            }
        } else {
            ++loop_count;
        }
        if (loop_count >= TIMER_PERIOD) {
            time += 1;
            periodic_count += 1;
            loop_count = 0;
        }

        if (indicator) {
            VGA_CHAR_SEG[VGA_OFFSET(c,r)] = indicator;
            c += 1;
            if (c == VGA_COLS) {
                c = 0;
                r += 1;
                if (r == VGA_ROWS) {
                    r = 0;
                }
            }
        }
    }
}

void dhcpc_configured(const struct dhcpc_state *s) {
    uip_log("DHCP configured");
    uip_sethostaddr(s->ipaddr);
    uip_setnetmask(s->netmask);
    uip_setdraddr(s->default_router);

    char buf[4 * 4 + 1];
    __uitoa(s->ipaddr[0] & 0xff, buf + 0, 10);
    __uitoa(s->ipaddr[0] >> 8, buf + 4, 10);
    __uitoa(s->ipaddr[1] & 0xff, buf + 8, 10);
    __uitoa(s->ipaddr[1] >> 8, buf + 12, 10);
    memcpy(VGA_CHAR_SEG + VGA_OFFSET(0, VGA_ROWS - 3), buf, 16);
}

int getchar(void) { return 0; }
int putchar(int c) { return 0; }
