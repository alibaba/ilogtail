//
// Created by 许刘泽 on 2020/12/2.
//

#ifndef SIC_INCLUDE_LINUX_SYSTEM_INFORMATION_COLLECTOR_H_
#define SIC_INCLUDE_LINUX_SYSTEM_INFORMATION_COLLECTOR_H_

#include <type_traits>
#include "system_information_collector.h"
#include "common/FilePathUtils.h"

enum SicLinuxIOType
{
    IO_STATE_NONE,
    IO_STATE_PARTITIONS, // 2.4
    IO_STATE_DISKSTATS,  // 2.6
    IO_STAT_SYS          // 2.6
};

struct SicIODev
{
    std::string name;  // devName
    bool isPartition = false;
    SicDiskUsage diskUsage{};
};

struct Sic: SicBase
{
    ~Sic() override;

    size_t cpu_list_cores = 0;
    // int ticks;
    // the time at which the system booted, in seconds since the Unix epoch.
    uint64_t bootSeconds = 0;  // 秒级时间戳
    int pagesize = 0;
    int interfaceLength = 0;
    SicLinuxIOType ioType = IO_STATE_NONE;
    // SicLinuxProcessInfoCache linuxProcessInfo;
    std::unordered_map<uint64_t, std::shared_ptr<SicIODev>> fileSystemCache;
};

#include "common/test_support"
class LinuxSystemInformationCollector : public SystemInformationCollector
{
public:
    explicit LinuxSystemInformationCollector();
    explicit LinuxSystemInformationCollector(fs::path processDir);
    ~LinuxSystemInformationCollector() override = default;

    int SicInitialize() override;
    int SicGetThreadCpuInformation(pid_t pid, int tid, SicThreadCpu &) override;
    int SicGetCpuInformation(SicCpuInformation &information) override;
    int SicGetCpuListInformation(std::vector<SicCpuInformation> &) override;
    int SicGetMemoryInformation(SicMemoryInformation &information) override;
    int SicGetLoadInformation(SicLoadInformation &information) override;
    int SicGetSwapInformation(SicSwapInformation &swap) override;
    int SicGetInterfaceConfigList(SicInterfaceConfigList &interfaceConfigList) override;
    int SicGetInterfaceConfig(SicInterfaceConfig &interfaceConfig, const std::string &name) override;
    int SicGetInterfaceInformation(const std::string &name, SicNetInterfaceInformation &information) override;
    int SicGetNetState(SicNetState &netState,
                       bool useClient,
                       bool useServer,
                       bool useUdp,
                       bool useTcp,
                       bool useUnix,
                       int option = -1) override;
    int SicGetFileSystemListInformation(std::vector<SicFileSystem> &informations) override;
    int SicGetDiskUsage(SicDiskUsage &diskUsage, std::string dirName) override;
    int SicGetFileSystemUsage(SicFileSystemUsage &fileSystemUsage, std::string dirName) override;
    int SicGetProcessList(SicProcessList &processList, size_t limit, bool &overflow) override;
    int SicGetProcessState(pid_t pid, SicProcessState &processState) override;
    int SicGetProcessTime(pid_t pid, SicProcessTime &processTime, bool includeCTime = false) override;

    int SicGetProcessMemoryInformation(pid_t pid, SicProcessMemoryInformation &information) override;
    int SicGetProcessArgs(pid_t pid, SicProcessArgs &processArgs) override;
    int SicGetProcessExe(pid_t pid, SicProcessExe &processExe) override;
    int SicGetProcessCredName(pid_t pid, SicProcessCredName &processCredName) override;
    int SicGetProcessFd(pid_t pid, SicProcessFd &processFd) override;
    int SicGetUpTime(double &uptime) override;
    int SicGetSystemInfo(SicSystemInfo &systemInfo) override;

protected:
    std::map<std::string, std::string> queryDevSerialId(const std::string &devName) override;

private:
    // boot time in seconds
    int64_t SicGetBootSeconds();

    static int GetCpuMetric(const std::string &cpuLine, SicCpuInformation &cpu);

    static uint64_t GetMemoryValue(char unit, uint64_t value);

    int GetMemoryRam(uint64_t &ram);

    int SicReadNetFile(SicNetState &netState,
                       int type,
                       bool useClient,
                       bool useServer,
                       const fs::path &path);

    void SicGetNetAddress(const std::string &str, SicNetAddress &address);

    int SicUpdateNetState(SicNetConnectionInformation &netConnectionInformation, SicNetState &netState);

    void NetListenAddressAdd(SicNetConnectionInformation &connectionInformation);

    int GetDiskStat(dev_t rDev, const std::string &dirName, SicDiskUsage &disk,SicDiskUsage &deviceUsage);
    int SicGetIOstat(std::string &dirName, SicDiskUsage &disk, std::shared_ptr<SicIODev> &ioDev, SicDiskUsage &deviceUsage);
    int CalDiskUsage(SicIODev &ioDev, SicDiskUsage &diskUsage);

    void RefreshLocalDisk();
    std::shared_ptr<SicIODev> SicGetIODev(std::string &dirName);

    int SicReadProcessStat(pid_t pid, SicLinuxProcessInfoCache &processInfo);

    int SicGetProcessThreads(pid_t pid, uint64_t &threadCount);

    int ReadSocketStat(const fs::path &path, int &udp, int &tcp);

    int ReadNetLink(std::vector<uint64_t> &tcpStateCount);

    int ParseSSResult(const std::string &result, SicNetState &netState);

    /***
     * 结果与ss -s类似
     * tcpTotal计算方法：
     * 1）socketstat读取 "tw 105 alloc 19"两个字段之和
     * 2）netlink 读取 SIGAR_TCP_SYN_SENT 和 SIGAR_TCP_SYN_RECV;
     */
    int SicGetNetStateByNetLink(SicNetState &netState,
                                bool useClient,
                                bool useServer,
                                bool useUdp,
                                bool useTcp,
                                bool useUnix);

    int SicGetNetStateBySS(SicNetState &netState,
                           bool useClient,
                           bool useServer,
                           bool useUdp,
                           bool useTcp,
                           bool useUnix);

    int SicGetNetStateByReadFile(SicNetState &netState,
                                 bool useClient,
                                 bool useServer,
                                 bool useUdp,
                                 bool useTcp,
                                 bool useUnix);

    int SicGetVendorVersion(const std::string &release, std::string &version);

    int walkDigitDirs(const fs::path &root, const std::function<void(const std::string &)>& callback);
    int GetSwapPageInfo(SicSwapInformation &information);

    int ParseProcessStat(const fs::path &, pid_t, SicLinuxProcessInfo &);

    std::shared_ptr<Sic> SicPtr() {
        return std::dynamic_pointer_cast<Sic>(mSicPtr);
    }
    std::shared_ptr<const Sic> SicPtr() const {
        return std::const_pointer_cast<const Sic>(std::dynamic_pointer_cast<Sic>(mSicPtr));
    }

    const fs::path PROCESS_DIR{"/proc/"};
};
#include "common/test_support"

#endif //SIC_INCLUDE_LINUX_SYSTEM_INFORMATION_COLLECTOR_H_
