//
// Created by 韩呈杰 on 2023/4/3.
//

#ifndef ARGUSAGENT_EXCEPTIONS_H
#define ARGUSAGENT_EXCEPTIONS_H

#if !defined(_WINDOWS)
#   if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64)
#       define _WINDOWS
#   endif
#endif

#if defined(_WINDOWS)
#   include <Windows.h>
#   ifdef max
#       undef max
#       undef min
#   endif
#endif
#include <string>
#include <exception>
#include <functional>
#include <vector>
#include <boost/json.hpp>
#include "common/FilePathUtils.h"

void SetupProcess();
void SetupThread(const std::string &threadName = {});

std::string new_dump_file_path(int signo = 0);

void printException(const char *file, size_t line, const char *func, const std::exception &ex);
#define PRINT_EXCEPTION(ex) printException(__FILE__, __LINE__, __FUNCTION__, (ex))

// 在try{}catch{}或Windows SEH中安全运行
void safeRun(const std::function<void()> &fn);
void safeRunEnd();

const char *signalName(int signo, const char *def = nullptr);
std::string signalPrompt(int signo);
void printStacktrace(int signo, const std::string &caller, size_t skip = 1);

struct tagCoreDumpFile {
    std::string dumpTime;
    std::string filename; // 全路径
    uint64_t filesize;
    std::string callstackType; // Base64, Text
    std::string callstack; // 调用栈，有可能是二进制的base64编码(Windows)

    boost::json::object toJson() const;
};

class DumpCache {
    const bool clean;
    const size_t capacity;
    std::vector<fs::path> dumpFiles;
    unsigned int removeCount = 0;

public:
    explicit DumpCache(size_t cap, bool readonly = false);

    const std::vector<fs::path> &DumpFiles() const {
        return dumpFiles;
    }

    bool PrintLastCoreDown();
    tagCoreDumpFile GetLastCoreDown(std::chrono::seconds timeout = std::chrono::hours{1}) const;
private:
    void Append(const fs::path &dumpFile);
    void Clean();
};
#endif //ARGUSAGENT_EXCEPTIONS_H
