#ifdef WIN32
//#define WIN32_LEAN_AND_MEAN
// solve: ”使用未定义的 struct“sockaddr_in”
#   include <winsock2.h>
#   include <Windows.h>
#endif

#include "base_collect.h"

#include "common/StringUtils.h"
#include "common/SystemUtil.h"
#include "common/Logger.h"
#include "common/TimeProfile.h"
#include "common/NetTool.h"

using namespace std;
using namespace std::chrono;
using namespace common;

#define FL(l, fn, ...) res = mSystemInformationCollector->fn(__VA_ARGS__); \
    if (res != SIC_SUCCESS) { \
        Log(l, #fn" failed, err: {}", TrimSpace(SicPtr()->errorMessage)); \
        return res; \
    }

#define F(fn, ...) FL(LEVEL_WARN, fn, __VA_ARGS__)

#define COUNT(mib) sizeof(mib) / sizeof(mib[0])

namespace cloudMonitor {
    NetStat::NetStat() {
        constexpr const size_t size = sizeof(tcpStates) / sizeof(tcpStates[0]);
        for (size_t i = 0; i <= size; i++) {
            tcpStates[i] = -1;
        }
        // std::fill(std::begin(tcpStates), std::end(tcpStates), -1);
        updSession = 0;
    }

    BaseCollect::BaseCollect() {
        mSystemInformationCollector = SystemInformationCollector::New();
    }

    BaseCollect::~BaseCollect() = default;

    int BaseCollect::GetAllCpuStat(std::vector<SicCpuInformation> &cpuStats) const {
        cpuStats.clear();

        SicCpuInformation cpuInformation;
        int F(SicGetCpuInformation, cpuInformation)
        cpuStats.push_back(cpuInformation);

        std::vector<SicCpuInformation> cpuList;
        F(SicGetCpuListInformation, cpuList)
        cpuStats.insert(cpuStats.end(), cpuList.begin(), cpuList.end());

        return static_cast<int>(cpuStats.size());
    }

    int BaseCollect::GetMemoryStat(SicMemoryInformation &memoryInformation) const {
        int res;
        F(SicGetMemoryInformation, memoryInformation)
        return res;
    }

    int BaseCollect::GetSwapStat(SicSwapInformation &swapStat) const {
        int res;
        F(SicGetSwapInformation, swapStat)
        return res;
    }

    int BaseCollect::GetLoadStat(SicLoadInformation &loadStat) const {
        int F(SicGetLoadInformation, loadStat)
        return res;
    }

    static std::string validIP(const std::string &ip) {
        return BaseCollect::IsValidIp(ip)? ip: std::string();
    }
    int BaseCollect::GetInterfaceConfigs(std::vector<InterfaceConfig> &interfaceConfigs) const {
        std::vector<SicInterfaceConfig> sicIfConfigs;
        int F(SicGetInterfaceConfigList, sicIfConfigs)
        for (const auto &sicIf: sicIfConfigs) {
            if (IsExcludedInterface(sicIf.name)) {
                continue;
            }
            InterfaceConfig interfaceConfig;
            interfaceConfig.name = sicIf.name;
            interfaceConfig.ipv4 = validIP(sicIf.address.str());
            interfaceConfig.ipv6 = validIP(sicIf.address6.str());
            if (!interfaceConfig.ipv4.empty() || !interfaceConfig.ipv6.empty()) {
                interfaceConfigs.push_back(interfaceConfig);
            } else {
                LogDebug("skip invalid ip({}) interface: {}", interfaceConfig.ipv4, sicIf.name);
            }
        }
        return static_cast<int>(interfaceConfigs.size());
    }

    int BaseCollect::GetInterfaceStat(const string &name, SicNetInterfaceInformation &interfaceStat) {
        int F(SicGetInterfaceInformation, const_cast<string &>(name), interfaceStat)
        return res;
    }

    int BaseCollect::GetNetStat(NetStat &netStat) const {
#ifdef ENABLE_UDP_SESSION
#   define useUdp true
#else
#   define useUdp false
#endif
        SicNetState netState;
        int F(SicGetNetState, netState, true, true, useUdp, true, true)

        static_assert(COUNT(netStat.tcpStates) == COUNT(netState.tcpStates), "tcpStates isn't equal");
        for (int i = 0; i < SIC_TCP_STATE_END; i++) {
            netStat.tcpStates[i] = netState.tcpStates[i];
        }
        netStat.updSession = static_cast<decltype(netStat.updSession)>(netState.udpSession);
        return 0;
    }

    int BaseCollect::GetFileSystemInfos(std::vector<FileSystemInfo> &fileSystemInfos) const {
        std::vector<SicFileSystem> sicFileSystemList;
        int F(SicGetFileSystemListInformation, sicFileSystemList)

        for (auto &sicFileSystem: sicFileSystemList) {
            if (sicFileSystem.type != SIC_FILE_SYSTEM_TYPE_LOCAL_DISK) {
                continue;
            }
            FileSystemInfo fileSystemInfo;
            fileSystemInfo.dirName = sicFileSystem.dirName;
            fileSystemInfo.devName = sicFileSystem.devName;
            fileSystemInfo.type = sicFileSystem.sysTypeName;
            fileSystemInfos.push_back(fileSystemInfo);
        }
        return 0;
    }

    int BaseCollect::GetFileSystemStat(const string &dirName, SicFileSystemUsage &sicFileSystemUsage) const {
        const int status = mSystemInformationCollector->SicGetFileSystemUsage(sicFileSystemUsage, dirName);
        if (status != SIC_SUCCESS) {
            LogWarn("SicGetFileSystemUsage Failed, err: {}, dirName: {}",
                         SicPtr()->errorMessage.c_str(), dirName.c_str());
            return status;
        }
        return 0;
    }

    int BaseCollect::GetProcessPids(std::vector<pid_t> &pids, size_t limit, bool *overflow) const {
        SicProcessList pidList;
        bool tmpOverflow = false;
        overflow = overflow ? overflow : &tmpOverflow;
        int F(SicGetProcessList, pidList, limit, *overflow)

        pids = std::move(pidList.pids);

        LogDebug("SicGetProcessList get process num: {}", pids.size());
        return 0;
    }

#if defined(__APPLE__) || defined(__FreeBSD__)
#   define PROC_LOG_LEVEL LEVEL_TRACE
#else
#   define PROC_LOG_LEVEL LEVEL_WARN
#endif

    int BaseCollect::GetProcessState(pid_t pid, SicProcessState &processState) const {
        int FL(PROC_LOG_LEVEL, SicGetProcessState, pid, processState)
        return res;
    }

    int BaseCollect::GetProcessCpu(pid_t pid, SicProcessCpuInformation &processCpu, bool includeCTime) const {
        int FL(PROC_LOG_LEVEL, SicGetProcessCpuInformation, pid, processCpu, includeCTime)
        return res;
    }

    int BaseCollect::GetProcessInfo(pid_t pid, ProcessInfo &processInfo) const {
        TimeProfile tp;

        SicProcessArgs sicProcessArgs;
        int FL(PROC_LOG_LEVEL, SicGetProcessArgs, pid, sicProcessArgs)
        LogDebug("get process({}) args cost: {} ms, arg count: {}",
                 pid, convert(tp.millis(), true), sicProcessArgs.args.size());

        string path, cwd, root;
        SicProcessExe sicProcessExe;
        int status = mSystemInformationCollector->SicGetProcessExe(pid, sicProcessExe);
        if (status != SIC_SUCCESS) {
            if (!sicProcessArgs.args.empty()) {
                path = sicProcessArgs.args.front();
            }
        } else {
            path = sicProcessExe.name;
            cwd = sicProcessExe.cwd;
            root = sicProcessExe.root;
        }
        LogDebug("get process({}) exe cost: {} ms, return: {}, [{}]",
                 pid, convert(tp.millis(), true), status, path);

        string name = SystemUtil::GetPathBase(path);
        if (name == ".") {
            name = "unknown";
        }

        string user = "unknown";
        SicProcessCredName sicProcessCredName;
        if (mSystemInformationCollector->SicGetProcessCredName(pid, sicProcessCredName) == SIC_SUCCESS) {
            user = sicProcessCredName.user;
        }
        LogDebug("get process({}) cred name cost: {} ms, [{}]", pid, convert(tp.millis(), true), user);

        processInfo.pid = pid;
        processInfo.name = name;
        processInfo.path = path;
        processInfo.cwd = cwd;
        processInfo.root = root;
        processInfo.args = StringUtils::join(sicProcessArgs.args, " ");
        processInfo.user = user;

        return res;
    }

    int BaseCollect::GetProcessMemory(pid_t pid, SicProcessMemoryInformation &processMemory) const {
        int FL(PROC_LOG_LEVEL, SicGetProcessMemoryInformation, pid, processMemory)
        return res;
    }

    // 获取进程(pid)打开的文件数
    int BaseCollect::GetProcessFdNumber(pid_t pid, SicProcessFd &sicProcessFd) const {
        int FL(PROC_LOG_LEVEL, SicGetProcessFd, pid, sicProcessFd)
        return res;
    }

    bool BaseCollect::IsExcludedInterface(const string &name) {
        return StringUtils::StartWith(name, "tun");
    }

    bool BaseCollect::IsValidIp(const string &ip) {
        // return ip != "127.0.0.1" && ip != "0.0.0.0" && ip != "255.255.255.255";
        return IP::IsGlobalUniCast(ip);
    }

    // Get uptime in seconds
    int64_t BaseCollect::GetUptime(bool isMicro) const {
        double upTime = 0.0;
        int status = mSystemInformationCollector->SicGetUpTime(upTime);
        if (status != SIC_SUCCESS) {
            LogWarn("Get uptime error: {}", SicPtr()->errorMessage.c_str());
            return 0;
        }
        if (isMicro) {
            upTime *= 1000 * 1000;
        }
        return static_cast<int64_t>(upTime);
    }

    int BaseCollect::GetSysInfo(SicSystemInfo &sysInfo) const {
        int F(SicGetSystemInfo, sysInfo)
        return res;
    }
}