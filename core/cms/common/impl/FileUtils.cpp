#include "common/FileUtils.h"
#include "common/Logger.h"
#include "common/Common.h"

#include <sstream>
#include <fstream>

using namespace std;

int GetLines(std::istream &is, bool enableEmptyLine,
             const std::function<void(const std::string &)> &pushBack, std::string *errorMessage) {
    std::string line;
    // 此处必须判断eof，具体原因参见: https://stackoverflow.com/questions/40561482/getline-throws-basic-iosclear-exception-after-reading-the-last-line
    while (!is.eof() && std::getline(is, line)) {
        if (enableEmptyLine || !line.empty()) {
            pushBack(line);
        }
    }
    return 0;
}
int GetLines(const fs::path &filename, bool enableEmptyLine,
              const std::function<void(const std::string &)> &pushBack, std::string *errorMessage) {
    int ret = 0;
    try {
        std::ifstream fin;
        fin.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fin.open(filename.string(), std::ios_base::in);
        defer(fin.close());
        fin.exceptions(std::ifstream::goodbit);

        GetLines(fin, enableEmptyLine, pushBack, errorMessage);
    } catch (const std::exception &fail) {
        if (errorMessage != nullptr) {
            std::ostringstream oss;
            oss << "[" << common::typeName(&fail) << "] open file " << filename.string() << " failed: " << fail.what();
            *errorMessage = oss.str();
        }
        ret = -1;
    }
    return ret;
}

bool WriteFileContent(const fs::path &file, const std::string &content, boost::system::error_code *ec) {
    boost::system::error_code local_ec;
    if (ec == nullptr) {
        ec = &local_ec;
    }

    fs::path fileDir = file.parent_path();
    bool ok = fileDir.empty() || fs::exists(fileDir) || fs::create_directories(fileDir, *ec);
    if (ok) {
        std::ofstream ofs(file.string(), std::ios::out | std::ios::trunc);
        ok = ofs.is_open();
        if (ok) {
            defer(ofs.close());
            ofs << content;
        }

        if (ok) {
            // 0644
            const auto perms = fs::perms::add_perms |
                               fs::perms::owner_read | fs::perms::owner_write |
                               fs::perms::group_read |
                               fs::perms::others_read;
            fs::permissions(file, perms, *ec);
        }
    }
    return ok;
}

std::string ReadFileContent(const fs::path &path, std::string *errorMessage) {
    std::string content;
#if 0
    // 有些文件无法获取大小
    std::fstream ifs(path.string(), std::ifstream::binary);
    if (ifs.is_open()) {
        defer(ifs.close());

        std::string line;
        while(std::getline(ifs, line)) {
            content.append(line).append("\n");
        }

        // get pointer to associated buffer object
        std::filebuf *buf = ifs.rdbuf();

        // 有些文件无法获取大小
        // get file size using buffer's members
        std::size_t size = buf->pubseekoff(0, std::ifstream::end, std::ifstream::in);
#ifdef ENABLE_COVERAGE
        std::cout << "sizeof(" << file << ") => 0x" << convertHex(size) << " bytes" << std::endl;
#endif
        buf->pubseekpos(0, std::ifstream::in);

        // allocate memory to contain file data
        content.resize(size);

        // get file data
        buf->sgetn(const_cast<char *>(content.c_str()), size);
    }
#else
    File file(path, File::ModeReadText);
    if (file) {
        std::stringstream ss;

        char buf[4096];
        size_t readLen;
        while ((readLen = file.Read(buf, sizeof(buf))) > 0) {
            ss.write(buf, static_cast<std::streamsize>(readLen));
        }

        if (file.IsEnd()) {
            content = ss.str();
        } else if (errorMessage != nullptr && file.IsError()) {
            *errorMessage = "ReadFileContent(" + path.string() + ") error";
        }
    } else if (errorMessage != nullptr) {
        *errorMessage = StrError(errno, ("FeadFileContent(" + path.string() + ")").c_str());
    }
#endif
    return content;
}

std::vector<char> ReadFileBinary(const fs::path &path, std::string *errorMessage) {
    std::vector<char> ret;

    File file(path, File::ModeReadBin);
    if (file) {
        fseek(file.Fp(), 0, SEEK_END);
        auto len = ftell(file.Fp());

        if (len > 0) {
            ret.resize(len);

            fseek(file.Fp(), 0, SEEK_SET);
            file.Read(&ret[0], ret.size());
        }
    } else if (errorMessage != nullptr) {
        *errorMessage = StrError(errno, ("ReadFileBinary(" + path.string() + ")").c_str());
    }
    RETURN_RVALUE(ret);
}

bool IsDir(const fs::path &path) {
    return fs::exists(path) && fs::is_directory(path);
}

bool IsFile(const fs::path &path) {
    return fs::exists(path) && fs::is_regular_file(path);
}

namespace common {
    namespace FileUtils {
        int ReadFileContent(const fs::path &file, std::string &fileContent) {
            if (!fs::exists(file)) {
                return -1;
            }
            std::string errMsg;
            fileContent = ::ReadFileContent(file, &errMsg);
            if (!errMsg.empty()) {
                LogInfo("ReadFileContent: {}", errMsg);
            }
            return static_cast<int>(fileContent.size());
        }

        int ReadFileLines(const fs::path &file, std::vector<std::string> &lines) {
            std::string errMsg;
            int ret = GetFileLines(file, lines, false, &errMsg);
            if (ret != 0 && !errMsg.empty()) {
                LogInfo("ReadFileLines: {}", errMsg);
            }
            return 0 == ret ? static_cast<int>(lines.size()) : -1;
        }

        int WriteFileContent(const fs::path &filename, const std::string &fileContent) {
            boost::system::error_code ec;
            bool ok = ::WriteFileContent(filename, fileContent, &ec);
            if (!ok) {
                LogWarn("WriteFileContent({}) error: {}", filename.string(), ec.message());
            }
            return ok ? static_cast<int>(fileContent.size()) : -1;
        }

        bool isDir(const std::string &path) {
            return ::IsDir(path);
        }

        bool isFile(const std::string &path) {
            return ::IsFile(path);
        }
    }
}


/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// File

#ifdef WIN32
#   define fileno _fileno // defined in <stdio.h>

#   include <io.h>     // _get_osfhandle
#endif

#ifdef WIN32
const File::Mode File::ModeReadText{ "r+t" };
const File::Mode File::ModeWriteText{ "w+t" };
const File::Mode File::ModeReadBin{ "r+b" };
const File::Mode File::ModeWriteBin{ "w+b" };
#else
const File::Mode File::ModeReadText{"r+"};
const File::Mode File::ModeWriteText{"w+"};
const File::Mode File::ModeReadBin{"r+"};
const File::Mode File::ModeWriteBin{"w+"};
#endif

File::File(const fs::path &file, const Mode &mode) {
    fp = fopen(file.string().c_str(), mode.mode);
}

File::~File() {
    if (fp != nullptr) {
        fclose(fp);
        fp = nullptr;
    }
}

size_t File::Read(char *buf, size_t capacity) const {
    return fread(buf, 1, capacity, fp);
}

size_t File::Write(const char *buf, size_t size) {
    return fwrite(buf, 1, size, fp);
}

// 是否已达文件尾部
bool File::IsEnd() const {
    return IsOpen() && feof(fp) != 0;
}

bool File::IsError() const {
    return IsOpen() && ferror(fp) != 0;
}

int File::FileNo() const {
    return IsOpen()? fileno(fp) : -1;
}

//#ifdef WIN32
//HANDLE File::Handle() const {
//    int fno = this->FileNo();
//    HANDLE handle = INVALID_HANDLE_VALUE;
//    if (fno != -1) {
//        handle = (HANDLE)_get_osfhandle(fno);
//    }
//    return handle;
//}
//#endif
