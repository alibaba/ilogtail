#include "cloudMonitor/base_collect.h"
#include <gtest/gtest.h>

#include <iostream>
#include <thread>

#include "common/SystemUtil.h"
#include "common/ConsumeCpu.h"
#include "common/Logger.h"

using namespace std;
using namespace common;

namespace cloudMonitor {
    class Cms_BaseCollectTest : public testing::Test {
    protected:
        void SetUp() override {
            p_shared = new BaseCollect();
        }

        void TearDown() override {
            delete p_shared;
            p_shared = nullptr;
        }

        BaseCollect *p_shared = nullptr;
    };

    TEST_F(Cms_BaseCollectTest, GetAllCpuStat) {
        vector<SicCpuInformation> cpuStats;
        EXPECT_NE(p_shared->collector().get(), nullptr);
        EXPECT_EQ(p_shared->GetAllCpuStat(cpuStats) > 0, true);
        // cpuStats中含有『总』Cpu统计，比实际cpu核数多1
        size_t cpuNum = std::thread::hardware_concurrency() + 1;
        EXPECT_EQ(cpuNum, cpuStats.size());
        SicCpuInformation cpuStat = cpuStats[0];
        auto total = cpuStat.user
                     + cpuStat.sys
                     + cpuStat.nice
                     + cpuStat.idle
                     + cpuStat.wait
                     + cpuStat.irq
                     + cpuStat.softIrq
                     + cpuStat.stolen;
        cout << "total=" << total.count() << " " << cpuStat.total().count() << endl;
        EXPECT_EQ(total, cpuStat.total());
    }

    TEST_F(Cms_BaseCollectTest, GetCpuPercent) {
        vector<SicCpuInformation> cpuStats1, cpuStats2;
        EXPECT_EQ(p_shared->GetAllCpuStat(cpuStats1) > 0, true);
        consumeCpu();
        EXPECT_EQ(p_shared->GetAllCpuStat(cpuStats2) > 0, true);
        SicCpuPercent cpuPercent = cpuStats1[0] / cpuStats2[0];
        SicCpuInformation cpuStat1 = cpuStats1[0];
        SicCpuInformation cpuStat2 = cpuStats2[0];
        if (cpuStat2.total() > cpuStat1.total()) {
            double user = static_cast<double>((cpuStat2.user - cpuStat1.user).count()) / static_cast<double>((cpuStat2.total() - cpuStat1.total()).count());
            cout << "user1=" << user << ":user2=" << cpuPercent.user << endl;
            cout << "idle=" << cpuPercent.idle << ";system:" << cpuPercent.sys << endl;
            EXPECT_EQ(user, cpuPercent.user);
        }
    }

    TEST_F(Cms_BaseCollectTest, GetMemoryStat) {
        SicMemoryInformation memoryStat;
        EXPECT_EQ(0, p_shared->GetMemoryStat(memoryStat));
    }

    TEST_F(Cms_BaseCollectTest, GetSwapStat) {
        SicSwapInformation swapStat;
        EXPECT_EQ(0, p_shared->GetSwapStat(swapStat));
    }

#if !defined(WIN32)
    TEST_F(Cms_BaseCollectTest, GetLoad) {
        SicLoadInformation load;
        EXPECT_EQ(0, p_shared->GetLoadStat(load));
        LogDebug("Load: {:.2f} {:.2f} {:.2f}", load.load1, load.load5, load.load15);
    }
#endif

    TEST_F(Cms_BaseCollectTest, GetInterfaceConfigs) {
        vector<InterfaceConfig> interfaceConfigs;
        p_shared->GetInterfaceConfigs(interfaceConfigs);
        for (const auto& interConfig : interfaceConfigs) {
            cout << "name:" << interConfig.name << " ipv4:" << interConfig.ipv4 << " ipv6:" << interConfig.ipv6 << endl;
            SicNetInterfaceInformation interfaceStat;
            p_shared->GetInterfaceStat(interConfig.name, interfaceStat);
            cout << "speed:" << interfaceStat.speed << " rx_bytes:" << interfaceStat.rxBytes << endl;
        }
    }

    TEST_F(Cms_BaseCollectTest, GetNetStat) {
        NetStat netStat;
        EXPECT_EQ(0, p_shared->GetNetStat(netStat));
        for (int i = 1; i < SIC_TCP_STATE_END; i++) {
            cout << GetTcpStateName((EnumTcpState)i) << ": " << netStat.tcpStates[i] << endl;
        }

        EXPECT_EQ(netStat.tcpStates[SIC_TCP_TOTAL] - netStat.tcpStates[SIC_TCP_ESTABLISHED],
                  netStat.tcpStates[SIC_TCP_NON_ESTABLISHED]);
    }

    TEST_F(Cms_BaseCollectTest, GetFileSystemInfos) {
        vector<FileSystemInfo> fileSystemInfos;
        EXPECT_EQ(0, p_shared->GetFileSystemInfos(fileSystemInfos));
        for (auto & fileSystemInfo : fileSystemInfos) {
            cout << fileSystemInfo.devName << ":" << fileSystemInfo.dirName << ":" << fileSystemInfo.type
                 << endl;
        }
    }

    TEST_F(Cms_BaseCollectTest, GetFileSystemStat) {
        SicFileSystemUsage fileSystemStat;
        EXPECT_EQ(0, p_shared->GetFileSystemStat("/", fileSystemStat));
        cout << "Reads:" << fileSystemStat.disk.reads << endl;
        cout << "total " << fileSystemStat.total << endl;
        cout << "userPercent:" << fileSystemStat.use_percent << endl;
        cout << "files:" << fileSystemStat.files << endl;
        cout << "freeFiles:" << fileSystemStat.freeFiles << endl;
    }

    TEST_F(Cms_BaseCollectTest, GetDiskStat) {
        SicFileSystemUsage fileSystemStat;
        auto GetDiskStat = [&](const std::string& dirName, SicDiskUsage& disk) {
            return p_shared->mSystemInformationCollector->SicGetDiskUsage(disk, dirName);
        };
#if defined(WIN32) || defined(__linux__)
        int ret = GetDiskStat((isWin32 ? "C:" : "/"), fileSystemStat.disk);
        if (ret != 0) {
            std::cout << p_shared->SicPtr()->errorMessage << std::endl;
        }
        if (common::SystemUtil::IsContainer()) {
            const char* expectErr = "disk folder";
            EXPECT_NE(p_shared->SicPtr()->errorMessage.find(expectErr), std::string::npos);
        }
        else {
            EXPECT_EQ(0, ret);
        }
        std::cout << fileSystemStat.disk.string() << std::endl;
#endif
        EXPECT_NE(0, GetDiskStat("/nonexistent_directory", fileSystemStat.disk));
        std::cout << p_shared->SicPtr()->errorMessage << std::endl;
    }

    TEST_F(Cms_BaseCollectTest, GetProcessPids) {
        vector<pid_t> pids;
        EXPECT_EQ(0, p_shared->GetProcessPids(pids, 0, nullptr));

        EXPECT_NE(pids.end(), std::find(pids.begin(), pids.end(), GetPid()));

#if defined(WIN32) || defined(__APPLE__) || defined(__FreeBSD__)
        pid_t pid = GetPid();
#else
        pid_t pid = 1;
#endif
        SicProcessState processState;
        EXPECT_EQ(0, p_shared->GetProcessState(pid, processState));
        EXPECT_GT(processState.threads, decltype(processState.threads)(0));
        SicProcessCpuInformation processCpu;
        EXPECT_EQ(0, p_shared->GetProcessCpu(pid, processCpu, true));
        cout << "GetProcessCpu" << endl;
        cout << "total=" << processCpu.total << endl;
        cout << "percent=" << processCpu.percent << endl;
        EXPECT_GT(processCpu.startTime, 0);
        cout << "start_time=" << processCpu.startTime << endl;
        cout << "sys_up_time=" << p_shared->GetUptime() << endl;
        EXPECT_EQ(processCpu.total, processCpu.sys + processCpu.user);
        ProcessInfo processInfo;
        EXPECT_EQ(0, p_shared->GetProcessInfo(pid, processInfo));
        cout << "name=" << processInfo.name << ",path=" << processInfo.path <<
             ",args=" << processInfo.args << ",user=" << processInfo.user << endl;
        SicProcessMemoryInformation processMemory;
        EXPECT_EQ(0, p_shared->GetProcessMemory(pid, processMemory));
        cout << "resident=" << processMemory.resident << ",size=" << processMemory.size << endl;

        SicProcessFd procFd;
        EXPECT_EQ(0, p_shared->GetProcessFdNumber(pid, procFd));
        cout << "FdNum=" << procFd.total << ", exact=" << (procFd.exact ? "true" : "false") << endl;
    }

    TEST_F(Cms_BaseCollectTest, IsExcludedInterface) {
        EXPECT_EQ(p_shared->IsExcludedInterface("tun321"), true);
        EXPECT_EQ(p_shared->IsExcludedInterface("eth0"), false);
    }
}