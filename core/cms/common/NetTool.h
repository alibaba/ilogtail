//
// Created by 韩呈杰 on 2023/10/24.
//

#ifndef ARGUSAGENT_NETTOOL_H
#define ARGUSAGENT_NETTOOL_H

#include <string>

namespace IP {
    bool IsIPv6(const std::string &);

    bool IsGlobalUniCast(const std::string &, bool *failed = nullptr);
    bool IsGlobalUniCastV4(const std::string &ip, bool *failed = nullptr);
    bool IsGlobalUniCastV6(const std::string &ip, bool *failed = nullptr);

    bool IsPrivate(const std::string &ip, bool *failed = nullptr);
    bool IsPrivateV4(const std::string &ip, bool *failed = nullptr);
    bool IsPrivateV6(const std::string &ip, bool *failed = nullptr);
}
#endif //ARGUSAGENT_NETTOOL_H
