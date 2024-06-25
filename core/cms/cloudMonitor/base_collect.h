#ifndef ARGUS_CLOUD_MONITOR_BASE_COLLECT_H
#define ARGUS_CLOUD_MONITOR_BASE_COLLECT_H

#include <vector>
#include <string>
#include <chrono>
#include <memory>
#include "sic/system_information_collector.h"

namespace common {
    struct MetricData;
    struct CollectData;
}
namespace argus {
    struct MetricItem;
}

namespace cloudMonitor {
    struct NetStat {
        // 最后两个为TCP_TOTAL,NON_ESTABLISHED
        int tcpStates[SIC_TCP_STATE_END];
        int updSession;

        NetStat() {
            for (int i = 0; i <= sizeof(tcpStates) / sizeof(tcpStates[0]); i++) {
                tcpStates[i] = -1;
            }
            updSession = 0;
        }
    };

    struct InterfaceConfig {
        std::string name;
        std::string ipv4;
        std::string ipv6;
    };
    struct FileSystemInfo {
        std::string dirName;
        std::string devName;
        std::string type;
    };
    struct ProcessInfo {
        pid_t pid;
        std::string name;
        std::string path;
        std::string cwd;
        std::string root;
        std::string args;
        std::string user;
    };

#include "common/test_support"
class BaseCollect {
public:
    BaseCollect();
    ~BaseCollect();

protected:

    int GetAllCpuStat(std::vector<SicCpuInformation> &cpuStats) const;

    int GetMemoryStat(SicMemoryInformation &memoryInformation) const;

    int GetSwapStat(SicSwapInformation &swapStat) const;

    int GetLoadStat(SicLoadInformation &loadStat) const;

    int GetInterfaceConfigs(std::vector<InterfaceConfig> &interfaceConfigs) const;
    int GetInterfaceStat(const std::string &name, SicNetInterfaceInformation &netInterfaceInformation);
    int GetNetStat(NetStat &netStat) const;

    int GetFileSystemInfos(std::vector<FileSystemInfo> &fileSystemInfos) const;
    int GetFileSystemStat(const std::string &dirName, SicFileSystemUsage &sicFileSystemUsage) const;

    int GetProcessPids(std::vector<pid_t> &pids, size_t limit, bool *overflow) const;
    int GetProcessState(pid_t pid, SicProcessState &processState) const;
    int GetProcessCpu(pid_t pid, SicProcessCpuInformation &processCpu, bool includeCTime) const;
    int GetProcessInfo(pid_t pid, ProcessInfo &processInfo) const;
    int GetProcessMemory(pid_t pid, SicProcessMemoryInformation &processMemory) const;
    int GetProcessFdNumber(pid_t pid, SicProcessFd &) const;

    // isMicro 是否微秒，如果为否，则返回秒数
    int64_t GetUptime(bool isMicro = true) const;

    std::chrono::seconds GetUpSeconds() const {
        return std::chrono::seconds{GetUptime(false)};
    }

    std::chrono::microseconds GetUpMicros() const {
        return std::chrono::microseconds{GetUptime(true)};
    }

    int GetSysInfo(SicSystemInfo &sysInfo) const;

    std::shared_ptr<SicBase> SicPtr() const {
        return mSystemInformationCollector->SicPtr();
    }

protected:

    std::shared_ptr<SystemInformationCollector> collector() const {
        return mSystemInformationCollector;
    }

private:
    std::shared_ptr<SystemInformationCollector> mSystemInformationCollector;

public:
    static bool IsValidIp(const std::string &ip);
    static bool IsExcludedInterface(const std::string &name);
};
#include "common/test_support"

}
#endif // !ARGUS_CLOUD_MONITOR_BASE_COLLECT_H