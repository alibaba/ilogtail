//
// Created by 韩呈杰 on 2023/10/24.
//
#include "common/NetTool.h"

#include <algorithm>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/address_v6.hpp>

using namespace boost::asio;

namespace IP {
    bool IsIPv6(const std::string &s) {
        // 关于IP地址格式: https://www.ibm.com/docs/zh/i/7.2?topic=concepts-ipv6-address-formats
        // 如果IPv6，则前5个字符中有冒号前5个字符中有冒号
        const char *ch = s.c_str();
        const char *const chEnd = ch + std::min((size_t) 5, s.size());
        for (; ch < chEnd; ++ch) {
            if (*ch == ':') {
                return true;
            }
        }
        return false;
    }

    bool IsGlobalUniCast(const std::string &s, bool *failed) {
        return (IsIPv6(s) ? IsGlobalUniCastV6 : IsGlobalUniCastV4)(s, failed);
    }

    bool IsGlobalUniCastV4(const std::string &ip, bool *failed) {
        // link local uni-cast
        auto is_link_local = [](const ip::address_v4 &ipv4) {
            auto bytes = ipv4.to_bytes();
            return bytes[0] == 169 && bytes[1] == 254;
        };

        boost::system::error_code ec;
        ip::address_v4 ipv4 = ip::make_address_v4(ip, ec);
        if (failed) {
            *failed = ec.failed();
        }
        if (!ec.failed()) {
            return !ipv4.is_loopback() // 127.0.0.1
                   && !ipv4.is_unspecified() // 0.0.0.0
                   && !(ipv4 == ip::address_v4::broadcast()) // 255.255.255.255
                   && !ipv4.is_multicast()
                   && !is_link_local(ipv4);
        }

        return false;
    }

    bool IsGlobalUniCastV6(const std::string &ip, bool *failed) {
        auto is_broadcast = [](const ip::address_v6 &ipv6) {
            auto bytes = ipv6.to_bytes();
            bool v4Inv6Prefix = bytes[0] == 0 && bytes[1] == 0 && bytes[2] == 0 && bytes[3] == 0 &&
                                bytes[4] == 0 && bytes[5] == 0 && bytes[6] == 0 && bytes[7] == 0 &&
                                bytes[8] == 0 && bytes[9] == 0 && bytes[10] == 0xff && bytes[11] == 0xff;
            if (v4Inv6Prefix) {
                auto v4bd = ip::address_v4::broadcast().to_bytes();
                return v4bd[0] == bytes[12] && v4bd[1] == bytes[13] &&
                       v4bd[2] == bytes[14] && v4bd[3] == bytes[15];
            }

            return false;
        };

        boost::system::error_code ec;
        ip::address_v6 ipv6 = ip::make_address_v6(ip, ec);
        if (failed) {
            *failed = ec.failed();
        }
        if (!ec.failed()) {
            return !ipv6.is_loopback()  // ::1
                   && !ipv6.is_unspecified() // ::0
                   && !is_broadcast(ipv6)
                   && !ipv6.is_multicast()
                   && !ipv6.is_link_local();  // link local uni-cast
        }

        return false;
    }

    bool IsPrivate(const std::string &ip, bool *failed) {
        return (IsIPv6(ip) ? IsPrivateV6 : IsPrivateV4)(ip, failed);
    }

    bool IsPrivateV4(const std::string &ip, bool *failed) {
        boost::system::error_code ec;
        ip::address_v4 ipv4 = ip::make_address_v4(ip, ec);
        if (failed) {
            *failed = ec.failed();
        }
        if (!ec.failed()) {
            // Following RFC 1918, Section 3. Private Address Space which says:
            //   The Internet Assigned Numbers Authority (IANA) has reserved the
            //   following three blocks of the IP address space for private internets:
            //     10.0.0.0        -   10.255.255.255  (10/8 prefix)
            //     172.16.0.0      -   172.31.255.255  (172.16/12 prefix)
            //     192.168.0.0     -   192.168.255.255 (192.168/16 prefix)
            auto bytes = ipv4.to_bytes();
            return bytes[0] == 10
                   || (bytes[0] == 172 && (bytes[1] & 0xf0) == 16)
                   || (bytes[0] == 192 && bytes[1] == 168)
#ifndef ENABLE_CLOUD_MONITOR
                || bytes[0] == 11
                || bytes[0] == 100 // @峯华、@泽承 他们认为11网段和100网段也是私有地址
#endif
                    ;
        }
        return false;
    }

    bool IsPrivateV6(const std::string &ip, bool *failed) {
        boost::system::error_code ec;
        ip::address_v6 ipv6 = ip::make_address_v6(ip, ec);
        if (failed) {
            *failed = ec.failed();
        }
        if (!ec.failed()) {
            // Following RFC 4193, Section 8. IANA Considerations which says:
            //   The IANA has assigned the FC00::/7 prefix to "Unique Local Unicast".
            auto bytes = ipv6.to_bytes();
            return (bytes[0] & 0xfe) == 0xfc;
        }
        return false;
    }
} // namespace IP