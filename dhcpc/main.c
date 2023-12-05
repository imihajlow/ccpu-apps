#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <libsys/vga.h>
#include <libsys/eth.h>
#include <libsys/crc.h>
#include <libsys/ps2keyboard.h>
#include <libsys/fat/fat.h>

#include "uip.h"
#include "uip_arp.h"
#include "clock-arch.h"

#define CFG_FILE "/IP.CFG"

void iptest_appcall_udp(void) {
    dhcpc_appcall();
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

    dhcpc_init(eth_mac, 6);

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
    }
}

static char *ip2str(char *dst, uip_ip4addr_t *addr) {
    uint8_t b1 = (*addr)[0];
    uint8_t b2 = (*addr)[0] >> 8;
    uint8_t b3 = (*addr)[1];
    uint8_t b4 = (*addr)[1] >> 8;
    dst = __uitoa(b1, dst, 10);
    *dst = '.';
    ++dst;
    dst = __uitoa(b2, dst, 10);
    *dst = '.';
    ++dst;
    dst = __uitoa(b3, dst, 10);
    *dst = '.';
    ++dst;
    dst = __uitoa(b4, dst, 10);
    return dst;
}

static char hi_buf[256] __attribute__((section("bss_hi")));

static bool save_config(const struct dhcpc_state *s) {
    strcpy(hi_buf, CFG_FILE);
    uint8_t fd = fat_open_path(hi_buf, O_CREAT);
    if (fd == FAT_BAD_DESC) {
        return false;
    }

    bool result = true;
    char *p = hi_buf;
    p = ip2str(p, &s->ipaddr);
    *p = '\n';
    ++p;
    p = ip2str(p, &s->netmask);
    *p = '\n';
    ++p;
    p = ip2str(p, &s->default_router);
    *p = '\n';
    ++p;
    p = ip2str(p, &s->dnsaddr);
    *p = '\n';
    ++p;

    if (!fat_write(fd, hi_buf, p - hi_buf))
    {
        result = false;
        goto done;
    }

    if (!fat_truncate(fd))
    {
        result = false;
        goto done;
    }

done:
    fat_close(fd);
    return result;
}

void dhcpc_configured(const struct dhcpc_state *s) {
    char buf[16];
    ip2str(buf, &s->ipaddr);
    strcpy(VGA_CHAR_SEG + VGA_OFFSET(0, 0), buf);
    ip2str(buf, &s->netmask);
    strcpy(VGA_CHAR_SEG + VGA_OFFSET(0, 1), buf);
    ip2str(buf, &s->default_router);
    strcpy(VGA_CHAR_SEG + VGA_OFFSET(0, 2), buf);
    ip2str(buf, &s->dnsaddr);
    strcpy(VGA_CHAR_SEG + VGA_OFFSET(0, 3), buf);

    if (save_config(s)) {
        syscall(12);
    } else {
        strcpy(VGA_CHAR_SEG + VGA_OFFSET(0, 4), "error");
    }
}

int getchar(void) { return 0; }
int putchar(int c) { return 0; }
