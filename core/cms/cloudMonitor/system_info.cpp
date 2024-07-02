#include "system_info.h"

#include <algorithm>

#include "common/SystemUtil.h"
#include "common/StringUtils.h"
#include "common/NetTool.h"
#ifdef ENABLE_COVERAGE
#   include "common/Logger.h"
#endif
#include "sic/system_information_collector_util.h"

using namespace std;
using namespace common;

namespace cloudMonitor {
    // 公网优先 > IPv4优先
    void highPriorityIPFirst(std::vector<std::string> &ips) {
        if (!ips.empty()) {
            size_t targetPos = 0;
            bool targetIsIPv4 = !IP::IsIPv6(ips[targetPos]);
            bool targetIsPub = !IP::IsPrivate(ips[targetPos]);

            const size_t ipCount = ips.size();
            for (size_t i = 1; i < ipCount; i++) {
                const std::string &ip = ips[i];
                bool curIsPub = !IP::IsPrivate(ip);
                bool exchange;
                if (targetIsPub != curIsPub) {
                    exchange = curIsPub;
                } else {
                    exchange = !targetIsIPv4 && !IP::IsIPv6(ip);
                }
                if (exchange) {
                    targetPos = i;
                    targetIsIPv4 = !IP::IsIPv6(ip);
                    targetIsPub = curIsPub;
                }
            }
            if (targetPos > 0) {
                std::swap(ips[0], ips[targetPos]);
            }
        }
    }

    void SystemInfo::collectSystemInfo(SicSystemInfo &sysInfo, std::vector<std::string> &ips) const {
        GetSysInfo(sysInfo);

        vector<InterfaceConfig> interfaceConfigs;
        GetInterfaceConfigs(interfaceConfigs);
        for (auto &interfaceConfig: interfaceConfigs) {
            if (!interfaceConfig.ipv4.empty() && !StringUtils::Contain(ips, interfaceConfig.ipv4)) {
                ips.push_back(interfaceConfig.ipv4);
            }
            if (!interfaceConfig.ipv6.empty() && !StringUtils::Contain(ips, interfaceConfig.ipv6)) {
                ips.push_back(interfaceConfig.ipv6);
            }
        }
        highPriorityIPFirst(ips);
    }

    boost::json::object SystemInfo::MakeJson(const SicSystemInfo &sysInfo, const std::vector<std::string> &ips,
                                             const std::string &sn, int64_t freeMemory) const {
        return boost::json::object{
                {"serialNumber", sn},
                {"hostname",     SystemUtil::getHostname()},
                {"localIPs",     boost::json::array(ips.begin(), ips.end())},
                {"name",         string(sysInfo.vendorName) + " (" + string(sysInfo.vendor) + ")"},
                {"version",      sysInfo.vendorVersion},
                {"arch",         sysInfo.arch},
                {"freeSpace",    (freeMemory < 0 ? this->GetSystemFreeSpace() : freeMemory)},
        };
    }

    boost::json::object SystemInfo::GetSystemInfo(const string &serialNumber, int64_t freeMemory) const {
        SicSystemInfo sysInfo;
        vector<string> ipV4s;
        collectSystemInfo(sysInfo, ipV4s);

        return MakeJson(sysInfo, ipV4s, serialNumber, freeMemory);
    }

#ifdef ENABLE_COVERAGE

    std::string SystemInfo::MakeJsonString(const SicSystemInfo &sysInfo, const std::vector<std::string> &ipV4s,
                                           const std::string &sn, int64_t freeMemory) const {
        string name = string(sysInfo.vendorName) + " (" + string(sysInfo.vendor) + ")";
        string hostname = SystemUtil::getHostname();
        string localIps = StringUtils::join(ipV4s, "\",\"", "\"", "\"");

        freeMemory = (freeMemory < 0 ? this->GetSystemFreeSpace() : freeMemory);

        string systemInfoJson = "{";
        systemInfoJson += string("\"serialNumber\":") + "\"" + sn + "\"";
        systemInfoJson += string(",\"hostname\":") + "\"" + hostname + "\"";
        systemInfoJson += string(",\"localIPs\":") + "[" + localIps + "]";
        systemInfoJson += string(",\"name\":") + "\"" + name + "\"";
        systemInfoJson += string(",\"version\":") + "\"" + string(sysInfo.vendorVersion) + "\"";
        systemInfoJson += string(",\"arch\":") + "\"" + string(sysInfo.arch) + "\"";
        systemInfoJson += string(",\"freeSpace\":") + StringUtils::NumberToString(freeMemory);
        systemInfoJson += "}";
        return systemInfoJson;
    }

#endif

    int64_t SystemInfo::GetSystemFreeSpace() const {
        int64_t freeSpace = 0;
        vector<FileSystemInfo> fileSystemInfos;
        GetFileSystemInfos(fileSystemInfos);
        pid_t pid = GetPid();

        ProcessInfo processInfo;
        GetProcessInfo(pid, processInfo);
#ifdef ENABLE_COVERAGE
        LogDebug("processInfo.path: {}", processInfo.path);
#endif
        for (auto &fileSystemInfo: fileSystemInfos) {
#ifdef ENABLE_COVERAGE
            LogDebug(R"(fileSystemInfo(devName: "{}", dirName: "{}"))", fileSystemInfo.devName, fileSystemInfo.dirName);
#endif
            if (StringUtils::StartWith(processInfo.path, fileSystemInfo.dirName)) {
                SicFileSystemUsage fileSystemStat;
                if (GetFileSystemStat(fileSystemInfo.dirName, fileSystemStat) == 0) {
                    freeSpace = static_cast<int64_t>(fileSystemStat.free);
                }
            }
        }
        return freeSpace;
    }
}
