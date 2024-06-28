//
// Created by 韩呈杰 on 2024/5/11.
// 独立小型依赖by win32service
//

#ifdef WIN32
#ifndef ARGUSAGENT_WIN32_SIC_UTILS_H
#define ARGUSAGENT_WIN32_SIC_UTILS_H
#include <Windows.h>
#include <string>
#include "common/FilePathUtils.h"

enum class EnumDirectoryType {
    System32,
    Windows,
    ProgramFiles,
};
fs::path GetSpecialDirectory(EnumDirectoryType);
static inline fs::path System32() {
    return GetSpecialDirectory(EnumDirectoryType::System32);
}

std::string ErrorStr(DWORD errCode, const std::string& functionName);
DWORD LastError(const std::string &functionName, std::string* errMsg);

std::string WcharToChar(const wchar_t* wideCharStr, DWORD CodePage = CP_ACP);

#endif //ARGUSAGENT_WIN32_SIC_UTILS_H
#endif // WIN32
