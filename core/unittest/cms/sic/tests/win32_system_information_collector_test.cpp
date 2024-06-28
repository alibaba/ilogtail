//
// Created by liuze.xlz on 2020/11/23.
//
#include "gtest/gtest.h"
#include <sigar.h>
#include <iostream>
#include <memory>
#include <chrono>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <IPTypes.h>

#include "sic/win32_system_information_collector.h"
#include "sic/system_information_collector_util.h"
#include "common/FileUtils.h"
#include "common/Arithmetic.h"
#include "common/TimeFormat.h"
#include "common/ConsumeCpu.h"
#include "common/UnitTestEnv.h"
#include "common/ExecCmd.h"
#include "common/Chrono.h"

#ifdef WITH_SIGAR
extern "C" {
#include <sigar_format.h>
}
#endif

#ifdef max
#	undef max
#	undef min
#endif

using namespace std::chrono;
using namespace WinConst;

extern const size_t SRC_ROOT_OFFSET;

#ifdef WIN32
#	define ToGBK UTF8ToGBK
#else
#	define ToGBK(s) (s)
#endif

class SicWin32CollectorTest : public testing::Test
{
protected:
	void SetUp() override
	{
#ifdef WITH_SIGAR
		sigar_open(&mSigar);
#endif
		systemCollector = std::make_shared<Win32SystemInformationCollector>();
		mSicPtr = systemCollector->SicPtr();
		int ret = systemCollector->SicInitialize();
		ASSERT_EQ(ret, SIC_EXECUTABLE_SUCCESS);
	}
	void TearDown() override
	{
#ifdef WITH_SIGAR
		sigar_close(mSigar);
#endif
	}

	std::shared_ptr<Sic> mSicPtr;
	std::shared_ptr<Win32SystemInformationCollector> systemCollector;
#ifdef WITH_SIGAR
	sigar_t* mSigar = nullptr;
#endif
};

TEST_F(SicWin32CollectorTest, SicInitialize)
{
	auto S = [](const char *s) {
		return s? s: "<nil>";
	};
	std::cout << "ENV[\"PATH\"]: " << std::getenv("PATH") << std::endl;
	std::cout << "ENV[\"HOME\"]: " << S(std::getenv("HOME")) << std::endl;
	std::cout << "ENV[\"USER\"]: " << S(std::getenv("USER")) << std::endl;
	std::cout << "ENV[\"TMP\"]: " << S(std::getenv("TMP")) << std::endl;
	std::string result;
	ExecCmd(R"(%SystemRoot%\System32\wbem\wmic cpu get NumberOfLogicalProcessors | %SystemRoot%\System32\findstr /v "NumberOfLogicalProcessors")", &result);
	EXPECT_EQ(stoi(result), mSicPtr->cpu_nums);
}

#if 0
// 该结果可与typeperf进行比较 
// 该命令的文档: https://learn.microsoft.com/en-us/windows-server/administration/windows-commands/typeperf
// typeperf "\Processor(_Total)\% User Time" "\Processor(_Total)\% Idle Time" "\Processor(_Total)\% Privileged Time" "\Processor(_Total)\% Interrupt Time"
TEST_F(SicWin32CollectorTest, TypePerf) {
#define COUT(VAR) std::cout << date::format<3>(std::chrono::system_clock::now()) << " " << VAR.user*100 << ", " << VAR.idle*100 << ", " << VAR.sys*100 << ", " << VAR.irq*100 << std::endl
	SicCpuInformation oldCpuInformationPerf = {};
	for (int i = 0; i < 10; i++) {
		SicCpuInformation curPerf;
		std::string errorMessage;
		systemCollector->PerfLibGetCpuInformation(curPerf, errorMessage);
		if (i > 0) {
			auto diff = oldCpuInformationPerf / curPerf;
			COUT(diff);
		}
		oldCpuInformationPerf = curPerf;
		std::this_thread::sleep_for(std::chrono::seconds{ 1 });
	}
#undef COUT
}
#endif

TEST_F(SicWin32CollectorTest, SicGetCpuInformationApiTest)
{
	const double maxValue = 10;

	std::string errorMessage;
	SicCpuInformation oldCpuInformationNtdll = {};
	systemCollector->NtSysGetCpuInformation(oldCpuInformationNtdll, errorMessage);
	SicCpuInformation oldCpuInformationPerf = {};
	systemCollector->PerfLibGetCpuInformation(oldCpuInformationPerf, errorMessage);

	//std::cout << "oldCpuInformationNtdll = " << oldCpuInformationNtdll.str(true);
	//std::cout << "oldCpuInformationPerf = " << oldCpuInformationPerf.str(true);

	//Sleep(2000);
	std::this_thread::sleep_for(std::chrono::seconds{ 1 });

	SicCpuInformation newCpuInformationNtdll = {};
	SicCpuInformation newCpuInformationPerf = {};
	systemCollector->NtSysGetCpuInformation(newCpuInformationNtdll, errorMessage);
	systemCollector->PerfLibGetCpuInformation(newCpuInformationPerf, errorMessage);
	//std::cout << "newCpuInformationNtdll = " << newCpuInformationNtdll.str(true);
	//std::cout << "newCpuInformationPerf = " << newCpuInformationPerf.str(true);

	//std::cout << "----------------------------------------------------------" << std::endl;
	SicCpuPercent cpuPercentNtdll = oldCpuInformationNtdll / newCpuInformationNtdll;
	std::cout << "cpuPercentNtdll = " << cpuPercentNtdll.string() << std::endl;
	SicCpuPercent cpuPercentPerf = oldCpuInformationPerf / newCpuInformationPerf;
	std::cout << "cpuPercentPerf = " << cpuPercentPerf.string() << std::endl;
	//std::cout << "----------------------------------------------------------" << std::endl;

	EXPECT_LE(abs(cpuPercentNtdll.user - cpuPercentPerf.user) * 100, maxValue);
	EXPECT_LE(abs(cpuPercentNtdll.sys - cpuPercentPerf.sys) * 100, maxValue);
	EXPECT_LE(abs(cpuPercentNtdll.nice - cpuPercentPerf.nice) * 100, maxValue);
	EXPECT_LE(abs(cpuPercentNtdll.idle - cpuPercentPerf.idle) * 100, maxValue);
	EXPECT_LE(abs(cpuPercentNtdll.wait - cpuPercentPerf.wait) * 100, maxValue);
	EXPECT_LE(abs(cpuPercentNtdll.irq - cpuPercentPerf.irq) * 100, maxValue);
	EXPECT_LE(abs(cpuPercentNtdll.softIrq - cpuPercentPerf.softIrq) * 100, maxValue);
	EXPECT_LE(abs(cpuPercentNtdll.stolen - cpuPercentPerf.stolen) * 100, maxValue);
	EXPECT_LE(abs(cpuPercentNtdll.combined - cpuPercentPerf.combined) * 100, maxValue);

	std::vector<SicCpuInformation> oldInformationsNtdll = {};
	std::vector<SicCpuInformation> oldInformationsPerf = {};
	systemCollector->NtSysGetCpuListInformation(oldInformationsNtdll, errorMessage);
	systemCollector->PerfLibGetCpuListInformation(oldInformationsPerf, errorMessage);

	//Sleep(2000);
	std::this_thread::sleep_for(std::chrono::seconds{ 1 });

	std::vector<SicCpuInformation> newInformationsNtdll = {};
	std::vector<SicCpuInformation> newInformationsPerf = {};
	systemCollector->NtSysGetCpuListInformation(newInformationsNtdll, errorMessage);
	systemCollector->PerfLibGetCpuListInformation(newInformationsPerf, errorMessage);
	EXPECT_EQ(oldInformationsNtdll.size(), newInformationsNtdll.size());
	EXPECT_EQ(oldInformationsPerf.size(), newInformationsPerf.size());

	for (size_t i = 0; i < oldInformationsNtdll.size(); ++i)
	{
		cpuPercentNtdll = oldInformationsNtdll[i] / newInformationsNtdll[i];
		cpuPercentPerf = oldInformationsPerf[i] / newInformationsPerf[i];
		//systemCollector->SicCalculateCpuPerc(oldInformationsNtdll[i],
		//	newInformationsNtdll[i],
		//	cpuPercentNtdll);
		//systemCollector->SicCalculateCpuPerc(oldInformationsPerf[i],
		//	newInformationsPerf[i],
		//	cpuPercentPerf);
		EXPECT_LE(abs(cpuPercentNtdll.user - cpuPercentPerf.user) * 100, maxValue);
		EXPECT_LE(abs(cpuPercentNtdll.sys - cpuPercentPerf.sys) * 100, maxValue);
		EXPECT_LE(abs(cpuPercentNtdll.nice - cpuPercentPerf.nice) * 100, maxValue);
		EXPECT_LE(abs(cpuPercentNtdll.idle - cpuPercentPerf.idle) * 100, maxValue);
		EXPECT_LE(abs(cpuPercentNtdll.wait - cpuPercentPerf.wait) * 100, maxValue);
		EXPECT_LE(abs(cpuPercentNtdll.irq - cpuPercentPerf.irq) * 100, maxValue);
		EXPECT_LE(abs(cpuPercentNtdll.softIrq - cpuPercentPerf.softIrq) * 100, maxValue);
		EXPECT_LE(abs(cpuPercentNtdll.stolen - cpuPercentPerf.stolen) * 100, maxValue);
		EXPECT_LE(abs(cpuPercentNtdll.combined - cpuPercentPerf.combined) * 100, maxValue);
	}

	SicCpuInformation endCpuInformationNtdll = {};
	systemCollector->NtSysGetCpuInformation(endCpuInformationNtdll, errorMessage);
	SicCpuInformation endCpuInformationPerf = {};
	systemCollector->PerfLibGetCpuInformation(endCpuInformationPerf, errorMessage);
	//std::cout << "endCpuInformationNtdll = " << endCpuInformationNtdll.str(true);
	//std::cout << "endCpuInformationPerf = " << endCpuInformationPerf.str(true);

	std::cout << "cpuUseNtdll = " << UTF8ToGBK((endCpuInformationNtdll - oldCpuInformationNtdll).str(true)) << std::endl;
	std::cout << "cpuUsePerf = " << UTF8ToGBK((endCpuInformationPerf - oldCpuInformationPerf).str(true)) << std::endl;
}

TEST_F(SicWin32CollectorTest, SicGetCpuInformationTest)
{
	SicCpuInformation prevCpuInformation;
	systemCollector->SicGetCpuInformation(prevCpuInformation);
#ifdef WITH_SIGAR
	sigar_cpu_t prevSigarCpuInformation;
	sigar_cpu_get(mSigar, &prevSigarCpuInformation);
#endif
	Sleep(2000);

	SicCpuInformation curCpuInformation;
	systemCollector->SicGetCpuInformation(curCpuInformation);
#ifdef WITH_SIGAR
	sigar_cpu_t curSigarCpuInformation;
	sigar_cpu_get(mSigar, &curSigarCpuInformation);
	SicCpuPercent cpuPercent = prevCpuInformation / curCpuInformation;
	//systemCollector->SicCalculateCpuPerc(prevCpuInformation, curCpuInformation, cpuPercent);
	//SicCpuPercent sigarCpuPercent = prevCpuInformation / curCpuInformation;
	//systemCollector->SicCalculateCpuPerc(prevCpuInformation, curCpuInformation, cpuPercent);
	SicCpuPercent sigarCpuPercent = (SicCpuInformation &)prevSigarCpuInformation / (SicCpuInformation&)curSigarCpuInformation;

	const double expectMax = 0.5;
	EXPECT_LE(abs(cpuPercent.user - sigarCpuPercent.user) * 100, expectMax);
	EXPECT_LE(abs(cpuPercent.sys - sigarCpuPercent.sys) * 100, expectMax);
	EXPECT_LE(abs(cpuPercent.nice - sigarCpuPercent.nice) * 100, expectMax);
	EXPECT_LE(abs(cpuPercent.idle - sigarCpuPercent.idle) * 100, expectMax);
	EXPECT_LE(abs(cpuPercent.wait - sigarCpuPercent.wait) * 100, expectMax);
	EXPECT_LE(abs(cpuPercent.irq - sigarCpuPercent.irq) * 100, expectMax);
	EXPECT_LE(abs(cpuPercent.softIrq - sigarCpuPercent.softIrq) * 100, expectMax);
	EXPECT_LE(abs(cpuPercent.stolen - sigarCpuPercent.stolen) * 100, expectMax);
	EXPECT_LE(abs(cpuPercent.combined - sigarCpuPercent.combined) * 100, expectMax);
#endif
}

TEST_F(SicWin32CollectorTest, SicGetCpuListInformationTest)
{
	std::vector<SicCpuInformation> prevCpuList;
	systemCollector->SicGetCpuListInformation(prevCpuList);
#ifdef WITH_SIGAR
	sigar_cpu_list_t sigarPrevCpuList{};
	sigar_cpu_list_get(mSigar, &sigarPrevCpuList);

	std::cout << "Sleep(2000)" << std::endl;
	Sleep(2000);

	std::vector<SicCpuInformation> curCpuList;
	systemCollector->SicGetCpuListInformation(curCpuList);
	sigar_cpu_list_t sigarCurCpuList{};
	sigar_cpu_list_get(mSigar, &sigarCurCpuList);
	for (size_t i = 0; i < curCpuList.size(); ++i)
	{
		SicCpuPercent cpuPercent = prevCpuList[i] / curCpuList[i];
		SicCpuPercent sigarCpuPercent = (SicCpuInformation&)(sigarPrevCpuList.data[i]) / (SicCpuInformation &)(sigarCurCpuList.data[i]);
		//systemCollector->SicCalculateCpuPerc(prevCpuList[i], curCpuList[i], cpuPercent);
		//systemCollector->SicCalculateCpuPerc(*(SicCpuInformation*)&(sigarPrevCpuList.data[i]),
		//	*(SicCpuInformation*)&(sigarCurCpuList.data[i]),
		//	sigarCpuPercent);
		std::cout << "cpuPercent, " << cpuPercent.string() << std::endl;
		std::cout << "sigarCpuPercent, " << sigarCpuPercent.string() << std::endl;
		const double expectMax = 1.5;
		EXPECT_LE(abs(cpuPercent.user - sigarCpuPercent.user) * 100, expectMax);
		EXPECT_LE(abs(cpuPercent.sys - sigarCpuPercent.sys) * 100, expectMax);
		EXPECT_LE(abs(cpuPercent.nice - sigarCpuPercent.nice) * 100, expectMax);
		EXPECT_LE(abs(cpuPercent.wait - sigarCpuPercent.wait) * 100, expectMax);
		// sigar not collect irq in ntdll api
		//EXPECT_LE(abs(cpuPercent.idle - sigarCpuPercent.idle) * 100, expectMax);
		//EXPECT_LE(abs(cpuPercent.irq - sigarCpuPercent.irq) * 100, expectMax);
		EXPECT_LE(abs(cpuPercent.softIrq - sigarCpuPercent.softIrq) * 100, expectMax);
		EXPECT_LE(abs(cpuPercent.stolen - sigarCpuPercent.stolen) * 100, expectMax);
		EXPECT_LE(abs(cpuPercent.combined - sigarCpuPercent.combined) * 100, expectMax);
	}
	sigar_cpu_list_destroy(mSigar, &sigarPrevCpuList);
	sigar_cpu_list_destroy(mSigar, &sigarCurCpuList);
#endif
}

TEST_F(SicWin32CollectorTest, SystemWithEnv)
{
	const char *cmd = R"(%SystemRoot%\System32\tasklist)";
	std::cout << "command: " << cmd << std::endl;

	EXPECT_EQ(0, system(cmd));
}

TEST_F(SicWin32CollectorTest, SicGetMemoryInformationTest)
{
	SicMemoryInformation memoryInformation = {};
	std::string errorMessage;
	EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, systemCollector->SicGetMemoryInformation(memoryInformation));
#ifdef WITH_SIGAR
	sigar_mem_t sigarMem{};
	sigar_mem_get(mSigar, &sigarMem);
#endif

	std::string result;
	ExecCmd(R"(%SystemRoot%\System32\wbem\wmic memorychip get Capacity | %SystemRoot%\System32\findstr /v "Capacity")", &result);
	int64_t memCap = 0;
	while (result != "") {
		memCap += (int64_t)std::stoull(result);
		size_t pos = result.find("\n");
		if (pos != std::string::npos) {
			pos++; // include '\n'
		}
		result.erase(0, pos);
	}
	memCap /= 1024 * 1024;

	EXPECT_GT(memoryInformation.used / (1024 * 1024), 0);
	EXPECT_GT(memoryInformation.free / (1024 * 1024), 0);
	EXPECT_GT(memoryInformation.total / (1024 * 1024), 0);
	EXPECT_GT(memoryInformation.actualFree / (1024 * 1024), 0);
	EXPECT_GT(memoryInformation.actualUsed / (1024 * 1024), 0);
	auto memTotal = (int64_t)(memoryInformation.total / (1024 * 1024));
	std::cout << "result: " << memCap << ", total: " << memTotal << ", diff: " << (memCap - memTotal) << std::endl;
	EXPECT_LE(memCap - memTotal, 1024);

#ifdef WITH_SIGAR
	EXPECT_EQ(memoryInformation.ram, sigarMem.ram);
	EXPECT_LE(AbsMinus(memoryInformation.usedPercent, sigarMem.used_percent), 1);
	EXPECT_LE(AbsMinus(memoryInformation.freePercent, sigarMem.free_percent), 1);
	EXPECT_LE(AbsMinus(memoryInformation.used, sigarMem.used), 50 * 1024 * 1024);
	EXPECT_LE(AbsMinus(memoryInformation.free, sigarMem.free), 50 * 1024 * 1024);
	EXPECT_LE(AbsMinus(memoryInformation.total, sigarMem.total), 50 * 1024 * 1024);
	EXPECT_LE(AbsMinus(memoryInformation.actualFree, sigarMem.actual_free), 50 * 1024 * 1024);
	EXPECT_LE(AbsMinus(memoryInformation.actualUsed, sigarMem.actual_used), 50 * 1024 * 1024);
#endif
}

TEST_F(SicWin32CollectorTest, SicGetSwapInformationAPITest)
{
	SicSwapInformation swapInformation = {};
	std::string errorMessage;
	systemCollector->SicGetSwapInformation(swapInformation);
	EXPECT_GE(swapInformation.pageIn, 0);
	EXPECT_GE(swapInformation.pageOut, 0);
	EXPECT_EQ(swapInformation.total - swapInformation.used, swapInformation.free);

	RawData rawData{};
	systemCollector->PerfLibGetValue(rawData,
		PERF_MEM_COUNTER_KEY,
		PERF_COUNTER_MEM_COMMIT_LIMIT,
		"",
		errorMessage);
	EXPECT_LE(abs((long long)swapInformation.total - (long long)rawData.Data) / (1024 * 1024), 500);

	rawData = {};
	systemCollector->PerfLibGetValue(rawData,
		PERF_MEM_COUNTER_KEY,
		PERF_COUNTER_MEM_COMMITTED,
		"",
		errorMessage);
	EXPECT_LE(abs((long long)swapInformation.used - (long long)rawData.Data) / (1024 * 1024), 500);
}

TEST_F(SicWin32CollectorTest, SicGetSwapInformationTest)
{
	SicSwapInformation prevSwapInformation = {};
	systemCollector->SicGetSwapInformation(prevSwapInformation);
#ifdef WITH_SIGAR
	sigar_swap_t prevSigarSwap;
	sigar_swap_get(mSigar, &prevSigarSwap);
	EXPECT_LE(AbsMinus(prevSwapInformation.total, prevSigarSwap.total), 50 * 1024 * 1024);
	EXPECT_LE(AbsMinus(prevSwapInformation.free, prevSigarSwap.free), 50 * 1024 * 1024);
	EXPECT_LE(AbsMinus(prevSwapInformation.used, prevSigarSwap.used), 50 * 1024 * 1024);
#endif
	Sleep(8000);
	SicSwapInformation curSwapInformation = {};
	systemCollector->SicGetSwapInformation(curSwapInformation);
#ifdef WITH_SIGAR
	sigar_swap_t curSigarSwap;
	sigar_swap_get(mSigar, &curSigarSwap);
	EXPECT_LE(AbsMinus(curSwapInformation.total, curSigarSwap.total), 50 * 1024 * 1024);
	EXPECT_LE(AbsMinus(curSwapInformation.free, curSigarSwap.free), 50 * 1024 * 1024);
	EXPECT_LE(AbsMinus(curSwapInformation.used, curSigarSwap.used), 50 * 1024 * 1024);
#endif

	// sigar swap 采集有问题
	EXPECT_GE((curSwapInformation.pageIn - prevSwapInformation.pageIn) / 8.0, 0);
	EXPECT_GE((curSwapInformation.pageOut - prevSwapInformation.pageOut) / 8.0, 0);
}

TEST_F(SicWin32CollectorTest, SicGetInterfaceConfigListTest)
{
	int ret = -1;
	SicInterfaceConfigList interfaceConfigList;
	ret = systemCollector->SicGetInterfaceConfigList(interfaceConfigList);
	EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);
#ifdef WITH_SIGAR
	sigar_net_interface_list_t sigarNetInterfaceList;
	sigar_net_interface_list_get(mSigar, &sigarNetInterfaceList);
	EXPECT_EQ(sigarNetInterfaceList.number, interfaceConfigList.names.size());
#endif
	std::unordered_map<std::string, int> interfaceMap;
	for (auto const& name : interfaceConfigList.names)
	{
		interfaceMap[name] = 1;
	}
#ifdef WITH_SIGAR
	for (int i = sigarNetInterfaceList.number - 1; i >= 0; --i)
	{
		EXPECT_EQ(interfaceMap.find(GBKToUTF8(sigarNetInterfaceList.data[i])) != interfaceMap.end(), true);
	}
	EXPECT_EQ(2, 2);
	sigar_net_interface_list_destroy(mSigar, &sigarNetInterfaceList);
#endif
}

TEST_F(SicWin32CollectorTest, SicNetTest)
{
	int ret = -1;
	SicInterfaceConfigList interfaceConfigList;
	ret = systemCollector->SicGetInterfaceConfigList(interfaceConfigList);
	EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);
#ifdef WITH_SIGAR
	sigar_net_interface_list_t sigarNetInterfaceList;
	sigar_net_interface_list_get(mSigar, &sigarNetInterfaceList);
	EXPECT_EQ(interfaceConfigList.names.size(), (size_t)sigarNetInterfaceList.number);
#endif
	const size_t size = interfaceConfigList.names.size();
	int i = 0;
	for (const std::string &ifName: interfaceConfigList.names)
	{
		SicNetInterfaceInformation interfaceInformation{};
		systemCollector->SicGetInterfaceInformation(ifName, interfaceInformation);
#ifdef WITH_SIGAR
		sigar_net_interface_stat_t sigarNetInterfaceStat;
		sigar_net_interface_stat_get(mSigar, sigarNetInterfaceList.data[i], &sigarNetInterfaceStat);
		EXPECT_LE(AbsMinus(sigarNetInterfaceStat.rx_packets, interfaceInformation.rxPackets), 0.05 * sigarNetInterfaceStat.rx_packets);
		EXPECT_LE(AbsMinus(sigarNetInterfaceStat.rx_bytes, interfaceInformation.rxBytes), 0.05 * sigarNetInterfaceStat.rx_bytes);
		EXPECT_LE(AbsMinus(sigarNetInterfaceStat.rx_errors, interfaceInformation.rxErrors), 0.05 * sigarNetInterfaceStat.rx_errors);
		EXPECT_LE(AbsMinus(sigarNetInterfaceStat.rx_dropped, interfaceInformation.rxDropped), 0.05 * sigarNetInterfaceStat.rx_dropped);
		EXPECT_LE(AbsMinus(sigarNetInterfaceStat.tx_packets, interfaceInformation.txPackets), 0.05 * sigarNetInterfaceStat.tx_packets);
		EXPECT_LE(AbsMinus(sigarNetInterfaceStat.tx_bytes, interfaceInformation.txBytes), 0.05 * sigarNetInterfaceStat.tx_bytes);
		EXPECT_LE(AbsMinus(sigarNetInterfaceStat.tx_errors, interfaceInformation.txErrors), 0.05 * sigarNetInterfaceStat.tx_errors);
		EXPECT_LE(AbsMinus(sigarNetInterfaceStat.tx_dropped, interfaceInformation.txDropped), 0.05 * sigarNetInterfaceStat.tx_dropped);
		EXPECT_LE(AbsMinus(sigarNetInterfaceStat.speed, interfaceInformation.speed), 0);
#endif
		SicInterfaceConfig interfaceConfig;
		int res = systemCollector->SicGetInterfaceConfig(interfaceConfig, ifName);
		EXPECT_EQ(res, SIC_EXECUTABLE_SUCCESS);
#ifdef WITH_SIGAR
		sigar_net_interface_config_t sigarNetInterfaceConfig;
		sigar_net_interface_config_get(mSigar, sigarNetInterfaceList.data[i], &sigarNetInterfaceConfig);
		EXPECT_EQ(interfaceConfig.name, GBKToUTF8(sigarNetInterfaceConfig.name));
		EXPECT_EQ(interfaceConfig.address.addr.in, sigarNetInterfaceConfig.address.addr.in);
		EXPECT_EQ(interfaceConfig.address6.addr.in, sigarNetInterfaceConfig.address6.addr.in);
#endif
		std::string address4 = interfaceConfig.address.str();
		std::string address6 = interfaceConfig.address6.str();
		std::cout << "[" << (++i) << "/" << size << "] ifname : [" << ToGBK(interfaceConfig.name) << "], IPv4: \"" << address4 << "\", IPv6: \"" << address6 << "\"" << std::endl;
	}
#ifdef WITH_SIGAR
	sigar_net_interface_list_destroy(mSigar, &sigarNetInterfaceList);

	SicNetState netState;
	for (int i = 0; i < sizeof(netState.tcpStates) / sizeof(netState.tcpStates[0]); i++) {
		ASSERT_EQ(0, netState.tcpStates[i]);
	}
	systemCollector->SicGetNetState(netState, true, true, true, true, false);
	sigar_net_stat_t sigarNetStat;
	sigar_net_stat_get(mSigar,
		&sigarNetStat,
		SIGAR_NETCONN_CLIENT | SIGAR_NETCONN_SERVER | SIGAR_NETCONN_TCP | SIGAR_NETCONN_UDP);
	EXPECT_EQ(netState.tcpInboundTotal, sigarNetStat.tcp_inbound_total);
	EXPECT_EQ(netState.tcpOutboundTotal, sigarNetStat.tcp_outbound_total);
	EXPECT_EQ(netState.allInboundTotal, sigarNetStat.all_inbound_total);
	EXPECT_EQ(netState.allOutboundTotal, sigarNetStat.all_outbound_total);
	EXPECT_EQ(netState.udpSession, sigarNetStat.udp_session);
	ASSERT_EQ(0, netState.tcpStates[0]);
	for (int i = 1; i < SIC_TCP_UNKNOWN; ++i) {
		EXPECT_EQ(netState.tcpStates[i], sigarNetStat.tcp_states[i]);
	}
	for (int i = 1; i < SIC_TCP_STATE_END; i++) {
		std::cout << "[" << i << "] " << GetTcpStateName(EnumTcpState(i)) << " => " << netState.tcpStates[i] << std::endl;
	}
#endif
}

TEST_F(SicWin32CollectorTest, SicGetUptimeTest)
{
	int ret = -1;
	ret = systemCollector->SicInitialize();
	EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);

	double upTime;
	systemCollector->SicGetUpTime(upTime);

#ifdef WITH_SIGAR
	sigar_uptime_t sigarUptime{};
	sigar_uptime_get(mSigar, &sigarUptime);

	EXPECT_LE(std::abs(sigarUptime.uptime - upTime), 5);
#endif
}

TEST_F(SicWin32CollectorTest, SicNetAddressToString)
{
	int ret = -1;
	ret = systemCollector->SicInitialize();
	EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);

#ifdef WITH_SIGAR
	unsigned int ip = 0;
	int step = 2503;
	for (size_t count = 0; count < 4294967295 / step; ++count)
	{
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
		EXPECT_EQ(sicName, addr);

		ip += step;
	}

	ip = 0;
	step = 250003;
	for (size_t count = 0; count < 4294967295 / step; ++count)
	{
		ip = 4294962778;
		char addr[256];
		memset(addr, 0, sizeof(addr));
		sigar_net_address_t sigarNetAddress{};
		SicNetAddress sicNetAddress;
		sicNetAddress.family = SicNetAddress::SIC_AF_INET6;
		sicNetAddress.addr.in6[0] = ip;
		sicNetAddress.addr.in6[1] = ip;
		sicNetAddress.addr.in6[2] = ip;
		sicNetAddress.addr.in6[3] = ip;
		sigarNetAddress.family = sigar_net_address_t::SIGAR_AF_INET6;
		sigarNetAddress.addr.in6[0] = ip;
		sigarNetAddress.addr.in6[1] = ip;
		sigarNetAddress.addr.in6[2] = ip;
		sigarNetAddress.addr.in6[3] = ip;
		ret = sigar_net_address_to_string(mSigar, &sigarNetAddress, addr);
		EXPECT_EQ(ret, 0);

		std::string sicName = sicNetAddress.str();

		ip += step;
	}
#endif
}

TEST_F(SicWin32CollectorTest, IphlpapiGetAdaptersAddressesTest)
{
	int ret = -1;
	SicInterfaceConfigList interfaceConfigList;
	ret = systemCollector->SicGetInterfaceConfigList(interfaceConfigList);
	EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);
	for (const std::string &ifName: interfaceConfigList.names)
	{
		SicNetInterfaceInformation interfaceInformation{};
		SicInterfaceConfig interfaceConfig;

		systemCollector->SicGetInterfaceInformation(ifName, interfaceInformation);
		EXPECT_NE(interfaceInformation.rxPackets, MAXUINT64);
		EXPECT_NE(interfaceInformation.rxBytes, MAXUINT64);
		EXPECT_NE(interfaceInformation.rxErrors, MAXUINT64);
		EXPECT_NE(interfaceInformation.rxDropped, MAXUINT64);
		EXPECT_NE(interfaceInformation.txPackets, MAXUINT64);
		EXPECT_NE(interfaceInformation.txBytes, MAXUINT64);
		EXPECT_NE(interfaceInformation.txErrors, MAXUINT64);
		EXPECT_NE(interfaceInformation.txDropped, MAXUINT64);
		EXPECT_NE(interfaceInformation.speed, MAXUINT64);

		systemCollector->SicGetInterfaceConfig(interfaceConfig, ifName);
		EXPECT_EQ(2, 2);
		EXPECT_NE(interfaceConfig.name, "");
		EXPECT_NE(interfaceConfig.type, "");
		EXPECT_NE(interfaceConfig.description, "");
	}
	SicNetState netState;
	systemCollector->SicGetNetState(netState, true, true, true, true, false);
	EXPECT_GT(netState.tcpOutboundTotal, (unsigned int)(0));
	EXPECT_GT(netState.allOutboundTotal, (unsigned int)(0));
	EXPECT_GT(netState.udpSession, (unsigned int)(0));
}

TEST_F(SicWin32CollectorTest, SicGetFileSystemListInformationTest)
{
	std::vector<SicFileSystem> informations;
	systemCollector->SicGetFileSystemListInformation(informations);
#ifdef WITH_SIGAR
	sigar_file_system_list_t sigarFileSystemList;
	sigar_file_system_list_get(mSigar, &sigarFileSystemList);

	EXPECT_EQ(informations.size(), sigarFileSystemList.number);
	for (size_t i = 0; i < informations.size(); ++i)
	{
		EXPECT_EQ(informations[i].dirName, sigarFileSystemList.data[i].dir_name);
		EXPECT_EQ(informations[i].devName, sigarFileSystemList.data[i].dev_name);
		EXPECT_EQ(informations[i].options, sigarFileSystemList.data[i].options);
	}
	sigar_file_system_list_destroy(mSigar, &sigarFileSystemList);
#endif
}

TEST_F(SicWin32CollectorTest, SicGetFileSystemUsageTest)
{
	std::vector<SicFileSystem> informations;
	systemCollector->SicGetFileSystemListInformation(informations);

	for (size_t i = 0; i < informations.size(); ++i)
	{
		std::string dirName = informations[i].dirName;

		SicFileSystemUsage sicPrevFileSystemUsage;
		systemCollector->SicGetFileSystemUsage(sicPrevFileSystemUsage, dirName);
#ifdef WITH_SIGAR
		sigar_file_system_usage_t sigarPrevFileSystemUsage;
		sigar_file_system_usage_get(mSigar, dirName.c_str(), &sigarPrevFileSystemUsage);
#endif
		int interval = 1000; //1000ms
		Sleep(interval);

		SicFileSystemUsage sicCurrFileSystemUsage;
		systemCollector->SicGetFileSystemUsage(sicCurrFileSystemUsage, dirName);
#ifdef WITH_SIGAR
		sigar_file_system_usage_t sigarCurrFileSystemUsage;
		sigar_file_system_usage_get(mSigar, dirName.c_str(), &sigarCurrFileSystemUsage);

		EXPECT_LE(AbsMinus(sicCurrFileSystemUsage.total, sigarCurrFileSystemUsage.total), 0.05 * sigarCurrFileSystemUsage.total);
		EXPECT_LE(AbsMinus(sicCurrFileSystemUsage.free, sigarCurrFileSystemUsage.free), 0.05 * sigarCurrFileSystemUsage.free);
		EXPECT_LE(AbsMinus(sicCurrFileSystemUsage.used, sigarCurrFileSystemUsage.used), 0.05 * sigarCurrFileSystemUsage.used);
		EXPECT_LE(AbsMinus(sicCurrFileSystemUsage.avail, sigarCurrFileSystemUsage.avail), 0.05 * sigarCurrFileSystemUsage.avail);
		EXPECT_LE(AbsMinus(sicCurrFileSystemUsage.files, sigarCurrFileSystemUsage.files), 0.05 * sigarCurrFileSystemUsage.files);
		EXPECT_LE(AbsMinus(sicCurrFileSystemUsage.freeFiles, sigarCurrFileSystemUsage.free_files), 0.05 * sigarCurrFileSystemUsage.free_files);
		EXPECT_LE(abs(sicCurrFileSystemUsage.use_percent - sigarCurrFileSystemUsage.use_percent), 2);

		EXPECT_LE(AbsMinus(sicPrevFileSystemUsage.total, sigarPrevFileSystemUsage.total), 0.05 * sigarPrevFileSystemUsage.total);
		EXPECT_LE(AbsMinus(sicPrevFileSystemUsage.free, sigarPrevFileSystemUsage.free), 0.05 * sigarPrevFileSystemUsage.free);
		EXPECT_LE(AbsMinus(sicPrevFileSystemUsage.used, sigarPrevFileSystemUsage.used), 0.05 * sigarPrevFileSystemUsage.used);
		EXPECT_LE(AbsMinus(sicPrevFileSystemUsage.avail, sigarPrevFileSystemUsage.avail), 0.05 * sigarPrevFileSystemUsage.avail);
		EXPECT_LE(AbsMinus(sicPrevFileSystemUsage.files, sigarPrevFileSystemUsage.files), 0.05 * sigarCurrFileSystemUsage.files);
		EXPECT_LE(AbsMinus(sicPrevFileSystemUsage.freeFiles, sigarPrevFileSystemUsage.free_files), 0.05 * sigarCurrFileSystemUsage.free_files);
		EXPECT_LE(abs(sicPrevFileSystemUsage.use_percent - sigarPrevFileSystemUsage.use_percent), 2);
#endif
	}
}

TEST_F(SicWin32CollectorTest, SicGetProcessListTest)
{
	SicProcessList sicProcList;
	bool overflow = false;
	systemCollector->SicGetProcessList(sicProcList, 2, overflow);
	EXPECT_TRUE(overflow);

#ifdef WITH_SIGAR
	sigar_proc_list_t sigarProcList;
	sigar_proc_list_get(mSigar, &sigarProcList);

	EXPECT_EQ(sicProcList.pids.size(), sigarProcList.number);
	for (size_t i = 0; i < sicProcList.pids.size(); ++i)
	{
		unsigned long long sigarPid = sigarProcList.data[i];
		unsigned long long sicPid = sicProcList.pids[i];
		EXPECT_EQ(sigarPid, sicPid);
	}
#endif
}

TEST_F(SicWin32CollectorTest, SicGetProcessStateTest)
{
	SicProcessList sicProcList;
	bool overflow = false;
	systemCollector->SicGetProcessList(sicProcList, 0, overflow);
	EXPECT_FALSE(overflow);
	int64_t millis = NowMillis();
	int count = 0;
	for (size_t i = 0; count < 5 && i < sicProcList.pids.size(); i++, count++)
	{
		pid_t pid = sicProcList.pids[i];
		std::cout << pid << std::endl;
#ifdef WITH_SIGAR
		sigar_proc_state_t sigarProcState;
		int sigarStatus = sigar_proc_state_get(mSigar, pid, &sigarProcState);
#endif
		// 这个比较耗时，大约在250毫秒左右
		SicProcessState processState;
		int sicStatus = systemCollector->SicGetProcessState(pid, processState);

#ifdef WITH_SIGAR
		if (sigarStatus == SIGAR_OK && sicStatus == SIC_EXECUTABLE_SUCCESS)
		{
			EXPECT_EQ(GBKToUTF8(sigarProcState.name), processState.name);
			EXPECT_EQ(sigarProcState.state, processState.state);
			EXPECT_EQ(sigarProcState.ppid, processState.parentPid);
			EXPECT_EQ(sigarProcState.priority, processState.priority);
			EXPECT_EQ(sigarProcState.nice, processState.nice);
			EXPECT_EQ(sigarProcState.tty, processState.tty);
			EXPECT_LE(AbsMinus(sigarProcState.threads, processState.threads), 5);
			EXPECT_EQ(sigarProcState.processor, processState.processor);
		}
#endif
	}
	int64_t endMillis = NowMillis();
	std::cout << "total " << count << " processes, use: " << (endMillis - millis) << " ms, avg: "
		<< ((endMillis - millis) / count) << " ms/count" << std::endl;

}

TEST_F(SicWin32CollectorTest, SicGetProcessTimeTest)
{
	int ret = -1;
	SicProcessList sicProcList;
	bool overflow = true;
	systemCollector->SicGetProcessList(sicProcList, 100*10000, overflow);
	EXPECT_FALSE(overflow);
	for (size_t i = 0; i < sicProcList.pids.size(); i += 5)
	{
#ifdef WITH_SIGAR
		sigar_proc_time_t sigarProcTime;
		ret = sigar_proc_time_get(mSigar, sicProcList.pids[i], &sigarProcTime);
#endif
		SicProcessTime processTime{};
		ret = systemCollector->SicGetProcessTime(sicProcList.pids[i], processTime);
		if (ret == 0)
		{
#ifdef WITH_SIGAR
			EXPECT_LE(AbsMinus(processTime.startMillis(), static_cast<int64_t>(sigarProcTime.start_time)), 100);
			EXPECT_LE(AbsMinus(processTime.sys.count(), static_cast<int64_t>(sigarProcTime.sys)), 100);
			EXPECT_LE(AbsMinus(processTime.user.count(), static_cast<int64_t>(sigarProcTime.user)), 100);
			EXPECT_LE(AbsMinus(processTime.total.count(), static_cast<int64_t>(sigarProcTime.total)), 100);
#endif
		}
	}
}

TEST_F(SicWin32CollectorTest, SicGetProcessCpuInformationTest)
{
	int ret = -1;
	struct TestCase {
		pid_t pid;
		SicProcessCpuInformation processCpuInformation;
#ifdef WITH_SIGAR
		sigar_proc_cpu_t sigarProcCpu;
#endif
	};
	std::vector<TestCase> procs;
	procs.reserve(5);
	SicProcessList sicProcList;
	bool overflow;
	systemCollector->SicGetProcessList(sicProcList, 0, overflow);
	EXPECT_FALSE(overflow);

	for (size_t i = 0; i < sicProcList.pids.size(); i += 5)
	{
		TestCase testCase;
		testCase.pid = sicProcList.pids[i];
#ifdef WITH_SIGAR
		ret = sigar_proc_cpu_get(mSigar, testCase.pid, &testCase.sigarProcCpu);
#endif
		// 耗時很短，通常不足1毫秒
		ret = systemCollector->SicGetProcessCpuInformation(testCase.pid, testCase.processCpuInformation);
		if (ret == 0) {
			procs.push_back(testCase);
		}
	}
	Sleep(2000);
	for (TestCase& testCase : procs) {
#ifdef WITH_SIGAR
		ret = sigar_proc_cpu_get(mSigar, testCase.pid, &testCase.sigarProcCpu);
#endif
		ret = systemCollector->SicGetProcessCpuInformation(testCase.pid, testCase.processCpuInformation);
#ifdef WITH_SIGAR
		EXPECT_LE(AbsMinus(testCase.processCpuInformation.total, testCase.sigarProcCpu.total), 100);
		EXPECT_LE(AbsMinus(testCase.processCpuInformation.startTime, static_cast<int64_t>(testCase.sigarProcCpu.start_time)), 100);
		EXPECT_LE(AbsMinus(testCase.processCpuInformation.lastTime, static_cast<int64_t>(testCase.sigarProcCpu.last_time)), 100);
		EXPECT_LE(AbsMinus(testCase.processCpuInformation.user, testCase.sigarProcCpu.user), 100);
		EXPECT_LE(AbsMinus(testCase.processCpuInformation.sys, testCase.sigarProcCpu.sys), 100);
		EXPECT_LE(AbsMinus(testCase.processCpuInformation.percent, testCase.sigarProcCpu.percent), 3);
#endif
		std::cout << "pid : " << testCase.pid << " percent : " << std::fixed << std::setprecision(3)
			<< testCase.processCpuInformation.percent << std::endl;
	}
}

TEST_F(SicWin32CollectorTest, SicGetProcessMemoryInformationTest)
{
	int64_t millis = NowMillis();
	SicProcessList sicProcList;
	bool overflow;
	systemCollector->SicGetProcessList(sicProcList, 0, overflow);
	EXPECT_FALSE(overflow);

	struct TestCase {
		pid_t pid;
		SicProcessMemoryInformation memory;
	};
	std::list<TestCase> testCaseList;

	std::cout << "SicGetProcessList => " << (NowMillis() - millis) << " ms" << std::endl;  // 低于100毫秒
	for (size_t i = 0; i < sicProcList.pids.size(); i += (sicProcList.pids.size() / 4))
	{
		millis = NowMillis();
		pid_t pid = sicProcList.pids[i];
		SicProcessMemoryInformation sicPrevProcessMemoryInformation;
		systemCollector->SicGetProcessMemoryInformation(pid, sicPrevProcessMemoryInformation);
		// 平均100毫秒左右
		std::cout << "SicGetProcessMemoryInformation(" << pid << ") => " << (NowMillis() - millis) << " ms" << std::endl;

#ifdef WITH_SIGAR
		sigar_proc_mem_t sigarProcMem;
		sigar_proc_mem_get(mSigar, pid, &sigarProcMem);
		std::cout << "ProcessMemoryInformation(" << pid << ") => " << std::endl
				<< "size: sigar=" << sigarProcMem.size << ", agent=" << sicPrevProcessMemoryInformation.size << std::endl
				<< "resident: sigar=" << sigarProcMem.resident << ", agent=" << sicPrevProcessMemoryInformation.resident
				<< std::endl;
		EXPECT_LE(AbsMinus(sicPrevProcessMemoryInformation.size, sigarProcMem.size), 0.05 * sigarProcMem.size);
		// sigar 的 share 计算有问题，不将其结果作为对比。
		// EXPECT_LE(AbsMinus(sicPrevProcessMemoryInformation.share, sigarProcMem.share), 100 * 1024);
		EXPECT_LE(AbsMinus(sicPrevProcessMemoryInformation.resident, sigarProcMem.resident), 0.05 * sigarProcMem.resident);
#endif
		testCaseList.push_back(TestCase{ pid, sicPrevProcessMemoryInformation });
	}

	Sleep(1000);

	for (TestCase& testCase : testCaseList) {
		SicProcessMemoryInformation sicCurrProcessMemoryInformation;
		systemCollector->SicGetProcessMemoryInformation(testCase.pid, sicCurrProcessMemoryInformation);
		auto pageFaults =
			sicCurrProcessMemoryInformation.pageFaults == testCase.memory.pageFaults ? 0.0 :
			(double)(sicCurrProcessMemoryInformation.pageFaults - testCase.memory.pageFaults)
			/ ((double)(sicCurrProcessMemoryInformation.tick - testCase.memory.tick)
				/ testCase.memory.frequency);

		std::cout << "current pid : " << testCase.pid << std::endl;
		std::cout << "page faults : " << pageFaults << " /sec" << std::endl;
		std::cout << "size : " << testCase.memory.size << std::endl;
		std::cout << "resident : " << testCase.memory.resident << std::endl;
		std::cout << "share : " << testCase.memory.share << std::endl;
	}
}

TEST_F(SicWin32CollectorTest, SicGetProcessArgsTest)
{
	SicProcessList sicProcList;
	bool overflow;
	systemCollector->SicGetProcessList(sicProcList, 0, overflow);
	EXPECT_FALSE(overflow);
	int count = 0;
	int64_t totalMillis = 0;
	for (size_t i = 0; count < 10 && i < sicProcList.pids.size(); i += 5)
	{
		pid_t pid = sicProcList.pids[i];
#ifdef WITH_SIGAR
		sigar_proc_args_t sigarProcArgs;
		int sigarStatus = sigar_proc_args_get(mSigar, pid, &sigarProcArgs);
#endif
		int64_t millis = NowMillis();
		SicProcessArgs processArgs;
		int sicStatus = systemCollector->SicGetProcessArgs(pid, processArgs);
		if (sicStatus == SIC_EXECUTABLE_SUCCESS) {
			std::cout << (__FILE__ + SRC_ROOT_OFFSET) << "[" << __LINE__ << "], Pid: " << pid << ": ";
			for (auto arg : processArgs.args) {
				std::cout << "[" << UTF8ToGBK(arg) << "]";
			}
			std::cout << std::endl;
		}
#ifdef WITH_SIGAR
		if (pid != GetPid()) {
			EXPECT_EQ(sicStatus, sigarStatus);
		}
		if (sigarStatus == SIGAR_OK && sicStatus == SIC_EXECUTABLE_SUCCESS)
		{
			totalMillis += (NowMillis() - millis);
			count++;
			EXPECT_EQ(processArgs.args.size(), sigarProcArgs.number);
			for (size_t j = 0; j < processArgs.args.size(); ++j)
			{
				EXPECT_EQ(UTF8ToGBK(processArgs.args[j]), sigarProcArgs.data[j]);
			}
		}
#endif
	}

	std::cout << "total " << count << " process, use: " << totalMillis << " ms, avg: "
		<< (totalMillis / (count > 0 ? count : 1)) << " ms/count" << std::endl;
}

TEST_F(SicWin32CollectorTest, PebGetProcessExeTest)
{
	int ret = -1;
	SicProcessExe processExe;
	SicProcessList sicProcList;
	bool overflow;
	systemCollector->SicGetProcessList(sicProcList, 0, overflow);
	EXPECT_FALSE(overflow);
	int count = 0;
	int64_t totalMillis = 0;
	for (size_t i = 0; count < 10 && i < sicProcList.pids.size(); i += 5)
	{
		pid_t pid = sicProcList.pids[i];
		std::cout << pid << std::endl;
		processExe = {};
#ifdef WITH_SIGAR
		sigar_proc_exe_t sigarProcExe;
		ret = sigar_proc_exe_get(mSigar, pid, &sigarProcExe);
#endif
		int64_t millis = NowMillis();
		ret = systemCollector->SicGetProcessExe(pid, processExe);
		if (ret == SIC_EXECUTABLE_SUCCESS)
		{
			count++;
			totalMillis += (NowMillis() - millis);
#ifdef WITH_SIGAR
			EXPECT_EQ(sigarProcExe.name, processExe.name);
			EXPECT_EQ(sigarProcExe.cwd, processExe.cwd);
			EXPECT_EQ(sigarProcExe.root, processExe.root);
#endif
		}
	}

	std::cout << "total " << count << " process, use: " << totalMillis << " ms, avg: " << (totalMillis / count) << " ms/count" << std::endl;
}

TEST_F(SicWin32CollectorTest, SicGetProcessCredNameTest)
{
	int ret = -1;
	SicProcessCredName processCredName;
	SicProcessList sicProcList;
	bool overflow;
	systemCollector->SicGetProcessList(sicProcList, 0, overflow);
	EXPECT_FALSE(overflow);
	for (size_t i = 0; i < sicProcList.pids.size(); i += 5)
	{
		pid_t pid = sicProcList.pids[i];
#ifdef WITH_SIGAR
		sigar_proc_cred_name_t sigarProcCredName;
		ret = sigar_proc_cred_name_get(mSigar, pid, &sigarProcCredName);
#endif
		ret = systemCollector->SicGetProcessCredName(pid, processCredName);
		if (ret == 0)
		{
#ifdef WITH_SIGAR
			EXPECT_EQ(sigarProcCredName.user, processCredName.user);
			EXPECT_EQ(sigarProcCredName.group, processCredName.group);
#endif
			std::cout << "Pid : " << pid << " User : " << processCredName.user << " Group : " << processCredName.group
				<< std::endl;
		}
	}
}

TEST_F(SicWin32CollectorTest, SicGetProcessFdTest)
{
	int ret = -1;
	SicProcessFd processFd;
	SicProcessList sicProcList;
	bool overflow;
	systemCollector->SicGetProcessList(sicProcList, 0, overflow);
	EXPECT_FALSE(overflow);
	int count = 0;
	for (size_t i = 0; count < 10 && i < sicProcList.pids.size(); i += 5)
	{
		pid_t pid = sicProcList.pids[i];
#ifdef WITH_SIGAR
		sigar_proc_fd_t sigarProcFd;
		ret = sigar_proc_fd_get(mSigar, pid, &sigarProcFd);
#endif
		ret = systemCollector->SicGetProcessFd(pid, processFd);
		if (ret == 0)
		{
			count++;
#ifdef WITH_SIGAR
			EXPECT_EQ(sigarProcFd.total, processFd.total);
			std::cout << "[" << count << "] Pid : " << pid << " Fd : " << processFd.total << ", sigarFd: " << sigarProcFd.total << std::endl;
#endif
		}
	}
}

TEST_F(SicWin32CollectorTest, SicGetSystemInfoTest)
{
	int ret = -1;
	ret = systemCollector->SicInitialize();
	EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);

	SicSystemInfo systemInfo;
	systemCollector->SicGetSystemInfo(systemInfo);

#ifdef WITH_SIGAR
	sigar_sys_info_t sigarSysInfo;
	sigar_sys_info_get(mSigar, &sigarSysInfo);

	EXPECT_EQ(systemInfo.name, sigarSysInfo.name);
	EXPECT_EQ(systemInfo.version, sigarSysInfo.version);
	EXPECT_EQ(systemInfo.arch, sigarSysInfo.arch);
	EXPECT_EQ(systemInfo.machine, sigarSysInfo.machine);
	EXPECT_EQ(systemInfo.description, sigarSysInfo.description);
	EXPECT_EQ(systemInfo.vendor, sigarSysInfo.vendor);
	EXPECT_EQ(systemInfo.vendorVersion, sigarSysInfo.vendor_version);
	EXPECT_EQ(systemInfo.vendorName, sigarSysInfo.vendor_name);
#endif
}


TEST_F(SicWin32CollectorTest, ErrNo) {
	auto path = fs::path("/user") / "hello.a.not-exist~~~~~~";
	File fp{ path, File::ModeReadBin };
	EXPECT_FALSE((bool)fp);

	auto collector = SystemInformationCollector::New();
	EXPECT_NE(0, collector->ErrNo(path.string()));
}

TEST_F(SicWin32CollectorTest, errorMessage) {
	auto collector = SystemInformationCollector::New();
	EXPECT_TRUE(collector->errorMessage().empty());
}

TEST_F(SicWin32CollectorTest, EnumCpuByNtQuerySystemInformation) {
	Win32SystemInformationCollector collector;
	int ret = -1;
	ret = collector.SicInitialize();
	EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);
	EXPECT_EQ(collector.SicPtr()->errorMessage, "");

	int coreCount = 0;
	EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector.EnumCpuByNtQuerySystemInformation(collector.SicPtr()->errorMessage, [&](unsigned int, unsigned int, const SicCpuInformation&) {
		coreCount++;
		}));
	EXPECT_GT(coreCount, 1);
}

TEST_F(SicWin32CollectorTest, makeErrorCounterNotFound) {
	extern std::string makeErrorCounterNotFound(const std::map<DWORD, PERF_COUNTER_DEFINITION*>&);

	std::map<DWORD, PERF_COUNTER_DEFINITION*> mapData = {
		{1, nullptr},
	};
	std::string errMsg = makeErrorCounterNotFound(mapData);
	EXPECT_EQ(errMsg, "Counter 1 not found.");

	mapData[2] = nullptr;
	errMsg = makeErrorCounterNotFound(mapData);
	EXPECT_EQ(errMsg, "Counter 1, 2 not found.");
}

TEST_F(SicWin32CollectorTest, ErrorStr) {
	extern DWORD LastError(const std::string&, std::string*);

	GetProcessId(nullptr);
	std::string errMsg;
	DWORD errCode = LastError("GetProcessId", &errMsg);
	std::cout << errMsg << std::endl;
	EXPECT_NE(errMsg, "");
}

TEST_F(SicWin32CollectorTest, PsapiEnumProcesses) {
	Win32SystemInformationCollector collector;
	{
		int ret = collector.SicInitialize();
		EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);
		EXPECT_EQ(collector.SicPtr()->errorMessage, "");
	}
	SicProcessList processList;
	std::string errorMessage;
	// 触发do{}while()循环
	EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector.PsapiEnumProcesses(processList, errorMessage, 10));
	EXPECT_FALSE(processList.pids.empty());
}

void doTest(const std::function<void(Win32SystemInformationCollector&)>& callback) {
	Win32SystemInformationCollector collector;
	{
		int ret = collector.SicInitialize();
		EXPECT_EQ(ret, SIC_EXECUTABLE_SUCCESS);
		EXPECT_EQ(collector.SicPtr()->errorMessage, "");
	}

	callback(collector);
}
TEST_F(SicWin32CollectorTest, IphlpapiGetInterfaceName02) {
	extern int IphlpapiGetInterfaceName(MIB_IFROW & ifRow, PIP_ADAPTER_ADDRESSES addresses, std::string & name, std::string & errorMessage);

	MIB_IFROW ifRow;
	std::string name, errMsg;
	EXPECT_EQ(SIC_EXECUTABLE_FAILED, IphlpapiGetInterfaceName(ifRow, nullptr, name, errMsg));
	EXPECT_NE(errMsg.find("addresses"), std::string::npos);
}

TEST_F(SicWin32CollectorTest, GetSerialNo) {
	EXPECT_EQ(3, Win32SystemInformationCollector::GetSerialNo("svchost#3"));
	EXPECT_EQ(0, Win32SystemInformationCollector::GetSerialNo("svchost#"));
	EXPECT_EQ(0, Win32SystemInformationCollector::GetSerialNo("svchost"));
}

TEST_F(SicWin32CollectorTest, GetPerformanceData) {
	doTest([](Win32SystemInformationCollector& collector) {
		{
			std::string errorMessage;
			std::shared_ptr<BYTE> buffer = collector.GetPerformanceData("~~~ERROR_FILE_NOT_FOUND---&&&", errorMessage, std::numeric_limits<size_t>::max());
#ifdef _WIN64
			EXPECT_TRUE(buffer);
#else
			EXPECT_FALSE(buffer);
			EXPECT_NE(errorMessage.find("malloc"), std::string::npos);
#endif
		}
		{
			std::string errorMessage;
			std::shared_ptr<BYTE> buffer = collector.GetPerformanceData("~~~ERROR_FILE_NOT_FOUND---&&&", errorMessage, 1, std::numeric_limits<size_t>::max());
#ifdef _WIN64
			EXPECT_TRUE(buffer);
#else
			EXPECT_FALSE(buffer);
			std::cout << "errorMessage: " << errorMessage << std::endl;
			EXPECT_NE(errorMessage.find("realloc"), std::string::npos);
#endif
		}
	});
}


TEST_F(SicWin32CollectorTest, SicGetThreadCpuInformation) {
	doTest([](Win32SystemInformationCollector& collector) {
		SicThreadCpu cpu;
		EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector.SicGetThreadCpuInformation(GetPid(), GetThisThreadId(), cpu));
		EXPECT_EQ(cpu.total, cpu.user + cpu.sys);
		EXPECT_GT(cpu.total.count(), (int64_t)0);
		std::stringstream ss;
		ss << "cpu.sys: " << cpu.sys.count() << ", cpu.user: " << cpu.user << ", cpu.total: " << cpu.total;
		std::cout << ToGBK(ss.str()) << std::endl;
		});
}

TEST_F(SicWin32CollectorTest, GetSpecialDirectory) {
	auto system32Path = GetSpecialDirectory(EnumDirectoryType::System32);
	EXPECT_FALSE(system32Path.empty());
	auto windowsPath = GetSpecialDirectory(EnumDirectoryType::Windows);
	EXPECT_FALSE(windowsPath.empty());
	auto programFilesPath = GetSpecialDirectory(EnumDirectoryType::ProgramFiles);
	EXPECT_FALSE(programFilesPath.empty());
	std::cout << "GetWindowsDirectory() => " << windowsPath << std::endl;
	std::cout << "GetSystemDirectory() => " << system32Path << std::endl;
	std::cout << "GetProgramFilesDirectory() => " << programFilesPath << std::endl;
}

#ifdef _WIN64
#include "common/TimeFormat.h"
TEST_F(SicWin32CollectorTest, SicProcessMemory) {
	std::cout << date::format<3>(std::chrono::system_clock::now()) << ": Eat 5G memory!\n";
	std::vector<char*> buffers;
	const int64_t bufSize = 128 * 1024 * 1024;
	for (int64_t i = 0; i < int64_t(5 * 1024) * 1024 * 1024; i += bufSize) {
		char* buf = new char[bufSize];
		char* bufEnd = buf + bufSize;
		for (char* it = buf; it < bufEnd; it += 4096) {
			*it = 0x5e;
		}
		buffers.push_back(buf);
	}
	std::cout << date::format<3>(std::chrono::system_clock::now()) << ": Eat 5G memory OK!\n";

	SicProcessMemoryInformation procMem;
	const pid_t pid = GetPid();
	EXPECT_EQ(0, systemCollector->SicGetProcessMemoryInformation(pid, procMem));
	std::cout << date::format<3>(std::chrono::system_clock::now())
		<< ": process(" << pid << ") resident memory: " << convert(procMem.resident / 1024, true) << " KB" << std::endl;
	// agent为x86时采集的进程内存>4G时，resident不超过4G
	EXPECT_GT(procMem.resident, uint64_t(0x0ffffffff)); // 判断超4G

	std::cout << date::format<3>(std::chrono::system_clock::now()) << ": Release 5G memory!\n";
	for (auto& it : buffers) {
		delete[] it;
	}
	buffers.clear();
	std::cout << date::format<3>(std::chrono::system_clock::now()) << ": Released 5G memory!\n";
}
#endif
