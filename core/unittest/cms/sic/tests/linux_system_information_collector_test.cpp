//
// Created by 许刘泽 on 2020/12/3.
//
// #if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
#include <gtest/gtest.h>

#include <memory>
#include <iostream>
#include <thread>
#include <chrono>
#include <cmath> // isnan

#include <boost/format.hpp>

#include <sys/time.h> // gettimeofday
#ifndef _BSD_SOURCE
#   define _BSD_SOURCE             /* See feature_test_macros(7) */
#endif
#include <sys/types.h>      // makedev
#if defined(__linux__) || defined(__unix__)
#include <sys/sysmacros.h>  // makedev，不同的Linux发行版， makedev位置不同，有的在sys/types.h下
#endif
#include "sic/linux_system_information_collector.h"
#include "common/TimeFormat.h"
#include "common/Arithmetic.h"
#include "common/UnitTestEnv.h"
#include "common/StringUtils.h" // sout, convert
#include "common/FileUtils.h"
#include "common/Common.h"      // GetPid
#include "common/Chrono.h"
#include "common/ChronoLiteral.h"

extern "C" {
#include <sigar.h>
#include <sigar_log.h>
#include <sigar_format.h>
}

using namespace std::chrono;

#define _EXPECT_EQ(A, COLLECTOR, B) do{ \
    int result = (COLLECTOR).B;  \
    EXPECT_EQ(A, result);  \
    if ((A) != result) {     \
        std::cout << #COLLECTOR"."#B << " fail: " << (COLLECTOR).errorMessage() << std::endl; \
    } \
}while(false)

extern unsigned int Hex2Int(const std::string &);
extern uint64_t parseProcMtrr(const std::vector<std::string> &);
extern void completeSicMemoryInformation(SicMemoryInformation &memInfo,
                                         uint64_t buffers,
                                         uint64_t cached,
                                         uint64_t available,
                                         const std::function<void(uint64_t &)> &fnGetMemoryRam);
extern bool IsDev(const std::string &dirName);
extern std::string __printDirStat(const char *file, int line, const std::string &dirName, struct stat &ioStat);

template<typename T1, typename T2, typename T3>
static inline void absMinusExpectLessThan(const T1 &a, const T2 &b, const T3 &expectMin) {
    EXPECT_LE(AbsMinus(a, b), decltype(a - b)(expectMin));
}

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SicLinuxCollectorTest
class SicLinuxCollectorTest : public testing::Test {
protected:
    void SetUp() override {
        sigar_open(&mSigar);
        sigar_log_impl_set(mSigar, nullptr, &SicLinuxCollectorTest::sigarLogImpl);
        sigar_log_level_set(mSigar, SIGAR_LOG_TRACE);
        EXPECT_TRUE(sigar_log_level_get(mSigar) >= SIGAR_LOG_DEBUG);
    }

    void TearDown() override {
        sigar_close(mSigar);
    }

    sigar_t *mSigar = nullptr;
public:
    static void sigarLogImpl(sigar_t *sigar, void *, int level, char *msg) {
        std::cout << "sigar: " << msg << std::endl;
    }
};

TEST_F(SicLinuxCollectorTest, Hex2Int) {
    EXPECT_EQ(decltype(Hex2Int(""))(15), Hex2Int("F"));
    EXPECT_EQ(decltype(Hex2Int(""))(15), Hex2Int("f"));
    EXPECT_EQ(decltype(Hex2Int(""))(255), Hex2Int("fF"));
}

TEST_F(SicLinuxCollectorTest, __printDirStat) {
    struct stat ioStat{};
    stat("/", &ioStat);
    std::string s = __printDirStat(__FILE__, __LINE__, "/", ioStat);
    std::cout << s << std::endl;
}

TEST_F(SicLinuxCollectorTest, UtilsTest) {
    EXPECT_EQ((unsigned long)(10), convert<unsigned long>("10"));
    EXPECT_EQ((unsigned long)(-10), convert<unsigned long>("-10"));
    EXPECT_EQ((unsigned long)(10), convert<unsigned long>("10a"));
    EXPECT_EQ((unsigned long)(1), convert<unsigned long>("1a0"));
    EXPECT_EQ((unsigned long)(0), convert<unsigned long>("a10"));
    EXPECT_EQ((unsigned long)(0), convert<unsigned long>("ab"));

    EXPECT_EQ((unsigned long)(0x10),  convertHex<unsigned long>("10"));
    EXPECT_EQ((unsigned long)(-0x10), convertHex<unsigned long>("-10"));
    EXPECT_EQ((unsigned long)(0x10a), convertHex<unsigned long>("10a"));
    EXPECT_EQ((unsigned long)(0x1),   convertHex<unsigned long>("1z0"));
    EXPECT_EQ((unsigned long)(0xa10), convertHex<unsigned long>("a10"));
    EXPECT_EQ((unsigned long)(0xab),  convertHex<unsigned long>("abh"));

    EXPECT_EQ(10, convert<int>("10"));
    EXPECT_EQ(-10, convert<int>("-10"));
    EXPECT_EQ(10, convert<int>("10a"));
    EXPECT_EQ(1, convert<int>("1a0"));
    EXPECT_EQ(0, convert<int>("a10"));
    EXPECT_EQ(0, convert<int>("ab"));
}


TEST_F(SicLinuxCollectorTest, SicGetCpuInformationTest) {
    {
        LinuxSystemInformationCollector collector(TEST_SIC_CONF_PATH / "conf/cpu/2.6.11+/");
        int ret = -1;
        ret = collector.SicInitialize();
        EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);

        SicCpuInformation cpuInformation;
        _EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector, SicGetCpuInformation(cpuInformation));

        EXPECT_EQ(cpuInformation.user, std::chrono::milliseconds{179991770});
        EXPECT_EQ(cpuInformation.nice, std::chrono::milliseconds{365760});
        EXPECT_EQ(cpuInformation.sys, std::chrono::milliseconds{92189620});
        EXPECT_EQ(cpuInformation.idle, std::chrono::milliseconds{61260058650});
        EXPECT_EQ(cpuInformation.wait, std::chrono::milliseconds{8178550});
        EXPECT_EQ(cpuInformation.irq, std::chrono::milliseconds{0});
        EXPECT_EQ(cpuInformation.softIrq, std::chrono::milliseconds{265920});
        EXPECT_EQ(cpuInformation.stolen, std::chrono::milliseconds{});

        EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);
    }

    {
        LinuxSystemInformationCollector collector{TEST_SIC_CONF_PATH / "conf/cpu/2.6.11-/"};
        int ret = -1;
        ret = collector.SicInitialize();
        EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);

        SicCpuInformation cpuInformation;
        _EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector, SicGetCpuInformation(cpuInformation));

        EXPECT_EQ(cpuInformation.user, std::chrono::milliseconds{179991770});
        EXPECT_EQ(cpuInformation.nice, std::chrono::milliseconds{365760});
        EXPECT_EQ(cpuInformation.sys, std::chrono::milliseconds{92189620});
        EXPECT_EQ(cpuInformation.idle, std::chrono::milliseconds{61260058650});
        EXPECT_EQ(cpuInformation.wait, std::chrono::milliseconds{8178550});
        EXPECT_EQ(cpuInformation.irq, std::chrono::milliseconds{0});
        EXPECT_EQ(cpuInformation.softIrq, std::chrono::milliseconds{265920});
        EXPECT_EQ(cpuInformation.stolen, std::chrono::milliseconds{0});

        EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);
    }

    {
        LinuxSystemInformationCollector collector{TEST_SIC_CONF_PATH / "conf/cpu/2.6-/"};
        int ret = -1;
        ret = collector.SicInitialize();
        EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);

        SicCpuInformation cpuInformation;
        _EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector, SicGetCpuInformation(cpuInformation));

        EXPECT_EQ(cpuInformation.user, std::chrono::milliseconds{179991770});
        EXPECT_EQ(cpuInformation.nice, std::chrono::milliseconds{365760});
        EXPECT_EQ(cpuInformation.sys, std::chrono::milliseconds{92189620});
        EXPECT_EQ(cpuInformation.idle, std::chrono::milliseconds{61260058650});
        EXPECT_EQ(cpuInformation.wait, std::chrono::milliseconds{0});
        EXPECT_EQ(cpuInformation.irq, std::chrono::milliseconds{0});
        EXPECT_EQ(cpuInformation.softIrq, std::chrono::milliseconds{0});
        EXPECT_EQ(cpuInformation.stolen, std::chrono::milliseconds{});

        EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);
    }
}

TEST_F(SicLinuxCollectorTest, Ctor2) {
    auto sicPtr = std::make_shared<Sic>();

    LinuxSystemInformationCollector collector{"/p-proc/"};
    collector.SicPtr()->cpu_list_cores = std::numeric_limits<decltype(Sic::cpu_list_cores)>::max();
    EXPECT_EQ(collector.PROCESS_DIR.string(), "/p-proc/");
}

TEST_F(SicLinuxCollectorTest, SicGetCpuListInformation2) {
    auto sicPtr = std::make_shared<Sic>();

    LinuxSystemInformationCollector collector{};
    collector.SicPtr()->cpu_list_cores = std::numeric_limits<decltype(Sic::cpu_list_cores)>::max();
    std::vector<SicCpuInformation> cpus;
    _EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector, SicGetCpuListInformation(cpus));
    EXPECT_EQ(size_t(1), cpus.size());
}

TEST_F(SicLinuxCollectorTest, SicGetCpuListInformation1) {
    LinuxSystemInformationCollector collector;
    int ret = collector.SicInitialize();
    EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);

    std::vector<SicCpuInformation> informations;
    collector.SicGetCpuListInformation(informations);

    sigar_cpu_list_t cpulist;
    sigar_cpu_list_get(mSigar, &cpulist);

    EXPECT_EQ(informations.size(), cpulist.number);
    ASSERT_EQ(collector.SicPtr()->cpu_list_cores, informations.size());

    for (size_t i = 0; i < collector.SicPtr()->cpu_list_cores; ++i) {
        absMinusExpectLessThan(informations[i].user, std::chrono::milliseconds{cpulist.data[i].user}, 100_ms);
        absMinusExpectLessThan(informations[i].nice, std::chrono::milliseconds{cpulist.data[i].nice}, 100_ms);
        absMinusExpectLessThan(informations[i].sys, std::chrono::milliseconds{cpulist.data[i].sys}, 100_ms);
        absMinusExpectLessThan(informations[i].idle, std::chrono::milliseconds{cpulist.data[i].idle}, 100_ms);
        absMinusExpectLessThan(informations[i].wait, std::chrono::milliseconds{cpulist.data[i].wait}, 100_ms);
        absMinusExpectLessThan(informations[i].irq, std::chrono::milliseconds{cpulist.data[i].irq}, 100_ms);
        absMinusExpectLessThan(informations[i].softIrq, std::chrono::milliseconds{cpulist.data[i].soft_irq}, 100_ms);
        absMinusExpectLessThan(informations[i].stolen, std::chrono::milliseconds{cpulist.data[i].stolen}, 100_ms);
        absMinusExpectLessThan(informations[i].total(), std::chrono::milliseconds{cpulist.data[i].total}, 200_ms);
    }
}

TEST_F(SicLinuxCollectorTest, GetMemoryValue) {
    EXPECT_EQ(uint64_t(1024), LinuxSystemInformationCollector::GetMemoryValue('k', 1));
    EXPECT_EQ(uint64_t(1024), LinuxSystemInformationCollector::GetMemoryValue('K', 1));

    EXPECT_EQ(uint64_t(1024 * 1024), LinuxSystemInformationCollector::GetMemoryValue('m', 1));
    EXPECT_EQ(uint64_t(1024 * 1024), LinuxSystemInformationCollector::GetMemoryValue('M', 1));
}

TEST_F(SicLinuxCollectorTest, GetMemoryRam) {
    {
        LinuxSystemInformationCollector collect;
        uint64_t ram = 2;
        _EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collect, GetMemoryRam(ram));
        auto mtrr = collect.PROCESS_DIR / "mtrr";
        if (!fs::exists(mtrr)) {
            EXPECT_EQ(decltype(ram)(0), ram);
        } else {
            EXPECT_GE(ram, decltype(ram)(0));
        }
    }
    {
        std::string mtrr{R"(reg00: base=0x00000000 (   0MB), size= 256MB: write-back, count=1
reg01: base=0xe8000000 (3712MB), size=  32MB: write-combining, count=1
reg02: base=0xe8000000 (3712MB), size=  32: write-combining, count=1
)"};
        std::vector<std::string> lines;
        GetLines(std::istringstream{mtrr}, lines);

        uint64_t ram = parseProcMtrr(lines);
        EXPECT_EQ(uint64_t(256 * 1024 * 1024), ram);
    }
}

TEST_F(SicLinuxCollectorTest, completeSicMemoryInformation) {
    {
        SicMemoryInformation memInfo;
        memInfo.total = 256 * 1024 * 1024;
        memInfo.free = 127 * 1024 * 1024;
        completeSicMemoryInformation(memInfo, 50 * 1024 * 1024, 79 * 1024 * 1024, -1, [](uint64_t &ram) {
            ram = 356 * 1024 * 1024;
        });
        EXPECT_EQ(decltype(memInfo.ram)(356), memInfo.ram);
    }
    {
        SicMemoryInformation memInfo;
        memInfo.total = 257 * 1024 * 1024;
        memInfo.free = 127 * 1024 * 1024;
        completeSicMemoryInformation(memInfo, 50 * 1024 * 1024, 79 * 1024 * 1024, -1,
                                     [](uint64_t &ram) { ram = 0; });
        EXPECT_EQ(decltype(memInfo.ram)(256 + 8), memInfo.ram);
    }
}

TEST_F(SicLinuxCollectorTest, SicGetMemoryInformationTest) {
    {
        LinuxSystemInformationCollector mSystemInformationCollector(TEST_SIC_CONF_PATH / "not-exist-dir/");
        int ret = -1;
        ret = mSystemInformationCollector.SicInitialize();
        EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);

        SicMemoryInformation a;
        _EXPECT_EQ(SIC_EXECUTABLE_FAILED, mSystemInformationCollector, SicGetMemoryInformation(a));
    }
    {
        LinuxSystemInformationCollector mSystemInformationCollector(TEST_SIC_CONF_PATH / "conf/mem/");
        int ret = -1;
        ret = mSystemInformationCollector.SicInitialize();
        EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);

        SicMemoryInformation information;
        EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, mSystemInformationCollector.SicGetMemoryInformation(information));

        EXPECT_EQ(information.total, decltype(information.total)(66977550336));
        EXPECT_EQ(information.free, decltype(information.free)(54607618048));
        EXPECT_EQ(information.used, decltype(information.used)(12369932288));
        EXPECT_EQ(information.ram, decltype(information.ram)(63874));
        EXPECT_EQ(information.actualUsed, decltype(information.actualUsed)(2062565376));
        EXPECT_EQ(information.actualFree, decltype(information.actualFree)(64914984960));
        absMinusExpectLessThan(information.freePercent, 96.9205124, 0.1);
        absMinusExpectLessThan(information.usedPercent, 3.0794876, 0.1);
    }

    {
        LinuxSystemInformationCollector mSystemInformationCollector;
        int ret = -1;
        ret = mSystemInformationCollector.SicInitialize();
        EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);

        SicMemoryInformation information;
        int ret1 = mSystemInformationCollector.SicGetMemoryInformation(information);
        EXPECT_EQ(ret1, SIC_EXECUTABLE_SUCCESS);

        sigar_mem_t sigarMem;
        int sigar_ret = sigar_mem_get(mSigar, &sigarMem);
        EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, sigar_ret);
        std::cout << "sigar, memory: total: " << sigarMem.total << ", free: " << sigarMem.free << ", used: "
                  << sigarMem.used << std::endl;

        EXPECT_EQ(information.total, sigarMem.total);
    }
}

TEST_F(SicLinuxCollectorTest, SicGetLoadInformationTest) {
    LinuxSystemInformationCollector mSystemInformationCollector;
    int ret = -1;
    ret = mSystemInformationCollector.SicInitialize();
    EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);

    SicLoadInformation information;
    _EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, mSystemInformationCollector, SicGetLoadInformation(information));

    sigar_loadavg_t loadavg;
    sigar_loadavg_get(mSigar, &loadavg);

    EXPECT_LE(std::abs(information.load1 - loadavg.loadavg[0]), 0.001);
    EXPECT_LE(std::abs(information.load5 - loadavg.loadavg[1]), 0.001);
    EXPECT_LE(std::abs(information.load15 - loadavg.loadavg[2]), 0.001);
}

TEST_F(SicLinuxCollectorTest, SicGetUptimeTest) {
    LinuxSystemInformationCollector mSystemInformationCollector;
    int ret = -1;
    ret = mSystemInformationCollector.SicInitialize();
    EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);

    double upTime;
    mSystemInformationCollector.SicGetUpTime(upTime);

    sigar_uptime_t sigarUptime{};
    sigar_uptime_get(mSigar, &sigarUptime);

    EXPECT_LE(std::abs(sigarUptime.uptime - upTime), 0.015);
}

template<typename T>
void output(const char *file, int line, const T &r) {
#define F(f) "  " << #f << ": " << ((r.f)/(1024*1024)) << " MB" << std::endl
    std::cout << file << ":" << line << ": swap{" << std::endl
              << F(total) << F(used) << F(free)
              << "}" << std::endl;
}

TEST_F(SicLinuxCollectorTest, SicGetSwapInformation01) {
    LinuxSystemInformationCollector collector;
    int ret = -1;
    ret = collector.SicInitialize();
    EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);

    SicSwapInformation information;
    collector.SicGetSwapInformation(information);
    output(__FILE__, __LINE__, information);

    sigar_swap_t swap;
    sigar_swap_get(mSigar, &swap);
    output(__FILE__, __LINE__, swap);

    absMinusExpectLessThan(information.total, swap.total, 100);
    absMinusExpectLessThan(information.used, swap.used, 100);
    absMinusExpectLessThan(information.free, swap.free, 100);
    absMinusExpectLessThan(information.pageIn, swap.page_in, 10);
    absMinusExpectLessThan(information.pageOut, swap.page_out, 10);
}

TEST_F(SicLinuxCollectorTest, GetSwapPageInfo) {
    LinuxSystemInformationCollector collector{TEST_SIC_CONF_PATH / "conf" / "proc_stat_swap"};
    _EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector, SicInitialize());

    SicSwapInformation information;
    _EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector, GetSwapPageInfo(information));
    EXPECT_EQ(decltype(information.pageIn)(1), information.pageIn);
    EXPECT_EQ(decltype(information.pageOut)(2), information.pageOut);
}

TEST_F(SicLinuxCollectorTest, SicGetSwapInformation02) {
    LinuxSystemInformationCollector collector{TEST_SIC_CONF_PATH};
    int ret = -1;
    ret = collector.SicInitialize();
    EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);

    SicSwapInformation information;
    _EXPECT_EQ(SIC_EXECUTABLE_FAILED, collector, SicGetSwapInformation(information));
}

template<typename T, typename TCompatible>
bool contains(const std::set<T> &sets, const TCompatible &key) {
    return sets.end() != sets.find(key);
}

TEST_F(SicLinuxCollectorTest, SicGetInterfaceConfigListTest) {
    {
        LinuxSystemInformationCollector mSystemInformationCollector;
        int ret = -1;
        ret = mSystemInformationCollector.SicInitialize();
        EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);

        SicInterfaceConfigList interfaceConfigList;
        _EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, mSystemInformationCollector, SicGetInterfaceConfigList(interfaceConfigList));

        sigar_net_interface_list_t interfaceList;
        sigar_net_interface_list_get(mSigar, &interfaceList);
        for (decltype(interfaceList.number) index = 0; index < interfaceList.number; ++index) {
            EXPECT_TRUE(contains(interfaceConfigList.names, interfaceList.data[index]));
        }
        sigar_net_interface_list_destroy(mSigar, &interfaceList);
    }

    {
        std::shared_ptr<LinuxSystemInformationCollector> mSystemInformationCollector;
        mSystemInformationCollector =
                std::make_shared<LinuxSystemInformationCollector>(TEST_SIC_CONF_PATH / "conf");
        int ret = -1;
        ret = mSystemInformationCollector->SicInitialize();
        EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);

        SicInterfaceConfigList interfaceConfigList;
        mSystemInformationCollector->SicGetInterfaceConfigList(interfaceConfigList);

        // int index = static_cast<int>(interfaceConfigList.names.size()) - 3;

        EXPECT_TRUE(contains(interfaceConfigList.names, "eth11"));
        EXPECT_TRUE(contains(interfaceConfigList.names, "eth12"));
        EXPECT_TRUE(contains(interfaceConfigList.names, "eth13"));


        EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);
    }
}

TEST_F(SicLinuxCollectorTest, SicGetInterfaceInformationTest) {
    {
        auto collector = std::make_shared<LinuxSystemInformationCollector>(TEST_SIC_CONF_PATH / "conf");
        int ret = -1;
        ret = collector->SicInitialize();
        EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);

        SicInterfaceConfigList interfaceConfigList;
        collector->SicGetInterfaceConfigList(interfaceConfigList);
        for (const auto &name: interfaceConfigList.names) {
            SicNetInterfaceInformation information{};
            collector->SicGetInterfaceInformation(name, information);
            if (name == "eth11") {
                EXPECT_EQ(information.rxBytes, decltype(information.rxBytes)(10622051605));
                EXPECT_EQ(information.rxPackets, decltype(information.rxPackets)(25051833));
                EXPECT_EQ(information.rxErrors, decltype(information.rxErrors)(0));
                EXPECT_EQ(information.rxDropped, decltype(information.rxDropped)(0));
                EXPECT_EQ(information.rxOverruns, decltype(information.rxOverruns)(0));
                EXPECT_EQ(information.rxFrame, decltype(information.rxFrame)(0));
                EXPECT_EQ(information.txBytes, decltype(information.txBytes)(13458290010));
                EXPECT_EQ(information.txPackets, decltype(information.txPackets)(38406699));
                EXPECT_EQ(information.txErrors, decltype(information.txErrors)(0));
                EXPECT_EQ(information.txDropped, decltype(information.txDropped)(0));
                EXPECT_EQ(information.txOverruns, decltype(information.txOverruns)(0));
                EXPECT_EQ(information.txCollisions, decltype(information.txCollisions)(0));
                EXPECT_EQ(information.txCarrier, decltype(information.txCarrier)(0));
                EXPECT_EQ(information.speed, decltype(information.speed)(-1));
            } else if (name == "eth12") {
                EXPECT_EQ(information.rxBytes, decltype(information.rxBytes)(115998540));
                EXPECT_EQ(information.rxPackets, decltype(information.rxPackets)(464906));
                EXPECT_EQ(information.rxErrors, decltype(information.rxErrors)(0));
                EXPECT_EQ(information.rxDropped, decltype(information.rxDropped)(0));
                EXPECT_EQ(information.rxOverruns, decltype(information.rxOverruns)(0));
                EXPECT_EQ(information.rxFrame, decltype(information.rxFrame)(0));
                EXPECT_EQ(information.txBytes, decltype(information.txBytes)(115998540));
                EXPECT_EQ(information.txPackets, decltype(information.txPackets)(464906));
                EXPECT_EQ(information.txErrors, decltype(information.txErrors)(0));
                EXPECT_EQ(information.txDropped, decltype(information.txDropped)(0));
                EXPECT_EQ(information.txOverruns, decltype(information.txOverruns)(0));
                EXPECT_EQ(information.txCollisions, decltype(information.txCollisions)(0));
                EXPECT_EQ(information.txCarrier, decltype(information.txCarrier)(0));
                EXPECT_EQ(information.speed, decltype(information.speed)(-1));
            }
        }
    }

    {
        std::shared_ptr<LinuxSystemInformationCollector> mSystemInformationCollector;
        mSystemInformationCollector = std::make_shared<LinuxSystemInformationCollector>();
        int ret = -1;
        ret = mSystemInformationCollector->SicInitialize();
        EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);

        SicInterfaceConfigList interfaceConfigList;
        mSystemInformationCollector->SicGetInterfaceConfigList(interfaceConfigList);

        for (auto &name: interfaceConfigList.names) {
            SicNetInterfaceInformation information{};
            mSystemInformationCollector->SicGetInterfaceInformation(name, information);
            sigar_net_interface_stat_t sigarNetInterfaceStat;
            sigar_net_interface_stat_get(mSigar, name.c_str(), &sigarNetInterfaceStat);
            absMinusExpectLessThan(information.rxBytes, sigarNetInterfaceStat.rx_bytes, 10);
            absMinusExpectLessThan(information.rxPackets, sigarNetInterfaceStat.rx_packets, 10);
            absMinusExpectLessThan(information.rxErrors, sigarNetInterfaceStat.rx_errors, 10);
            absMinusExpectLessThan(information.rxDropped, sigarNetInterfaceStat.rx_dropped, 10);
            absMinusExpectLessThan(information.rxOverruns, sigarNetInterfaceStat.rx_overruns, 10);
            absMinusExpectLessThan(information.rxFrame, sigarNetInterfaceStat.rx_frame, 10);
            absMinusExpectLessThan(information.txPackets, sigarNetInterfaceStat.tx_packets, 10);
            absMinusExpectLessThan(information.txBytes, sigarNetInterfaceStat.tx_bytes, 10);
            absMinusExpectLessThan(information.txErrors, sigarNetInterfaceStat.tx_errors, 10);
            absMinusExpectLessThan(information.txDropped, sigarNetInterfaceStat.tx_dropped, 10);
            absMinusExpectLessThan(information.txOverruns, sigarNetInterfaceStat.tx_overruns, 10);
            absMinusExpectLessThan(information.txCollisions, sigarNetInterfaceStat.tx_collisions, 10);
            absMinusExpectLessThan(information.txCarrier, sigarNetInterfaceStat.tx_carrier, 10);
            absMinusExpectLessThan(information.speed, sigarNetInterfaceStat.speed, 10);
        }
    }
}

TEST_F(SicLinuxCollectorTest, SicNetAddressToString) {
    auto mSystemInformationCollector = std::make_shared<LinuxSystemInformationCollector>();
    int ret = -1;
    ret = mSystemInformationCollector->SicInitialize();
    EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);

    unsigned int ip = 0;
    int step = 2503;
    for (int count = 0; count < 1024 * 1024 / step; ++count) {
        char addr[256];
        memset(addr, 0, 256);
        sigar_net_address_t sigarNetAddress;
        SicNetAddress sicNetAddress;
        sicNetAddress.family = SicNetAddress::SIC_AF_INET;
        sicNetAddress.addr.in = ip;
        sigarNetAddress.family = sigar_net_address_t::SIGAR_AF_INET;
        sigarNetAddress.addr.in = ip;
        sigar_net_address_to_string(mSigar, &sigarNetAddress, addr);
        std::string sicName = sicNetAddress.str();
        // std::cout << std::hex << "0x" << ip << std::dec << ":" << sicName << std::endl;
        EXPECT_EQ(sicName, addr);

        ip += step;
    }

    ip = 0;
    step = 2503;
    for (int count = 0; count < 1 * 1024 * 1024 / step; ++count) {
        char addr[256];
        memset(addr, 0, 256);
        sigar_net_address_t sigarNetAddress{};
        sigarNetAddress.family = sigar_net_address_t::SIGAR_AF_INET6;
        sigarNetAddress.addr.in6[0] = ip;
        sigarNetAddress.addr.in6[1] = ip;
        sigarNetAddress.addr.in6[2] = ip;
        sigarNetAddress.addr.in6[3] = ip;
        sigar_net_address_to_string(mSigar, &sigarNetAddress, addr);

        SicNetAddress sicNetAddress;
        sicNetAddress.family = SicNetAddress::SIC_AF_INET6;
        sicNetAddress.addr.in6[0] = ip;
        sicNetAddress.addr.in6[1] = ip;
        sicNetAddress.addr.in6[2] = ip;
        sicNetAddress.addr.in6[3] = ip;
        std::string sicName = sicNetAddress.str();

        // std::cout << std::hex << "0x" << ip << std::dec << ":" << sicName << std::endl;
        EXPECT_EQ(sicName, addr);

        ip += step;
    }

    EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);

    {
        SicNetAddress netAddr;
        netAddr.family = SicNetAddress::SIC_AF_UNSPEC;
        std::string name = netAddr.str();
        EXPECT_EQ(name, "");
    }
    {
        SicNetAddress netAddr;
        netAddr.family = SicNetAddress::SIC_AF_LINK;
        unsigned char mac[] = {1, 2, 3, 4, 5, 15, 0, 0};
        ASSERT_EQ(sizeof(netAddr.addr.mac), sizeof(mac));
        memcpy(netAddr.addr.mac, mac, sizeof(mac));
        std::string name = netAddr.str();
        EXPECT_EQ(name, "01:02:03:04:05:0F");
    }
}

TEST_F(SicLinuxCollectorTest, SicGetInterfaceConfigTest) {
    {
        std::shared_ptr<LinuxSystemInformationCollector> mSystemInformationCollector;
        mSystemInformationCollector =
                std::make_shared<LinuxSystemInformationCollector>(TEST_SIC_CONF_PATH / "conf");
        int ret = -1;
        ret = mSystemInformationCollector->SicInitialize();
        EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);

        SicInterfaceConfigList interfaceConfigList;
        mSystemInformationCollector->SicGetInterfaceConfigList(interfaceConfigList);
        for (auto &name: interfaceConfigList.names) {
            SicInterfaceConfig interfaceConfig;
            mSystemInformationCollector->SicGetInterfaceConfig(interfaceConfig, name);
            if (name == "eth11") {
                EXPECT_EQ(interfaceConfig.address6.addr.in6[0], uint32_t(33022));
                EXPECT_EQ(interfaceConfig.address6.addr.in6[1], uint32_t(0));
                EXPECT_EQ(interfaceConfig.address6.addr.in6[2], uint32_t(4282258946));
                EXPECT_EQ(interfaceConfig.address6.addr.in6[3], uint32_t(4062314750));
                EXPECT_EQ(interfaceConfig.prefix6Length, 64);
                EXPECT_EQ(interfaceConfig.scope6, 32);
            } else if (name == "eth12") {
                EXPECT_EQ(interfaceConfig.address6.addr.in6[0], uint32_t(0));
                EXPECT_EQ(interfaceConfig.address6.addr.in6[1], uint32_t(0));
                EXPECT_EQ(interfaceConfig.address6.addr.in6[2], uint32_t(0));
                EXPECT_EQ(interfaceConfig.address6.addr.in6[3], uint32_t(16777216));
                EXPECT_EQ(interfaceConfig.prefix6Length, 128);
                EXPECT_EQ(interfaceConfig.scope6, 16);
            }
        }
    }

    {
        std::shared_ptr<LinuxSystemInformationCollector> mSystemInformationCollector;
        mSystemInformationCollector =
                std::make_shared<LinuxSystemInformationCollector>();
        int ret = -1;
        ret = mSystemInformationCollector->SicInitialize();
        EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);

        SicInterfaceConfigList interfaceConfigList;
        mSystemInformationCollector->SicGetInterfaceConfigList(interfaceConfigList);
        for (auto &name: interfaceConfigList.names) {
            SicInterfaceConfig interfaceConfig;
            mSystemInformationCollector->SicGetInterfaceConfig(interfaceConfig, name);

            sigar_net_interface_config_t sigarNetInterfaceConfig;
            sigar_net_interface_config_get(mSigar, name.c_str(), &sigarNetInterfaceConfig);

            EXPECT_EQ(interfaceConfig.name, sigarNetInterfaceConfig.name);
            EXPECT_EQ(interfaceConfig.address.addr.in, sigarNetInterfaceConfig.address.addr.in);
            EXPECT_EQ(interfaceConfig.mtu, sigarNetInterfaceConfig.mtu);
            EXPECT_EQ(interfaceConfig.metric, sigarNetInterfaceConfig.metric);
            EXPECT_EQ(interfaceConfig.txQueueLen, sigarNetInterfaceConfig.tx_queue_len);
            EXPECT_EQ(interfaceConfig.netmask.addr.in, sigarNetInterfaceConfig.netmask.addr.in);
            EXPECT_EQ(interfaceConfig.address6.addr.in, sigarNetInterfaceConfig.address6.addr.in);
        }
    }
}

TEST_F(SicLinuxCollectorTest, SicGetNetStateTest1) {
    LinuxSystemInformationCollector mSystemInformationCollector;
    int ret = mSystemInformationCollector.SicInitialize();
    EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);

    const int maxTry = 3;
    for (int i = 1; i <= maxTry; i++) {
        std::cout << "------ Try Count: " << i << " -----------------" << std::endl;
        SicNetState netLink, ss, netState;
        mSystemInformationCollector.SicGetNetStateByNetLink(netLink, true, true, true, true, true);
        std::cout << "[netlink]" << netLink.toString() << std::endl;
        mSystemInformationCollector.SicGetNetStateBySS(ss, true, true, true, true, true);
        std::cout << "[ss -s]" << ss.toString() << std::endl;
        mSystemInformationCollector.SicGetNetStateByReadFile(netState, true, true, true, true, true);
        std::cout << "[net state]" << netState.toString() << std::endl;

        if (i >= maxTry || (netState == ss && netState == netLink)) {
            EXPECT_EQ(netState.udpSession, ss.udpSession);
            EXPECT_EQ(netState.udpSession, netLink.udpSession);

            EXPECT_GE(netLink.tcpStates[SIC_TCP_TOTAL], ss.tcpStates[SIC_TCP_TOTAL]);
            EXPECT_LE(netLink.tcpStates[SIC_TCP_TOTAL] - ss.tcpStates[SIC_TCP_TOTAL], 1);

            EXPECT_GE(netLink.tcpStates[SIC_TCP_NON_ESTABLISHED], ss.tcpStates[SIC_TCP_NON_ESTABLISHED]);
            EXPECT_LE(netLink.tcpStates[SIC_TCP_NON_ESTABLISHED] - ss.tcpStates[SIC_TCP_NON_ESTABLISHED], 1);
        }
    }
}

bool operator==(const SicNetState &netState, const sigar_net_stat_t &sigarNetStat) {
    bool equal = (netState.tcpOutboundTotal == sigarNetStat.tcp_outbound_total)
                 && (netState.tcpInboundTotal == sigarNetStat.tcp_inbound_total)
                 && (netState.allInboundTotal == sigarNetStat.all_inbound_total)
                 && (netState.allOutboundTotal == sigarNetStat.all_outbound_total)
                 && (netState.udpSession == sigarNetStat.udp_session);
    for (int i = 0; equal && i < SIC_TCP_UNKNOWN; ++i) {
        equal = (netState.tcpStates[i] == sigarNetStat.tcp_states[i]);
    }
    return equal;
}

bool operator!=(const SicNetState &netState, const sigar_net_stat_t &sigarNetStat) {
    return !(netState == sigarNetStat);
}

TEST_F(SicLinuxCollectorTest, SicGetNetStateTest2) {
    LinuxSystemInformationCollector mSystemInformationCollector;
    int ret = mSystemInformationCollector.SicInitialize();
    EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);

    const int maxTry = 3;
    for (int tryCount = 1; tryCount <= maxTry; tryCount++) {
        SicNetState netState;
        ret = mSystemInformationCollector.SicGetNetStateByReadFile(netState, true, true, true, true, false);
        EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);

        sigar_net_stat_t sigarNetStat;
        ret = sigar_net_stat_get(mSigar, &sigarNetStat,
                                 SIGAR_NETCONN_CLIENT | SIGAR_NETCONN_SERVER | SIGAR_NETCONN_TCP | SIGAR_NETCONN_UDP);
        EXPECT_EQ(ret, SIGAR_OK);
        if (tryCount < maxTry && netState != sigarNetStat) {
            continue; // 允许重试
        }

        EXPECT_EQ(netState.tcpOutboundTotal, sigarNetStat.tcp_outbound_total);
        EXPECT_EQ(netState.tcpInboundTotal, sigarNetStat.tcp_inbound_total);
        EXPECT_EQ(netState.allInboundTotal, sigarNetStat.all_inbound_total);
        EXPECT_EQ(netState.allOutboundTotal, sigarNetStat.all_outbound_total);
        EXPECT_EQ(netState.udpSession, sigarNetStat.udp_session);
        for (int i = 0; i <= SIC_TCP_UNKNOWN; ++i) {
            if (netState.tcpStates[i] != sigarNetStat.tcp_states[i]) {
                std::string name = GetTcpStateName(EnumTcpState(i));
                std::cout << "*** netStat[" << name << "]: " << netState.tcpStates[i]
                          << ", sigarNetStat[" << name << "]: " << sigarNetStat.tcp_states[i]
                          << std::endl;
            }
            EXPECT_EQ(netState.tcpStates[i], sigarNetStat.tcp_states[i]);
        }
    }
    {
#define get_net_state mSystemInformationCollector.SicGetNetState
        SicNetState netState;
        EXPECT_EQ(SIC_EXECUTABLE_FAILED, get_net_state(netState, true, true, true, true, true, 0));
        get_net_state(netState, true, true, true, true, true, 1);
        get_net_state(netState, true, true, true, true, true, 2);
        get_net_state(netState, true, true, true, true, true, 4);
#undef get_net_state
    }
}

TEST_F(SicLinuxCollectorTest, SicGetFileSystemListInformationTest) {
    std::shared_ptr<LinuxSystemInformationCollector> mSystemInformationCollector;
    mSystemInformationCollector =
            std::make_shared<LinuxSystemInformationCollector>();
    int ret = -1;
    ret = mSystemInformationCollector->SicInitialize();
    EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);

    std::vector<SicFileSystem> fileSystems;
    mSystemInformationCollector->SicGetFileSystemListInformation(fileSystems);

    sigar_file_system_list_t fslist;
    sigar_file_system_list_get(mSigar, &fslist);

    for (size_t i = 0; i < fileSystems.size(); ++i) {
        std::cout << fileSystems[i].dirName << " " << fileSystems[i].devName << std::endl;
        EXPECT_EQ((int) fileSystems[i].type, (int) fslist.data[i].type);
        EXPECT_EQ(fileSystems[i].dirName, fslist.data[i].dir_name);
        EXPECT_EQ(fileSystems[i].devName, fslist.data[i].dev_name);
        EXPECT_EQ(fileSystems[i].typeName, fslist.data[i].type_name);
        EXPECT_EQ(fileSystems[i].sysTypeName, fslist.data[i].sys_type_name);
    }
}

std::string toString(const sigar_disk_usage_t &r) {
    std::stringstream os;
    os << "SicDiskUsage {" << std::endl
       << "         time: " << r.time << "," << std::endl
       << "        rTime: " << r.rtime << "," << std::endl
       << "        wTime: " << r.wtime << "," << std::endl
       << "        qTime: " << r.qtime << "," << std::endl
       << "        reads: " << r.reads << "," << std::endl
       << "       writes: " << r.writes << "," << std::endl
       << "    readBytes: " << r.read_bytes << "," << std::endl
       << "   writeBytes: " << r.write_bytes << "," << std::endl
       << "     snapTime: " << r.snaptime << "," << std::endl
       << "  serviceTime: " << r.service_time << "," << std::endl
       << "        queue: " << r.queue << "," << std::endl
       << "}";
    return os.str();
}

// #ifndef ONE_AGENT // OneAgent开发环境下，有很多虚拟磁盘，sigar会失败。
TEST_F(SicLinuxCollectorTest, SicGetFileSystemUsageTest) {
    LinuxSystemInformationCollector mSystemInformationCollector;
    EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, mSystemInformationCollector.SicInitialize());

    std::vector<SicFileSystem> fileSystems;
    mSystemInformationCollector.SicGetFileSystemListInformation(fileSystems);

    int count = 0;
    for (auto const &fileSystem: fileSystems) {
        if (!IsDev(fileSystem.devName)) {
            continue;
        }
        std::cout << "[" << (count++) << "] devName: " << fileSystem.devName << ", dirName: " << fileSystem.dirName
                  << std::endl;

        const int maxTryCount = 3;
        bool toBeContinue = true;
        for (int i = 1; toBeContinue; i++) {
            mSystemInformationCollector.SicPtr()->errorMessage.clear();
            SicFileSystemUsage fileSystemUsage;
            int ret = mSystemInformationCollector.SicGetFileSystemUsage(fileSystemUsage, fileSystem.dirName);
            if (ret == SIC_EXECUTABLE_SUCCESS) {
                std::cout << "    total: " << fileSystemUsage.total / (1024 * 1024) << " G, used: "
                          << fileSystemUsage.used / (1024 * 1024) << " G" << std::endl;
            } else {
                std::cout << "SicGetFileSystemUsage(" << fileSystem.dirName << ") error(" << ret << "): "
                          << strerror(ret) << std::endl;
            }
            if (!mSystemInformationCollector.SicPtr()->errorMessage.empty()) {
                std::cout << mSystemInformationCollector.SicPtr()->errorMessage << std::endl;
            }

            sigar_file_system_usage_t sigarFileSystemUsage;
            int ret1 = sigar_file_system_usage_get(mSigar, fileSystem.dirName.c_str(), &sigarFileSystemUsage);
            if (ret1 != SIGAR_OK) {
                std::cout << "sigar_file_system_usage_get(" << fileSystem.dirName << ") error(" << ret1 << "): "
                          << strerror(ret1) << std::endl;
            }

            auto predEqual = [](const SicFileSystemUsage &a1, const sigar_file_system_usage_t &b1) {
                const auto &a = a1.disk;
                const auto &b = b1.disk;
                return a.time == b.time && a.qTime == b.qtime &&
                       std::isfinite(a.serviceTime) == std::isfinite(b.service_time) &&
                       std::isfinite(a.queue) == std::isfinite(b.queue);
            };
            // 先保证两者读到的数据一致，再往下进行比对，最大重试三次
            toBeContinue = (i == 1) ||  // 先读一次，目的是同步缓存
                           (i < maxTryCount && (ret != ret1 || !predEqual(fileSystemUsage, sigarFileSystemUsage)));
            if (toBeContinue) {
                continue;
            }
#ifdef __linux__
            const char *filename = "/proc/diskstats";
            std::cout << filename << ":" << std::endl << ReadFileContent(filename, nullptr)
                 << "----------------------------------------------------------------" << std::endl;
#endif
            std::cout << "Try Count: " << i << std::endl;

            if (ret == SIC_EXECUTABLE_SUCCESS && ret == ret1) {
                absMinusExpectLessThan(fileSystemUsage.total, sigarFileSystemUsage.total, 1024);
                absMinusExpectLessThan(fileSystemUsage.free, sigarFileSystemUsage.free, 1024);
                absMinusExpectLessThan(fileSystemUsage.used, sigarFileSystemUsage.used, 1024);
                absMinusExpectLessThan(fileSystemUsage.avail, sigarFileSystemUsage.avail, 1024);
                absMinusExpectLessThan(fileSystemUsage.files, sigarFileSystemUsage.files, 100);
                absMinusExpectLessThan(fileSystemUsage.freeFiles, sigarFileSystemUsage.free_files, 100);
            }

            /// ////////////////////////////////////////////////////////////////////////////////////////////////////////
            const SicDiskUsage &diskUsage(fileSystemUsage.disk);
            // ret = mSystemInformationCollector.SicGetDiskUsage(diskUsage, fileSystem.dirName);
            if (ret == SIC_EXECUTABLE_SUCCESS) {
                std::cout << "fileSystem.dirname(" << fileSystem.dirName << ") => " << diskUsage.string() << std::endl;
            } else {
                std::cout << "SicGetDiskUsage(" << fileSystem.dirName << ") error(" << ret << "): " << strerror(ret)
                          << std::endl;
            }

            const sigar_disk_usage_t &sigarDiskUsage(sigarFileSystemUsage.disk);
            // ret1 = sigar_disk_usage_get(mSigar, fileSystem.dirName.c_str(), &sigarDiskUsage);
            if (ret1 != SIGAR_OK) {
                std::cout << "sigar_disk_usage_get(" << fileSystem.dirName << ") error(" << ret1 << "): "
                          << strerror(ret1) << std::endl;
            } else {
                std::cout << "sigar_disk_usage_get(" << fileSystem.dirName << ") => " << toString(sigarDiskUsage)
                          << std::endl;
            }

            if (ret == SIC_EXECUTABLE_SUCCESS && ret == ret1 && (!diskUsage.devName.empty() || !diskUsage.dirName.empty())) {
                absMinusExpectLessThan(diskUsage.time, sigarDiskUsage.time, 100);
                absMinusExpectLessThan(diskUsage.rTime, sigarDiskUsage.rtime, 100);
                absMinusExpectLessThan(diskUsage.wTime, sigarDiskUsage.wtime, 100);
                absMinusExpectLessThan(diskUsage.qTime, sigarDiskUsage.qtime, 100);
                absMinusExpectLessThan(diskUsage.reads, sigarDiskUsage.reads, 100);
                absMinusExpectLessThan(diskUsage.writes, sigarDiskUsage.writes, 100);
                absMinusExpectLessThan(diskUsage.writeBytes, sigarDiskUsage.write_bytes, 1024 * 1024);
                absMinusExpectLessThan(diskUsage.readBytes, sigarDiskUsage.read_bytes, 1024 * 1024);
                absMinusExpectLessThan(diskUsage.snapTime, sigarDiskUsage.snaptime, 100);
                if (std::isnan(sigarDiskUsage.service_time)) {
                    EXPECT_TRUE(diskUsage.serviceTime == 0.0 || std::isnan(diskUsage.serviceTime));
                } else if (!std::isinf(sigarDiskUsage.service_time)) {
                    EXPECT_FALSE(std::isfinite(diskUsage.serviceTime));
                } else {
                    absMinusExpectLessThan(diskUsage.serviceTime, sigarDiskUsage.service_time, 100);
                }
                if (std::isnan(sigarDiskUsage.queue)) {
                    EXPECT_TRUE(diskUsage.queue == 0.0 || std::isnan(diskUsage.queue));
                } else if (!std::isfinite(sigarDiskUsage.queue)) {
                    EXPECT_FALSE(std::isfinite(diskUsage.queue));
                } else {
                    absMinusExpectLessThan(diskUsage.queue, sigarDiskUsage.queue, 100);
                }
            }
        }
    }
}
// #endif // !ONE_AGENT

TEST_F(SicLinuxCollectorTest, SicGetProcessListTest) {
    constexpr const int maxLoop = 5;
    for (int i = 1; i <= maxLoop; i++) {
        LinuxSystemInformationCollector collector;
        int ret = -1;
        ret = collector.SicInitialize();
        EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);

        SicProcessList processList;
        bool overflow = false;
        collector.SicGetProcessList(processList, 0, overflow);
        EXPECT_FALSE(overflow);

        sigar_proc_list_t procList;
        sigar_proc_list_get(mSigar, &procList);

        if (processList.pids.size() == procList.number || i == maxLoop) {
            EXPECT_EQ(processList.pids.size(), procList.number);
            break;
        }
    }
}

TEST_F(SicLinuxCollectorTest, SicGetProcessStateTest) {
    auto collector = std::make_shared<LinuxSystemInformationCollector>(TEST_SIC_CONF_PATH / "conf");
    int ret = -1;
    ret = collector->SicInitialize();
    EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);

    SicProcessCredName processCredName;
    collector->SicGetProcessCredName(111667, processCredName);

    SicProcessState processState;
    collector->SicGetProcessState(111667, processState);

    EXPECT_EQ(processState.name, "/usr/local/clou");
    EXPECT_EQ(processState.state, 'S');
    EXPECT_EQ(processState.parentPid, 111665);
    EXPECT_EQ(processState.tty, 1);
    EXPECT_EQ(processState.priority, 20);
    EXPECT_EQ(processState.nice, 6);
    EXPECT_EQ(processState.processor, 15);

    SicProcessArgs processArgs;
    collector->SicGetProcessArgs(111667, processArgs);
    EXPECT_EQ(processArgs.args[0],
              "java-Dfile.encoding= utf-8-jar/root/cise/agent.jar-ramqp://admin:cisemq@cise-worker-mq.alibaba-inc.com-samqp://admin:cisemq@cise-mq.alibaba-inc.com-whttp://cise-sys.oss-cn-hangzhou-zmf.aliyuncs.com/self/worker.jar-ohttp://cise-sys.oss-cn-hangzhou-zmf.aliyuncs.com/self/opshell.zip-p/root/cise/agent.pid-l/root/cise/agent.log");

    SicProcessMemoryInformation information;
    collector->SicGetProcessMemoryInformation(111667, information);
    EXPECT_EQ(information.resident, decltype(information.resident)(16240640));
    EXPECT_EQ(information.share, decltype(information.share)(5582848));
    EXPECT_EQ(information.size, decltype(information.size)(1036840960));
    EXPECT_EQ(information.minorFaults, decltype(information.minorFaults)(3120099));
    EXPECT_EQ(information.majorFaults, decltype(information.majorFaults)(2));
}

TEST_F(SicLinuxCollectorTest, SicGetProcessTest) {
    auto mSystemInformationCollector = std::make_shared<LinuxSystemInformationCollector>();
    EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, mSystemInformationCollector->SicInitialize());

    int okCount = 0;
    SicProcessList processList;
    bool overflow = false;
    mSystemInformationCollector->SicGetProcessList(processList, 0, overflow);
    for (pid_t pid: processList.pids) {
        sigar_proc_state_t sigarProcStat;
        int sigarRet = sigar_proc_state_get(mSigar, pid, &sigarProcStat);
        SicProcessState processState;
        int myRet = mSystemInformationCollector->SicGetProcessState(pid, processState);
        if (sigarRet != 0 || myRet != 0) {
            continue; // 允许个别进程错，但不能全错
        }
        okCount++;
        EXPECT_EQ(sigarRet, myRet);

        EXPECT_EQ(sigarProcStat.processor, processState.processor);
        EXPECT_EQ(sigarProcStat.tty, processState.tty);
        EXPECT_EQ(sigarProcStat.nice, processState.nice);
        EXPECT_EQ(sigarProcStat.priority, processState.priority);
        EXPECT_EQ(sigarProcStat.ppid, processState.parentPid);
        EXPECT_EQ(sigarProcStat.name, processState.name);
        EXPECT_EQ(sigarProcStat.threads, processState.threads);
        absMinusExpectLessThan(sigarProcStat.nice, processState.nice, 5);

        sigar_proc_args_t sigarArgs;
        sigarRet = sigar_proc_args_get(mSigar, pid, &sigarArgs);
        SicProcessArgs processArgs;
        myRet = mSystemInformationCollector->SicGetProcessArgs(pid, processArgs);
        EXPECT_EQ(sigarRet, myRet);
        EXPECT_EQ(sigarArgs.number, processArgs.args.size());
        for (size_t i = 0; i < processArgs.args.size(); ++i) {
            EXPECT_EQ(sigarArgs.data[i], processArgs.args[i]);
        }

        sigar_proc_fd_t sigarProcFd;
        sigar_proc_fd_get(mSigar, pid, &sigarProcFd);
        SicProcessFd processFd;
        mSystemInformationCollector->SicGetProcessFd(pid, processFd);
        absMinusExpectLessThan(sigarProcFd.total, processFd.total, 2);

        sigar_proc_cred_name_t sigarProcCredName;
        sigar_proc_cred_name_get(mSigar, pid, &sigarProcCredName);
        SicProcessCredName processCredName;
        mSystemInformationCollector->SicGetProcessCredName(pid, processCredName);
        EXPECT_EQ(processCredName.user, sigarProcCredName.user);
        EXPECT_EQ(processCredName.group, sigarProcCredName.group);

        sigar_proc_exe_t sigarProcExe;
        int resSigar = sigar_proc_exe_get(mSigar, 18917, &sigarProcExe);
        SicProcessExe processExe;
        int resSic = mSystemInformationCollector->SicGetProcessExe(18917, processExe);
        if (resSic == SIC_EXECUTABLE_SUCCESS || resSigar == SIGAR_OK) {
            EXPECT_EQ(sigarProcExe.name, processExe.name);
            EXPECT_EQ(sigarProcExe.cwd, processExe.cwd);
            EXPECT_EQ(sigarProcExe.root, processExe.root);
        }
    }
    EXPECT_GT(okCount, 0);
}

TEST_F(SicLinuxCollectorTest, SicGetVendorVersionTest) {
    std::unordered_map<std::string, std::string> releaseVendorMap{
            {"Red Hat Enterprise Linux Server release 6.0 (Santiago)",             "6.0"},
            {"Fedora release 14 (Laughlin)",                                       "14"},
            {"openSUSE 12.1 (x86_64)\n"
             "VERSION = 12.1\n"
             "CODENAME = Asparagus",                                               "12.1"},
            {"Gentoo Base System version 1.6.12",                                  "1.6.12"},
            {"Slackware 14.2 ",                                                    "14.2"},
            {"Slackware 14.2+",                                                    "14.2+"},
            {"Mandrakelinux release 10.1 (Official) for i586",                     "10.1"},
            {"VMware ESX Server 3.0.1 [Releasebuild-32039], built on Sep 25 2006", "3.0.1"},
            {"DISTRIB_ID=Ubuntu\n"
             "DISTRIB_RELEASE=9.04\n"
             "DISTRIB_CODENAME=jaunty\n"
             "DISTRIB_DESCRIPTION=\"Ubuntu 9.04\"",                                "9.04"},
            {"5.0.2",                                                              "5.0.2"}
    };

    std::shared_ptr<LinuxSystemInformationCollector> mSystemInformationCollector;
    mSystemInformationCollector = std::make_shared<LinuxSystemInformationCollector>();
    int ret = -1;
    ret = mSystemInformationCollector->SicInitialize();
    EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);

    for (auto const &vendorMap: releaseVendorMap) {
        std::string res;
        mSystemInformationCollector->SicGetVendorVersion(const_cast<std::string &>(vendorMap.first), res);
        EXPECT_EQ(res, vendorMap.second);
    }

    SicSystemInfo sicSystemInfo;
    mSystemInformationCollector->SicGetSystemInfo(sicSystemInfo);


    EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);
}

TEST_F(SicLinuxCollectorTest, SicGetProcessFd) {
    std::shared_ptr<SystemInformationCollector> collector =
            std::make_shared<LinuxSystemInformationCollector>();
    SicProcessFd procFd;
    auto ret = collector->SicGetProcessFd(GetPid(), procFd);
    EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);
    std::cout << "procFd.total = " << procFd.total << std::endl;
}

TEST_F(SicLinuxCollectorTest, New) {
    auto collector = SystemInformationCollector::New();
    EXPECT_NE(nullptr, dynamic_cast<LinuxSystemInformationCollector *>(collector.get()));
}

TEST_F(SicLinuxCollectorTest, ErrNo) {
    auto collector = SystemInformationCollector::New();

    auto path = fs::path("/user") / "hello.a.not-exist~~~~~~";
    FILE *fp = fopen(path.c_str(), "r");
    EXPECT_TRUE(fp == nullptr);
    EXPECT_NE(0, collector->ErrNo(sout{} << "fopen(" << path.string() << ")"));

    EXPECT_EQ(EINVAL, collector->ErrNo(EINVAL, sout{} << "fopen(" << path.string() << ")"));
}

TEST_F(SicLinuxCollectorTest, errorMessage) {
    auto collector = SystemInformationCollector::New();
    EXPECT_TRUE(collector->errorMessage().empty());
}

TEST_F(SicLinuxCollectorTest, SicReadNetFile) {
    LinuxSystemInformationCollector collector{TEST_SIC_CONF_PATH / "conf"};

    {
        SicNetState netState;
        EXPECT_EQ(SIC_EXECUTABLE_FAILED, collector.SicReadNetFile(netState, SIC_NET_CONN_TCP, true, true, ""));
    }
    {
        SicNetState netState;
        auto file = collector.PROCESS_DIR / "net/tcp-malform";
        _EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector, SicReadNetFile(netState, SIC_NET_CONN_TCP, false, false, file));
        std::cout << netState.toString() << std::endl;
        int sum = 0;
        for (auto n: netState.tcpStates) {
            sum += n;
        }
        EXPECT_EQ(decltype(sum)(0), sum);
    }
    {
        SicNetState netState;
        auto file = collector.PROCESS_DIR / "net/tcp";
        _EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector, SicReadNetFile(netState, SIC_NET_CONN_TCP, true, true, file));
        std::cout << netState.toString() << std::endl;
        EXPECT_EQ(4, netState.tcpStates[SIC_TCP_TIME_WAIT]);
    }

    {
        SicNetState netState;
        auto file = collector.PROCESS_DIR / "net/tcp";
        _EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector, SicReadNetFile(netState, SIC_NET_CONN_TCP, false, false, file));
        std::cout << netState.toString() << std::endl;
        EXPECT_EQ(0, netState.tcpStates[SIC_TCP_TIME_WAIT]);
    }
}

TEST_F(SicLinuxCollectorTest, SicGetFileSystemType01) {
    SicFileSystem fs;
    fs.type = SIC_FILE_SYSTEM_TYPE_UNKNOWN;
    fs.sysTypeName = "iso9660";
    LinuxSystemInformationCollector::SicGetFileSystemType(fs.sysTypeName, fs.type, fs.typeName);
    EXPECT_EQ(SIC_FILE_SYSTEM_TYPE_CDROM, fs.type);
    EXPECT_EQ(fs.typeName, "cdrom");
}

TEST_F(SicLinuxCollectorTest, SicGetFileSystemType02) {
    SicFileSystem fs;
    fs.type = SIC_FILE_SYSTEM_TYPE_UNKNOWN;
    fs.sysTypeName = "fat";
    LinuxSystemInformationCollector::SicGetFileSystemType(fs.sysTypeName, fs.type, fs.typeName);
    EXPECT_EQ(SIC_FILE_SYSTEM_TYPE_LOCAL_DISK, fs.type);
    EXPECT_EQ(fs.typeName, "local");
}

TEST_F(SicLinuxCollectorTest, SicGetFileSystemType03) {
    SicFileSystem fs;
    fs.type = SIC_FILE_SYSTEM_TYPE_MAX;
    fs.sysTypeName = "Unknown File System Type";
    LinuxSystemInformationCollector::SicGetFileSystemType(fs.sysTypeName, fs.type, fs.typeName);
    EXPECT_EQ(SIC_FILE_SYSTEM_TYPE_NONE, fs.type);
    EXPECT_EQ(fs.typeName, "none");
}

TEST_F(SicLinuxCollectorTest, RefreshLocalDisk) {
    LinuxSystemInformationCollector collector;
    _EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector, SicInitialize());

    collector.RefreshLocalDisk();
    int count = 1;
    for (auto &it: collector.SicPtr()->fileSystemCache) {
        std::cout << "[" << count++ << "] name: " << it.second->name
                  << ", isPartition: " << std::boolalpha << it.second->isPartition << std::noboolalpha << std::endl;
    }
}

TEST_F(SicLinuxCollectorTest, GetDiskStat) {
    LinuxSystemInformationCollector collector;
    _EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector, SicInitialize());
    collector.RefreshLocalDisk();

    SicDiskUsage disk{}, device{};
    _EXPECT_EQ(SIC_EXECUTABLE_FAILED, collector, GetDiskStat(0, "/no-no-no", disk, device));
    EXPECT_NE(collector.SicPtr()->errorMessage.find("not exist"), std::string::npos);
}

TEST_F(SicLinuxCollectorTest, GetDiskStat2) {
    LinuxSystemInformationCollector collector{TEST_SIC_CONF_PATH / "conf"};
    _EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector, SicInitialize());
    collector.RefreshLocalDisk();

    std::cout << "proc dir: " << collector.PROCESS_DIR << std::endl;

    SicDiskUsage disk{}, device{};
    _EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector, GetDiskStat(makedev(254, 1), "", disk, device));
    EXPECT_EQ(disk.readBytes, decltype(disk.readBytes)(1851478 * 512));
}

TEST_F(SicLinuxCollectorTest, SicGetIOstat) {
    LinuxSystemInformationCollector collector;
    _EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector, SicInitialize());
    collector.RefreshLocalDisk();

    if (!collector.SicPtr()->fileSystemCache.empty()) {
        {
            SicDiskUsage disk, deviceUsage;
            std::shared_ptr<SicIODev> ioDev;
            std::string dirName;
            for (auto &it: collector.SicPtr()->fileSystemCache) {
                dirName = it.second->name;
            }
            // docker上这个是跑不过去的, 因此暂时不判断返回值
            collector.SicGetIOstat(dirName, disk, ioDev, deviceUsage);
        }
        {
            SicDiskUsage disk, deviceUsage;
            std::shared_ptr<SicIODev> ioDev;
            std::string dirName;
            for (auto &it: collector.SicPtr()->fileSystemCache) {
                dirName = it.second->name;
            }
            const_cast<fs::path &>(collector.PROCESS_DIR) = TEST_SIC_CONF_PATH / "conf" / "diskstats_onlyram";
            _EXPECT_EQ(SIC_EXECUTABLE_FAILED, collector, SicGetIOstat(dirName, disk, ioDev, deviceUsage));

            SicDiskUsage diskUsage;
            _EXPECT_EQ(SIC_EXECUTABLE_FAILED, collector, SicGetDiskUsage(diskUsage, dirName));
        }
        {
            const_cast<fs::path &>(collector.PROCESS_DIR) = TEST_SIC_CONF_PATH / "conf";
            struct stat ioStat{0};
            ioStat.st_rdev = makedev(254, 1);
            SicDiskUsage disk, deviceUsage;
            _EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector,
                       GetDiskStat(ioStat.st_rdev, "/pesudo-file", disk, deviceUsage));
            EXPECT_EQ(decltype(deviceUsage.reads)(31508), deviceUsage.reads);
            EXPECT_EQ(decltype(disk.reads)(31327), disk.reads);
        }
    }

}

TEST_F(SicLinuxCollectorTest, CalDiskUsage) {
    LinuxSystemInformationCollector collector;
    _EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector, SicInitialize());

    auto doTest = [&](SicLinuxIOType ioType) {
        collector.SicPtr()->ioType = ioType;
        double uptime;
        collector.SicGetUpTime(uptime);

        SicIODev ioDev;
        ioDev.isPartition = true;
        ioDev.diskUsage.snapTime = uptime - 1000.0; // 前提1s
        ioDev.diskUsage.time = 0;
        ioDev.diskUsage.qTime = 0;

        SicDiskUsage diskUsage;
        diskUsage.snapTime = 0;
        diskUsage.time = 500;
        diskUsage.qTime = 100;

        collector.CalDiskUsage(ioDev, diskUsage);
        EXPECT_NE(-1, diskUsage.queue);
    };
    doTest(IO_STATE_DISKSTATS);
    doTest(IO_STATE_PARTITIONS);
}

TEST_F(SicLinuxCollectorTest, SicGetDiskUsage02) {
    LinuxSystemInformationCollector collector;
    _EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector, SicInitialize());

    collector.SicPtr()->ioType = IO_STATE_PARTITIONS;
    SicDiskUsage diskUsage;
    EXPECT_EQ(ENOENT, collector.SicGetDiskUsage(diskUsage, "/"));
}

TEST_F(SicLinuxCollectorTest, SicGetIODev) {
    LinuxSystemInformationCollector collector;
    _EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector, SicInitialize());

    std::string dirName{"not-exist-device"};
    auto ioDev = collector.SicGetIODev(dirName);
    EXPECT_FALSE((bool) ioDev);
    EXPECT_EQ(std::string{"/dev/not-exist-device"}, dirName);
    EXPECT_NE(collector.SicPtr()->errorMessage.find("stat"), std::string::npos);
}

TEST_F(SicLinuxCollectorTest, SicGetProcessExe) {
    LinuxSystemInformationCollector collector;
    _EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector, SicInitialize());

    SicProcessExe procExe;
    _EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector, SicGetProcessExe(GetPid(), procExe));
    std::cout << "PID: " << GetPid() << std::endl
              << "     cwd: " << procExe.cwd << std::endl
              << "    name: " << procExe.name << std::endl
              << "    root: " << procExe.root << std::endl;
    EXPECT_NE(procExe.cwd, "");
    EXPECT_NE(procExe.name, "");
    EXPECT_NE(procExe.root, "");
}

TEST_F(SicLinuxCollectorTest, SicReadProcessStat_Fail) {
    LinuxSystemInformationCollector collector;
    _EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector, SicInitialize());

    // collector.SicPtr()->linuxProcessInfo.pid = -1; // 清缓存
    {
        const_cast<fs::path &>(collector.PROCESS_DIR) =
                TEST_SIC_CONF_PATH / "conf" / "proc_pid_stat_without_name";
        SicProcessState state;
        _EXPECT_EQ(SIC_EXECUTABLE_FAILED, collector, SicGetProcessState(1, state));
        EXPECT_NE(collector.SicPtr()->errorMessage.find("process name"), std::string::npos);
    }
    // collector.SicPtr()->linuxProcessInfo.pid = -1; // 清缓存
    {
        const_cast<fs::path &>(collector.PROCESS_DIR) =
                TEST_SIC_CONF_PATH / "conf" / "proc_pid_stat_without_name";
        SicLinuxProcessInfoCache processInfo{};
        _EXPECT_EQ(SIC_EXECUTABLE_FAILED, collector, SicReadProcessStat(1, processInfo));
        EXPECT_NE(collector.SicPtr()->errorMessage.find("process name"), std::string::npos);

        // collector.SicPtr()->linuxProcessInfo.result = 123456; // 确定使用了缓存
        // _EXPECT_EQ(123456, collector, SicReadProcessStat(1));
    }
    // collector.SicPtr()->linuxProcessInfo.pid = -1; // 清缓存
    {
        const_cast<fs::path &>(collector.PROCESS_DIR) =
                TEST_SIC_CONF_PATH / "conf" / "proc_pid_stat_item_lack";
        SicLinuxProcessInfoCache processInfo{};
        _EXPECT_EQ(SIC_EXECUTABLE_FAILED, collector, SicReadProcessStat(1, processInfo));
        EXPECT_NE(collector.SicPtr()->errorMessage.find("unexpected item count"), std::string::npos);
    }
}

TEST_F(SicLinuxCollectorTest, SicReadProcessStat) {
    LinuxSystemInformationCollector collector;
    _EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector, SicInitialize());

    SicLinuxProcessInfoCache procInfo{};
    _EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector, SicReadProcessStat(GetPid(), procInfo));

    // auto &procInfo = collector.SicPtr()->linuxProcessInfo;
    auto nowMillis = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    extern const int ClkTck;  // defined in linux_system_information_collector.cpp
    namespace sc = std::chrono;
    std::cout << "ticks    : " << ClkTck << std::endl
              << "bootTime : " << collector.SicPtr()->bootSeconds
              << " [" << date::format<0>(std::chrono::FromSeconds(collector.SicPtr()->bootSeconds)) << "]" << std::endl
              << "startTime: " << sc::duration_cast<sc::seconds>(procInfo.startTime.time_since_epoch()).count()
              << " [" << date::format<6>(procInfo.startTime) << "]" << std::endl
              << "now      : " << date::format<3>(system_clock::time_point{nowMillis}) << std::endl;

    EXPECT_GT(procInfo.startMillis(), 0);

    // NOTE: 如果失败，请优化单测用例，使其在30分钟内跑完
    EXPECT_LT(Diff(nowMillis.count(), procInfo.startMillis()), 30 * 60 * 1000);
}

TEST_F(SicLinuxCollectorTest, SicGetProcessCpuInformation) {
    LinuxSystemInformationCollector collector;
    _EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector, SicInitialize());

    {
        SicProcessCpuInformation cpuInformation;
        _EXPECT_EQ(SIC_EXECUTABLE_FAILED, collector, SicGetProcessCpuInformation(-1, cpuInformation));
        std::cout << collector.SicPtr()->errorMessage << std::endl;
    }
    {
        SicProcessCpuInformation cpuInformation;
        _EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector, SicGetProcessCpuInformation(GetPid(), cpuInformation));
        // 走缓存
        SicProcessCpuInformation cpuInformation2;
        _EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector, SicGetProcessCpuInformation(GetPid(), cpuInformation2));
        EXPECT_EQ(decltype(cpuInformation2.percent)(0), cpuInformation2.percent);
        // 跑1ms以使当前进程产生cpu使用
        int loopCount = 0;
        auto now = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - now < std::chrono::milliseconds{1}) {
            loopCount++;
        }
        std::cout << "loopCount: " << loopCount << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1050));
        SicProcessCpuInformation cpuInfo;
        _EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector, SicGetProcessCpuInformation(GetPid(), cpuInfo));

        using namespace std::chrono;
        auto timeToString = [](uint64_t millis) -> std::string {
            return date::format<3>(FromMillis(millis));
        };
        auto print = [timeToString](const SicProcessCpuInformation &cpuInfo, const std::string &name) {
            using namespace std::chrono;
            auto span = duration_cast<microseconds>(steady_clock::now() - cpuInfo.lastTime);
            std::cout << name << " = SicProcessCpuInformation {" << std::endl
                      << "  startTime: " << cpuInfo.startTime << "(" << timeToString(cpuInfo.startTime) << ")"
                      << std::endl
                      << "       user: " << cpuInfo.user << std::endl
                      << "        sys: " << cpuInfo.sys << std::endl
                      << "      total: " << cpuInfo.total << std::endl
                      << "   lastTime: " << timeToString(ToMillis(std::chrono::system_clock::now() - span))
                      << std::endl
                      << "}" << std::endl;
        };
        print(cpuInformation, "cpuInformation");
        print(cpuInfo, "cpuInfo");

        EXPECT_GT(cpuInfo.lastTime, cpuInformation.lastTime);
    }
}

TEST_F(SicLinuxCollectorTest, NowMillis) {
    uint64_t millis1 = NowMillis();

    const static uint64_t MILLISECOND = 1000;
    const static uint64_t MICROSECOND = 1000 * 1000;

    timeval time{};
    gettimeofday(&time, nullptr);
    uint64_t millis2 = ((time.tv_sec * MICROSECOND) + time.tv_usec) / MILLISECOND;

    std::cout << "system_clock::now(): " << millis1 << " ms" << std::endl
              << "     gettimeofday(): " << millis2 << " ms" << std::endl;
    uint64_t diff = millis2 - millis1;
    EXPECT_LT(diff, uint32_t(50));
    EXPECT_GE(diff, uint32_t(0));
}

TEST_F(SicLinuxCollectorTest, SicCleanProcessCpuCacheIfNecessary) {
    LinuxSystemInformationCollector collector;
    _EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector, SicInitialize());

    auto now = std::chrono::steady_clock::now();
    auto &cache = collector.SicPtr()->processCpuCache;
    cache.cleanPeriod = 1_s;
    cache.nextCleanTime = now - cache.cleanPeriod;
    cache.entryExpirePeriod = 0_s;
    cache.entries[{1, false}].expireTime = now - 10_s; // 需要被清理掉
    cache.entries[{2, false}].expireTime = now + 1_min;
    collector.SicCleanProcessCpuCacheIfNecessary();
    EXPECT_EQ(decltype(cache.entries.size())(1), cache.entries.size());

    EXPECT_NE(cache.entries.end(), cache.entries.find({2, false}));
}

TEST_F(SicLinuxCollectorTest, SicGetProcessThreads) {
    LinuxSystemInformationCollector collector;
    _EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector, SicInitialize());

    {
        const_cast<fs::path &>(collector.PROCESS_DIR) = TEST_SIC_CONF_PATH / "conf";
        uint64_t threads = 0;
        EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector.SicGetProcessThreads(111667, threads));
        EXPECT_EQ(decltype(threads)(25), threads);
    }
    {
        const_cast<fs::path &>(collector.PROCESS_DIR) =
                TEST_SIC_CONF_PATH / "conf" / "proc_1_status_no_threads";
        uint64_t threads = {1};
        EXPECT_EQ(SIC_EXECUTABLE_FAILED, collector.SicGetProcessThreads(1, threads));
        EXPECT_EQ(decltype(threads)(0), threads);
    }
}

TEST_F(SicLinuxCollectorTest, SicGetProcessMemoryInformation) {
    LinuxSystemInformationCollector collector;
    _EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector, SicInitialize());
    EXPECT_EQ(collector.SicPtr()->errorMessage, "");

    const_cast<fs::path &>(collector.PROCESS_DIR) = TEST_SIC_CONF_PATH / "conf" / "proc_err_statm";

    SicProcessMemoryInformation info;
    _EXPECT_EQ(SIC_EXECUTABLE_FAILED, collector, SicGetProcessMemoryInformation(1, info));
    std::cout << collector.SicPtr()->errorMessage << std::endl;
    EXPECT_NE(collector.SicPtr()->errorMessage.find("not a valid statm"), std::string::npos);
}

TEST_F(SicLinuxCollectorTest, SicGetLoadInformation) {
    LinuxSystemInformationCollector collector;
    _EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector, SicInitialize());
    EXPECT_EQ(collector.SicPtr()->errorMessage, "");

    {
        SicLoadInformation load;
        load.load1 = -1;
        _EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector, SicGetLoadInformation(load));
        EXPECT_GE(load.load1, 0);
    }
    const_cast<fs::path &>(collector.PROCESS_DIR) = TEST_SIC_CONF_PATH / "conf" / "proc_load_invalid";
    {
        SicLoadInformation load;
        _EXPECT_EQ(SIC_EXECUTABLE_FAILED, collector, SicGetLoadInformation(load));
    }
}

TEST_F(SicLinuxCollectorTest, SicUpdateNetState) {
    LinuxSystemInformationCollector collector;
    _EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector, SicInitialize());
    EXPECT_EQ(collector.SicPtr()->errorMessage, "");

    SicNetState netState;

    const int LISTEN_PORT = 80;
    {
        // Listen
        SicNetConnectionInformation connInfo;
        connInfo.type = SIC_NET_CONN_TCP;
        connInfo.state = SIC_TCP_LISTEN;
        connInfo.localPort = LISTEN_PORT;
        EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector.SicUpdateNetState(connInfo, netState));
        EXPECT_EQ(size_t(1), collector.SicPtr()->netListenCache.size());
        EXPECT_NE(collector.SicPtr()->netListenCache.find(80), collector.SicPtr()->netListenCache.end());
        EXPECT_EQ(decltype(netState.tcpInboundTotal)(0), netState.tcpInboundTotal);
        EXPECT_EQ(decltype(netState.tcpOutboundTotal)(0), netState.tcpOutboundTotal);
        EXPECT_EQ(decltype(netState.udpSession)(0), netState.udpSession);
    }
    {
        SicNetConnectionInformation connInfo;
        connInfo.type = SIC_NET_CONN_TCP;
        connInfo.state = SIC_TCP_ESTABLISHED;
        connInfo.localPort = 32786;
        EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector.SicUpdateNetState(connInfo, netState));
        EXPECT_EQ(size_t(1), collector.SicPtr()->netListenCache.size());
        EXPECT_NE(collector.SicPtr()->netListenCache.find(80), collector.SicPtr()->netListenCache.end());
        EXPECT_EQ(decltype(netState.tcpInboundTotal)(0), netState.tcpInboundTotal);
        EXPECT_EQ(decltype(netState.tcpOutboundTotal)(1), netState.tcpOutboundTotal);
        EXPECT_EQ(decltype(netState.udpSession)(0), netState.udpSession);
    }
    {
        SicNetConnectionInformation connInfo;
        connInfo.type = SIC_NET_CONN_TCP;
        connInfo.state = SIC_TCP_ESTABLISHED;
        connInfo.localPort = LISTEN_PORT;
        EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector.SicUpdateNetState(connInfo, netState));
        EXPECT_EQ(size_t(1), collector.SicPtr()->netListenCache.size());
        EXPECT_NE(collector.SicPtr()->netListenCache.find(80), collector.SicPtr()->netListenCache.end());
        EXPECT_EQ(decltype(netState.tcpInboundTotal)(1), netState.tcpInboundTotal);
        EXPECT_EQ(decltype(netState.tcpOutboundTotal)(1), netState.tcpOutboundTotal);
        EXPECT_EQ(decltype(netState.udpSession)(0), netState.udpSession);
    }
    {
        SicNetConnectionInformation connInfo;
        connInfo.type = SIC_NET_CONN_UDP;
        EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector.SicUpdateNetState(connInfo, netState));
        EXPECT_EQ(size_t(1), collector.SicPtr()->netListenCache.size());
        EXPECT_NE(collector.SicPtr()->netListenCache.find(80), collector.SicPtr()->netListenCache.end());
        EXPECT_EQ(decltype(netState.tcpInboundTotal)(1), netState.tcpInboundTotal);
        EXPECT_EQ(decltype(netState.tcpOutboundTotal)(1), netState.tcpOutboundTotal);
        EXPECT_EQ(decltype(netState.udpSession)(1), netState.udpSession);
    }

    {
        // Listen on IPv6
        SicNetConnectionInformation connInfo;
        connInfo.type = SIC_NET_CONN_TCP;
        connInfo.state = SIC_TCP_LISTEN;
        connInfo.localPort = LISTEN_PORT;
        connInfo.localAddress.family = SicNetAddress::SIC_AF_INET6; // 不统计IPv6
        EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector.SicUpdateNetState(connInfo, netState));
        EXPECT_EQ(size_t(1), collector.SicPtr()->netListenCache.size());
        EXPECT_NE(collector.SicPtr()->netListenCache.find(80), collector.SicPtr()->netListenCache.end());
        EXPECT_EQ(decltype(netState.tcpInboundTotal)(1), netState.tcpInboundTotal);
        EXPECT_EQ(decltype(netState.tcpOutboundTotal)(1), netState.tcpOutboundTotal);
        EXPECT_EQ(decltype(netState.udpSession)(1), netState.udpSession);
    }
}

TEST_F(SicLinuxCollectorTest, SicGetNetAddress) {
    auto collector = static_cast<LinuxSystemInformationCollector *>(nullptr);
    {
        SicNetAddress address;
        collector->SicGetNetAddress("020011AC020011AC020011AC", address);
        EXPECT_EQ(SicNetAddress::SIC_AF_INET6, address.family);
    }
    {
        SicNetAddress address;
        collector->SicGetNetAddress("020011AC", address);
        EXPECT_EQ(SicNetAddress::SIC_AF_INET, address.family);
    }
}

TEST_F(SicLinuxCollectorTest, SicGetVendorVersion) {
    LinuxSystemInformationCollector collector;
    _EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector, SicInitialize());
    EXPECT_EQ(collector.SicPtr()->errorMessage, "");

    std::string version;
    collector.SicGetVendorVersion("hahahah", version);
    EXPECT_EQ(version, "");

    collector.SicGetVendorVersion("LSB Version:\t:core-4.1-amd64:core-4.1-noarch", version);
    EXPECT_EQ(version, "4.1");
}

TEST_F(SicLinuxCollectorTest, WalkDirs) {
    LinuxSystemInformationCollector collector;
    int count{0};
    collector.walkDigitDirs(TEST_SIC_CONF_PATH / "conf", [&](const std::string &s) {
        std::cout << s << std::endl;
        EXPECT_TRUE(IsInt(s));
        ++count;
    });
    EXPECT_GT(count, 0);
}

TEST_F(SicLinuxCollectorTest, SicGetThreadCpuInformation) {
    LinuxSystemInformationCollector collector(TEST_SIC_CONF_PATH / "conf");
    EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector.SicInitialize());

    SicThreadCpu cpu;
    _EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector, SicGetThreadCpuInformation(111667, 111668, cpu));
    EXPECT_EQ(std::chrono::milliseconds{183030}, cpu.user);
    EXPECT_EQ(std::chrono::milliseconds{44260}, cpu.sys);
    EXPECT_EQ(cpu.total, cpu.sys + cpu.user);
}

TEST_F(SicLinuxCollectorTest, Which) {
    {
        fs::path path = Which("ls");
        EXPECT_FALSE(path.empty());
        std::cout << boost::format("which ls%20t => %1%") % path << std::endl;
    }
    // {
    //     // 不一定所有的Linux上都有dmidecode
    //     fs::path path = Which("dmidecode");
    //     EXPECT_FALSE(path.empty());
    //     std::cout << boost::format("which dmidecode%20t => %1%") % path << std::endl;
    // }

    {
        std::string self = GetExecPath().filename().string();
        fs::path path = Which(self);
        EXPECT_TRUE(path.empty());
        std::cout << boost::format("which %1%%20t => %2%") % self % path << std::endl;
    }
    {
        std::string self = GetExecPath().filename().string();
        fs::path path = Which(self, {GetExecPath().parent_path()});
        EXPECT_FALSE(path.empty());
        std::cout << boost::format("which %1%%20t => %2%") % self % path << std::endl;
    }
}

TEST_F(SicLinuxCollectorTest, queryDevSerialId) {
    LinuxSystemInformationCollector collector;
    EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector.SicInitialize());

    if (fs::exists("/dev/vda")) {
        std::map<std::string, std::string> maps = collector.queryDevSerialId("/dev/vda");
        EXPECT_EQ(size_t(1), maps.size());
        std::cout << "/dev/vda serial id: " << maps["/dev/vda"] << std::endl;
    }
}

// #endif // linux、unix
