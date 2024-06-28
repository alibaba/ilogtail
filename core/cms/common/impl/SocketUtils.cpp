//
// Created by 韩呈杰 on 2023/3/21.
//
#include "common/SocketUtils.h"

#include <unordered_map>
#include <sstream>

#ifdef WIN32
#   include <winsock2.h>
#else
#   include <sys/socket.h>
#endif

static std::string query(int v, const std::unordered_map<int, std::string> &data) {
    std::string name;

    auto it = data.find(v);
    if (it != data.end()) {
        name = it->second;
    } else {
        std::stringstream ss;
        ss << "UNKNOWN(" << v << ")";
        name = ss.str();
    }
    return name;
}

#define MAP_ENTRY(N) {(N), #N}
static const auto &familyMap = *new std::unordered_map<int, std::string>{
        MAP_ENTRY(AF_UNSPEC),
        MAP_ENTRY(AF_UNIX),
#ifndef WIN32
        MAP_ENTRY(AF_LOCAL),
#endif
        MAP_ENTRY(AF_INET),
        MAP_ENTRY(AF_INET6),
        MAP_ENTRY(AF_IPX),
        MAP_ENTRY(AF_APPLETALK),
        MAP_ENTRY(AF_APPLETALK),
#ifdef AF_NETBIOS
        MAP_ENTRY(AF_NETBIOS),
#endif
#if defined(WIN32)
        MAP_ENTRY(AF_IRDA),
        MAP_ENTRY(AF_BTH),
#elif defined(__linux__)
        MAP_ENTRY(AF_X25),
        MAP_ENTRY(AF_NETLINK),
        MAP_ENTRY(AF_PACKET),
#endif
};

std::string SocketFamilyName(int family) {
    return query(family, familyMap);
}

static const auto &typeMap = *new std::unordered_map<int, std::string>{
        MAP_ENTRY(SOCK_STREAM),
        MAP_ENTRY(SOCK_DGRAM),
        MAP_ENTRY(SOCK_SEQPACKET),
        MAP_ENTRY(SOCK_RAW),
        MAP_ENTRY(SOCK_RDM),
#ifdef SOCK_PACKET
        MAP_ENTRY(SOCK_PACKET),
#endif
#ifdef SOCK_CLOEXEC
        MAP_ENTRY(SOCK_CLOEXEC),
#endif
#ifdef SOCK_NONBLOCK
        MAP_ENTRY(SOCK_NONBLOCK),
#endif
};

std::string SocketTypeName(int type) {
    return query(type, typeMap);
}

// 以下宏定义来源于 https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-socket
#ifndef IPPROTO_ICMP
#   define IPPROTO_ICMP 1
#endif
#ifndef IPPROTO_IGMP
#   define IPPROTO_IGMP 2
#endif
#ifndef BTHPROTO_RFCOMM
#   define BTHPROTO_RFCOMM 3
#endif
#ifndef IPPROTO_TCP
#   define IPPROTO_TCP 6
#endif
#ifndef IPPROTO_UDP
#   define IPPROTO_UDP 17
#endif
#ifndef IPPROTO_ICMPV6
#   define IPPROTO_ICMPV6 58
#endif
#ifndef IPPROTO_RM
#   define IPPROTO_RM 113
#endif
static auto &protocolMap = *new std::unordered_map<int, std::string>{
        MAP_ENTRY(IPPROTO_ICMP),
        MAP_ENTRY(IPPROTO_IGMP),
        MAP_ENTRY(BTHPROTO_RFCOMM),
        MAP_ENTRY(IPPROTO_TCP),
        MAP_ENTRY(IPPROTO_UDP),
        MAP_ENTRY(IPPROTO_ICMPV6),
        MAP_ENTRY(IPPROTO_RM),
};

std::string SocketProtocolName(int protocol) {
    return query(protocol, protocolMap);
}

#undef MAP_ENTRY

std::string SocketDesc(const char *szFunction, int family, int type, int protocol) {
    std::stringstream ss;
    ss << (szFunction == nullptr ? "socket" : szFunction) << "("
       << SocketFamilyName(family) << ", "
       << SocketTypeName(type) << ", "
       << SocketProtocolName(protocol)
       << ")";
    return ss.str();
}
