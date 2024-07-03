#ifdef WIN32
#	pragma warning(disable: 4819)
#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN
#   endif
#endif

#if !defined(WIN32)
#   if !defined(_GNU_SOURCE)
#       define _GNU_SOURCE
#   endif
#endif
#ifdef ENABLE_COVERAGE

#   include "common/gtest_auto_link.h"

#endif

#include "common/Common.h"

#include <iostream>
#include <cstring> // strerrorlen_s, strerror_s, strerror

#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/address_v6.hpp>

#include "common/Random.h"
#include "common/TimeFormat.h"
#include "common/ExecCmd.h"
#include "common/Defer.h"

#include "common/Md5.h"
#include "common/Logger.h"
#include "common/HttpClient.h"
#include "common/CompileInfo.h"
#include "common/CpuArch.h"
#include "common/Chrono.h"
#include "common/StringUtils.h"
#include "common/Defer.h"
#include "common/FileUtils.h"

#include <apr-1/apr_time.h>
#ifdef LINUX
#   include <apr-1/apr_user.h>
#endif

#if defined(__APPLE__) || defined(__FreeBSD__)
    // There is no header file; as the manpage says you simply need to declare it yourself:
    // https://stackoverflow.com/questions/31346887/header-for-environ-on-mac
    extern char **environ;
#endif


#ifdef WIN32
#include <Windows.h>  // CloseHandle, GetCurrentThreadId
#include <tlhelp32.h> // CreateToolhelp32Snapshot, Process32First
#include <process.h>  // _getpid

#	ifndef getpid
#		define getpid _getpid
#	endif
#else
#include <unistd.h>  // getpid, getppid
// https://blog.csdn.net/weixin_42604188/article/details/112614807
#include <sys/syscall.h> // SYS_thread_selfid, SYS_gettid
#endif

#define MAX_STR_LEN 8192

using namespace std;
using namespace std::chrono;

#ifdef ENABLE_COVERAGE
#   ifdef VERSION
#       undef VERSION
#   endif
#endif

#ifndef VERSION
#   define VERSION "3.5.9999"
#endif


std::string StrError(int errNo, const char *op) {
    std::ostringstream oss;
    if (op != nullptr) {
        oss << op << ", ";
    }
#if defined(__STDC_LIB_EXT1__)
    size_t msgLen = strerrorlen_s(errNo);
    std::string errMsg(static_cast<int>(msgLen), '\0');
    strerror_s(const_cast<char *>(errMsg.data()), errMsg.size() + 1, errNo);
    oss << errMsg;
#elif defined(_MSC_VER)
    // _MSC_VER = 1900 : VC2015
    // VC比较奇葩，不支持strerrorlen_s
    char buf[1024];
    strerror_s(buf, sizeof(buf), errNo);
    oss << buf;
#else
    oss << strerror(errNo);
#endif

    return oss.str();
}

pid_t GetPid() {
    return getpid();
}
#if !defined(WIN32)
pid_t GetParentPid() {
    return getppid();
}
#else
pid_t ppid = 0;
static std::once_flag ppidOnce;
pid_t GetParentPid() {
	std::call_once(ppidOnce, [&]() {
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnapshot != INVALID_HANDLE_VALUE) {
			defer(CloseHandle(hSnapshot));

			PROCESSENTRY32 pe32;
			pe32.dwSize = sizeof(PROCESSENTRY32);
			if (Process32First(hSnapshot, &pe32)) {
				pid_t pid = GetPid();
				do {
					if (pe32.th32ProcessID == pid) {
						ppid = pe32.th32ParentProcessID;
						break;
					}
				} while (Process32Next(hSnapshot, &pe32));
			}
		}
	});

	return ppid;
}
#endif // Win32

// See: https://blog.csdn.net/weixin_42604188/article/details/112614807#获取当前id_高效获取当前线程的 id
// 获取当前线Id
int GetThisThreadId() {
#if defined(WIN32)
    return ::GetCurrentThreadId();
#elif defined(__APPLE__) || defined(__FreeBSD__)
    return syscall(SYS_thread_selfid);
#else
    return syscall(SYS_gettid);
#endif
}


std::string MacString(const unsigned char *mac) {
    std::string str;
    if (mac != nullptr) {
        str = fmt::sprintf("%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    return str;
}

std::string IPv4String(uint32_t address) {
    using namespace boost::asio::ip;
    // address = boost::endian::big_to_native(address);
    address_v4 v4(*(address_v4::bytes_type *) (&address));
    return v4.to_string();
}

std::string IPv6String(const uint32_t address[4]) {
    using namespace boost::asio::ip;
    // address = boost::endian::big_to_native(address);
    address_v6 v6(*(address_v6::bytes_type *) (address));
    return v6.to_string();
}

namespace common {
    std::string getVersion(bool withProfile) {
        if (!withProfile || PROFILE[0] == '\0') {
            return VERSION;
        } else {
            return VERSION "_" PROFILE;
        }
    }

    static const char *month_names = "JanFebMarAprMayJunJulAugSepOctNovDec";

    std::string compileTime() {
        char s_month[5];
        int day, year;

        // Jul 1 2023
        sscanf(__DATE__, "%s %d %d", s_month, &day, &year);
        auto month = (strstr(month_names, s_month) - month_names) / 3;

        std::string ct = fmt::format("{:04d}-{:02d}-{:02d} {}", year, month + 1, day, __TIME__);
#ifdef COMPILER_HOST_TZ
        ct.append(" ").append(COMPILER_HOST_TZ);
#endif
        return ct;
    }

    static std::string sourceBranch() {
        std::string v;
#if defined(ARGUS_GIT_BRANCH)
        v += "git branch  : " ARGUS_GIT_BRANCH;
#endif
#if defined(ARGUS_GIT_COMMIT_ID)
        v += std::string(v.empty()? "": "\n") + "git commitId: " ARGUS_GIT_COMMIT_ID;
#endif
        return v;
    }

    std::string getVersionDetail() {
        std::stringstream ss;
        ss << "argusagent version " << getVersion()
            << " (last change:" << compileTime() << ")"
            << " for " << OS_NAME "-" CPU_ARCH;
        std::string source = sourceBranch();
        if (!source.empty()) {
            ss << "\n" << source;
        }
        return ss.str();
    }

    static std::once_flag globalInitFlag;

    int globalInit() {
        call_once(globalInitFlag, [&]() {
            apr_status_t rv = apr_initialize();
            if (rv != APR_SUCCESS) {
                LogError("apr_initialized failed, program exit");
                _exit(-1);
            }
            apr_pool_t *mp = nullptr;
            apr_pool_create(&mp, nullptr);
            defer(apr_pool_destroy(mp));
            apr_allocator_t *pa = apr_pool_allocator_get(mp);
            if (pa) {
                apr_allocator_max_free_set(pa, 1 * 1024 * 1024);
            } else {
                LogError("apr_pool_allocator_get return nullptr, program exit");
                _exit(-1);
            }
            // apr_pool_destroy(mp);
            //atexit(apr_terminate);
        });
        return 0;
    }

    static volatile struct CGlobalInit {
        CGlobalInit() {
            globalInit();
        }
    } _globalCurlInit{};

    string getTimeString(int64_t micros) {
        // 微秒
        return date::format<6>(std::chrono::FromMicros(micros));
    }

    string getErrorString(int errorNo) {
        if (errorNo > 0) {
            return StrError(errorNo, nullptr);
        } else {
            std::stringstream ss;
            ss << "(" << errorNo << ") not defined error";
            return ss.str();
        }
    }

    int getCmdOutput(const std::string &cmd, string &output) {
        tagExecCmdOption option;
        option.fnTrim = [](const std::string &r) { return Trim(r, "\r\n"); };
        return ExecCmd(cmd, &output, &option);
    }

    // 这个功能太『狗』了，逻辑乱七八糟的
    std::string enableString(const std::string &str) {
        size_t len = str.size();
        if (len == 0 || len > MAX_STR_LEN - 1000) {
            return str;
        }

        string tmp;
        tmp.reserve(len + 32);
        char quote = 0;
        size_t pos = 0;
        for (size_t i = 0; i < len; i++) {
            const char ch = str[i];
            const char *esc = nullptr;
            if (ch == '\\') {
                esc = "\\";
            }
#ifndef WIN32
            else if (ch == '"' || ch == '\'') {
                if (!quote && i > 0 && str[i - 1] == ' ') {
                    quote = ch;
                    esc = (ch == '"' ? R"("\)" : R"('\)"); // 实际上："\" ..... \""
                } else if (quote == ch && str[i - 1] != '\\') {
                    quote = 0;
                    esc = (ch == '"' ? R"(\")" : R"(\')");
                }
            }
            // else if (ch == '\'') {
            //     if (!quote && i > 0 && str[i - 1] == ' ') {
            //         quote = ch;
            //         esc = "\'\\";
            //     } else if (quote == '\'' && i > 0 && str[i - 1] != '\\') {
            //         quote = 0;
            //         esc = "\\\'";
            //     }
            // }
#endif
            if (esc) {
                tmp.append(&str[pos], i - pos);
                tmp.append(esc);
                pos = i;
            }
            // tmp += str[i++];
        }
        if (pos < str.size()) {
            tmp.append(&str[pos]);
        }
        tmp.shrink_to_fit();

        return tmp;
    }

    std::string replaceMacro(const std::string &str, const std::chrono::system_clock::time_point &tp) {
        if (str.empty()) {
            return str;
        }
        const size_t len = str.size();

        using namespace std::chrono;
        std::tm expt = fmt::localtime(system_clock::to_time_t(IsZero(tp) ? system_clock::now() : tp));

#ifdef WIN32
#   define BASE_DIR "c:/staragent/";
#else
#   define BASE_DIR "/usr/alisys/dragoon/";
#endif
        // std::tm https://cplusplus.com/reference/ctime/tm/
        std::map<std::string, std::function<std::string(const std::tm &)>> mapReplacer{
                {"$YEAR$",       [](const std::tm &expt) { return fmt::format("{}", expt.tm_year + 1900); }},
                {"%year%",       [](const std::tm &expt) { return fmt::format("{}", expt.tm_year + 1900); }},
                {"$SHORTYEAR$",  [](const std::tm &expt) { return fmt::format("{:02d}", (expt.tm_year % 100)); }},
                {"$MONTH$",      [](const std::tm &expt) { return fmt::format("{:02d}", expt.tm_mon + 1); }},
                {"%month%",      [](const std::tm &expt) { return fmt::format("{:02d}", expt.tm_mon + 1); }},
                {"$SHORTMONTH$", [](const std::tm &expt) { return fmt::format("{}", expt.tm_mon + 1); }},
                {"$DATE$",       [](const std::tm &expt) { return fmt::format("{:02d}", expt.tm_mday); }},
                {"$DAY$",        [](const std::tm &expt) { return fmt::format("{:02d}", expt.tm_mday); }},
                {"%day%",        [](const std::tm &expt) { return fmt::format("{:02d}", expt.tm_mday); }},
                {"$SHORTDATE$",  [](const std::tm &expt) { return fmt::format("{}", expt.tm_mday); }},
                {"$HOUR$",       [](const std::tm &expt) { return fmt::format("{:02d}", expt.tm_hour); }},
                {"%hour%",       [](const std::tm &expt) { return fmt::format("{:02d}", expt.tm_hour); }},
                {"$SHORTHOUR$",  [](const std::tm &expt) { return fmt::format("{}", expt.tm_hour); }},
                {"$WEEK$",       [](const std::tm &expt) { return fmt::format("{}", expt.tm_wday); }}, // 0-周日
                {"%week%",       [](const std::tm &expt) { return fmt::format("{}", expt.tm_wday); }},
                {"$MINUTE$",     [](const std::tm &expt) { return fmt::format("{:02d}", expt.tm_min); }},
                {"$SECOND$",     [](const std::tm &expt) { return fmt::format("{:02d}", expt.tm_sec); }},
                {"$BASEDIR$",    [](const std::tm &expt) -> std::string { return BASE_DIR; }},
        };
#undef BASE_DIR

        std::string result;
        result.reserve(str.size());
        for (size_t pos = 0; pos < len; pos++) {
            const char ch = str[pos];
            if (ch == '$' || ch == '%') {
                auto end = str.find(ch, pos + 1);
                if (end != std::string::npos) {
                    auto it = mapReplacer.find(str.substr(pos, end + 1 - pos));
                    if (it != mapReplacer.end()) {
                        pos += it->first.size() - 1;
                        result.append(it->second(expt));
                        continue;
                    }
                }
            }
            result.push_back(ch);
        }
        result.shrink_to_fit();

        return result;
    }

    int GetMd5(const std::string &fileName, std::string &md5) {
        File file(fileName, File::ModeReadBin);
        int ret = (file ? 0 : -1);
        if (ret == 0) {
            MD5_CTX context;
            MD5Init(&context);

            char buf[2048];
            size_t readLen;
            while ((readLen = file.Read(buf, sizeof(buf))) > 0) {
                MD5Update(&context, (const unsigned char *) buf, readLen);
            }

            unsigned char digest[16];
            MD5Final(digest, &context);

            md5.resize(2 * sizeof(digest));
            char *s = const_cast<char *>(md5.c_str());
            for (unsigned char *c = digest; c < digest + sizeof(digest); c += 4) {
                s += sprintf(s, "%02x%02x%02x%02x", c[0], c[1], c[2], c[3]);
            }
        }
        return ret;
    }

    int StoreFile(const std::string &content, const std::string &fileName, std::string user, std::string &errMsg) {
        if (fileName.empty()) {
            errMsg = "filename empty";
            LogWarn(errMsg);
            return -1;
        }

#ifdef LINUX
        if (user.empty()) {
            user = "nobody";
        }
        //check user
        apr_uid_t uid = 0;
        apr_gid_t gid = 0;
        apr_status_t st = apr_uid_get(&uid, &gid, user.c_str(), nullptr);
        if (st != 0) {
            errMsg = "no such user:" + user;
            LogWarn(errMsg);
            // return -1;  // 没看出这里的必要性，忽略该错误
        }

        if (user == "nobody" && fileName.substr(0, 5) != "/tmp/") {
            errMsg = user + " store " + fileName + ", no permit";
            LogWarn(errMsg);
            return -1;
        }

        string tmpfile = "/home/" + user + "/";
        if (user != "root" && fileName.substr(0, tmpfile.size()) != tmpfile && fileName.substr(0, 5) != "/tmp/") {
            errMsg = user + " store " + fileName + ", no permit";
            LogWarn(errMsg);
            return -1;
        }
#endif

        boost::system::error_code ec;
        if (!WriteFileContent(fileName, content, &ec)) {
            LogWarn("write file, {}, failed: {}", fileName.c_str(), ec.message().c_str());
            return -1;
        }

#ifdef LINUX
        if (user != "root" && uid > 0) {
            chown(fileName.c_str(), uid, gid);
        }
#endif

        return 0;
    }

    std::string UrlEncode(const std::string &str) {
        return HttpClient::UrlEncode(str);
    }

    std::string UrlDecode(const std::string &str) {
        return HttpClient::UrlDecode(str);
    }

    // 这段代码实际上是抄的nginx的，https://blog.fatedier.com/2015/08/24/how-to-modify-process-name/
    // 2024-01-04. hcj argusagent不需要搞的这么麻烦
    // int changeProcessTitle(const char *newTitle, int argc, char **argv) {
    //     char *argv_last = nullptr;
    //     char *p;
    //     size_t size;
    //     unsigned int i;
    //     size = 0;
    //     for (i = 0; environ[i]; i++) {
    //         size += strlen(environ[i]) + 1;
    //     }
    //     p = (char *) malloc(size);
    //     if (p == nullptr) {
    //         return 1;
    //     }
    //     /*
    //         这是为了找出argv和environ指向连续内存空间结尾的位置，为了能处理argv[i]被修改后，指向非进程启动时所分配的连续内存，而采用了下面的算法。但是实际上，这样还是处理不了这种情况。仅仅是个人愚见！！！
    //         */
    //     argv_last = argv[0];
    //
    //     for (i = 0; argv[i]; i++) {
    //         if (argv_last == argv[i]) {
    //             argv_last = argv[i] + strlen(argv[i]) + 1;
    //         }
    //     }
    //     for (i = 0; environ[i]; i++) {
    //         if (argv_last == environ[i]) {
    //             size = strlen(environ[i]) + 1;
    //             argv_last = environ[i] + size;
    //             strncpy(p, (const char *) environ[i], size);
    //             environ[i] = (char *) p;
    //             p += size;
    //         }
    //     }
    //     argv_last--;
    //     argv[1] = nullptr;
    //     p = (char *) argv[0];
    //     strncpy(p, newTitle, argv_last - (char *) p);
    //     p += strlen(newTitle);
    //     if (argv_last - (char *) p) {
    //         memset(p, '\0', argv_last - (char *) p);
    //     }
    //     return 0;
    // }

    std::string joinOutput(const std::vector<std::pair<std::string, std::string>> &output) {
        std::stringstream ss;
        ss << "[";
        const char *sep = "";
        for (const auto &pair: output) {
            ss << sep << "{\"" << pair.first << "\":\"" << pair.second << "\"}";
            sep = ",";
        }
        ss << "]";
        return ss.str();
    }
} // namespace common
