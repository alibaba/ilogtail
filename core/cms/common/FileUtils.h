#ifndef ARGUS_AGENT_FILE_UTILS_H
#define ARGUS_AGENT_FILE_UTILS_H

#include <string>
#include <vector>
#include <functional>
#include "common/ArgusMacros.h"
#include "common/FilePathUtils.h"
#include "common/Defer.h"

int GetLines(std::istream &is, bool enableEmptyLine,
              const std::function<void(const std::string &)> &pushBack,
              std::string *errorMessage);
int GetLines(const fs::path &filename, bool enableEmptyLine,
              const std::function<void(const std::string &)> &pushBack,
              std::string *errorMessage);

template<typename TSrc, typename TContainer>
int GetLines(TSrc &&is, TContainer &res, bool enableEmptyLine = true, std::string *errorMessage = nullptr) {
    return GetLines(is, enableEmptyLine, [&res](const std::string &s) { res.push_back(s); }, errorMessage);
}

template<typename TContainer>
int GetFileLines(const fs::path &filename, TContainer &res, bool enableEmptyLine = true,
                 std::string *errorMessage = nullptr) {
    return GetLines(filename, enableEmptyLine, [&res](const std::string &s) { res.push_back(s); }, errorMessage);
}

template<typename TContainer>
int GetFileLines(const fs::path &file, TContainer &res, bool enableEmptyLine, std::string &errorMessage) {
    return GetFileLines(file, res, enableEmptyLine, &errorMessage);
}

static std::vector<std::string> GetFileLines(const fs::path &filename, bool enableEmptyLine, std::string *errMsg) {
    std::vector<std::string> ret;
    GetFileLines(filename, ret, enableEmptyLine, errMsg);
    RETURN_RVALUE(ret);
}

static std::vector<std::string> GetFileLines(const fs::path &path, bool enableEmptyLine, std::string &errorMessage) {
    return GetFileLines(path, enableEmptyLine, &errorMessage);
}

std::string ReadFileContent(const fs::path &file, std::string *errorMessage = nullptr);
std::vector<char> ReadFileBinary(const fs::path &file, std::string *errorMessage = nullptr);

inline std::vector<std::string> ReadFileLines(const fs::path &file) {
    return GetFileLines(file, true, nullptr);
}

bool WriteFileContent(const fs::path &file, const std::string &content, boost::system::error_code *ec = nullptr);

bool IsDir(const fs::path &path);
bool IsFile(const fs::path &path);

namespace common {
    namespace FileUtils {
        int ReadFileContent(const fs::path &filename, std::string &fileContent);
        static inline int ReadFileContent(const std::string &filename, std::string &fileContent) {
            return ReadFileContent(fs::path(filename), fileContent);
        }
        static inline int ReadFileContent(const char *filename, std::string &fileContent) {
            return ReadFileContent(fs::path(filename), fileContent);
        }
        int ReadFileLines(const fs::path &file, std::vector<std::string> &lines);
        static inline int ReadFileLines(const std::string &file, std::vector<std::string> &lines) {
            return ReadFileLines(fs::path(file), lines);
        }
        static inline int ReadFileLines(const char *file, std::vector<std::string> &lines) {
            return ReadFileLines(fs::path(file), lines);
        }
        int WriteFileContent(const fs::path &filename, const std::string &fileContent);
        static inline int WriteFileContent(const std::string &filename, const std::string &fileContent) {
            return WriteFileContent(fs::path(filename), fileContent);
        }
        static inline int WriteFileContent(const char *filename, const std::string &fileContent) {
            return WriteFileContent(fs::path(filename), fileContent);
        }
        bool isDir(const std::string &path);
        bool isFile(const std::string &path);
    };
}


class File {
    FILE *fp = nullptr;
public:
    class Mode {
        friend class File;
        explicit Mode(const char* t) :mode(t) {}
    public:
        const char* const mode;
    };
    static const Mode ModeReadText;
    static const Mode ModeWriteText;
    static const Mode ModeReadBin;
    static const Mode ModeWriteBin;

public:
    explicit File(const fs::path &file, const Mode &mode = ModeReadText);
    ~File();

    // fread
    size_t Read(char *buf, size_t capacity) const;
    // fwrite
    size_t Write(const char* buf, size_t size);
    size_t Write(const std::string &s) {
        return Write(s.c_str(), s.size());
    }

    explicit operator bool() const {
        return IsOpen();
    }
    bool IsOpen() const {
        return fp != nullptr;
    }

    FILE *Get() const {
        return fp;
    }

    // 是否已达文件尾部
    bool IsEnd() const;
    bool IsError() const;

    // fileno, 文件未打开，或打开失败，则返回-1
    int FileNo() const;
    FILE *Fp() const {
        return fp;
    }
//#ifdef WIN32
//    // 文件未打开，或打开失败，则返回 INVALID_HANDLE_VALUE(-1）
//    HANDLE Handle() const;
//#endif
};

#endif
