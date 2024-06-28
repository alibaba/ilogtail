//
// Created by liuze.xlz on 2020/11/23.
//

#ifndef SIC_INCLUDE_WIN32_SYSTEM_INFORMATION_COLLECTOR_H_
#define SIC_INCLUDE_WIN32_SYSTEM_INFORMATION_COLLECTOR_H_

#include <windows.h>
#include <winreg.h>
#include <winperf.h>
#include <tlhelp32.h>
#include <Pdh.h>

#include <cstddef>
#include <sys/types.h>
#include <malloc.h>
#include <cstdio>
#include <cerrno>
#include <IPTypes.h>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <strsafe.h>
#include <iostream>
#include <sstream>
#include <string>
#include <functional>

#include "win32_sic_utils.h"
#include "system_information_collector.h"
#include "system_information_collector_os.h"
#include "wmi.h"

#ifdef ENABLE_TEST
#include "gtest/gtest_prod.h"
#endif

class Win32API;

namespace WinConst {
    static const int MAX_CPU_CORES = 128;
    static const int INIT_OBJECT_BUFFER_SIZE = 512 * 1024; // 40960;  // Initial buffer size to use when querying specific objects.
    static const int BUFFER_INCREMENT = 128 * 1024;         // Increment buffer size in 16K chunks.
    static const int MAX_INSTANCE_NAME_LEN = 255;      // Max length of an instance name.
    static const int MAX_FULL_INSTANCE_NAME_LEN = 511; // Form is parentinstancename/instancename#nnn.

    static const char* MS_LOOPBACK_ADAPTER = "Microsoft Loopback Adapter";
    static const char* LOOPBACK_ADAPTER = "LoopBackAdapter";
    static const char* LOOPBACK = "LoopBack";

    static const char* PERF_SYS_COUNTER_KEY = "2";
    static const char* PERF_MEM_COUNTER_KEY = "4";
    static const char* PERF_PROC_COUNTER_KEY = "230";
    static const char* PERF_CPU_COUNTER_KEY = "238";
    static const char* PERF_DISK_COUNTER_KEY = "236";

    static const unsigned long PERF_COUNTER_SYS_UP_TIME = 674;

    // CpuCounter可以使用如下命令进行查看:
    // 该命令的文档: https://learn.microsoft.com/en-us/windows-server/administration/windows-commands/typeperf
    // typeperf "\Processor(_Total)\% User Time" "\Processor(_Total)\% Idle Time" "\Processor(_Total)\% Privileged Time" "\Processor(_Total)\% Interrupt Time"
    static const unsigned long PERF_COUNTER_CPU_USER = 142;         //User Time
    static const unsigned long PERF_COUNTER_CPU_IDLE = 1746;        //Idle Time
    static const unsigned long PERF_COUNTER_CPU_SYS = 144;          //Privileged Time
    static const unsigned long PERF_COUNTER_CPU_IRQ = 698;          //Interrupt Time

    static const unsigned long PERF_COUNTER_MEM_SYS_CACHE = 76;     //System Cache Resident Bytes(Cache Bytes)
    static const unsigned long PERF_COUNTER_MEM_PAGES_OUT = 48;     //Pages Output/sec
    static const unsigned long PERF_COUNTER_MEM_PAGES_IN = 822;     //Pages In/sec
    static const unsigned long PERF_COUNTER_MEM_COMMIT_LIMIT = 30;  //Commit Limit(Page file) Bytes
    static const unsigned long PERF_COUNTER_MEM_COMMITTED = 26;     //Committed (Page file in use) Bytes

    static const unsigned long PERF_COUNTER_DISK_TIME = 200;        // % Disk Time
    static const unsigned long PERF_COUNTER_DISK_READ_TIME = 202;   // % Disk Read Time
    static const unsigned long PERF_COUNTER_DISK_WRITE_TIME = 204;  // % Disk Write Time
    static const unsigned long PERF_COUNTER_DISK_IDLE_TIME = 1482;  // % Disk Idle Time
    static const unsigned long PERF_COUNTER_DISK_READ = 214;        // Disk Reads/sec
    static const unsigned long PERF_COUNTER_DISK_WRITE = 216;       // Disk Writes/sec
    static const unsigned long PERF_COUNTER_DISK_READ_BYTES = 220;  // Disk Read Bytes/sec
    static const unsigned long PERF_COUNTER_DISK_WRITE_BYTES = 222; // Disk Write Bytes/sec
    static const unsigned long PERF_COUNTER_DISK_QUEUE = 198;       // Current Disk Queue Length

    static const unsigned long PERF_COUNTER_PROC_CPU_TIME = 6;              // % Processor Time
    static const unsigned long PERF_COUNTER_PROC_PAGE_FAULTS = 28;          // Page Faults/sec
    static const unsigned long PERF_COUNTER_PROC_MEM_VIRTUAL_SIZE = 174;    // Virtual Bytes
    static const unsigned long PERF_COUNTER_PROC_PRIVATE_SIZE = 1478;       // Working Set - Private Bytes
    static const unsigned long PERF_COUNTER_PROC_MEM_SIZE = 180;            // Working Set Bytes
    static const unsigned long PERF_COUNTER_PROC_THREAD_COUNT = 680;        // Thread count
    static const unsigned long PERF_COUNTER_PROC_HANDLE_COUNT = 952;        // Handle count
    static const unsigned long PERF_COUNTER_PROC_PID = 784;                 // PID
    static const unsigned long PERF_COUNTER_PROC_PARENT_PID = 1410;         // Parent Pid
    static const unsigned long PERF_COUNTER_PROC_PRIORITY = 682;            // process priority
    static const unsigned long PERF_COUNTER_PROC_START_TIME = 684;          // process start time
    static const unsigned long PERF_COUNTER_PROC_IO_READ_BYTES_SEC = 1420;  // IO Read Bytes/sec
    static const unsigned long PERF_COUNTER_PROC_IO_WRITE_BYTES_SEC = 1422; // IO Write Bytes/sec

    static const int SIC_MAX_PATH_LENGTH = 4096;
    static const int BUFFER_SIZE = 8192;
    static const int SIC_CRED_NAME_MAX = 512;
    static const int SIC_CMDLINE_MAX = 16384;
    static const uint64_t Milli_SEC_TO_UNIX_EPOCH = 11644473600000LL;
    static const uint64_t WINDOWS_TICK = 10000LL;
}

// Contains the elements required to calculate a counter value.
struct RawData
{
	DWORD CounterType = 0;
	ULONGLONG Data = 0;          // Raw counter data
	LONGLONG Time = 0;           // Is a time value or a base value
	DWORD MultiCounterData = 0;  // Second raw counter value for multi-valued counters
	LONGLONG Frequency = 0;
};

// cache
struct ProcessParameterCache    
{
    pid_t pid = 0;
    // time_t time;
    std::chrono::steady_clock::time_point expire;
    RTL_USER_PROCESS_PARAMETERS processParameters = {0};
    // ProcessParameterCache();
};

struct HDiskCounter {
    HCOUNTER readBytes = nullptr;    // Disk Read Bytes/sec
    HCOUNTER writeBytes = nullptr;   // Disk Write Bytes/sec
    HCOUNTER reads = nullptr;        // Disk Reads/sec
    HCOUNTER writes = nullptr;       // Disk Writes/sec

    HCOUNTER writeTime = nullptr;    // % Disk Write Time
    HCOUNTER readTime = nullptr;     // % Disk Read Time
    HCOUNTER time = nullptr;         // % Disk Time
    HCOUNTER idleTime = nullptr;     // % Disk Idle Time

    explicit HDiskCounter() = default;
    ~HDiskCounter();

    // noncopyable
    HDiskCounter(const HDiskCounter&) = delete;
    HDiskCounter &operator=(const HDiskCounter&) = delete;
    HDiskCounter(HDiskCounter&&) = delete;
    HDiskCounter& operator=(HDiskCounter&&) = delete;
};

struct Sic: SicBase
{
	unsigned int cpu_nums = 0;
	bool useNetInterfaceShortName = false;
	// pdh query
	HQUERY diskCounterQuery = nullptr;
	std::unordered_map<std::string, MIB_IFROW> netInterfaceCache;
	std::unordered_map<unsigned long, std::string> netInterfaceIndexToName;
	std::unordered_map<unsigned long, MIB_IPADDRROW> netIpAddrCache;
	std::string machine;
	HKEY handle = nullptr;
	long pagesize = 0;
	int ifConfigLength = 0;
	SicWin32ProcessInfo win32ProcessInfo = {};
    const std::chrono::seconds lastProcExpire{10}; // 进程信息缓存10秒

    ~Sic() override;
	void addNetListen(const SicNetConnectionInformation& info);
};

#include "common/test_support"
class Win32SystemInformationCollector : public SystemInformationCollector
{
public:
    explicit Win32SystemInformationCollector();
    int SicInitialize() override;

    ~Win32SystemInformationCollector() override;

    int SicGetThreadCpuInformation(pid_t pid, int tid, SicThreadCpu &) override;

    int SicGetCpuInformation(SicCpuInformation &information) override;
    int SicGetCpuListInformation(std::vector<SicCpuInformation> &informations) override;

    int SicGetMemoryInformation(SicMemoryInformation &information) override;

    int SicGetUpTime(double &uptime) override;

    int SicGetLoadInformation(SicLoadInformation &information) override;

    int SicGetSwapInformation(SicSwapInformation &information) override;

    int SicGetInterfaceConfigList(SicInterfaceConfigList &interfaceConfigList) override;
    int SicGetInterfaceConfig(SicInterfaceConfig &interfaceConfig, const std::string &name) override;

    int SicGetInterfaceInformation(const std::string &name,
                                   SicNetInterfaceInformation &information) override;

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

    // Windows下没有cutime、cstime
    int SicGetProcessTime(pid_t pid, SicProcessTime &processTime, bool = false) override;

    int SicGetProcessMemoryInformation(pid_t pid, SicProcessMemoryInformation &information) override;

    int SicGetProcessArgs(pid_t pid, SicProcessArgs &processArgs) override;

    int SicGetProcessExe(pid_t pid, SicProcessExe &processExe) override;

    int SicGetProcessCredName(pid_t pid, SicProcessCredName &processCredName) override;

    int SicGetProcessFd(pid_t pid, SicProcessFd &processFd) override;

private:
    int EnumCpuByNtQuerySystemInformation(std::string &errorMessage, const std::function<void(unsigned int, unsigned int, const SicCpuInformation &)> &callback);

    int SicEnablePrivilege(const std::string &name);

    int NtSysGetCpuInformation(SicCpuInformation &information, std::string &errorMessage);

    int NtSysGetCpuListInformation(std::vector<SicCpuInformation> &informations, std::string &errorMessage);

    int KernelGetMemoryInformation(SicMemoryInformation &information, std::string &errorMessage);

    int GetWindowsMemoryRam(SicMemoryInformation &information);

    int KernelGetSwapInformation(SicSwapInformation &information, std::string &errorMessage);

    int IphlpapiGetInterfaceTable(PMIB_IFTABLE &iftable, std::string &errorMessage);

    int IphlpapiGetIfEntry(MIB_IFROW *ifRow, std::string &errorMessage);

    int IphlpapiGetIfRow(MIB_IFROW *&ifRow, const std::string &name, std::string &errorMessage);

    int IphlpapiGetIpAddrTable(PMIB_IPADDRTABLE &ipAddrTable, std::string &errorMessage);

    int IphlpapiGetIfIpAddrRow(DWORD index, MIB_IPADDRROW &ipAddr, std::string &errorMessage);

    int IphlpapiGetIfIpv6Config(DWORD index, SicInterfaceConfig &interfaceConfig, std::string &errorMessage);

    int IphlpapiGetTcpTable(MIB_TCPTABLE *&tcpTable, std::string &errorMessage);

    int IphlpapiGetUdpTable(MIB_UDPTABLE *&udpTable, std::string &errorMessage);

    void NetListenAddressAdd(SicNetConnectionInformation &connectionInformation);

    double CalculateFileSystemUsage(SicFileSystemUsage &usage);

    int PsapiEnumProcesses(SicProcessList &processList, std::string &errorMessage, size_t step = WinConst::BUFFER_SIZE);

    int ParseProcessArgs(const WCHAR *buffer, SicProcessArgs &processArgs);

    int SicProcessArgsWmiGet(pid_t pid, SicProcessArgs &processArgs);

    int SicProcessExeWmiGet(pid_t pid, SicProcessExe &processExe);

    int RemoteProcessArgsGet(pid_t pid,
                             SicProcessArgs &processArgs,
                             std::string &errorMessage);

    int PebGetProcessArgs(HANDLE process,
                          SicProcessArgs &processArgs,
                          std::string &errorMessage,
                          pid_t pid);

    int PebGetProcessExe(HANDLE process,
                         SicProcessExe &processExe,
                         std::string &errorMessage,
                         pid_t pid);

    int RtlProcessParametersGet(HANDLE process,
                                RTL_USER_PROCESS_PARAMETERS &rtl,
                                std::string &errorMessage);

    int ProcessBasicInformationGet(HANDLE process, PEB &peb, std::string &errorMessage);

    std::shared_ptr<BYTE> GetPerformanceData(LPCSTR counterKey, std::string &errorMessage, size_t init = WinConst::INIT_OBJECT_BUFFER_SIZE, size_t step = WinConst::BUFFER_INCREMENT);

    static unsigned long GetSerialNo(const std::string &pInstanceName);

    PERF_INSTANCE_DEFINITION *GetParentInstance(PERF_OBJECT_TYPE *object, DWORD instancePosition);

    bool GetValue(LPBYTE perfDataHead,
                  PERF_OBJECT_TYPE *object,
                  PERF_COUNTER_DEFINITION *counter,
                  PERF_COUNTER_BLOCK *counterDataBlock,
                  RawData *rawData);

    bool GetFullInstanceName(LPBYTE perfDataHead,
                             DWORD codePage,
                             char *name,
                             std::string &errorMessage,
                             PERF_INSTANCE_DEFINITION *instance);

    PERF_INSTANCE_DEFINITION *GetObjectInstance(LPBYTE perfDataHead,
                                                PERF_OBJECT_TYPE *object,
                                                const std::string &instanceName,
                                                std::string &errorMessage);

    //PERF_COUNTER_DEFINITION *GetCounter(PERF_OBJECT_TYPE *object, DWORD counterToFind);
    //int GetCounter(PERF_OBJECT_TYPE *object, std::map<DWORD, PERF_COUNTER_DEFINITION *> &indexToCounterMap);

    PERF_OBJECT_TYPE *GetObject(LPBYTE perfDataHead, LPCSTR objectToFind);

    PERF_COUNTER_BLOCK *GetCounterBlock(LPBYTE perfDataHead,
                                        PERF_OBJECT_TYPE *object,
                                        const std::string &instanceName,
                                        std::string &errorMessage);

    static PERF_COUNTER_BLOCK *GetCounterBlock(PERF_INSTANCE_DEFINITION *instance);

    bool ConvertNameToUnicode(UINT codePage, LPCSTR nameToConvert, DWORD nameToConvertLen, LPWSTR convertedName);

    bool GetCounterValues(LPBYTE perfDataHead,
                          LPCSTR objectIndex,
                          const std::string &instanceName,
                          RawData *rawData,
                          std::string &errorMessage,
                          DWORD counterIndex);

    int PerfLibGetValue(RawData &rawData,
                        LPCSTR counterKey,
                        DWORD counterIndex,
                        std::string instanceName,
                        std::string &errorMessage);

    int PerfLibGetCpuInformation(SicCpuInformation &information, std::string &errorMessage);

    int PerfLibGetCpuListInformation(std::vector<SicCpuInformation> &informations, std::string &errorMessage);

    int PerfLibGetMemoryInformation(SicMemoryInformation *memoryInformation,
                                    SicSwapInformation *swapInformation,
                                    std::string &errorMessage);

    int PerfLibGetDiskUsage(SicDiskUsage &usage,
                            std::string &dirName,
                            std::string &errorMessage);

    int PdhDiskInit(std::string &errorMessage);

    int PdhDiskAddCounter(const std::string &dirName, std::string &errorMessage);

    int PdhDiskDestroy();

    int PdhGetDiskUsage(SicDiskUsage &usage, const std::string &dirName, std::string &errorMessage);

    int PerfLibGetProcessInfo(pid_t pid, std::string &errorMessage);

    int PerfLibGetUpTime(double &uptime, std::string &errorMessage);

    std::map<std::string, std::string> queryDevSerialId(const std::string &devName) override;
    int initProcessParameter(HANDLE process, pid_t pid, std::string &errorMessage,
                             RTL_USER_PROCESS_PARAMETERS &processParameters);
public:
    int SicGetSystemInfo(SicSystemInfo &systemInfo) override;

private:
    std::unordered_map<std::string, std::shared_ptr<HDiskCounter>> mDiskCounter;

    //DllModuleMap mdllModuleMap;
    ProcessParameterCache mProcessParameterCache;
    const Win32API& win32API;

	std::shared_ptr<Sic> SicPtr() const;
};
#include "common/test_support"

#endif //SIC_INCLUDE_WIN32_SYSTEM_INFORMATION_COLLECTOR_H_
