#ifndef ARGUS_COMMON_FILE_PATH_UTILS_H
#define ARGUS_COMMON_FILE_PATH_UTILS_H

#include <string>
#include <cstdint> // std::uintmax_t
#include <chrono>
#include <boost/filesystem.hpp>
#include "common/StringUtils.h"

namespace fs = boost::filesystem;

// 获取程序的绝对路径(含文件名)
fs::path GetExecPath();
fs::path GetExecDir();
// argus的安装路径
fs::path GetBaseDir();

#if !defined(WIN32)
fs::path Which(const std::string &binFile, const std::initializer_list<fs::path> & = {});
std::string ReadLink(const std::string &path);
#endif

bool RemoveFile(const fs::path &file);

template<typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
fs::path operator/(const fs::path &lhs, const T &rhs) {
    return lhs / convert(rhs);
}

template<typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
fs::path operator/(const T &lhs, const fs::path &rhs) {
    return convert(lhs) / rhs;
}

namespace common {

    class FilePathUtils {
    public:
        static std::string GetDirPath(const std::string &filePath);
        static std::string GetFileName(const std::string &filePath);
        static std::string GetFilePath(const std::string &dir, const std::string &fileName);
        // static apr_finfo_t GetFileInfo(const std::string &filePath);
        static std::uintmax_t GetFileSize(const std::string &filePath);
        // 文件修改时间
        static std::chrono::system_clock::time_point GetFileMTime(const std::string &filePath) {
            return GetFileMTime(boost::filesystem::path{filePath});
        }
        static std::chrono::system_clock::time_point GetFileMTime(const boost::filesystem::path &filePath);
        static bool SetFileMTime(const boost::filesystem::path &file,
                                 const std::chrono::system_clock::time_point &mtime);
        static bool SetFileMTime(const boost::filesystem::path &file, std::chrono::seconds utcTimestamp) {
            return SetFileMTime(file, std::chrono::system_clock::time_point{utcTimestamp});
        }
        static std::string GetLastFile(const std::string &dir, const std::string &fileName,
                                       std::chrono::system_clock::time_point lastSaveTime);
    };
}

#endif // !ARGUS_COMMON_FILE_PATH_UTILS_H
