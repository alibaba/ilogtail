#ifndef _SYSTEM_INFO_H_
#define _SYSTEM_INFO_H_

#include "base_collect.h"
#include <string>
#include <boost/json.hpp>

namespace cloudMonitor {

#include "common/test_support"
class SystemInfo : public BaseCollect {
public:
    SystemInfo() = default;
    ~SystemInfo() = default;

    boost::json::object GetSystemInfo(const std::string &serialNumber, int64_t freeMemory = -1) const;

    int64_t GetSystemFreeSpace() const;

private:
    void collectSystemInfo(SicSystemInfo &sysInfo, std::vector<std::string> &ips) const;

    boost::json::object MakeJson(const SicSystemInfo &sysInfo,
                                 const std::vector<std::string> &ipV4s,
                                 const std::string &sn,
                                 int64_t freeMemory) const;
#ifdef ENABLE_COVERAGE
    std::string MakeJsonString(const SicSystemInfo &sysInfo,
                               const std::vector<std::string> &ipV4s,
                               const std::string &sn,
                               int64_t freeMemory) const;
#endif
};
#include "common/test_support"

}
#endif
