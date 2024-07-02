//
// Created by 韩呈杰 on 2023/10/19.
//

#ifndef ARGUSAGENT_SYSTEM_INFORMATION_COLLECTOR_DARWIN_H
#define ARGUSAGENT_SYSTEM_INFORMATION_COLLECTOR_DARWIN_H

#include <type_traits>
#include "system_information_collector.h"

struct Sic;

#include "common/test_support"

class DarwinSystemInformationCollector : public SystemInformationCollector {
    friend std::shared_ptr<SystemInformationCollector> SystemInformationCollector::New();
public:
    explicit DarwinSystemInformationCollector();

    int SicGetThreadCpuInformation(pid_t pid, int tid, SicThreadCpu &) override;
    int SicGetCpuInformation(SicCpuInformation &information) override;
    int SicGetCpuListInformation(std::vector<SicCpuInformation> &) override;
    int SicGetMemoryInformation(SicMemoryInformation &information) override;
    int SicGetUpTime(double &upTime) override;
    int SicGetLoadInformation(SicLoadInformation &information) override;
    int SicGetSwapInformation(SicSwapInformation &information) override;

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
    int SicGetFileSystemListInformation(std::vector<SicFileSystem> &) override;
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

    int SicGetSystemInfo(SicSystemInfo &systemInfo) override;

protected:
    int SicInitialize() override;
    std::map<std::string, std::string> queryDevSerialId(const std::string &devName) override;

private:

    std::shared_ptr<Sic> SicPtr();
    std::shared_ptr<const Sic> SicPtr() const;
};

#include "common/test_support"

#endif //ARGUSAGENT_SYSTEM_INFORMATION_COLLECTOR_DARWIN_H
