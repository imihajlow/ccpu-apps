#pragma once
#include <stdint.h>

/**
 * 8 bit datatype
 *
 * This typedef defines the 8-bit type used throughout uIP.
 *
 * \hideinitializer
 */
typedef uint8_t u8_t;

/**
 * 16 bit datatype
 *
 * This typedef defines the 16-bit type used throughout uIP.
 *
 * \hideinitializer
 */
typedef uint16_t u16_t;

/**
 * 32 bit datatype
 *
 * This typedef defines the 32-bit type used throughout uIP.
 *
 * \hideinitializer
 */
typedef uint32_t u32_t;

/**
 * Statistics datatype
 *
 * This typedef defines the dataype used for keeping statistics in
 * uIP.
 *
 * \hideinitializer
 */
typedef unsigned short uip_stats_t;

/**
 * Maximum number of TCP connections.
 *
 * \hideinitializer
 */
#define UIP_CONF_MAX_CONNECTIONS 40

/**
 * Maximum number of listening TCP ports.
 *
 * \hideinitializer
 */
#define UIP_CONF_MAX_LISTENPORTS 2

/**
 * uIP buffer size.
 *
 * \hideinitializer
 */
#define UIP_CONF_BUFFER_SIZE     900
#define UIP_CONF_EXTERNAL_BUFFER

/**
 * CPU byte order.
 *
 * \hideinitializer
 */
#define UIP_CONF_BYTE_ORDER      LITTLE_ENDIAN

/**
 * Logging on or off
 *
 * \hideinitializer
 */
#define UIP_CONF_LOGGING         0

/**
 * UDP support on or off
 *
 * \hideinitializer
 */
#define UIP_CONF_UDP             1

#define UIP_CONF_TCP             0

#define UIP_CONF_ACTIVE_OPEN     1

/**
 * UDP checksums on or off
 *
 * \hideinitializer
 */
#define UIP_CONF_UDP_CHECKSUMS   1

/**
 * uIP statistics on or off
 *
 * \hideinitializer
 */
#define UIP_CONF_STATISTICS      1

#define UIP_CONF_FIXEDETHADDR    1

#define UIP_ETHADDR0 0xFE
#define UIP_ETHADDR1 0xFA
#define UIP_ETHADDR2 0xF6
#define UIP_ETHADDR3 0xF2
#define UIP_ETHADDR4 0xEE
#define UIP_ETHADDR5 0xEA

#define UIP_UDP_APPCALL     iptest_appcall_udp

typedef int uip_tcp_appstate_t;
typedef int uip_udp_appstate_t;

void iptest_appcall(void);
void iptest_appcall_udp(void);

#include "uip/apps/dhcpc/dhcpc.h"
