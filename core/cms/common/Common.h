#ifndef ARGUS_COMMON_HEADER
#define ARGUS_COMMON_HEADER

#include <string>
#include <map>
#include <chrono>
#include <boost/core/demangle.hpp>
#include "common/Random.h"

#define PROFILE     ""       //super, test, nolog, user(jst, police, beidou, sqa)
#define MAX_RETRY_TIME 3

#if defined(WIN32) || defined(_WIN32)
typedef int pid_t;
#else
#include <unistd.h> // pid_t
#endif

// 获取当前进程Id
pid_t GetPid();
pid_t GetParentPid();
int GetThisThreadId();

// 基于errno的错误信息
std::string StrError(int errNo, const char *op);

std::string IPv6String(const uint32_t address[4]);
std::string IPv4String(uint32_t address);
// 网卡mac地址(mac至少需6个字节)
std::string MacString(const unsigned char *mac);

namespace common {
    std::string getVersion(bool withProfile = true);
    std::string getVersionDetail();
    int globalInit();
    std::string getTimeString(int64_t micros);
    std::string getErrorString(int errorNo);
	std::string enableString(const std::string &str);
    std::string replaceMacro(const std::string &str, const std::chrono::system_clock::time_point &tp = {});

    int GetMd5(const std::string &fileName, std::string &md5);
    int StoreFile(const std::string &content, const std::string &fileName, std::string user, std::string &errMsg);

    template<typename Rep1, typename Period1, typename Rep2, typename Period2>
    std::chrono::microseconds getHashDuration(const std::chrono::duration<Rep1, Period1> &interval,
                                              const std::chrono::duration<Rep2, Period2> &factor) {
        using namespace std::chrono;
        auto span = duration_cast<microseconds>(interval > factor ? factor : interval);

        decltype(span.count()) micros = 0;
        if (span.count() > 1) {
            // 整数的rand是闭区间，为了避免产生与interval一样的值，此处减1
            Random<decltype(micros)> rand{0, span.count() - 1};
            micros = rand();
        }
        return microseconds{micros};
    }

    template<typename Clock, typename Duration, typename Rep1, typename Period1, typename Rep2, typename Period2>
    std::chrono::time_point<Clock, Duration> getHashStartTime(const std::chrono::time_point<Clock, Duration> &now,
                                                              const std::chrono::duration<Rep1, Period1> &interval,
                                                              const std::chrono::duration<Rep2, Period2> &factor) {
        return now + getHashDuration(interval, factor);
    }

    std::string UrlEncode(const std::string &str);
    std::string UrlDecode(const std::string &str);

    int getCmdOutput(const std::string &cmd, std::string &output);
    // int changeProcessTitle(const char *newTitle, int argc, char **argv);

    std::string compileTime();

    template<typename T>
    std::string typeName(const T *obj) {
        return obj ? boost::core::demangle(typeid(*obj).name()) : std::string{"null"};
    }

    std::string joinOutput(const std::vector<std::pair<std::string, std::string>> &output);
}
#endif																		
