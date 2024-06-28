//
// Created by 韩呈杰 on 2023/3/21.
//

#ifndef ARGUS_GOOGLETEST_SOCKETUTILS_H
#define ARGUS_GOOGLETEST_SOCKETUTILS_H
#include <string>

std::string SocketFamilyName(int family);
std::string SocketTypeName(int type);
std::string SocketProtocolName(int protocol);

std::string SocketDesc(const char *szFunction, int family, int type, int protocol);

#endif //ARGUS_GOOGLETEST_SOCKETUTILS_H
