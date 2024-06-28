//
// Created by 韩呈杰 on 2024/3/13.
//

#ifndef ARGUSAGENT_PROTOCOL_ADT_H
#define ARGUSAGENT_PROTOCOL_ADT_H

#include <cstdint>
#include <boost/predef/other/endian.h> // BOOST_ENDIAN_BIG_BYTE, BOOST_ENDIAN_LITTLE_BYTE

struct ip {
#if BOOST_ENDIAN_LITTLE_BYTE // BYTE_ORDER == LITTLE_ENDIAN
    uint8_t ip_hl: 4, /* header length */
            ip_v : 4;  /* version */
#endif
#if BOOST_ENDIAN_BIG_BYTE // BYTE_ORDER == BIG_ENDIAN
    uint8_t ip_v : 4, /* version */
            ip_hl: 4;/* header length */
#endif
    uint8_t ip_tos;     // Type of service
    uint16_t ip_len;    // Total length of the packet
    uint16_t ip_id;     // Unique identification
    uint16_t ip_offset; // Fragment offset
    uint8_t ip_ttl;     // Time to live
    uint8_t ip_p;       // Protocol (TCP, UDP etc)
    uint16_t ip_sum;    // IP checksum
    uint32_t ip_src;    // Source IP
    uint32_t dest_ip;   // Destination IP
};
static_assert(sizeof(ip) == 20, "unexpected sizeof(ip)");

#define ICMP_ECHO 8
#define ICMP_ECHOREPLY 0
struct icmp {
    uint8_t icmp_type;
    uint8_t icmp_code; // Type sub code
    uint16_t icmp_cksum;
    uint16_t icmp_id;
    uint16_t icmp_seq;
    uint32_t timestamp;
    uint32_t data[4];
};
static_assert(sizeof(icmp) == 28, "unexpected sizeof(icmp)");

#endif //ARGUSAGENT_PROTOCOL_ADT_H
