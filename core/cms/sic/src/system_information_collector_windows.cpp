#pragma warning(disable: 4819 4996)
#pragma comment(lib, "pdh.lib")
//
// Created by liuze.xlz on 2020/11/23.
//
#include <winsock2.h>
#include <ws2tcpip.h>
#include <IPTypes.h>

#include <windows.h>
#include <iprtrmib.h>
#include <pdh.h>
#include <shellapi.h>
// #include <process.h>  // _exit

#ifdef max
#	undef max
#endif
#ifdef min
#	undef min
#endif

#include "sic/win32_sic_utils.h"
#include "sic/win32_system_information_collector.h"
#include "sic/system_information_collector.h"
#include "sic/system_information_collector_util.h"

#include "common/ScopeGuard.h"
#include "common/Arithmetic.h"
#include "common/FieldEntry.h"
#include "common/Logger.h"
#include "common/TimeProfile.h"
#include "common/ChronoLiteral.h"

#include <thread>
#include <utility>
#include <cstring>
#include <functional>
#include <map>
#include <cassert>
#include <type_traits> // add_pointer
#include <cstdlib>     // malloc, free, _exit

static const char *SIC_NIC_LOOPBACK = "Local Loopback";
static const char *SIC_NIC_ETHERNET = "Ethernet";

// scope values from linux-2.6/include/net/ipv6.h
static const int SIC_IPV6_ADDR_ANY = 0x0000;
static const int SIC_IPV6_ADDR_UNICAST = 0x0001;
static const int SIC_IPV6_ADDR_MULTICAST = 0x0002;
static const int SIC_IPV6_ADDR_LOOPBACK = 0x0010;
static const int SIC_IPV6_ADDR_LINKLOCAL = 0x0020;
static const int SIC_IPV6_ADDR_SITELOCAL = 0x0040;
static const int SIC_IPV6_ADDR_COMPATv4 = 0x0080;

#define FAIL_GUARD(code) bool failed = true; \
	defer(if (failed) {code; })

using namespace WinConst;
using namespace std::chrono;

typedef ULONG(WINAPI* SicIphlpapiGetAdaptersAddresses)(ULONG, ULONG, PVOID, PIP_ADAPTER_ADDRESSES, PULONG);
// kernel32.dll
typedef HANDLE(WINAPI* SicOpenProcess)(DWORD,BOOL,DWORD);
typedef BOOL(WINAPI* SicCloseHandle)(HANDLE);
typedef BOOL(WINAPI *SicReadProcessMemory)(HANDLE, LPCVOID, LPVOID, SIZE_T, SIZE_T *);

// advapi32.dll
typedef BOOL (WINAPI* SicOpenProcessToken)(HANDLE, DWORD, PHANDLE);

#define PdhFirstCounter(object) ((PERF_COUNTER_DEFINITION *)((BYTE *) object + object->HeaderLength))
#define PdhNextCounter(counter) ((PERF_COUNTER_DEFINITION *)((BYTE *) counter + counter->ByteLength))

template<typename T> void Free(T *&ptr) {
	if (ptr != nullptr) {
		free(ptr);
		ptr = nullptr;
	}
}

void ExitArgus(int status) {
    _exit(status);
}

std::string makeErrorCounterNotFound(const std::map<DWORD, PERF_COUNTER_DEFINITION*>& mapData) {
	const char* sep = "";

	std::string msg = "Counter ";
	for (auto const& item : mapData) {
		if (item.second == nullptr) {
			msg.append(sep);
			msg.append(std::to_string(item.first));
			sep = ", ";
		}
	}
	msg.append(" not found.");

	return msg;
}

template<typename T>
T minT(T a, T b) {
	return a < b ? a : b;
}

// num单位为：100纳秒
std::chrono::microseconds Nano100ToMicroSeconds(unsigned long long num) {
	return microseconds{ num / 10 };
}

static HANDLE hPdhLibrary = LoadLibrary("pdh.dll");
std::string PdhErrorStr(PDH_STATUS errCode, const std::string& functionName)
{
	std::string msg;
	if (hPdhLibrary != nullptr) {
		// Retrieve the system error message for the last-error code
		LPVOID lpMsgBuf = nullptr;
		FormatMessage(
			FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
			hPdhLibrary,
			errCode,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,
			0,
			NULL);
		defer(LocalFree(lpMsgBuf));

		std::ostringstream oss;
		oss << functionName << (functionName.empty() ? "" : " ")
			<< "failed with error " << errCode << ": "
			<< (LPCTSTR)(lpMsgBuf ? lpMsgBuf : "");
		msg = TrimRightSpace(oss.str());
	}
	return msg;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Win32API
template<typename FProc>
class DllEntry {
	typename std::add_pointer<FProc>::type fnProc = nullptr;
public:
	const char* const dllName = nullptr;
	const std::string procName;

	DllEntry(const char* dllName, const char* procName, std::map<std::string, HMODULE>& dll)
		:dllName(dllName), procName(procName) {
		if (dll.find(dllName) == dll.end()) {
			dll[dllName] = LoadLibrary(dllName);
		}
		HMODULE hModule = dll[dllName];
		if (hModule != INVALID_HANDLE_VALUE) {
			FARPROC proc = GetProcAddress(hModule, procName);
			fnProc = (decltype(fnProc))proc;
			//auto func = std::function<FProc>((std::add_pointer<FProc>::type)proc);
			//this->swap(func);
		}
		//std::cout << "LoadLibrary(" << dllName << ", " << procName << ") => " << std::boolalpha << (bool)(*this) << std::endl;
	}

	explicit operator bool() const {
		return fnProc != nullptr;
	}

	template<typename ...Args>
	auto operator()(Args &&...args) const -> decltype(fnProc(std::forward<Args>(args)...)) {
		//TimeProfile tp;
		//defer(LogDebug("[{}]::{}(...) cost: {}", dllName, procName, tp.costString<boost::chrono::microseconds>()));
		return fnProc(std::forward<Args>(args)...);
	}

	DllEntry(const DllEntry&) = delete;
	DllEntry& operator=(const DllEntry&) = delete;
};

#define DLL_ENTRY(F) DllEntry<std::remove_pointer<F>::type>
class Win32API {
	std::map<std::string, HMODULE> dll;
public:
	DLL_ENTRY(SicNtQuerySystemInformation) NtQuerySystemInformation = { "ntdll.dll", "NtQuerySystemInformation", dll };
	DLL_ENTRY(SicNtQueryProcessInformation) NtQueryInformationProcess = { "ntdll.dll", "NtQueryInformationProcess", dll };

	DLL_ENTRY(SicKernelMemoryStatus) GlobalMemoryStatusEx = { "kernel32.dll", "GlobalMemoryStatusEx", dll };
	DLL_ENTRY(SicOpenProcess) OpenProcess = { "kernel32.dll", "OpenProcess", dll };
	DLL_ENTRY(SicCloseHandle) CloseHandle = { "kernel32.dll", "CloseHandle", dll };
	DLL_ENTRY(SicReadProcessMemory) ReadProcessMemory = { "kernel32.dll", "ReadProcessMemory", dll };

	DLL_ENTRY(SicIphlpapiGetIpForwardTable) GetIpForwardTable = { "iphlpapi.dll", "GetIpForwardTable", dll };
	DLL_ENTRY(SicIphlpapiGetIpAddrTable) GetIpAddrTable = { "iphlpapi.dll", "GetIpAddrTable", dll };
	DLL_ENTRY(SicIphlpapiGetIfTable) GetIfTable = { "iphlpapi.dll", "GetIfTable", dll };
	DLL_ENTRY(SicIphlpapiGetIfEntry) GetIfEntry = { "iphlpapi.dll", "GetIfEntry", dll };
	//DLL_ENTRY(SicIphlpapiGetNumberOfInterfaces) GetNumberOfInterfaces = { "iphlpapi.dll", "GetNumberOfInterfaces", dll };
	DLL_ENTRY(SicIphlpapiGetTcpTable) GetTcpTable = { "iphlpapi.dll", "GetTcpTable", dll };
	DLL_ENTRY(SicIphlpapiGetUdpTable) GetUdpTable = { "iphlpapi.dll", "GetUdpTable", dll };
	//DLL_ENTRY(SicIphlpapiGetTcpStatistics) GetTcpStatistics = { "iphlpapi.dll", "GetTcpStatistics", dll };
	//DLL_ENTRY(SicIphlpapiGetNetworkParams) GetNetworkParams = { "iphlpapi.dll", "GetNetworkParams", dll };
	//DLL_ENTRY(SicIphlpapiGetAdaptersInfo) GetAdaptersInfo = { "iphlpapi.dll", "GetAdaptersInfo", dll };
	DLL_ENTRY(SicIphlpapiGetAdaptersAddresses) GetAdaptersAddresses = { "iphlpapi.dll", "GetAdaptersAddresses", dll };
	//DLL_ENTRY(SicIphlpapiGetIpNetTable) GetIpNetTable = { "iphlpapi.dll", "GetIpNetTable", dll };

	DLL_ENTRY(SicMprGetNetConnection) WNetGetConnection = { "mpr.dll", "WNetGetConnection", dll };

	//DLL_ENTRY(SicIphlpapiEnumProcessModules) EnumProcessModules = { "psapi.dll", "EnumProcessModules", dll };
	//DLL_ENTRY(SICPsapiGetModuleFileNameExA) GetModuleFileNameExA = { "psapi.dll", "GetModuleFileNameExA", dll };
	DLL_ENTRY(SICPsapiEnumProcesses) EnumProcesses = { "psapi.dll", "EnumProcesses", dll };

	DLL_ENTRY(SicOpenProcessToken) OpenProcessToken = {"advapi32.dll", "OpenProcessToken", dll};

	int IphlpapiGetAdaptersAddresses(ULONG family, ULONG flags, PIP_ADAPTER_ADDRESSES& addresses, ULONG& size, std::string& errorMessage) const;

	~Win32API() {
		for (auto it = dll.begin(); it != dll.end(); it++) {
			FreeLibrary(it->second);
		}
		dll.clear();
	}
};
static const Win32API globalWin32API; // 程序启动时初始化一次即可，犯不着每次都实例化一个

struct HandleDeleter {
	void operator()(HANDLE handle) {
		if (handle) {
			globalWin32API.CloseHandle(handle);
		}
	}
};
using unique_handle = std::unique_ptr<std::remove_pointer<HANDLE>::type, HandleDeleter>;



#define RETURN_IF_NULL(RetVal, API) if (!API) {\
	std::stringstream ss; \
	ss << API.procName << " handle is nullptr, maybe can not find in " << API.dllName << "?"; \
	errorMessage = ss.str(); \
	return RetVal; \
}

int Win32API::IphlpapiGetAdaptersAddresses(ULONG family, ULONG flags, PIP_ADAPTER_ADDRESSES& addresses, ULONG& size, std::string& errorMessage) const {
	FAIL_GUARD({
		Free(addresses);
		size = 0;
		});

	RETURN_IF_NULL(SIC_EXECUTABLE_FAILED, this->GetAdaptersAddresses);

	const int maxTries = 3;
	ULONG res = ERROR_BUFFER_OVERFLOW;
	for(int i = 0; res == ERROR_BUFFER_OVERFLOW && i < maxTries; ++i) {
		res = this->GetAdaptersAddresses(family, flags, nullptr, addresses, &size);
		if (res == ERROR_BUFFER_OVERFLOW) {
			addresses = static_cast<PIP_ADAPTER_ADDRESSES>(realloc(addresses, size));
		}
	}

	failed = (res != ERROR_SUCCESS);
	if (failed) {
		errorMessage = ErrorStr(res, "GetAdaptersAddresses");
		return SIC_EXECUTABLE_FAILED;
	}
	return SIC_EXECUTABLE_SUCCESS;
}


// ////////////////////////////////////////////////////////////////////////////////////////////////////
// //
// ProcessParameterCache::ProcessParameterCache() {
// 	memset(this, 0, sizeof(*this));
// }

////////////////////////////////////////////////////////////////////////////////////////////////////
// 
// [out] name
// [out] errorMessge
int IphlpapiGetInterfaceName(MIB_IFROW& ifRow, PIP_ADAPTER_ADDRESSES addresses, std::string& name, std::string& errorMessage) {
	if (addresses == nullptr) {
		errorMessage = "addresses is nullptr";
		return SIC_EXECUTABLE_FAILED;
	}

	std::string interfaceName = WcharToChar(ifRow.wszName, CP_ACP);
	//char interfaceName[2 * MAX_INTERFACE_NAME_LEN + 1];
	//if (WideCharToMultiByte(CP_ACP, 0, ifRow.wszName, -1, interfaceName, sizeof(interfaceName), nullptr, nullptr) > 0) {
	if (!interfaceName.empty()) {
		for (PIP_ADAPTER_ADDRESSES next = addresses; next != nullptr; next = next->Next) {
			decltype(next->PhysicalAddressLength) index = 0;
			for (index = 0; index < next->PhysicalAddressLength; ++index) {
				if (next->PhysicalAddress[index] != ifRow.bPhysAddr[index]) {
					break;
				}
			}
			if (index == next->PhysicalAddressLength && interfaceName.find(next->AdapterName) != std::string::npos) {
				std::string tmp = WcharToChar(next->FriendlyName, CP_UTF8);
				if (!tmp.empty()) { // WideCharToMultiByte(CP_UTF8, 0, next->FriendlyName, -1, interfaceName, sizeof(interfaceName), nullptr, nullptr) > 0) {
					name = tmp;
					return SIC_EXECUTABLE_SUCCESS;
				}
			}
		}
	}

	errorMessage = std::string(interfaceName) + " not found ";
	return SIC_EXECUTABLE_FAILED;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// HDiskCounter
struct DiskCounterFieldEntry : FieldName<HDiskCounter, HCOUNTER> {
	std::function<double& (SicDiskUsage&)> _fnGetUsage;
public:
	const std::string desc;
	DiskCounterFieldEntry(const char* name,
		std::function<HCOUNTER& (HDiskCounter&)> fnGet,
		std::function<double& (SicDiskUsage&)> fnGetUsage,
		const std::string& s)
		:FieldName<HDiskCounter, HCOUNTER>(name, fnGet), _fnGetUsage(fnGetUsage), desc(s) {
	}

	using FieldName<HDiskCounter, HCOUNTER>::value;

	double value(const SicDiskUsage &p) const {
		return _fnGetUsage(const_cast<SicDiskUsage&>(p));
	}

	double &value(SicDiskUsage &p) const {
		return _fnGetUsage(p);
	}
};

#define DISK_COUNTER_KEY_ENTRY(member, umember, s) DiskCounterFieldEntry(#member, [](HDiskCounter &p){return std::ref(p.member);}, [](SicDiskUsage &p){return std::ref(p.umember);}, s)
static const DiskCounterFieldEntry diskCounterKeyIndex[] = {
		DISK_COUNTER_KEY_ENTRY(readBytes, readBytes, "Disk Read Bytes/sec"),
		DISK_COUNTER_KEY_ENTRY(writeBytes, writeBytes, "Disk Write Bytes/sec"),
		DISK_COUNTER_KEY_ENTRY(reads, reads, "Disk Reads/sec"),
		DISK_COUNTER_KEY_ENTRY(writes, writes, "Disk Writes/sec"),

		DISK_COUNTER_KEY_ENTRY(writeTime, wTime, "Disk Write Time"),
		DISK_COUNTER_KEY_ENTRY(readTime, rTime, "Disk Read Time"),
		DISK_COUNTER_KEY_ENTRY(time, time, "Disk Time"),
		DISK_COUNTER_KEY_ENTRY(idleTime, idle, "Disk Idle Time"),
};
#undef DISK_COUNTER_KEY_ENTRY
static const size_t diskCounterKeyIndexCount = sizeof(diskCounterKeyIndex) / sizeof(diskCounterKeyIndex[0]);
static const DiskCounterFieldEntry* diskCounterKeyIndexEnd = diskCounterKeyIndex + diskCounterKeyIndexCount;
static_assert(diskCounterKeyIndexCount == sizeof(HDiskCounter) / sizeof(HCOUNTER), "diskCounterKeyIndex size unexpected");

void enumerate(const std::function<void(const FieldName<HDiskCounter, HCOUNTER>&)>& callback) {
	for (auto it = diskCounterKeyIndex; it < diskCounterKeyIndexEnd; ++it) {
		callback(*it);
	}
}

HDiskCounter::~HDiskCounter() {
	for (auto& it : diskCounterKeyIndex) {
		auto& counter = it.value(*this);
		if (counter != nullptr) {
			PdhRemoveCounter(counter);
			counter = nullptr;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// 

Sic::~Sic() {
	if (diskCounterQuery) {
		PdhCloseQuery(diskCounterQuery);
		diskCounterQuery = nullptr;
	}
	if (handle) {
		RegCloseKey(handle);
		handle = nullptr;
	}
}

void Sic::addNetListen(const SicNetConnectionInformation& info)
{
	if (netListenCache.find(info.localPort) != netListenCache.end())
	{
		if (info.localAddress.family == SicNetAddress::SIC_AF_INET6)
		{
			return;
		}
	}
	netListenCache[info.localPort] = info.localAddress;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// 
Win32SystemInformationCollector::Win32SystemInformationCollector()
	: SystemInformationCollector(std::make_shared<Sic>()), win32API(globalWin32API)
{
}

Win32SystemInformationCollector::~Win32SystemInformationCollector() {
	PdhDiskDestroy();
}

std::shared_ptr<Sic> Win32SystemInformationCollector::SicPtr() const {
	return std::dynamic_pointer_cast<Sic>(mSicPtr);
}

int Win32SystemInformationCollector::SicInitialize()
{
	SicPtr()->machine = "";

	//OSVERSIONINFO version;
	//version.dwOSVersionInfoSize = sizeof(version);
	//GetVersionEx(&version);

	//LONG result = RegConnectRegistryA(SicPtr()->machine.c_str(), HKEY_PERFORMANCE_DATA, &SicPtr()->handle);

	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	SicPtr()->cpu_nums = systemInfo.dwNumberOfProcessors;
	SicPtr()->pagesize = systemInfo.dwPageSize;

	SicEnablePrivilege(SE_DEBUG_NAME);

	SicPtr()->useNetInterfaceShortName = false;

	SicPtr()->errorMessage = "";
	return PdhDiskInit(SicPtr()->errorMessage);
}

int Win32SystemInformationCollector::SicEnablePrivilege(const std::string& name)
{
	int status;
	HANDLE handle;

	if (!win32API.OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &handle)) {
		return LastError("OpenProcessToken", &SicPtr()->errorMessage);
	}
	unique_handle _guard{ handle };

	TOKEN_PRIVILEGES tokenPrivileges;
	memset(&tokenPrivileges, '\0', sizeof(tokenPrivileges));
	if (LookupPrivilegeValue(nullptr, name.c_str(), &tokenPrivileges.Privileges[0].Luid))
	{
		tokenPrivileges.PrivilegeCount = 1;
		tokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		if (AdjustTokenPrivileges(handle, FALSE, &tokenPrivileges, 0, nullptr, 0))
		{
			status = SIC_EXECUTABLE_SUCCESS;
		}
		else
		{
			status = LastError("AdjustTokenPrivileges", &SicPtr()->errorMessage);
		}
	}
	else
	{
		status = LastError("LookupPrivilegeValue", &SicPtr()->errorMessage);
	}

	// CloseHandle(handle);

	return status;
}

int Win32SystemInformationCollector::EnumCpuByNtQuerySystemInformation(std::string &errorMessage, const std::function<void(unsigned int, unsigned int, const SicCpuInformation &)> &callback) {
    RETURN_IF_NULL(SIC_EXECUTABLE_FAILED, win32API.NtQuerySystemInformation);

	size_t cpuCores = 0;
    for (int i = 0; i < 2; i++) {
		std::vector<SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION> info{ cpuCores };
        ULONG infoLen = static_cast<ULONG>(info.size() * sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION));
        ULONG res = 0;

        DWORD status = win32API.NtQuerySystemInformation(SystemProcessorPerformanceInformation, (info.empty()? nullptr: &info[0]), infoLen, &res);
        if (!res || (status != STATUS_SUCCESS && status != STATUS_INFO_LENGTH_MISMATCH && status != STATUS_BUFFER_TOO_SMALL)) {
            return LastError(win32API.NtQuerySystemInformation.procName + "(SystemProcessorPerformanceInformation, ...)", &errorMessage);
        }
        unsigned int cpuNum = res / sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION);
        if (cpuNum > cpuCores) {
			cpuCores = cpuNum;
            continue;
        }

        // information = {};
        for (unsigned int coreIndex = 0; coreIndex < cpuNum; coreIndex++) {
            SicCpuInformation cpuCore;
            cpuCore.idle = Nano100ToMicroSeconds(info[coreIndex].IdleTime.QuadPart);
            cpuCore.user = Nano100ToMicroSeconds(info[coreIndex].UserTime.QuadPart);
            cpuCore.sys = Nano100ToMicroSeconds(info[coreIndex].KernelTime.QuadPart - info[coreIndex].IdleTime.QuadPart);
            cpuCore.irq = Nano100ToMicroSeconds(info[coreIndex].InterruptTime.QuadPart);
            //cpuCore.total = cpuCore.idle + cpuCore.user + cpuCore.sys + cpuCore.irq;
            callback(cpuNum, coreIndex, cpuCore);
        }
		break;
    }
	return SIC_EXECUTABLE_SUCCESS;
}
int Win32SystemInformationCollector::NtSysGetCpuInformation(SicCpuInformation& information, std::string& errorMessage)
{
    information = {};
    return EnumCpuByNtQuerySystemInformation(errorMessage, [&](unsigned int, unsigned int, const SicCpuInformation &cpuCore){
        information += cpuCore;
    });
}

int Win32SystemInformationCollector::NtSysGetCpuListInformation(std::vector<SicCpuInformation>& informations, std::string& errorMessage)
{
    informations.clear();
    return EnumCpuByNtQuerySystemInformation(errorMessage, [&](unsigned int cpuNum, unsigned int coreIdx, const SicCpuInformation &cpuCore){
        if (coreIdx == 0) {
            informations.reserve(cpuNum);
        }
        informations.push_back(cpuCore);
    });
}

static const auto &counterKeyMap = *new std::unordered_map<std::string, std::string>{
	{PERF_SYS_COUNTER_KEY, "PERF_SYS_COUNTER_KEY"},
	{PERF_MEM_COUNTER_KEY, "PERF_MEM_COUNTER_KEY"},
	{PERF_PROC_COUNTER_KEY, "PERF_PROC_COUNTER_KEY"},
	{PERF_CPU_COUNTER_KEY, "PERF_CPU_COUNTER_KEY"},
	{PERF_DISK_COUNTER_KEY, "PERF_DISK_COUNTER_KEY"},
};
std::string counterKeyName(const std::string& counterKey) {
	auto it = counterKeyMap.find(counterKey);
	return it == counterKeyMap.end() ? counterKey : it->second;
}

class ObjectBufSize {
	mutable std::mutex mutex;
	std::unordered_map<std::string, DWORD> counterKeySize;
public:
	DWORD size(const std::string& counterKey, DWORD defaultValue) const {
		std::lock_guard<std::mutex> guard(mutex);
		auto it = counterKeySize.find(counterKey);
		return it == counterKeySize.end() ? defaultValue : it->second;
	}
	void setSize(const std::string& counterKey, DWORD size) {
		std::lock_guard<std::mutex> guard(mutex);
		auto it = counterKeySize.find(counterKey);
		if (it == counterKeySize.end()) {
			counterKeySize.emplace(counterKey, size);
		}
		else if (it->second < size) {
			it->second = size;
		}
	}
};
static ObjectBufSize objectBufSize;

// Retrieve a buffer that contains the specified performance data.
// The pwszSource parameter determines the data that GetRegistryBuffer returns.
//
// Typically, when calling RegQueryValueEx, you can specify zero for the size of the buffer
// and the RegQueryValueEx will set your size variable to the required buffer size. However,
// if the source is "Global" or one or more object index values, you will need to increment
// the buffer size in a loop until RegQueryValueEx does not return ERROR_MORE_DATA.
std::shared_ptr<BYTE> Win32SystemInformationCollector::GetPerformanceData(LPCSTR counterKey, std::string& errorMessage, size_t init, size_t step) {
	auto queryValue = [=](LPBYTE buf, DWORD& dwSize) -> LSTATUS {
		return RegQueryValueEx(HKEY_PERFORMANCE_DATA, counterKey, nullptr, nullptr, buf, &dwSize);
	};

	TimeProfile tp;
	DWORD dwBufferSize = objectBufSize.size(counterKey, static_cast<DWORD>(init));    //Size of buffer, used to increment buffer size

	DWORD dwSize = dwBufferSize;              //Size of buffer, used when calling RegQueryValueEx
	auto pBuffer = std::shared_ptr<BYTE>{ (LPBYTE)malloc(dwBufferSize), free };
	if (pBuffer) {
		defer(RegCloseKey(HKEY_PERFORMANCE_DATA));
		LONG status = ERROR_SUCCESS;
		int tryCount = 0;

		TimeProfile tp1;
		while (ERROR_MORE_DATA == (status = queryValue(pBuffer.get(), dwSize))) {
			LogDebug("RegQueryValueEx(HKEY_PERFORMANCE_DATA, {})[{}] ERROR_MORE_DATA, dwSize:{}, dwBufferSize: {}, cost: {}", counterKeyName(counterKey),
				tryCount++, convert(dwSize, true), convert(dwBufferSize, true),
				tp1.cost<std::chrono::microseconds>());

			pBuffer.reset();
			//Contents of dwSize is unpredictable if RegQueryValueEx fails, which is why
			//you need to increment dwBufferSize and use it to set dwSize.
			dwBufferSize += static_cast<decltype(dwBufferSize)>(step);
			dwSize = dwBufferSize;
			pBuffer = std::shared_ptr<BYTE>{ (LPBYTE)malloc(dwBufferSize), free };
			if (!pBuffer) {
				errorMessage = (sout{} << "realloc(" << dwBufferSize << ") error.").str();
				return std::shared_ptr<BYTE>();
			}
		}

		if (tryCount > 0 || tp.millis(false) > 2000) {
			LogInfo("RegQueryValueEx(HKEY_PERFORMANCE_DATA, {})[{}] {}, init: {}, step: {}, final: {}/{}, cost: {}",
				counterKeyName(counterKey), tryCount, (status == ERROR_SUCCESS ? "OK" : "Fail"),
				convert(init, true), convert(step, true), convert(dwSize, true), convert(dwBufferSize, true),
				tp.cost<std::chrono::microseconds>());
		}

		if (ERROR_SUCCESS != status) {
			errorMessage = ErrorStr(status, (sout{} << "RegQueryValueEx(HKEY_PERFORMANCE_DATA, \"" << counterKeyName(counterKey) << "\", ...)").str());
			return std::shared_ptr<BYTE>();
		}

		objectBufSize.setSize(counterKey, dwBufferSize);
	}
	else {
		// if malloc failed , pBuffer is nullptr
		errorMessage = (sout{} << "malloc(" << dwBufferSize << ") failed to allocate initial memory request.").str();
	}

	return pBuffer;
}

// Parses the instance name for the serial number. The convention is to use
// a serial number appended to the instance name to identify a specific
// instance when the instance names are not unique. For example, to specify
// the fourth instance of svchost, use svchost#3 (the first occurrence does
// not include a serial number).
unsigned long Win32SystemInformationCollector::GetSerialNo(const std::string& pInstanceName)
{
	unsigned long value = 0;
	size_t index = pInstanceName.find('#');
	if (index != std::string::npos)
	{
		value = convert<unsigned long>(pInstanceName.substr(index + 1));
	}
	return value;
}

// Find the nth instance of an object.
PERF_INSTANCE_DEFINITION* Win32SystemInformationCollector::GetParentInstance(PERF_OBJECT_TYPE* object, DWORD instancePosition)
{
	LPBYTE instance = (LPBYTE)object + object->DefinitionLength;

	for (DWORD i = 0; i < instancePosition; i++)
	{
		auto counter = (PERF_COUNTER_BLOCK*)(instance + ((PERF_INSTANCE_DEFINITION*)instance)->ByteLength);
		instance += ((PERF_INSTANCE_DEFINITION*)instance)->ByteLength + counter->ByteLength;
	}

	return (PERF_INSTANCE_DEFINITION*)instance;
}

// Retrieve the full name of the instance. The full name of the instance includes
// the name of this instance and its parent instance, if this instance is a
// child instance. The full name is in the form, "parent name/child name".
// For example, a thread instance is a child of a process instance.
//
// Providers are encouraged to use Unicode strings for instance names. If
// PERF_INSTANCE_DEFINITION.CodePage is zero, the name is in Unicode; otherwise,
// use the CodePage value to convert the string to Unicode.
bool Win32SystemInformationCollector::GetFullInstanceName(LPBYTE perfDataHead,
	DWORD codePage,
	char* name,
	std::string& errorMessage,
	PERF_INSTANCE_DEFINITION* instance)
{
	bool success = true;
	WCHAR wszInstanceName[MAX_INSTANCE_NAME_LEN + 1];
	WCHAR wszParentInstanceName[MAX_INSTANCE_NAME_LEN + 1];
	std::string szInstanceName;
	std::string szParentInstanceName;

	if (codePage == 0)  // Instance name is a Unicode string
	{
		// PERF_INSTANCE_DEFINITION->NameLength is in bytes, so convert to characters.
		DWORD length = minT<DWORD>(MAX_INSTANCE_NAME_LEN, instance->NameLength / 2);
		StringCchCopyNW(wszInstanceName,
			MAX_INSTANCE_NAME_LEN + 1,
			(LPWSTR)(((LPBYTE)instance) + instance->NameOffset),
			length);
		wszInstanceName[length] = '\0';
	}
	else  // Convert the multi-byte instance name to Unicode
	{
		success = ConvertNameToUnicode(codePage,
			(LPCSTR)(((LPBYTE)instance) + instance->NameOffset),  // Points to string
			instance->NameLength,
			wszInstanceName);

		if (!success)
		{
			errorMessage = "ConvertNameToUnicode for instance failed.";
			return success;
		}
	}

	// convert wchar to string;
	// codePage : 0 is unicode
	szInstanceName = WcharToChar(wszInstanceName, 0);

	if (instance->ParentObjectTitleIndex)
	{
		// Use the index to find the parent object. The pInstance->ParentObjectInstance
		// member tells you that the parent instance is the nth instance of the
		// parent object.
		PERF_OBJECT_TYPE* parentObject = GetObject(perfDataHead, std::to_string(instance->ParentObjectTitleIndex).c_str());
		PERF_INSTANCE_DEFINITION* parentInstance = GetParentInstance(parentObject, instance->ParentObjectInstance);

		if (codePage == 0)  // Instance name is a Unicode string
		{
			DWORD length = minT<DWORD>(MAX_INSTANCE_NAME_LEN, parentInstance->NameLength / 2);
			StringCchCopyNW(wszParentInstanceName,
				MAX_INSTANCE_NAME_LEN + 1,
				(LPWSTR)(((LPBYTE)parentInstance) + parentInstance->NameOffset),
				length);
			wszParentInstanceName[length] = '\0';
		}
		else  // Convert the multi-byte instance name to Unicode
		{
			success = ConvertNameToUnicode(codePage,
				(LPCSTR)(((LPBYTE)parentInstance) + parentInstance->NameOffset),  //Points to string.
				instance->NameLength,
				wszParentInstanceName);

			if (!success)
			{
				errorMessage = "ConvertNameToUnicode for parent instance failed.";
				return success;
			}
		}

		szParentInstanceName = WcharToChar(wszParentInstanceName, 0);
		StringCchPrintf(name,
			MAX_FULL_INSTANCE_NAME_LEN + 1,
			"%s/%s",
			szParentInstanceName.c_str(),
			szInstanceName.c_str());
	}
	else
	{
		StringCchPrintf(name, MAX_INSTANCE_NAME_LEN + 1, "%s", szInstanceName.c_str());
	}

	return success;
}

// Converts a multi-byte string to a Unicode string. If the input string is longer than
// MAX_INSTANCE_NAME_LEN, the input string is truncated.
bool Win32SystemInformationCollector::ConvertNameToUnicode(UINT codePage,
	LPCSTR nameToConvert,
	DWORD nameToConvertLen,
	LPWSTR convertedName)
{
	// dwNameToConvertLen is in bytes, so convert MAX_INSTANCE_NAME_LEN to bytes.
	DWORD length = minT<DWORD>(MAX_INSTANCE_NAME_LEN * sizeof(WCHAR), nameToConvertLen);

	int charsConverted =
		MultiByteToWideChar(codePage, 0, nameToConvert, length, convertedName, MAX_INSTANCE_NAME_LEN);
	if (charsConverted) {
		convertedName[length] = '\0';
	}

	return charsConverted != 0;
}

// Returns a pointer to the beginning of the PERF_COUNTER_BLOCK. The location of the
// of the counter data block depends on whether the object contains single instance
// counters or multiple instance counters (see PERF_OBJECT_TYPE.NumInstances).
PERF_COUNTER_BLOCK* Win32SystemInformationCollector::GetCounterBlock(LPBYTE perfDataHead, PERF_OBJECT_TYPE* object,
	const std::string& instanceName, std::string& errorMessage)
{
	PERF_COUNTER_BLOCK* block = nullptr;
	PERF_INSTANCE_DEFINITION* instance;

	// If there are no instances, the block follows the object and counter structures.
	if (0 == object->NumInstances || PERF_NO_INSTANCES == object->NumInstances)
	{
		block = (PERF_COUNTER_BLOCK*)((LPBYTE)object + object->DefinitionLength);
	}
	else if (object->NumInstances > 0 && !instanceName.empty())  // Find the instance. The block follows the instance
	{
		// structure and instance name.
		instance = GetObjectInstance(perfDataHead, object, instanceName, errorMessage);
		if (instance)
		{
			block = (PERF_COUNTER_BLOCK*)((LPBYTE)instance + instance->ByteLength);
		}
	}

	return block;
}

// If the instance names are unique (there will never be more than one instance
// with the same name), then finding the same instance is not an issue. However,
// if the instance names are not unique, there is no guarantee that the instance
// whose counter value you are retrieving is the same instance from which you previously
// retrieved data. This function expects the instance name to be well formed. For
// example, a process object could have four instances with each having svchost as its name.
// Since service hosts come and go, there is no way to determine if you are dealing with
// the same instance.
//
// The convention for specifying an instance is parentinstancename/instancename#nnn.
// If only instancename is specified, the first instance found that matches the name is used.
// Specify parentinstancename if the instance is the child of a parent instance.
PERF_INSTANCE_DEFINITION* Win32SystemInformationCollector::GetObjectInstance(LPBYTE perfDataHead,
	PERF_OBJECT_TYPE* object,
	const std::string& instanceName,
	std::string& errorMessage)
{
	auto* instance = (PERF_INSTANCE_DEFINITION*)((LPBYTE)object + object->DefinitionLength);
	bool success = false;
	CHAR szName[MAX_FULL_INSTANCE_NAME_LEN + 1];
	PERF_COUNTER_BLOCK* counterBlock;
	DWORD serialNo = GetSerialNo(instanceName);
	DWORD occurrencesFound = 0;
	size_t inputNameLen = instanceName.length();

	for (LONG i = 0; i < object->NumInstances; i++)
	{
		success = GetFullInstanceName(perfDataHead, object->CodePage, szName, errorMessage, instance);
		if (!success)
		{
			continue;
		}
		if (strlen(szName) <= inputNameLen)
		{
			if (std::string(szName) == instanceName)  // The name matches
			{
				occurrencesFound++;

				// If the input name does not specify an nth instance or
				// the nth instance has been found, return the instance.
				// It is 'dwSerialNo+1' because cmd#3 is the fourth occurrence.
				if (0 == serialNo || occurrencesFound == (serialNo + 1))
				{
					return instance;
				}
			}
		}

		counterBlock = (PERF_COUNTER_BLOCK*)((LPBYTE)instance + instance->ByteLength);
		instance =
			(PERF_INSTANCE_DEFINITION*)((LPBYTE)instance + instance->ByteLength + counterBlock->ByteLength);
	}

	errorMessage = instanceName + " object instance not found";
	return nullptr;
}

// Use the counter index to find the object in the performance data.
static PERF_COUNTER_DEFINITION *GetCounter(PERF_OBJECT_TYPE* object, DWORD counterToFind)
{
	DWORD numberOfCounters = object->NumCounters;
	bool foundCounter = false;

	auto* counter = PdhFirstCounter(object);
	for (DWORD i = 0; i < numberOfCounters; i++, counter = PdhNextCounter(counter))
	{
		if (counter->CounterNameTitleIndex == counterToFind)
		{
			foundCounter = true;
			break;
		}

		//counter++;
	}

	return (foundCounter) ? counter : nullptr;
}

static int GetCounter(PERF_OBJECT_TYPE* object, std::map<DWORD, PERF_COUNTER_DEFINITION*>& indexToCounterMap)
{
	DWORD numberOfCounters = object->NumCounters;
	int foundCounter = 0;

	auto* counter = PdhFirstCounter(object);
	for (DWORD i = 0; i < numberOfCounters; i++, counter = PdhNextCounter(counter))
	{
		if (indexToCounterMap.find(counter->CounterNameTitleIndex) != indexToCounterMap.end()
			&& indexToCounterMap[counter->CounterNameTitleIndex] == nullptr)
		{
			indexToCounterMap[counter->CounterNameTitleIndex] = counter;
			foundCounter++;
		}
		//counter++;
	}
	return foundCounter;
}

// Use the object index to find the object in the performance data.
PERF_OBJECT_TYPE* Win32SystemInformationCollector::GetObject(LPBYTE perfDataHead, LPCSTR objectToFind)
{
	LPBYTE pObject = perfDataHead + ((PERF_DATA_BLOCK*)perfDataHead)->HeaderLength;
	DWORD numberOfObjects = ((PERF_DATA_BLOCK*)perfDataHead)->NumObjectTypes;
	bool foundObject = false;

	for (DWORD i = 0; i < numberOfObjects; i++)
	{
		if (convert<unsigned long>(objectToFind) == ((PERF_OBJECT_TYPE*)pObject)->ObjectNameTitleIndex)
		{
			foundObject = true;
			break;
		}

		pObject += ((PERF_OBJECT_TYPE*)pObject)->TotalByteLength;
	}

	return (foundObject) ? (PERF_OBJECT_TYPE*)pObject : nullptr;
}

// Retrieve the raw counter value and any supporting data needed to calculate
// a displayable counter value. Use the counter type to determine the information
// needed to calculate the value.
bool Win32SystemInformationCollector::GetValue(LPBYTE perfDataHead,
	PERF_OBJECT_TYPE* object,
	PERF_COUNTER_DEFINITION* counter,
	PERF_COUNTER_BLOCK* counterDataBlock,
	RawData* rawData)
{
	PVOID data;
	UNALIGNED ULONGLONG* pullData;
	PERF_COUNTER_DEFINITION* baseCounter;
	bool success = true;

	//Point to the raw counter data.
	data = (PVOID)((LPBYTE)counterDataBlock + counter->CounterOffset);

	//Now use the PERF_COUNTER_DEFINITION.CounterType value to figure out what
	//other information you need to calculate a displayable value.
	switch (counter->CounterType)
	{

	case PERF_COUNTER_COUNTER:
	case PERF_COUNTER_QUEUELEN_TYPE:
	case PERF_SAMPLE_COUNTER:
		rawData->Data = (ULONGLONG)(*(DWORD*)data);
		rawData->Time = ((PERF_DATA_BLOCK*)perfDataHead)->PerfTime.QuadPart;
		if (PERF_COUNTER_COUNTER == counter->CounterType || PERF_SAMPLE_COUNTER == counter->CounterType)
		{
			rawData->Frequency = ((PERF_DATA_BLOCK*)perfDataHead)->PerfFreq.QuadPart;
		}
		break;

	case PERF_OBJ_TIME_TIMER:
		rawData->Data = (ULONGLONG)(*(DWORD*)data);
		rawData->Time = object->PerfTime.QuadPart;
		break;

	case PERF_COUNTER_100NS_QUEUELEN_TYPE:
		rawData->Data = *(UNALIGNED ULONGLONG*) data;
		rawData->Time = ((PERF_DATA_BLOCK*)perfDataHead)->PerfTime100nSec.QuadPart;
		break;

	case PERF_COUNTER_OBJ_TIME_QUEUELEN_TYPE:
		rawData->Data = *(UNALIGNED ULONGLONG*) data;
		rawData->Time = object->PerfTime.QuadPart;
		break;

	case PERF_COUNTER_TIMER:
	case PERF_COUNTER_TIMER_INV:
	case PERF_COUNTER_BULK_COUNT:
	case PERF_COUNTER_LARGE_QUEUELEN_TYPE:
		pullData = (UNALIGNED ULONGLONG*) data;
		rawData->Data = *pullData;
		rawData->Time = ((PERF_DATA_BLOCK*)perfDataHead)->PerfTime.QuadPart;
		if (counter->CounterType == PERF_COUNTER_BULK_COUNT)
		{
			rawData->Frequency = ((PERF_DATA_BLOCK*)perfDataHead)->PerfFreq.QuadPart;
		}
		break;

	case PERF_COUNTER_MULTI_TIMER:
	case PERF_COUNTER_MULTI_TIMER_INV:
		pullData = (UNALIGNED ULONGLONG*) data;
		rawData->Data = *pullData;
		rawData->Frequency = ((PERF_DATA_BLOCK*)perfDataHead)->PerfFreq.QuadPart;
		rawData->Time = ((PERF_DATA_BLOCK*)perfDataHead)->PerfTime.QuadPart;

		//These counter types have a second counter value that is adjacent to
		//this counter value in the counter data block. The value is needed for
		//the calculation.
		if ((counter->CounterType & PERF_MULTI_COUNTER) == PERF_MULTI_COUNTER)
		{
			++pullData;
			rawData->MultiCounterData = *(DWORD*)pullData;
		}
		break;

		//These counters do not use any time reference.
	case PERF_COUNTER_RAWCOUNT:
	case PERF_COUNTER_RAWCOUNT_HEX:
	case PERF_COUNTER_DELTA:
		rawData->Data = (ULONGLONG)(*(DWORD*)data);
		rawData->Time = 0;
		break;

	case PERF_COUNTER_LARGE_RAWCOUNT:
	case PERF_COUNTER_LARGE_RAWCOUNT_HEX:
	case PERF_COUNTER_LARGE_DELTA:
		rawData->Data = *(UNALIGNED ULONGLONG*) data;
		rawData->Time = 0;
		break;

		//These counters use the 100ns time base in their calculation.
	case PERF_100NSEC_TIMER:
	case PERF_100NSEC_TIMER_INV:
	case PERF_100NSEC_MULTI_TIMER:
	case PERF_100NSEC_MULTI_TIMER_INV:
		pullData = (UNALIGNED ULONGLONG*) data;
		rawData->Data = *pullData;
		rawData->Time = ((PERF_DATA_BLOCK*)perfDataHead)->PerfTime100nSec.QuadPart;

		//These counter types have a second counter value that is adjacent to
		//this counter value in the counter data block. The value is needed for
		//the calculation.
		if ((counter->CounterType & PERF_MULTI_COUNTER) == PERF_MULTI_COUNTER)
		{
			++pullData;
			rawData->MultiCounterData = *(DWORD*)pullData;
		}
		break;

		//These counters use two data points, this value and one from this counter's
		//base counter. The base counter should be the next counter in the object's
		//list of counters.
	case PERF_SAMPLE_FRACTION:
	case PERF_RAW_FRACTION:
		rawData->Data = (ULONGLONG)(*(DWORD*)data);
		baseCounter = counter + 1;  //Get base counter
		if ((baseCounter->CounterType & PERF_COUNTER_BASE) == PERF_COUNTER_BASE)
		{
			data = (PVOID)((LPBYTE)counterDataBlock + baseCounter->CounterOffset);
			rawData->Time = (LONGLONG)(*(DWORD*)data);
		}
		else
		{
			success = false;
		}
		break;

	case PERF_LARGE_RAW_FRACTION:
		rawData->Data = *(UNALIGNED ULONGLONG*) data;
		baseCounter = counter + 1;
		if ((baseCounter->CounterType & PERF_COUNTER_BASE) == PERF_COUNTER_BASE)
		{
			data = (PVOID)((LPBYTE)counterDataBlock + baseCounter->CounterOffset);
			rawData->Time = *(LONGLONG*)data;
		}
		else
		{
			success = false;
		}
		break;

	case PERF_PRECISION_SYSTEM_TIMER:
	case PERF_PRECISION_100NS_TIMER:
	case PERF_PRECISION_OBJECT_TIMER:
		rawData->Data = *(UNALIGNED ULONGLONG*) data;
		baseCounter = counter + 1;
		if ((baseCounter->CounterType & PERF_COUNTER_BASE) == PERF_COUNTER_BASE)
		{
			data = (PVOID)((LPBYTE)counterDataBlock + baseCounter->CounterOffset);
			rawData->Time = *(LONGLONG*)data;
		}
		else
		{
			success = false;
		}
		break;

	case PERF_AVERAGE_TIMER:
	case PERF_AVERAGE_BULK:
		rawData->Data = *(UNALIGNED ULONGLONG*) data;
		baseCounter = counter + 1;
		if ((baseCounter->CounterType & PERF_COUNTER_BASE) == PERF_COUNTER_BASE)
		{
			data = (PVOID)((LPBYTE)counterDataBlock + baseCounter->CounterOffset);
			rawData->Time = *(DWORD*)data;
		}
		else
		{
			success = false;
		}

		if (counter->CounterType == PERF_AVERAGE_TIMER)
		{
			rawData->Frequency = ((PERF_DATA_BLOCK*)perfDataHead)->PerfFreq.QuadPart;
		}
		break;

		//These are base counters and are used in calculations for other counters.
		//This case should never be entered.
	case PERF_SAMPLE_BASE:
	case PERF_AVERAGE_BASE:
	case PERF_COUNTER_MULTI_BASE:
	case PERF_RAW_BASE:
	case PERF_LARGE_RAW_BASE:
		rawData->Data = 0;
		rawData->Time = 0;
		success = false;
		break;

	case PERF_ELAPSED_TIME:
		rawData->Data = *(UNALIGNED ULONGLONG*) data;
		rawData->Time = object->PerfTime.QuadPart;
		rawData->Frequency = object->PerfFreq.QuadPart;
		break;

		//These counters are currently not supported.
	case PERF_COUNTER_TEXT:
	case PERF_COUNTER_NODATA:
	case PERF_COUNTER_HISTOGRAM_TYPE:
		rawData->Data = 0;
		rawData->Time = 0;
		success = false;
		break;

		//Encountered an unidentified counter.
	default:
		rawData->Data = 0;
		rawData->Time = 0;
		success = false;
		break;
	}

	return success;
}

// Use the object index to find the object in the performance data. Then, use the
// counter index to find the counter definition. The definition contains the offset
// to the counter data in the counter block. The location of the counter block
// depends on whether the counter is a single instance counter or multiple instance counter.
// After finding the counter block, retrieve the counter data.
bool Win32SystemInformationCollector::GetCounterValues(LPBYTE perfDataHead,
	LPCSTR objectIndex,
	const std::string& instanceName,
	RawData* rawData,
	std::string& errorMessage,
	DWORD counterIndex)
{
	PERF_OBJECT_TYPE* object;
	PERF_COUNTER_DEFINITION* counter;
	PERF_COUNTER_BLOCK* counterDataBlock;
	bool success = false;

	object = GetObject(perfDataHead, objectIndex);
	if (nullptr == object)
	{
		errorMessage = "Object  " + std::string(objectIndex) + " not found.";
		return success;
	}

	counter = GetCounter(object, counterIndex);
	if (nullptr == counter)
	{
		errorMessage = "Counter  " + std::to_string(counterIndex) + " not found.";
		return success;
	}

	// Retrieve a pointer to the beginning of the counter data block. The counter data
	// block contains all the counter data for the object.
	counterDataBlock = GetCounterBlock(perfDataHead, object, instanceName, errorMessage);
	if (nullptr == counterDataBlock)
	{
		errorMessage = "Instance  " + instanceName + " not found. " + errorMessage;
		return success;
	}

	*rawData = RawData{};
	rawData->CounterType = counter->CounterType;
	success = GetValue(perfDataHead, object, counter, counterDataBlock, rawData);

	return success;
}

int Win32SystemInformationCollector::PerfLibGetValue(RawData& rawData,
	LPCSTR counterKey,
	DWORD counterIndex,
	std::string instanceName,
	std::string& errorMessage)
{
	bool success = false;

	// Retrieve counter data for object.
	auto perfDataHead = GetPerformanceData(counterKey, errorMessage);
	if (!perfDataHead)
	{
		errorMessage = "GetPerformanceData failed : " + errorMessage;
		return SIC_EXECUTABLE_FAILED;
	}

	success = GetCounterValues(perfDataHead.get(), counterKey, instanceName, &rawData, errorMessage, counterIndex);

	// if (perfDataHead)
	// {
	//     free(perfDataHead);
	// }
	//
	if (!success)
	{
		errorMessage = "GetCounterValues failed. " + errorMessage;
		return SIC_EXECUTABLE_FAILED;
	}

	return SIC_EXECUTABLE_SUCCESS;
}

static const struct {
	DWORD perfKey;
	std::function<microseconds* (SicCpuInformation&)> fnValue;
}CpuPerfKeyToValues[]{
		{PERF_COUNTER_CPU_USER, [](SicCpuInformation& p) {return &p.user; }},
		{PERF_COUNTER_CPU_IDLE, [](SicCpuInformation& p) {return &p.idle; }},
		{PERF_COUNTER_CPU_SYS,  [](SicCpuInformation& p) {return &p.sys; }},
		{PERF_COUNTER_CPU_IRQ,  [](SicCpuInformation& p) {return &p.irq; }},
};
void initByCpuPerfKey(std::map<DWORD, PERF_COUNTER_DEFINITION*> &data) {
	for (const auto& it : CpuPerfKeyToValues) {
		data[it.perfKey] = nullptr;
	}
}
void initByCpuPerfKey(SicCpuInformation &cpu, std::map<DWORD, microseconds *> &data) {
	for (const auto& it : CpuPerfKeyToValues) {
		data[it.perfKey] = it.fnValue(cpu);
	}
}

int Win32SystemInformationCollector::PerfLibGetCpuInformation(SicCpuInformation& information, std::string& errorMessage)
{
	// LPBYTE perfDataHead = nullptr;
	// Retrieve counter data for the Processor object.
	auto perfDataHead = GetPerformanceData(PERF_CPU_COUNTER_KEY, errorMessage);
	if (!perfDataHead)
	{
		errorMessage = "GetPerformanceData failed : " + errorMessage;
		return SIC_EXECUTABLE_FAILED;
	}

	PERF_OBJECT_TYPE *object = GetObject(perfDataHead.get(), PERF_CPU_COUNTER_KEY);
	if (nullptr == object)
	{
		errorMessage = "Object  " + std::string(PERF_CPU_COUNTER_KEY) + " not found.";
		// free(perfDataHead);
		return SIC_EXECUTABLE_FAILED;
	}

	std::map<DWORD, PERF_COUNTER_DEFINITION*> indexToCounterMap;
	initByCpuPerfKey(indexToCounterMap);
	int foundCounter = GetCounter(object, indexToCounterMap);
	if (foundCounter < (int)indexToCounterMap.size())
	{
		errorMessage = makeErrorCounterNotFound(indexToCounterMap);
		// free(perfDataHead);
		return SIC_EXECUTABLE_FAILED;
	}

	PERF_COUNTER_BLOCK *counterDataBlock = GetCounterBlock(perfDataHead.get(), object, "_Total", errorMessage);
	if (nullptr == counterDataBlock)
	{
		errorMessage = "Instance _Total not found. " + errorMessage;
		// free(perfDataHead);
		return SIC_EXECUTABLE_FAILED;
	}

	information.reset();
	std::map<DWORD, microseconds *> indexToField;
	initByCpuPerfKey(information, indexToField);
	for (auto const& item : indexToCounterMap)
	{
		RawData rawData;
		rawData.CounterType = item.second->CounterType;
		int success = GetValue(perfDataHead.get(), object, item.second, counterDataBlock, &rawData);
		if (!success)
		{
			errorMessage = "GetCounterValues failed.";
			return success;
		}

		*indexToField[item.first] = Nano100ToMicroSeconds(rawData.Data);
		//switch (item.first)
		//{
		//case PERF_COUNTER_CPU_USER:
		//	target = &information.user;
		//	break;
		//case PERF_COUNTER_CPU_IDLE:
		//	information.idle = rawData.Data;
		//	break;
		//case PERF_COUNTER_CPU_SYS:
		//	information.sys = rawData.Data;
		//	break;
		//case PERF_COUNTER_CPU_IRQ:
		//	information.irq = rawData.Data;
		//	break;
		//}
	}
	//information.nice = 0;
	//information.wait = 0;
	//information.total = information.sys + information.user + information.idle + information.irq;
	// free(perfDataHead);
	return SIC_EXECUTABLE_SUCCESS;
}

int Win32SystemInformationCollector::SicGetCpuInformation(SicCpuInformation& information)
{
	int ret = -1;
	std::string& errorMessage{SicPtr()->errorMessage};
	//auto dllModule = mdllModuleMap.Get("ntdll.dll");

	//if (dllModule != nullptr && dllModule->getDllFunctions()["NtQuerySystemInformation"] != nullptr)
	if (win32API.NtQuerySystemInformation)
	{
		ret = NtSysGetCpuInformation(information, errorMessage);
	}
	else
	{
		ret = PerfLibGetCpuInformation(information, errorMessage);
	}

	return ret;
}

PERF_INSTANCE_DEFINITION* GetNextInstance(PERF_INSTANCE_DEFINITION* instance)
{
	auto* counterBlock = (PERF_COUNTER_BLOCK*)((LPBYTE)instance + instance->ByteLength);
	return (PERF_INSTANCE_DEFINITION*)((LPBYTE)instance + instance->ByteLength + counterBlock->ByteLength);
}

PERF_COUNTER_BLOCK* Win32SystemInformationCollector::GetCounterBlock(PERF_INSTANCE_DEFINITION* instance)
{
	return (PERF_COUNTER_BLOCK*)((LPBYTE)instance + instance->ByteLength);
}

int Win32SystemInformationCollector::PerfLibGetCpuListInformation(std::vector<SicCpuInformation>& informations,
	std::string& errorMessage)
{
	// LPBYTE perfDataHead = nullptr;
	// Retrieve counter data for the Processor object.
	auto perfDataHead = GetPerformanceData(PERF_CPU_COUNTER_KEY, errorMessage);
	if (!perfDataHead)
	{
		errorMessage = "GetPerformanceData failed : " + errorMessage;
		return SIC_EXECUTABLE_FAILED;
	}

	PERF_OBJECT_TYPE* object = GetObject(perfDataHead.get(), PERF_CPU_COUNTER_KEY);
	if (nullptr == object)
	{
		errorMessage = "Object  " + std::string(PERF_CPU_COUNTER_KEY) + " not found.";
		// free(perfDataHead);
		return SIC_EXECUTABLE_FAILED;
	}

	std::map<DWORD, PERF_COUNTER_DEFINITION*> indexToCounterMap;
	initByCpuPerfKey(indexToCounterMap);
	int foundCounter = GetCounter(object, indexToCounterMap);
	if (foundCounter < (int)indexToCounterMap.size())
	{
		errorMessage = makeErrorCounterNotFound(indexToCounterMap);
		// free(perfDataHead);
		return SIC_EXECUTABLE_FAILED;
	}

	// seek for the first instance and counterDataBlock
	auto* instance = (PERF_INSTANCE_DEFINITION*)((LPBYTE)object + object->DefinitionLength);;
	PERF_COUNTER_BLOCK *counterDataBlock = GetCounterBlock(instance);

	if (nullptr == counterDataBlock)
	{
		errorMessage = "Instance _Total not found. " + errorMessage;
		// free(perfDataHead);
		return SIC_EXECUTABLE_FAILED;
	}

	for (int index = 0; index < object->NumInstances - 1; ++index)
	{
		SicCpuInformation sicCpuInformation = {};
		std::map<DWORD, microseconds*> fieldMap;
		initByCpuPerfKey(sicCpuInformation, fieldMap);
		for (auto const& item : indexToCounterMap)
		{
			RawData rawData;
			rawData.CounterType = item.second->CounterType;
			int success = GetValue(perfDataHead.get(), object, item.second, counterDataBlock, &rawData);
			if (!success)
			{
				errorMessage = "GetCounterValues failed.";
				return success;
			}
			
			*fieldMap[item.first] = Nano100ToMicroSeconds(rawData.Data);
			//switch (item.first)
			//{
			//case PERF_COUNTER_CPU_USER:sicCpuInformation.user = rawData.Data;
			//	break;
			//case PERF_COUNTER_CPU_IDLE:sicCpuInformation.idle = rawData.Data;
			//	break;
			//case PERF_COUNTER_CPU_SYS:sicCpuInformation.sys = rawData.Data;
			//	break;
			//case PERF_COUNTER_CPU_IRQ:sicCpuInformation.irq = rawData.Data;
			//	break;
			//}
		}
		//sicCpuInformation.nice = 0;
		//sicCpuInformation.wait = 0;
		//sicCpuInformation.total =
		//	sicCpuInformation.sys + sicCpuInformation.user + sicCpuInformation.idle + sicCpuInformation.irq;
		informations.push_back(sicCpuInformation);

		instance = GetNextInstance(instance);
		counterDataBlock = GetCounterBlock(instance);
	}

	// free(perfDataHead);
	return SIC_EXECUTABLE_SUCCESS;
}

int Win32SystemInformationCollector::SicGetCpuListInformation(std::vector<SicCpuInformation>& informations)
{
	int ret = -1;
	std::string errorMessage;
	//auto dllModule = mdllModuleMap.Get("ntdll.dll");
	//if (dllModule != nullptr && dllModule->getDllFunctions()["NtQuerySystemInformation"] != nullptr)
	if (win32API.NtQuerySystemInformation)
	{
		ret = NtSysGetCpuListInformation(informations, errorMessage);
	}
	else
	{
		ret = PerfLibGetCpuListInformation(informations, errorMessage);
	}

	if (ret != SIC_EXECUTABLE_SUCCESS)
	{
		SicPtr()->SetErrorMessage(errorMessage);
	}

	return ret;
}

int Win32SystemInformationCollector::KernelGetMemoryInformation(SicMemoryInformation& information,
	std::string& errorMessage)
{
	//auto dllModule = mdllModuleMap.Get("kernel32.dll");
	//std::unordered_map<std::string, FARPROC> &dllFunctions = dllModule->getDllFunctions();
	//if (dllFunctions["GlobalMemoryStatusEx"] != nullptr)
	RETURN_IF_NULL(SIC_EXECUTABLE_FAILED, win32API.GlobalMemoryStatusEx);

	MEMORYSTATUSEX memoryStatusEx;
	memoryStatusEx.dwLength = sizeof(memoryStatusEx);
	BOOL res = win32API.GlobalMemoryStatusEx(&memoryStatusEx);

	if (!res)
	{
		return LastError("GlobalMemoryStatusEx", &errorMessage);
	}

	information.total = memoryStatusEx.ullTotalPhys;
	information.free = memoryStatusEx.ullAvailPhys;
	return SIC_EXECUTABLE_SUCCESS;
}

int Win32SystemInformationCollector::PerfLibGetMemoryInformation(SicMemoryInformation* memoryInformation,
	SicSwapInformation* swapInformation,
	std::string& errorMessage)
{

	// LPBYTE perfDataHead = nullptr;
	// Retrieve counter data for the Processor object.
	auto perfDataHead = GetPerformanceData(PERF_MEM_COUNTER_KEY, errorMessage);
	if (!perfDataHead)
	{
		errorMessage = "GetPerformanceData failed : " + errorMessage;
		return -1;
	}

	PERF_OBJECT_TYPE* object;
	PERF_COUNTER_BLOCK* counterDataBlock;
	int success = -1;

	object = GetObject(perfDataHead.get(), PERF_MEM_COUNTER_KEY);
	if (nullptr == object)
	{
		errorMessage = "Object  " + std::string(PERF_MEM_COUNTER_KEY) + " not found.";
		// free(perfDataHead);
		return success;
	}

	std::map<DWORD, PERF_COUNTER_DEFINITION*> indexToCounterMap = {
		{PERF_COUNTER_MEM_SYS_CACHE, nullptr},
		{PERF_COUNTER_MEM_PAGES_OUT, nullptr},
		{PERF_COUNTER_MEM_PAGES_IN, nullptr}
	};

	int foundCounter = GetCounter(object, indexToCounterMap);
	if (foundCounter < 0 || foundCounter < (int)indexToCounterMap.size())
	{
		errorMessage = makeErrorCounterNotFound(indexToCounterMap);
		// free(perfDataHead);
		return SIC_EXECUTABLE_FAILED;
	}

	counterDataBlock = GetCounterBlock(perfDataHead.get(), object, "", errorMessage);
	if (nullptr == counterDataBlock)
	{
		errorMessage = "Instance _Total not found. " + errorMessage;
		// free(perfDataHead);
		return success;
	}

	for (auto const& item : indexToCounterMap)
	{
		RawData rawData;
		rawData.CounterType = item.second->CounterType;
		success = GetValue(perfDataHead.get(), object, item.second, counterDataBlock, &rawData);
		if (!success)
		{
			errorMessage = "GetCounterValues failed.";
			return success;
		}

		switch (item.first)
		{
		case PERF_COUNTER_MEM_SYS_CACHE:
			if (memoryInformation != nullptr)
			{
				unsigned long long cache = rawData.Data;
				memoryInformation->actualFree = memoryInformation->free + cache;
				memoryInformation->actualUsed = Diff(memoryInformation->used, cache);
			}
			break;
		case PERF_COUNTER_MEM_PAGES_OUT:
			if (swapInformation != nullptr)
			{
				swapInformation->pageOut = rawData.Data;
			}
			break;
		case PERF_COUNTER_MEM_PAGES_IN:
			if (swapInformation != nullptr)
			{
				swapInformation->pageIn = rawData.Data;
			}
			break;
		}
	}
	// free(perfDataHead);
	return SIC_EXECUTABLE_SUCCESS;
}

int Win32SystemInformationCollector::GetWindowsMemoryRam(SicMemoryInformation& info)
{
	int ram = static_cast<int>(info.total / (1024 * 1024));
	int remainder = ram % 8;
	if (remainder > 0)
	{
		ram += (8 - remainder);
	}

	info.ram = ram;
	info.usedPercent = static_cast<double>(Diff(info.total, info.actualFree)) / static_cast<double>(info.total) * 100.0;
	info.freePercent = static_cast<double>(Diff(info.total, info.actualUsed)) / static_cast<double>(info.total) * 100.0;

	return SIC_EXECUTABLE_SUCCESS;
}

int Win32SystemInformationCollector::SicGetMemoryInformation(SicMemoryInformation& information)
{
	auto& errorMessage = SicPtr()->errorMessage;
	//auto dllModule = mdllModuleMap.Get("kernel32.dll");
	//if (dllModule != nullptr && dllModule->getDllFunctions()["GlobalMemoryStatusEx"] != nullptr)
	RETURN_IF_NULL(SIC_EXECUTABLE_FAILED, win32API.GlobalMemoryStatusEx);

	int ret = KernelGetMemoryInformation(information, errorMessage);
	if (ret != SIC_EXECUTABLE_SUCCESS)
	{
		return ret;
	}
	information.used = Diff(information.total, information.free);
	information.actualUsed = information.used;
	information.actualFree = information.free;
	ret = PerfLibGetMemoryInformation(&information, nullptr, errorMessage);
	if (ret != SIC_EXECUTABLE_SUCCESS)
	{
		return SIC_EXECUTABLE_FAILED;
	}

	GetWindowsMemoryRam(information);

	return SIC_EXECUTABLE_SUCCESS;
}

// PerfLibGetUpTime在有的机器上耗时500+毫秒
class UpSeconds {
    friend class Win32SystemInformationCollector;
    double seconds = std::numeric_limits<double>::quiet_NaN();
    mutable std::recursive_mutex mutex;
public:
    double upSeconds() const {
        std::lock_guard<std::recursive_mutex> guard(mutex);
        return seconds;
    }
};
static UpSeconds upSeconds;

int Win32SystemInformationCollector::SicGetUpTime(double& uptime)
{
	//if (SIC_EXECUTABLE_SUCCESS != PerfLibGetUpTime(uptime, SicPtr()->errorMessage)) {
	//	uptime = static_cast<double>(GetTickCount64()) / 1000;
	//}
	//return SIC_EXECUTABLE_SUCCESS;

	using namespace std::chrono;
	double seconds = upSeconds.upSeconds();
	if (std::isnan(seconds)) {
		std::lock_guard<std::recursive_mutex> guard(upSeconds.mutex);
		if (std::isnan(upSeconds.seconds)) {
			double upTime = 0.0;
			if (PerfLibGetUpTime(upTime, SicPtr()->errorMessage) == SIC_EXECUTABLE_SUCCESS) { // 系统启动到现在的时间(s)
				int64_t millis = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
				upSeconds.seconds = double(millis) / 1000 - upTime; // 系统启动时间
				LogInfo("millis: {} ms, upTime: {:.3f} ss, seocnds: {:.3f}", millis, upTime, upSeconds.seconds);
			}
		}
		seconds = upSeconds.seconds;
	}

	if (!std::isnan(seconds)) {
		int64_t millis = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
		uptime = double(millis) / 1000 - seconds;
	}
	else {
		uptime = static_cast<double>(GetTickCount64()) / 1000;
	}
	return SIC_EXECUTABLE_SUCCESS;
}

int Win32SystemInformationCollector::SicGetLoadInformation(SicLoadInformation& information)
{
	SicPtr()->SetErrorMessage("This os does not support the interface");
	return SIC_NOT_IMPLEMENT;
}

int Win32SystemInformationCollector::KernelGetSwapInformation(SicSwapInformation& information, std::string& errorMessage)
{
	//auto dllModule = mdllModuleMap.Get("kernel32.dll");
	//std::unordered_map<std::string, FARPROC> &dllFunctions = dllModule->getDllFunctions();
	//if (dllFunctions["GlobalMemoryStatusEx"] != nullptr)
	RETURN_IF_NULL(SIC_EXECUTABLE_FAILED, win32API.GlobalMemoryStatusEx);

	MEMORYSTATUSEX memoryStatusEx;
	memoryStatusEx.dwLength = sizeof(memoryStatusEx);
	// call NtQuerySystemInformation function
	bool res = win32API.GlobalMemoryStatusEx(&memoryStatusEx);

	if (!res)
	{
		return LastError("GlobalMemoryStatusEx", &errorMessage);
	}

	information.total = memoryStatusEx.ullTotalPageFile;
	information.free = memoryStatusEx.ullAvailPageFile;
	return SIC_EXECUTABLE_SUCCESS;
}

int Win32SystemInformationCollector::SicGetSwapInformation(SicSwapInformation& information)
{
	std::string& errorMessage{SicPtr()->errorMessage};
	//auto dllModule = mdllModuleMap.Get("kernel32.dll");
	//if (dllModule != nullptr && dllModule->getDllFunctions()["GlobalMemoryStatusEx"] != nullptr)
	RETURN_IF_NULL(SIC_EXECUTABLE_FAILED, win32API.GlobalMemoryStatusEx);

	int ret = KernelGetSwapInformation(information, errorMessage);
	if (ret == SIC_EXECUTABLE_SUCCESS)
	{
		information.used = Diff(information.total, information.free);
		ret = PerfLibGetMemoryInformation(nullptr, &information, errorMessage);
	}
	return ret;
}

int Win32SystemInformationCollector::IphlpapiGetInterfaceTable(PMIB_IFTABLE& ifTable, std::string& errorMessage)
{
	//std::unordered_map<std::string, FARPROC> &dllFunctions = dllModule->getDllFunctions();
	//if (dllFunctions["GetIfTable"] != nullptr)
	RETURN_IF_NULL(SIC_EXECUTABLE_FAILED, win32API.GetIfTable);

	//PMIB_IFTABLE ifConfigBuffer = (PMIB_IFTABLE)malloc(SicPtr()->ifConfigLength);
	FAIL_GUARD(Free(ifTable));

	ULONG size = SicPtr()->ifConfigLength;
	ifTable = (PMIB_IFTABLE)malloc(SicPtr()->ifConfigLength);
	DWORD res = win32API.GetIfTable(ifTable, &size, FALSE);
	if (res == ERROR_INSUFFICIENT_BUFFER)
	{
		SicPtr()->ifConfigLength = size;
		void* pTemp = realloc(ifTable, SicPtr()->ifConfigLength);
		if (pTemp) {
			ifTable = static_cast<PMIB_IFTABLE>(pTemp);
			DWORD res2 = win32API.GetIfTable(ifTable, &size, FALSE);
			if (res2 != SIC_EXECUTABLE_SUCCESS) {
				errorMessage = "call GetIfTable failed !";
				return res;
			}
		}
		else
		{
			errorMessage = "Reallocation error.";
			//free(ifConfigBuffer);
			//ifConfigBuffer = nullptr;
			return SIC_EXECUTABLE_FAILED;
		}
	}

	failed = false;
	return SIC_EXECUTABLE_SUCCESS;
}

int Win32SystemInformationCollector::SicGetInterfaceConfigList(SicInterfaceConfigList& interfaceConfigList)
{
	std::string errorMessage;

	PIP_ADAPTER_ADDRESSES addresses = nullptr;
	MIB_IFTABLE* ifTable = nullptr;
	defer(Free(ifTable); Free(addresses));

	int res = SIC_EXECUTABLE_FAILED;
	if (win32API.GetAdaptersAddresses) {
		ULONG size = 0;
		res = win32API.IphlpapiGetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, addresses, size, errorMessage);
	}
	if (res == SIC_EXECUTABLE_SUCCESS && win32API.GetIfTable) {
        // FIXME: hancj.2024-04-02 这里查询网卡时不排序，有可能会导致对网卡自动命名时的张冠李戴
		res = IphlpapiGetInterfaceTable(ifTable, errorMessage);
	}
	if (res != SIC_EXECUTABLE_SUCCESS) {
		return res;
	}

	int loopBackAdapterCount = 0;
	int loopBackCount = 0;
	int ethCount = 0;

    interfaceConfigList.names.clear();
	for (DWORD i = 0; i < ifTable->dwNumEntries; ++i)
	{
		std::string name;
		MIB_IFROW ifRow = *(ifTable->table + i);

		int res = -1;

		if (strcmp(reinterpret_cast<char const*>(ifRow.bDescr), MS_LOOPBACK_ADAPTER) == 0)
		{
			name = LOOPBACK_ADAPTER + std::to_string(loopBackAdapterCount++);
		}
		else if (ifRow.dwType == MIB_IF_TYPE_LOOPBACK)
		{
			if (!SicPtr()->useNetInterfaceShortName)
			{
				res = IphlpapiGetInterfaceName(ifRow, addresses, name, errorMessage);
			}
			if (res != SIC_EXECUTABLE_SUCCESS)
			{
				name = LOOPBACK + std::to_string(loopBackCount++);
			}
		}

		else if (ifRow.dwType == MIB_IF_TYPE_ETHERNET || ifRow.dwType == IF_TYPE_IEEE80211)
		{
			if (!SicPtr()->useNetInterfaceShortName
				&& strstr(reinterpret_cast<char* const>(ifRow.bDescr), "Scheduler") == nullptr
				&& strstr(reinterpret_cast<char* const>(ifRow.bDescr), "Filter") == nullptr)
			{
				res = IphlpapiGetInterfaceName(ifRow, addresses, name, errorMessage);
			}

			if (res != SIC_EXECUTABLE_SUCCESS)
			{
				if (SicPtr()->useNetInterfaceShortName)
				{
					name = "eth" + std::to_string(ethCount++);
				}
				else
				{
					name = reinterpret_cast<char const* const>(ifRow.bDescr);
				}
			}
		}
		else
		{
			continue;
		}

		interfaceConfigList.names.insert(name);

		SicPtr()->netInterfaceCache[name] = ifRow;
		SicPtr()->netInterfaceIndexToName[ifRow.dwIndex] = name;
	}

	return 0;
}

int Win32SystemInformationCollector::IphlpapiGetIfEntry(MIB_IFROW* ifRow, std::string& errorMessage)
{
	RETURN_IF_NULL(SIC_EXECUTABLE_FAILED, win32API.GetIfEntry);

	DWORD res = win32API.GetIfEntry(ifRow);
	if (res != NO_ERROR) {
		errorMessage = "call GetIfEntry failed";
		return res;
	}
	return SIC_EXECUTABLE_SUCCESS;
}

int Win32SystemInformationCollector::IphlpapiGetIfRow(MIB_IFROW*& ifRow, const std::string& name, std::string& errorMessage)
{
	int res;
	if (SicPtr()->netInterfaceCache.empty()) {
		// cached
		SicInterfaceConfigList interfaceConfigList;
		res = SicGetInterfaceConfigList(interfaceConfigList);
		if (res != SIC_EXECUTABLE_SUCCESS)
		{
			errorMessage = "call SicGetInterfaceConfigList failed";
			return res;
		}
	}

	if (SicPtr()->netInterfaceCache.find(name) == SicPtr()->netInterfaceCache.end())
	{
		errorMessage = "interface : " + name + " not found !";
		return SIC_EXECUTABLE_FAILED;
	}
	else
	{
		ifRow = &(SicPtr()->netInterfaceCache[name]);
	}

	if (!(SicPtr()->netInterfaceCache).empty())
	{
		// update ifRow
		res = IphlpapiGetIfEntry(ifRow, errorMessage);
		if (res != NO_ERROR)
		{
			return res;
		}
	}
	return SIC_EXECUTABLE_SUCCESS;
}

int Win32SystemInformationCollector::SicGetInterfaceInformation(const std::string& name,
	SicNetInterfaceInformation& information)
{
	MIB_IFROW* ifRow = nullptr;
	int res;
	std::string errorMessage;

	res = IphlpapiGetIfRow(ifRow, name, errorMessage);
	if (res != NO_ERROR)
	{
		SicPtr()->SetErrorMessage(errorMessage);
		return res;
	}

	information.rxPackets = ifRow->dwInUcastPkts + ifRow->dwInNUcastPkts;
	information.rxBytes = ifRow->dwInOctets;
	information.rxErrors = ifRow->dwInErrors;
	information.rxDropped = ifRow->dwInDiscards;
	information.rxOverruns = -1;
	information.rxFrame = -1;
	information.txPackets = ifRow->dwOutUcastPkts + ifRow->dwOutNUcastPkts;
	information.txBytes = ifRow->dwOutOctets;
	information.txErrors = ifRow->dwOutErrors;
	information.txDropped = ifRow->dwOutDiscards;
	information.txOverruns = -1;
	information.txCollisions = -1;
	information.txCarrier = -1;
	information.speed = ifRow->dwSpeed;

	return SIC_EXECUTABLE_SUCCESS;
}

int Win32SystemInformationCollector::IphlpapiGetIpAddrTable(PMIB_IPADDRTABLE& ipAddrTable, std::string& errorMessage)
{
	RETURN_IF_NULL(SIC_EXECUTABLE_FAILED, win32API.GetIpAddrTable);

	FAIL_GUARD(Free(ipAddrTable));

	ULONG size = SicPtr()->ifConfigLength;
	ipAddrTable = (PMIB_IPADDRTABLE)malloc(size);
	DWORD res = win32API.GetIpAddrTable(ipAddrTable, &size, FALSE);
	if (res == ERROR_INSUFFICIENT_BUFFER)
	{
		SicPtr()->ifConfigLength = size;
		PMIB_IPADDRTABLE pTemp = (PMIB_IPADDRTABLE)realloc(ipAddrTable, size);
		if (pTemp) {
			ipAddrTable = pTemp;
			res = win32API.GetIpAddrTable(ipAddrTable, &size, FALSE);
			if (res != SIC_EXECUTABLE_SUCCESS) {
				errorMessage = "call GetIpAddrTable failed !";
				return res;
			}
		}
		else
		{
			errorMessage = "Reallocation error.";
			//free(ifConfigBuffer);
			//ifConfigBuffer = nullptr;
			return SIC_EXECUTABLE_FAILED;
		}
	}
	failed = false;
	return SIC_EXECUTABLE_SUCCESS;
}

int Win32SystemInformationCollector::IphlpapiGetIfIpAddrRow(DWORD index, MIB_IPADDRROW &ipAddr, std::string& errorMessage)
{
	if (SicPtr()->netIpAddrCache.empty()) {
		MIB_IPADDRTABLE* ipAddrTable = nullptr;
		defer(Free(ipAddrTable));

		int res = IphlpapiGetIpAddrTable(ipAddrTable, errorMessage);
		if (res != SIC_EXECUTABLE_SUCCESS) {
			SicPtr()->SetErrorMessage(errorMessage);
			return res;
		}

		for (DWORD i = 0; i < ipAddrTable->dwNumEntries; ++i)
		{
			MIB_IPADDRROW* ipAddrRow = &ipAddrTable->table[i];
			if (!(ipAddrRow->wType & MIB_IPADDR_PRIMARY)) {
				continue;
			}

			SicPtr()->netIpAddrCache[ipAddrRow->dwIndex] = *ipAddrRow;
		}
	}

	auto target = SicPtr()->netIpAddrCache.find(index);
	if (target != SicPtr()->netIpAddrCache.end()) {
		ipAddr = target->second;
		return SIC_EXECUTABLE_SUCCESS;
	} else {
		errorMessage = "get " + std::to_string(index) + " ipAddr failed";
		return SIC_EXECUTABLE_FAILED;
	}
}

int Win32SystemInformationCollector::IphlpapiGetIfIpv6Config(DWORD index, SicInterfaceConfig& interfaceConfig, std::string& errorMessage)
{
	int res;
	interfaceConfig.address6.family = SicNetAddress::SIC_AF_INET6;
	interfaceConfig.prefix6Length = 0;
	interfaceConfig.scope6 = 0;

	ULONG ifConfigLength = SicPtr()->ifConfigLength;
	PIP_ADAPTER_ADDRESSES addresses = (ifConfigLength > 0? (PIP_ADAPTER_ADDRESSES)malloc(ifConfigLength): nullptr);
	defer(Free(addresses));

	const ULONG flags = GAA_FLAG_INCLUDE_PREFIX;
	res = win32API.IphlpapiGetAdaptersAddresses(AF_UNSPEC, flags, addresses, ifConfigLength, errorMessage);
	if (res != SIC_EXECUTABLE_SUCCESS) {
		return res;
	}
	SicPtr()->ifConfigLength = ifConfigLength;

	int ret = SIC_EXECUTABLE_FAILED;

	for (PIP_ADAPTER_ADDRESSES iter = addresses; iter != nullptr; iter = iter->Next)
	{
		if (iter->IfIndex != index) {
			continue;
		}

		for (auto addr = iter->FirstUnicastAddress; addr != nullptr; addr = addr->Next)
		{
			const sockaddr* sa = addr->Address.lpSockaddr;
			if (sa->sa_family == AF_INET6)
			{
				const in6_addr* in6Addr = &((sockaddr_in6*)sa)->sin6_addr;
				memcpy(&((interfaceConfig.address6).addr.in6), in6Addr, sizeof((interfaceConfig.address6).addr.in6));
				interfaceConfig.address6.family = SicNetAddress::SIC_AF_INET6;

				int &scope = interfaceConfig.scope6;
				if (IN6_IS_ADDR_LINKLOCAL(in6Addr)) {
					scope = SIC_IPV6_ADDR_LINKLOCAL;
				}
				else if (IN6_IS_ADDR_SITELOCAL(in6Addr)) {
					scope = SIC_IPV6_ADDR_SITELOCAL;
				}
				else if (IN6_IS_ADDR_V4COMPAT(in6Addr)) {
					scope = SIC_IPV6_ADDR_COMPATv4;
				}
				else if (IN6_IS_ADDR_LOOPBACK(in6Addr)) {
					scope = SIC_IPV6_ADDR_LOOPBACK;
				}
				else {
					scope = SIC_IPV6_ADDR_ANY;
				}

				if (iter->FirstPrefix) {
					interfaceConfig.prefix6Length = iter->FirstPrefix->PrefixLength;
				}
				ret = SIC_EXECUTABLE_SUCCESS;
				break;
			}
		}
	}

	return ret;
}

int Win32SystemInformationCollector::SicGetInterfaceConfig(SicInterfaceConfig& interfaceConfig, const std::string& name)
{
	MIB_IFROW* ifRow = nullptr;
	std::string errorMessage;

	int res = IphlpapiGetIfRow(ifRow, name, errorMessage);
	if (res != NO_ERROR) {
		SicPtr()->SetErrorMessage(errorMessage);
		return res;
	}

	interfaceConfig.name = name;
	interfaceConfig.mtu = ifRow->dwMtu;

	memcpy((interfaceConfig.hardWareAddr.addr.mac), ifRow->bPhysAddr, 6);
	interfaceConfig.hardWareAddr.family = SicNetAddress::SIC_AF_LINK;
	interfaceConfig.description = reinterpret_cast<char const* const>(ifRow->bDescr);

	MIB_IPADDRROW ipAddrRow;
	res = IphlpapiGetIfIpAddrRow(ifRow->dwIndex, ipAddrRow, errorMessage);
	if (res == SIC_EXECUTABLE_SUCCESS)
	{
		interfaceConfig.address.addr.in = ipAddrRow.dwAddr;
		interfaceConfig.address.family = SicNetAddress::SIC_AF_INET;
		interfaceConfig.netmask.addr.in = ipAddrRow.dwMask;
		interfaceConfig.netmask.family = SicNetAddress::SIC_AF_INET;

		if (ifRow->dwType != MIB_IF_TYPE_LOOPBACK)
		{
			if (ipAddrRow.dwBCastAddr)
			{
				long bCast = ipAddrRow.dwAddr & ipAddrRow.dwMask;
				bCast |= ~ipAddrRow.dwMask;
				interfaceConfig.broadcast.addr.in = bCast;
				interfaceConfig.broadcast.family = SicNetAddress::SIC_AF_INET;
			}
		}
	}

	// MS_LOOPBACK_ADAPTER
	if (strncmp(name.c_str(), LOOPBACK_ADAPTER, sizeof(LOOPBACK_ADAPTER) - 1) == 0)
	{
		ifRow->dwType = MIB_IF_TYPE_LOOPBACK;
	}

	if (ifRow->dwType == MIB_IF_TYPE_LOOPBACK)
	{
		interfaceConfig.type = SIC_NIC_LOOPBACK;
	}
	else
	{
		interfaceConfig.type = SIC_NIC_ETHERNET;
	}

	IphlpapiGetIfIpv6Config(ifRow->dwIndex, interfaceConfig, errorMessage);

	return SIC_EXECUTABLE_SUCCESS;
}

int Win32SystemInformationCollector::IphlpapiGetTcpTable(/*out*/MIB_TCPTABLE*& tcpTable, std::string& errorMessage)
{
	//DWORD res;
	//auto dllModule = mdllModuleMap.Get("iphlpapi.dll");
	FAIL_GUARD(Free(tcpTable));

	tcpTable = nullptr;
	//std::unordered_map<std::string, FARPROC> &dllFunctions = dllModule->getDllFunctions();
	//if (dllFunctions["GetTcpTable"] != nullptr)
	RETURN_IF_NULL(SIC_EXECUTABLE_FAILED, win32API.GetTcpTable);

	ULONG size = 0;
	DWORD res = win32API.GetTcpTable(tcpTable, &size, FALSE);
	if (res == ERROR_INSUFFICIENT_BUFFER)
	{
		auto pTemp = (PMIB_TCPTABLE)realloc(tcpTable, size);
		if (pTemp)
		{
			tcpTable = pTemp;
			res = win32API.GetTcpTable(tcpTable, &size, FALSE);
			if (res != SIC_EXECUTABLE_SUCCESS)
			{
				errorMessage = "call GetTcpTable failed !";
				return res;
			}
		}
		else
		{
			errorMessage = "Reallocation error.";
			return SIC_EXECUTABLE_FAILED;
		}
	}
	else
	{
		errorMessage = "call GetTcpTable failed";
		return GetLastError();
	}

	failed = false;
	return SIC_EXECUTABLE_SUCCESS;
}

int Win32SystemInformationCollector::IphlpapiGetUdpTable(MIB_UDPTABLE*& udpTable, std::string& errorMessage)
{
	FAIL_GUARD(Free(udpTable));

	RETURN_IF_NULL(SIC_EXECUTABLE_FAILED, win32API.GetUdpTable);

	ULONG size = 0;
	udpTable = nullptr;
	DWORD res = win32API.GetUdpTable(udpTable, &size, FALSE);
	if (res == ERROR_INSUFFICIENT_BUFFER) {
		udpTable = (PMIB_UDPTABLE)malloc(size);
		if (udpTable)
		{
			res = win32API.GetUdpTable(udpTable, &size, FALSE);
			if (res != SIC_EXECUTABLE_SUCCESS)
			{
				errorMessage = "call GetUdpTable failed !";
				return res;
			}
		}
		else
		{
			errorMessage = "Reallocation error.";
			//free(ifConfigBuffer);
			//ifConfigBuffer = nullptr;
			return SIC_EXECUTABLE_FAILED;
		}
	}
	else
	{
		errorMessage = "call GetUdpTable failed";
		return GetLastError();
	}

	failed = false;
	return SIC_EXECUTABLE_SUCCESS;
}


int Win32SystemInformationCollector::SicGetNetState(SicNetState& netState,
	bool useClient,
	bool useServer,
	bool useUdp,
	bool useTcp,
	bool useUnix,
	int option)
{
	int res = -1;
	std::string errorMessage;
	if (useTcp)
	{
		MIB_TCPTABLE* tcpTable = nullptr;
		defer(Free(tcpTable));

		res = IphlpapiGetTcpTable(tcpTable, errorMessage);
		if (res != SIC_EXECUTABLE_SUCCESS)
		{
			SicPtr()->SetErrorMessage(errorMessage);
			return res;
		}
		for (int i = tcpTable->dwNumEntries - 1; i >= 0; --i)
		{
			SicNetConnectionInformation connectionInformation;
			DWORD state = tcpTable->table[i].dwState;

			if (!((useServer && state == MIB_TCP_STATE_LISTEN) || (useClient && state != MIB_TCP_STATE_LISTEN)))
			{
				continue;
			}

			connectionInformation.localPort = htons((WORD)tcpTable->table[i].dwLocalPort);
			connectionInformation.remotePort = htons((WORD)tcpTable->table[i].dwRemotePort);
			connectionInformation.type = SIC_NET_CONN_TCP;

			connectionInformation.localAddress.addr.in = tcpTable->table[i].dwLocalAddr;
			connectionInformation.localAddress.family = SicNetAddress::SIC_AF_INET;

			connectionInformation.remoteAddress.addr.in = tcpTable->table[i].dwRemoteAddr;
			connectionInformation.remoteAddress.family = SicNetAddress::SIC_AF_INET;

			connectionInformation.sendQueue = connectionInformation.receiveQueue = -1;
			switch (state)
			{
			case MIB_TCP_STATE_CLOSED:connectionInformation.state = SIC_TCP_CLOSE;
				break;
			case MIB_TCP_STATE_LISTEN:connectionInformation.state = SIC_TCP_LISTEN;
				break;
			case MIB_TCP_STATE_SYN_SENT:connectionInformation.state = SIC_TCP_SYN_SENT;
				break;
			case MIB_TCP_STATE_SYN_RCVD:connectionInformation.state = SIC_TCP_SYN_RECV;
				break;
			case MIB_TCP_STATE_ESTAB:connectionInformation.state = SIC_TCP_ESTABLISHED;
				break;
			case MIB_TCP_STATE_FIN_WAIT1:connectionInformation.state = SIC_TCP_FIN_WAIT1;
				break;
			case MIB_TCP_STATE_FIN_WAIT2:connectionInformation.state = SIC_TCP_FIN_WAIT2;
				break;
			case MIB_TCP_STATE_CLOSE_WAIT:connectionInformation.state = SIC_TCP_CLOSE_WAIT;
				break;
			case MIB_TCP_STATE_CLOSING:connectionInformation.state = SIC_TCP_CLOSING;
				break;
			case MIB_TCP_STATE_LAST_ACK:connectionInformation.state = SIC_TCP_LAST_ACK;
				break;
			case MIB_TCP_STATE_TIME_WAIT:connectionInformation.state = SIC_TCP_TIME_WAIT;
				break;
			case MIB_TCP_STATE_DELETE_TCB:
			default:connectionInformation.state = SIC_TCP_UNKNOWN;
				break;
			}

			if (connectionInformation.type == SIC_NET_CONN_TCP)
			{
				netState.tcpStates[connectionInformation.state]++;
				if (connectionInformation.state == SIC_TCP_LISTEN)
				{
					SicPtr()->addNetListen(connectionInformation);
				}
				else
				{
					if (SicPtr()->netListenCache.find(connectionInformation.localPort) != SicPtr()->netListenCache.end())
					{
						netState.tcpInboundTotal++;
					}
					else
					{
						netState.tcpOutboundTotal++;
					}
				}
			}

			netState.allInboundTotal = netState.tcpInboundTotal;
			netState.allOutboundTotal = netState.tcpOutboundTotal;
		}

		//free(tcpTable);
	}

	if (useUdp)
	{
		MIB_UDPTABLE* udpTable = nullptr;
		defer(Free(udpTable));

		res = IphlpapiGetUdpTable(udpTable, errorMessage);
		if (res != SIC_EXECUTABLE_SUCCESS)
		{
			SicPtr()->SetErrorMessage(errorMessage);
			return res;
		}
		for (DWORD i = 0; i < udpTable->dwNumEntries; ++i)
		{
			SicNetConnectionInformation connectionInformation;
			if (!(useServer || useClient))
			{
				continue;
			}

			connectionInformation.localPort = htons((WORD)udpTable->table[i].dwLocalPort);
			connectionInformation.remotePort = 0;

			connectionInformation.type = SIC_NET_CONN_UDP;

			connectionInformation.localAddress.addr.in = udpTable->table[i].dwLocalAddr;
			connectionInformation.localAddress.family = SicNetAddress::SIC_AF_INET;

			connectionInformation.sendQueue = connectionInformation.receiveQueue = -1;

			if (connectionInformation.type == SIC_NET_CONN_UDP)
			{
				netState.udpSession++;
			}

		}
		//free(udpTable);
	}

	//for (int i = 0; i < sizeof(netState.tcpStates) / sizeof(netState.tcpStates[0]); i++) {
	//	if (netState.tcpStates[i] >= 0) {
	//		netState.tcpStates[i]++; // 因为计数是从-1起的，因此此处再加1
	//	}
	//}
    netState.calcTcpTotalAndNonEstablished();
	// netState.tcpStates[SIC_TCP_TOTAL] = 0;
	// for (int i = SIC_TCP_ESTABLISHED; i <= SIC_TCP_UNKNOWN; ++i) {
	// 	netState.tcpStates[SIC_TCP_TOTAL] += netState.tcpStates[i];
	// }
	// netState.tcpStates[SIC_TCP_NON_ESTABLISHED] = netState.tcpStates[SIC_TCP_TOTAL] - netState.tcpStates[SIC_TCP_ESTABLISHED];

	return SIC_EXECUTABLE_SUCCESS;
}

int Win32SystemInformationCollector::SicGetFileSystemListInformation(std::vector<SicFileSystem>& informations)
{
	// A pointer to a buffer that receives a series of null-terminated strings,
	// one for each valid drive in the system, plus with an additional null character.
	// Each string is a device name.
	// LIKE : C://\0A://\0
	char logicalDriverName[256];
	DWORD length = GetLogicalDriveStringsA(sizeof(logicalDriverName), logicalDriverName);

	if (length == 0)
	{
		return GetLastError();
	}

	char* ptr = logicalDriverName;

	while (*ptr)
	{
		SicFileSystem fileSystem;
		UINT driveType = GetDriveType(ptr);

		switch (driveType)
		{
		case DRIVE_FIXED:
			fileSystem.type = SIC_FILE_SYSTEM_TYPE_LOCAL_DISK;
			break;
		case DRIVE_REMOTE:
			fileSystem.type = SIC_FILE_SYSTEM_TYPE_NETWORK;
			break;
		case DRIVE_CDROM:
			fileSystem.type = SIC_FILE_SYSTEM_TYPE_CDROM;
			break;
		case DRIVE_RAMDISK:
			fileSystem.type = SIC_FILE_SYSTEM_TYPE_RAM_DISK;
			break;
		case DRIVE_REMOVABLE:
			// skip floppy, usb, etc. drives
			ptr += strlen(ptr) + 1;
			continue;
		default:
			fileSystem.type = SIC_FILE_SYSTEM_TYPE_NONE;
			break;
		}

		char fileSystemName[1024] = {0};
		DWORD serialNumber = 0;
		DWORD fileSystemFlags = 0;
		GetVolumeInformation(ptr,
			nullptr,
			0,
			&serialNumber,
			nullptr,
			&fileSystemFlags,
			fileSystemName,
			sizeof(fileSystemName));

		if (!serialNumber && (driveType == DRIVE_FIXED))
		{
			ptr += strlen(ptr) + 1;
			// skip unformatted partitions
			continue;
		}

		fileSystem.dirName = ptr;
		fileSystem.devName = ptr;

		//auto dllModule = mdllModuleMap.Get("mrp.dll");
		//if (driveType == DRIVE_REMOTE && dllModule != nullptr
		//    && dllModule->getDllFunctions()["WNetGetConnection"] != nullptr)
		if (driveType == DRIVE_REMOTE && win32API.WNetGetConnection)
		{
			char remoteName[SIC_MAX_PATH_LENGTH];
			length = sizeof(remoteName);
			char drive[3] = { '\0', ':', '\0' };
			drive[0] = fileSystem.dirName.front();
			int ret = win32API.WNetGetConnection(drive, remoteName, &length);
			if (ret == NO_ERROR) {
				fileSystem.devName = remoteName;
			}
		}

		SicGetFileSystemType(fileSystem.sysTypeName, fileSystem.type, fileSystem.typeName);

		if (*fileSystemName == '\0')
		{
			fileSystem.sysTypeName = fileSystem.typeName;
		}
		else
		{
			// like cdfs, ntfs
			fileSystem.sysTypeName = fileSystemName;
		}

		if (fileSystemFlags & FILE_READ_ONLY_VOLUME)
		{
			fileSystem.options = "ro";
		}
		else
		{
			fileSystem.options = "rw";
		}

		informations.push_back(fileSystem);
		ptr += strlen(ptr) + 1;
	}

	return 0;
}

int Win32SystemInformationCollector::PdhGetDiskUsage(SicDiskUsage& usage, const std::string& dirName, std::string& errorMessage)
{
	if (mDiskCounter.find(dirName) == mDiskCounter.end()) {
		PDH_STATUS status = PdhDiskAddCounter(dirName, errorMessage);
		if (ERROR_SUCCESS != status) {
			return SIC_EXECUTABLE_FAILED;
		}
	}

	PDH_STATUS status = PdhCollectQueryData(SicPtr()->diskCounterQuery);
	if (ERROR_SUCCESS != status) {
		errorMessage += PdhErrorStr(status, "PdhCollectQueryData");
		return SIC_EXECUTABLE_FAILED;
	}

	auto &targetDisk = mDiskCounter[dirName];
	for (auto& it : diskCounterKeyIndex) {
		PDH_FMT_COUNTERVALUE pdhValue;
		DWORD dwValue = 0;

		status = PdhGetFormattedCounterValue(it.value(*targetDisk), PDH_FMT_DOUBLE, &dwValue, &pdhValue);
		if (ERROR_SUCCESS != status) {
			errorMessage += PdhErrorStr(status, fmt::format("PdhGetFormattedCounterValue({}, {})", dirName, it.desc));
			return SIC_EXECUTABLE_FAILED;
		}
		else {
			it.value(usage) = pdhValue.doubleValue;
		}
	}
	return SIC_EXECUTABLE_SUCCESS;
}

int Win32SystemInformationCollector::PerfLibGetDiskUsage(SicDiskUsage& usage,
	std::string& dirName,
	std::string& errorMessage)
{
	// LPBYTE perfDataHead = nullptr;
	// Retrieve counter data for the Processor object.
	auto perfDataHead = GetPerformanceData(PERF_DISK_COUNTER_KEY, errorMessage);
	if (!perfDataHead)
	{
		errorMessage = "GetPerformanceData failed : " + errorMessage;
		return -1;
	}

	PERF_OBJECT_TYPE* object;
	PERF_COUNTER_BLOCK* counterDataBlock;
	int success = -1;

	object = GetObject(perfDataHead.get(), PERF_DISK_COUNTER_KEY);
	if (nullptr == object)
	{
		errorMessage = "Object  " + std::string(PERF_DISK_COUNTER_KEY) + " not found.";
		// free(perfDataHead);
		return success;
	}

	std::map<DWORD, PERF_COUNTER_DEFINITION*> indexToCounterMap = {
		{PERF_COUNTER_DISK_TIME, nullptr},
		{PERF_COUNTER_DISK_READ_TIME, nullptr},
		{PERF_COUNTER_DISK_WRITE_TIME, nullptr},
		{PERF_COUNTER_DISK_READ, nullptr},
		{PERF_COUNTER_DISK_WRITE, nullptr},
		{PERF_COUNTER_DISK_READ_BYTES, nullptr},
		{PERF_COUNTER_DISK_WRITE_BYTES, nullptr},
		{PERF_COUNTER_DISK_IDLE_TIME, nullptr}
	};

	int foundCounter = GetCounter(object, indexToCounterMap);
	if (foundCounter < 0 || foundCounter < static_cast<int>(indexToCounterMap.size()))
	{
		errorMessage = makeErrorCounterNotFound(indexToCounterMap);
		// free(perfDataHead);
		return SIC_EXECUTABLE_FAILED;
	}

	// seek for the first instance and counterDataBlock
	auto* instance = (PERF_INSTANCE_DEFINITION*)((LPBYTE)object + object->DefinitionLength);;
	counterDataBlock = GetCounterBlock(instance);

	if (nullptr == counterDataBlock)
	{
		errorMessage = "Instance _Total not found. " + errorMessage;
		// free(perfDataHead);
		return success;
	}

	for (int index = 0; index < object->NumInstances - 1; ++index)
	{
		auto* wCharDirName = (wchar_t*)((BYTE*)instance + instance->NameOffset);
		std::string currentDirName = WcharToChar(wCharDirName, CP_UTF8);
		if (currentDirName == dirName)
		{
			for (auto const& item : indexToCounterMap)
			{
				RawData rawData;
				rawData.CounterType = item.second->CounterType;
				success = GetValue(perfDataHead.get(), object, item.second, counterDataBlock, &rawData);
				if (!success)
				{
					errorMessage = "GetCounterValues failed.";
					return success;
				}

				bool hit = true;
				switch (item.first)
				{
				case PERF_COUNTER_DISK_TIME:
					usage.time = static_cast<double>(rawData.Data);
					break;
				case PERF_COUNTER_DISK_IDLE_TIME:
					usage.idle = static_cast<double>(rawData.Data);
					break;
				case PERF_COUNTER_DISK_READ_TIME:
					usage.rTime = static_cast<double>(rawData.Data);
					break;
				case PERF_COUNTER_DISK_WRITE_TIME:
					usage.wTime = static_cast<double>(rawData.Data);
					break;
				case PERF_COUNTER_DISK_READ:
					usage.reads = static_cast<double>(rawData.Data);
					usage.frequency = rawData.Frequency;
					break;
				case PERF_COUNTER_DISK_WRITE:
					usage.writes = static_cast<double>(rawData.Data);
					break;
				case PERF_COUNTER_DISK_READ_BYTES:
					usage.readBytes = static_cast<double>(rawData.Data);
					break;
				case PERF_COUNTER_DISK_WRITE_BYTES:
					usage.writeBytes = static_cast<double>(rawData.Data);
					break;
				case PERF_COUNTER_DISK_QUEUE:
					usage.queue = static_cast<double>(rawData.Data);
					break;
				default:
					hit = false;
				}
				if (hit) {
					usage.tick = rawData.Time;
				}
			}
			// free(perfDataHead);
			return SIC_EXECUTABLE_SUCCESS;
		}
		instance = GetNextInstance(instance);
		counterDataBlock = GetCounterBlock(instance);
	}
	// free(perfDataHead);
	return SIC_EXECUTABLE_SUCCESS;
}

int Win32SystemInformationCollector::SicGetDiskUsage(SicDiskUsage& diskUsage, std::string dirName)
{
    diskUsage.dirName = diskUsage.devName = dirName;
    SicPtr()->errorMessage.clear();
	return PdhGetDiskUsage(diskUsage, dirName, SicPtr()->errorMessage);
}

double Win32SystemInformationCollector::CalculateFileSystemUsage(SicFileSystemUsage& usage)
{
	uint64_t usedInMB = Diff(usage.total, usage.free) / 1024;
	uint64_t availInMB = usage.avail / 1024;
	uint64_t totalInMB = usedInMB + availInMB;
	usedInMB = (unsigned long)usedInMB;
	if (totalInMB != 0)
	{
		uint64_t used100 = usedInMB * 100;
		double percent = static_cast<double>(used100 / totalInMB + ((used100 % totalInMB != 0) ? 1 : 0));
		return percent / 100;
	}

	return 0;
}

int Win32SystemInformationCollector::SicGetFileSystemUsage(SicFileSystemUsage& fileSystemUsage, std::string dirName)
{
	int res;
	ULARGE_INTEGER availBytes, totalBytes, freeBytes;

	// prevent critical-error-handler message box when drive is empty
	UINT errMode = SetErrorMode(SEM_FAILCRITICALERRORS);

	res = GetDiskFreeSpaceEx(dirName.c_str(), &availBytes, &totalBytes, &freeBytes);

	SetErrorMode(errMode);

	if (!res)
	{
		return GetLastError();
	}

	fileSystemUsage.total = totalBytes.QuadPart / 1024;
	fileSystemUsage.free = freeBytes.QuadPart / 1024;
	fileSystemUsage.avail = availBytes.QuadPart / 1024;
	fileSystemUsage.used = Diff(fileSystemUsage.total, fileSystemUsage.free);
	fileSystemUsage.use_percent = CalculateFileSystemUsage(fileSystemUsage);

	fileSystemUsage.files = -1;
	fileSystemUsage.freeFiles = -1;

	if (dirName.size() > 2)
	{
		SicGetDiskUsage(fileSystemUsage.disk, dirName.substr(0, 2));
	}

	return SIC_EXECUTABLE_SUCCESS;
}

int Win32SystemInformationCollector::PsapiEnumProcesses(SicProcessList& processList, std::string& errorMessage, size_t step)
{
	RETURN_IF_NULL(SIC_EXECUTABLE_FAILED, win32API.EnumProcesses);

	DWORD neededSize = 0;
	DWORD bufferSize = 0;
	std::vector<DWORD> buffer;

	do
	{
		buffer.resize(buffer.size() + step);
		bufferSize = static_cast<decltype(bufferSize)>(buffer.size() * sizeof(buffer[0]));
		neededSize = 0;

		// There is no indication given when the buffer is too small to store all process identifiers.
		// Therefore, if lpcbNeeded equals cb, consider retrying the call with a larger array.
		if (!win32API.EnumProcesses(&buffer[0], bufferSize, &neededSize)) {
			return LastError("EnumProcesses", &errorMessage);
		}
	} while (neededSize >= bufferSize);

	LPDWORD pids = &buffer[0];
	LPDWORD pidsEnd = pids + (neededSize / sizeof(pids[0]));
	processList.pids.reserve(pidsEnd - pids);
	for (LPDWORD pid = pids; pid < pidsEnd; ++pid) {
		// exclude system idle process
		if (*pid != 0) {
			processList.pids.push_back(*pid);
		}
	}
	SicWMI::UpdateProcesses(buffer);
	return SIC_EXECUTABLE_SUCCESS;
}

int Win32SystemInformationCollector::SicGetProcessList(SicProcessList& processList, size_t limit, bool &overflow)
{
	std::string& errorMessage{ SicPtr()->errorMessage };

	RETURN_IF_NULL(SIC_EXECUTABLE_FAILED, win32API.EnumProcesses);

	int ret = PsapiEnumProcesses(processList, errorMessage);
    overflow = (ret == SIC_EXECUTABLE_SUCCESS && limit != 0 && processList.pids.size() > limit);
    return ret;
}

int Win32SystemInformationCollector::PerfLibGetProcessInfo(pid_t pid, std::string& errorMessage)
{
	SicWin32ProcessInfo& win32ProcessInfo = SicPtr()->win32ProcessInfo;
	auto timeNow = std::chrono::steady_clock::now();
	// 对重复查询一个pid做一些处理：2s之内查一次
	if (win32ProcessInfo.pid == pid && timeNow < win32ProcessInfo.expire)
	{
		return SIC_EXECUTABLE_SUCCESS;
	}

	// LPBYTE perfDataHead = nullptr;
	// Retrieve counter data for the Processor object.
	auto perfDataHead = GetPerformanceData(PERF_PROC_COUNTER_KEY, errorMessage);
	if (!perfDataHead)
	{
		errorMessage = "GetPerformanceData failed : " + errorMessage;
		return -1;
	}

	PERF_OBJECT_TYPE* object;
	PERF_COUNTER_BLOCK* counterDataBlock;
	int success = -1;

	object = GetObject(perfDataHead.get(), PERF_PROC_COUNTER_KEY);
	if (nullptr == object)
	{
		errorMessage = "Object  " + std::string(PERF_PROC_COUNTER_KEY) + " not found.";
		// free(perfDataHead);
		return success;
	}

	std::map<DWORD, PERF_COUNTER_DEFINITION*> indexToCounterMap = {
		{PERF_COUNTER_PROC_CPU_TIME, nullptr},
		{PERF_COUNTER_PROC_PAGE_FAULTS, nullptr},
		{PERF_COUNTER_PROC_MEM_VIRTUAL_SIZE, nullptr},
		{PERF_COUNTER_PROC_PRIVATE_SIZE, nullptr},
		{PERF_COUNTER_PROC_MEM_SIZE, nullptr},
		{PERF_COUNTER_PROC_THREAD_COUNT, nullptr},
		{PERF_COUNTER_PROC_HANDLE_COUNT, nullptr},
		{PERF_COUNTER_PROC_PID, nullptr},
		{PERF_COUNTER_PROC_PARENT_PID, nullptr},
		{PERF_COUNTER_PROC_PRIORITY, nullptr},
		{PERF_COUNTER_PROC_START_TIME, nullptr},
		{PERF_COUNTER_PROC_IO_READ_BYTES_SEC, nullptr},
		{PERF_COUNTER_PROC_IO_WRITE_BYTES_SEC, nullptr}
	};

	int foundCounter = GetCounter(object, indexToCounterMap);
	if (foundCounter < 0 || foundCounter < (int)indexToCounterMap.size())
	{
		errorMessage = makeErrorCounterNotFound(indexToCounterMap);
		// free(perfDataHead);
		return SIC_EXECUTABLE_FAILED;
	}

	win32ProcessInfo.pid = pid;
	win32ProcessInfo.expire = timeNow + SicPtr()->lastProcExpire;

	// seek for the first instance and counterDataBlock
	auto* instance = (PERF_INSTANCE_DEFINITION*)((LPBYTE)object + object->DefinitionLength);
	counterDataBlock = GetCounterBlock(instance);

	if (nullptr == counterDataBlock)
	{
		errorMessage = "Instance not found. " + errorMessage;
		// free(perfDataHead);
		return success;
	}

	for (int index = 0; index < object->NumInstances; ++index)
	{
		RawData rawData1;
		rawData1.CounterType = indexToCounterMap[PERF_COUNTER_PROC_PID]->CounterType;
		success = GetValue(perfDataHead.get(), object, indexToCounterMap[PERF_COUNTER_PROC_PID], counterDataBlock, &rawData1);
		unsigned long long currentPid = rawData1.Data;

		if (!success)
		{
			errorMessage = "GetCounterValues failed.";
			return success;
		}

		if (currentPid == pid)
		{
			auto* wCharProcessName = (wchar_t*)((BYTE*)instance + instance->NameOffset);
			std::string processName = WcharToChar(wCharProcessName, CP_UTF8);
			win32ProcessInfo.name = processName;
			win32ProcessInfo.state = SIC_PROC_STATE_RUN;
			unsigned long long memoryPrivateSize = 0;
			for (auto const& item : indexToCounterMap)
			{
				RawData rawData;
				rawData.CounterType = item.second->CounterType;
				success = GetValue(perfDataHead.get(), object, item.second, counterDataBlock, &rawData);
				if (!success)
				{
					errorMessage = "GetCounterValues failed.";
					return success;
				}

				switch (item.first)
				{
				case PERF_COUNTER_PROC_PAGE_FAULTS:win32ProcessInfo.pageFaults = rawData.Data;
					win32ProcessInfo.tick = rawData.Time;
					win32ProcessInfo.frequency = rawData.Frequency;
					break;
				case PERF_COUNTER_PROC_MEM_VIRTUAL_SIZE:win32ProcessInfo.size = rawData.Data;
					break;
				case PERF_COUNTER_PROC_PRIVATE_SIZE:memoryPrivateSize = rawData.Data;
					break;
				case PERF_COUNTER_PROC_MEM_SIZE:win32ProcessInfo.resident = rawData.Data;
					break;
				case PERF_COUNTER_PROC_THREAD_COUNT:win32ProcessInfo.threads = rawData.Data;
					break;
				case PERF_COUNTER_PROC_HANDLE_COUNT:win32ProcessInfo.handles = rawData.Data;
					break;
				case PERF_COUNTER_PROC_PID:win32ProcessInfo.pid = rawData.Data;
					break;
				case PERF_COUNTER_PROC_PARENT_PID:win32ProcessInfo.parentPid = static_cast<int>(rawData.Data);
					break;
				case PERF_COUNTER_PROC_PRIORITY:win32ProcessInfo.priority = static_cast<int>(rawData.Data);
					break;
				case PERF_COUNTER_PROC_IO_READ_BYTES_SEC:win32ProcessInfo.bytesRead = rawData.Data;
					win32ProcessInfo.tick = rawData.Time;
					win32ProcessInfo.frequency = rawData.Frequency;
					break;
				case PERF_COUNTER_PROC_IO_WRITE_BYTES_SEC: win32ProcessInfo.bytesWritten = rawData.Data;
					win32ProcessInfo.tick = rawData.Time;
					win32ProcessInfo.frequency = rawData.Frequency;
					break;
				}
			}
			if (memoryPrivateSize == 0)
			{
				win32ProcessInfo.share = -1;
			}
			else
			{
				win32ProcessInfo.share = Diff(win32ProcessInfo.resident, memoryPrivateSize);
			}

			// free(perfDataHead);
			return SIC_EXECUTABLE_SUCCESS;
		}

		instance = GetNextInstance(instance);
		counterDataBlock = GetCounterBlock(instance);
	}
	// free(perfDataHead);
	return SIC_EXECUTABLE_FAILED;
}

int Win32SystemInformationCollector::SicGetProcessState(pid_t pid, SicProcessState& processState)
{
	int ret = PerfLibGetProcessInfo(pid, SicPtr()->errorMessage);
	if (ret == SIC_EXECUTABLE_SUCCESS) {
        const SicWin32ProcessInfo &processInfo = SicPtr()->win32ProcessInfo;

        processState.name = processInfo.name;
        processState.state = processInfo.state;
        processState.parentPid = processInfo.parentPid;
        processState.priority = processInfo.priority;
        processState.nice = -1;
        processState.tty = -1;
        processState.threads = processInfo.threads;
        processState.processor = -1;
    }
	return ret;
}

inline uint64_t FileTimeToTimeInMillisecond(const FILETIME& filetime)
{
	uint64_t time = ((uint64_t)(filetime.dwHighDateTime) << 32) | filetime.dwLowDateTime; // 单位：纳秒
	return time / WINDOWS_TICK;   // 变为毫秒
}

inline int64_t FileTimeToUnixTimeInMillisecond(const FILETIME& filetime)
{
	// FILETIME中的时间是自1601-01-01 00:00:00 (UTC)的100纳秒数
	// 而UnixTime，则是1970-01-01 00:00:00 (UTC)起计数。
	return static_cast<int64_t>(FileTimeToTimeInMillisecond(filetime) - Milli_SEC_TO_UNIX_EPOCH);
}

template<typename T>
int GetTimesImpl(BOOL(NTAPI* fnGet)(HANDLE, LPFILETIME, LPFILETIME, LPFILETIME, LPFILETIME), const char* fnName, HANDLE handle, T& output, std::chrono::system_clock::time_point* startTime, std::string& errorMessage) {
	FILETIME start, exit, sys, user;
	BOOL ok = fnGet(handle, &start, &exit, &sys, &user);
	if (!ok) {
		return LastError(fnName, &errorMessage);
	}

	if (startTime != nullptr) {
		*startTime = system_clock::time_point{ milliseconds{FileTimeToUnixTimeInMillisecond(start)} };
	}

	output.sys = milliseconds{ FileTimeToTimeInMillisecond(sys) };
	output.user = milliseconds{ FileTimeToTimeInMillisecond(user) };
	output.total = output.sys + output.user;

	return SIC_EXECUTABLE_SUCCESS;
}
#define GetTimes(F, H, O, ST, EM) GetTimesImpl(F, #F ## "()", H, O, ST, EM)

int Win32SystemInformationCollector::SicGetThreadCpuInformation(pid_t, int tid, SicThreadCpu& threadCpu) {
	int thisThreadId = GetThisThreadId();
	HANDLE hThread = (thisThreadId == tid ? GetCurrentThread() : OpenThread(THREAD_QUERY_INFORMATION, FALSE, tid));
	defer(if (thisThreadId != tid)CloseHandle(hThread));

	return GetTimes(GetThreadTimes, hThread, threadCpu, nullptr, SicPtr()->errorMessage);
}

int Win32SystemInformationCollector::SicGetProcessTime(pid_t pid, SicProcessTime& processTime, bool)
{
	HANDLE processHandle = win32API.OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, (DWORD)pid);
	if (processHandle == nullptr)
	{
        return LastError((sout{} << "OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, " << pid << ")").str(), &SicPtr()->errorMessage);
	}
	defer(CloseHandle(processHandle)); // unique_handle _guard{ processHandle };

	return GetTimes(GetProcessTimes, processHandle, processTime, &processTime.startTime, SicPtr()->errorMessage);
	//int res = SIC_EXECUTABLE_SUCCESS;

	//FILETIME startTime, exitTime, systemTime, userTime;
	//if (GetProcessTimes(processHandle, &startTime, &exitTime, &systemTime, &userTime) == 0)
	//{
 //       return LastError((sout{} << "GetProcessTimes(pid: " << pid << ")").str(), &SicPtr()->errorMessage);
	//}

	//if (res == SIC_EXECUTABLE_SUCCESS)
	//{
	//	processTime.startTime = (startTime.dwHighDateTime != 0 ? FileTimeToUnixTimeInMillisecond(startTime) : 0);

	//	processTime.user = FileTimeToTimeInMillisecond(userTime);
	//	processTime.sys = FileTimeToTimeInMillisecond(systemTime);
	//	processTime.total = processTime.user + processTime.sys;
	//}
	//return res;
}

// int Win32SystemInformationCollector::SicGetProcessCpuInformation(pid_t pid, SicProcessCpuInformation& information)
// {
//     int64_t timeNow = NowMillis();
// 	unsigned long long otime;
// 	unsigned long long timeDiff, totalDiff;
// 	int res;
//
// 	auto& prev = SicGetProcessCpuInCache(pid);
//
// 	timeDiff = timeNow - prev.lastTime;
//
// 	if (timeDiff == 0)
// 	{
// 		information = prev;
// 		return SIC_EXECUTABLE_SUCCESS;
// 	}
//
//     // prev.lastTime = timeNow;
//     information.lastTime = timeNow;
//
//     otime = prev.total;
//
// 	SicProcessTime processTime{};
// 	res = SicGetProcessTime(pid, processTime);
// 	if (res != SIC_EXECUTABLE_SUCCESS)
// 	{
// 		return res;
// 	}
//
// 	information.startTime = processTime.startTime;
// 	information.user = processTime.user;
// 	information.sys = processTime.sys;
// 	information.total = processTime.total;
//
// 	prev = information; // update cache
//
// 	if (information.total < otime)
// 	{
// 		// this should not happen
// 		otime = 0;
// 	}
//
// 	if (otime == 0)
// 	{
// 		// first time called
// 		information.percent = 0.0;
// 		return SIC_EXECUTABLE_SUCCESS;
// 	}
//
// 	totalDiff = processTime.total - otime;
// 	information.percent = totalDiff / (double)timeDiff;
//
// 	return SIC_EXECUTABLE_SUCCESS;
// }

int Win32SystemInformationCollector::SicGetProcessMemoryInformation(pid_t pid,
	SicProcessMemoryInformation& information)
{
	int res = PerfLibGetProcessInfo(pid, SicPtr()->errorMessage);
	if (res != SIC_EXECUTABLE_SUCCESS)
	{
		return res;
	}

	const SicWin32ProcessInfo& processInfo = SicPtr()->win32ProcessInfo;
	information.size = processInfo.size;
	information.resident = processInfo.resident;
	information.share = processInfo.share;
	information.pageFaults = processInfo.pageFaults;
	information.minorFaults = -1;
	information.majorFaults = -1;

	information.tick = processInfo.tick;
	information.frequency = processInfo.frequency;

	return SIC_EXECUTABLE_SUCCESS;
}

int Win32SystemInformationCollector::ProcessBasicInformationGet(HANDLE process, PEB& peb, std::string& errorMessage)
{
	int res;
	PROCESS_BASIC_INFORMATION processBasicInformation{};
	DWORD size = sizeof(processBasicInformation);

	RETURN_IF_NULL(SIC_EXECUTABLE_FAILED, win32API.NtQueryInformationProcess);

	res = win32API.NtQueryInformationProcess(process, ProcessBasicInformation, &processBasicInformation, size, nullptr);
	if (res != 0) {
		errorMessage = ErrorStr(res, "NtQueryInformationProcess");
		return res;
	}

	if (processBasicInformation.PebBaseAddress == nullptr) {
		return ERROR_DATATYPE_MISMATCH; /* likely we are 32-bit, pid process is 64-bit */
	}

	size = sizeof(peb);

	if (win32API.ReadProcessMemory(process, processBasicInformation.PebBaseAddress, &peb, size, nullptr)) {
		return SIC_EXECUTABLE_SUCCESS;
	}
	else {
		return LastError("ReadProcessMemmory", &errorMessage);
	}
}

int Win32SystemInformationCollector::SicProcessArgsWmiGet(pid_t pid, SicProcessArgs& processArgs)
{
	//WCHAR buffer[SIC_CMDLINE_MAX];
	//{
	//	SicWMI wmi;
	//	if (!wmi.Open() || !wmi.ProcessCommandLineGet(pid, buffer)) {
	//		return wmi.GetLastError();
	//	}
	//}
	
	bstr_t cmdLine;
	int ret = SicWMI::GetProcessArgs(pid, cmdLine);
	return ret != 0? ret : ParseProcessArgs(cmdLine, processArgs);
}

int Win32SystemInformationCollector::ParseProcessArgs(const WCHAR* buffer, SicProcessArgs& processArgs)
{
	if (buffer == nullptr)
	{
		buffer = GetCommandLineW();
	}

	int argsNum = 0;

	// @depend(Windows 2000 Professional, Windows XP)
	LPWSTR* args = CommandLineToArgvW(buffer, &argsNum);

	if (args == nullptr)
	{
		// args -> ""
		return SIC_EXECUTABLE_SUCCESS;
	}

	for (int i = 0; i < argsNum; ++i)
	{
		std::string currentArg = WcharToChar(args[i], CP_UTF8);
		processArgs.args.push_back(currentArg);
	}

	GlobalFree(args);

	return SIC_EXECUTABLE_SUCCESS;
}

int Win32SystemInformationCollector::RtlProcessParametersGet(HANDLE process,
	RTL_USER_PROCESS_PARAMETERS& rtl, std::string& errorMessage)
{
	PEB peb;

	int res = ProcessBasicInformationGet(process, peb, errorMessage);
	if (res != SIC_EXECUTABLE_SUCCESS)
	{
		return res;
	}

	DWORD size = sizeof(rtl);

	if (win32API.ReadProcessMemory(process, peb.ProcessParameters, &rtl, size, nullptr))
	{
		return SIC_EXECUTABLE_SUCCESS;
	}
	else
	{
		return LastError("ReadProcessMemory", &errorMessage);
	}
}

int Win32SystemInformationCollector::initProcessParameter(
        HANDLE process, pid_t pid, std::string &errorMessage, RTL_USER_PROCESS_PARAMETERS &processParameters) {
    auto now = std::chrono::steady_clock::now();
    if (mProcessParameterCache.pid == pid && mProcessParameterCache.expire <= now)
    {
        processParameters = mProcessParameterCache.processParameters;
    }
    else
    {
        int res = RtlProcessParametersGet(process, processParameters, errorMessage);
        if (res != SIC_EXECUTABLE_SUCCESS)
        {
            return res;
        }
        mProcessParameterCache.pid = pid;
        mProcessParameterCache.expire = now + 2_s;
        mProcessParameterCache.processParameters = processParameters;
    }
    return SIC_SUCCESS;
}

int Win32SystemInformationCollector::PebGetProcessExe(HANDLE process,
	SicProcessExe& processExe, std::string& errorMessage, pid_t pid)
{
	RTL_USER_PROCESS_PARAMETERS processParameters;
    int res = initProcessParameter(process, pid, errorMessage, processParameters);
    if (res == SIC_SUCCESS) {
        WCHAR buffer[SIC_CMDLINE_MAX];
        DWORD size = ((sizeof(buffer) < processParameters.ImagePathName.Length) ? sizeof(buffer)
            : processParameters.ImagePathName.Length);
        memset(buffer, '\0', sizeof(buffer));
        if ((size > 0) && win32API.ReadProcessMemory(process, processParameters.ImagePathName.Buffer, buffer, size, nullptr))
        {
            processExe.name = WcharToChar(buffer, CP_UTF8);
        }

        size = ((sizeof(buffer) < processParameters.CurrentDirectoryName.Length) ? sizeof(buffer)
            : processParameters.CurrentDirectoryName.Length);
        memset(buffer, '\0', sizeof(buffer));
        if ((size > 0) && win32API.ReadProcessMemory(process, processParameters.CurrentDirectoryName.Buffer, buffer, size, nullptr))
        {
            processExe.cwd = WcharToChar(buffer, CP_UTF8);
        }
    }
	return res;
}

int Win32SystemInformationCollector::PebGetProcessArgs(
        HANDLE process, SicProcessArgs& processArgs, std::string& errorMessage, pid_t pid)
{
	// int res;
	// WCHAR buffer[SIC_CMDLINE_MAX];
	// RTL_USER_PROCESS_PARAMETERS processParameters;
	// DWORD size;
    //
	// if (mProcessParameterCache.pid == pid && mProcessParameterCache.time - time(nullptr) <= 2)
	// {
	// 	processParameters = mProcessParameterCache.processParameters;
	// }
	// else
	// {
	// 	res = RtlProcessParametersGet(process, processParameters, errorMessage);
	// 	if (res != SIC_EXECUTABLE_SUCCESS)
	// 	{
	// 		return res;
	// 	}
	// 	mProcessParameterCache.pid = pid;
	// 	mProcessParameterCache.time = time(nullptr);
	// 	mProcessParameterCache.processParameters = processParameters;
	// }
    RTL_USER_PROCESS_PARAMETERS processParameters;
    int res = initProcessParameter(process, pid, errorMessage, processParameters);
    if (res == SIC_SUCCESS) {
        WCHAR buffer[SIC_CMDLINE_MAX];
        DWORD size = ((sizeof(buffer) < processParameters.CommandLine.Length) ? sizeof(buffer)
            : processParameters.CommandLine.Length);
        if (size <= 0)
        {
            // change to use wmi
            return ERROR_DATATYPE_MISMATCH;
        }

        memset(buffer, '\0', sizeof(buffer));

        if (win32API.ReadProcessMemory(process, processParameters.CommandLine.Buffer, buffer, size, nullptr))
        {
            res = ParseProcessArgs(buffer, processArgs);
        }
        else
        {
            res = GetLastError();
        }
    }
    return res;
}

int Win32SystemInformationCollector::RemoteProcessArgsGet(pid_t pid, SicProcessArgs& processArgs, std::string& errorMessage)
{
	int res = SIC_EXECUTABLE_FAILED;
	{
		// @depend(Windows Server 2003)
		HANDLE process = win32API.OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, pid);
		if (process != nullptr) {
			unique_handle _guard{ process };
			res = PebGetProcessArgs(process, processArgs, errorMessage, pid);
			// CloseHandle(process);
			if (res == SIC_EXECUTABLE_SUCCESS) {
				return res;
			}
		}
	}

	/* likely we are 32-bit, pid process is 64-bit */
	res = SicProcessArgsWmiGet(pid, processArgs);
	if (res == ERROR_NOT_FOUND)
	{
		errorMessage += " No Such Process!";
	}

	return res;
}

int Win32SystemInformationCollector::SicGetProcessArgs(pid_t pid, SicProcessArgs& processArgs)
{
	SicPtr()->errorMessage = "";
	int res;
	if (pid == GetPid())
	{
		res = ParseProcessArgs(nullptr, processArgs);
	}
	else
	{
		res = RemoteProcessArgsGet(pid, processArgs, SicPtr()->errorMessage);
	}

	return res;
}

int Win32SystemInformationCollector::SicProcessExeWmiGet(pid_t pid, SicProcessExe& processExe)
{
	return SicWMI::GetProcessExecutablePath(pid, processExe.name);
	//{
	//	return wmi.GetLastError();
	//}

	//return processExe.name.empty()? ERROR_NOT_FOUND : SIC_EXECUTABLE_SUCCESS;
}

int Win32SystemInformationCollector::SicGetProcessExe(pid_t pid, SicProcessExe& processExe)
{
	int res = -1;
	std::string errorMessage;
	HANDLE process = win32API.OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, pid);
	if (process == nullptr)
	{
		SicPtr()->SetErrorMessage("Open Process " + std::to_string(pid) + " failed");
		return GetLastError();
	}

	{
		unique_handle _guard{ process };
		res = PebGetProcessExe(process, processExe, errorMessage, pid);
	}

	if (processExe.name.empty())
	{
		res = SicProcessExeWmiGet(pid, processExe);
		if (res == ERROR_NOT_FOUND)
		{
			res = -1;
		}
	}

	if (!processExe.cwd.empty())
	{
		/* strip trailing '\' */
		size_t len = processExe.cwd.size();
		if (processExe.cwd[len - 1] == '\\')
		{
			processExe.cwd.erase(len - 1, 2);
		}
		/* uppercase driver letter */
		processExe.cwd[0] = toupper(processExe.cwd[0]);
		/* e.g. C:\ */
		processExe.root = processExe.cwd.substr(0, 3);
	}

	if (!processExe.name.empty())
	{
		/* uppercase driver letter */
		processExe.name[0] = toupper(processExe.name[0]);
	}

	// CloseHandle(process);

	return res;
}

int Win32SystemInformationCollector::SicGetProcessCredName(pid_t pid,
	SicProcessCredName& processCredName)
{
	DWORD len;
	int ret;
	SID_NAME_USE type;

	HANDLE process = win32API.OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, pid);
	if (process == nullptr)
	{
		SicPtr()->SetErrorMessage("Open Process " + std::to_string(pid) + " failed");
		return GetLastError();
	}
	unique_handle _guardProcess{ process };

	HANDLE token = nullptr;
	if (!win32API.OpenProcessToken(process, STANDARD_RIGHTS_READ | READ_CONTROL | TOKEN_QUERY, &token))
	{
		// CloseHandle(process);
		SicPtr()->SetErrorMessage("Open Process " + std::to_string(pid) + " Token failed");
		return GetLastError();
	}
	unique_handle _guardToken{ token };

	// CloseHandle(process);

	// ret == 0: 失败
	ret = GetTokenInformation(token, TokenUser, nullptr, 0, &len);
	if (ret == 0 && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		auto user = std::shared_ptr < TOKEN_USER >{ static_cast<TOKEN_USER*>(malloc(len)), free };
		ret = GetTokenInformation(token, TokenUser, user.get(), len, &len);
		if (ret != 0) {
			char domain[SIC_CRED_NAME_MAX];
			DWORD domainLength = sizeof(domain);
			char processCredUser[SIC_CRED_NAME_MAX];
			DWORD userLength = sizeof(processCredName);
			ret = LookupAccountSid(nullptr, user->User.Sid, processCredUser,
				&userLength, domain, &domainLength, &type);
			processCredName.user = processCredUser;
		}
	}


	// if (user != nullptr)
	// {
	//     free(user);
	// }

	if (ret == 0)
	{
		// CloseHandle(token);
		SicPtr()->SetErrorMessage("Call LookupAccountSid in pid " + std::to_string(pid) + " Failed!");
		return GetLastError();
	}

	// TOKEN_PRIMARY_GROUP *group = nullptr;
	ret = GetTokenInformation(token, TokenPrimaryGroup, nullptr, 0, &len);
	if (ret == 0 && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		// group = static_cast<TOKEN_PRIMARY_GROUP *>(malloc(len));
		auto group = std::shared_ptr<TOKEN_PRIMARY_GROUP>{ static_cast<TOKEN_PRIMARY_GROUP*>(malloc(len)), free };
		ret = GetTokenInformation(token, TokenPrimaryGroup, group.get(), len, &len);
		if (ret != 0)
		{
			char domain[SIC_CRED_NAME_MAX];
			DWORD domainLength = sizeof(domain);
			char processCredGroup[SIC_CRED_NAME_MAX];
			DWORD groupLength = sizeof(processCredGroup);
			ret = LookupAccountSid(nullptr, group->PrimaryGroup, processCredGroup, &groupLength,
				domain, &domainLength, &type);
			processCredName.group = processCredGroup;
		}
	}


	// if (group != nullptr)
	// {
	//     free(group);
	// }

	// CloseHandle(token);

	if (ret == 0)
	{
		SicPtr()->SetErrorMessage("Call LookupAccountSid in pid " + std::to_string(pid) + " Failed!");
		return GetLastError();
	}

	return SIC_EXECUTABLE_SUCCESS;
}

int Win32SystemInformationCollector::SicGetProcessFd(pid_t pid, SicProcessFd& processFd)
{
	SicWin32ProcessInfo& processInfo = SicPtr()->win32ProcessInfo;
	// processInfo.pid = -1; // force update
	int res = PerfLibGetProcessInfo(pid, SicPtr()->errorMessage);

	if (res != SIC_EXECUTABLE_SUCCESS)
	{
		return res;
	}

	processFd.total = processInfo.handles;
    processFd.exact = true;

	return 0;
}

//int Win32SystemInformationCollector::SicDestroy()
//{
//	std::string errorMessage;
//	PdhDiskDestroy(errorMessage);
//	return SIC_EXECUTABLE_SUCCESS;
//}

// SicProcessCpuInformation& Win32SystemInformationCollector::SicGetProcessCpuInCache(pid_t pid)
// {
// 	SicCleanProcessCpuCacheIfNecessary();
// 	auto& entries = SicPtr()->processCpuCache.entries;
// 	if (entries.find(pid) != entries.end())
// 	{
// 		entries[pid].lastAccessTime = NowMillis();
// 		return entries[pid].processCpu;
// 	}
//
// 	// not found , need to create
// 	entries[pid] = {};
// 	entries[pid].lastAccessTime = NowMillis();
//
// 	return entries[pid].processCpu;
// }

int Win32SystemInformationCollector::PerfLibGetUpTime(double& uptime, std::string& errorMessage)
{
	// Retrieve counter data for the Processor object.
	auto perfDataHead = GetPerformanceData(PERF_SYS_COUNTER_KEY, errorMessage);
	if (!perfDataHead)
	{
		errorMessage = "GetPerformanceData failed : " + errorMessage;
		return -1;
	}

	PERF_OBJECT_TYPE* object;
	PERF_COUNTER_BLOCK* counterDataBlock;
	int success = -1;

	object = GetObject(perfDataHead.get(), PERF_SYS_COUNTER_KEY);
	if (nullptr == object)
	{
		errorMessage = "Object  " + std::string(PERF_SYS_COUNTER_KEY) + " not found.";
		// free(perfDataHead);
		return success;
	}

	std::map<DWORD, PERF_COUNTER_DEFINITION*> indexToCounterMap = {
		{PERF_COUNTER_SYS_UP_TIME, nullptr}
	};

	int foundCounter = GetCounter(object, indexToCounterMap);
	if (foundCounter < 0 || foundCounter < (int)indexToCounterMap.size())
	{
		errorMessage = makeErrorCounterNotFound(indexToCounterMap);
		// free(perfDataHead);
		return SIC_EXECUTABLE_FAILED;
	}

	counterDataBlock = GetCounterBlock(perfDataHead.get(), object, "", errorMessage);
	if (nullptr == counterDataBlock)
	{
		errorMessage = "Instance   not found. " + errorMessage;
		// free(perfDataHead);
		return success;
	}

	auto const& item = indexToCounterMap.begin();

	RawData rawData;
	rawData.CounterType = item->second->CounterType;
	success = GetValue(perfDataHead.get(), object, item->second, counterDataBlock, &rawData);
	if (!success)
	{
		errorMessage = "GetCounterValues failed.";
		return success;
	}

	uptime = static_cast<double>((rawData.Time - rawData.Data) / rawData.Frequency);
	// free(perfDataHead);
	return 0;
}

OSVERSIONINFOEX GetWindowsVersion() {
	OSVERSIONINFOEX version = { 0 };
	version.dwOSVersionInfoSize = sizeof(version);
	GetVersionEx((OSVERSIONINFO*)&version);
	return version;
}

// 参考Windows列表： https://learn.microsoft.com/en-us/windows/win32/sysinfo/operating-system-version
int Win32SystemInformationCollector::SicGetSystemInfo(SicSystemInfo& systemInfo)
{
	OSVERSIONINFOEX version = GetWindowsVersion();

	if (version.dwMajorVersion == 4)
	{
		systemInfo.vendorName = "Windows NT";
		systemInfo.vendorVersion = "NT";
	}
	else if (version.dwMajorVersion == 5)
	{
		switch (version.dwMinorVersion)
		{
		case 0:
			systemInfo.vendorName = "Windows 2000";
			systemInfo.vendorVersion = "2000";
			break;
		case 1:
			systemInfo.vendorName = "Windows XP";
			systemInfo.vendorVersion = "XP";
			break;
		case 2:
			systemInfo.vendorName = "Windows 2003";
			systemInfo.vendorVersion = "2003";
			break;
		default:
			systemInfo.vendorName = "Windows Unknown";
			break;
		}
	}
	else if (version.dwMajorVersion == 6)
	{
		if (version.wProductType == VER_NT_WORKSTATION)
		{
			if (version.dwMinorVersion == 0)
			{
				systemInfo.vendorName = "Windows Vista";
				systemInfo.vendorVersion = "Vista";
			}
			else if (version.dwMinorVersion == 1)
			{
				systemInfo.vendorName = "Windows 7";
				systemInfo.vendorVersion = "7";
			}
			else if (version.dwMinorVersion == 2)
			{
				systemInfo.vendorName = "Windows 8";
				systemInfo.vendorVersion = "8";
			}
			else
			{
				systemInfo.vendorName = "Windows 8.1";
				systemInfo.vendorVersion = "8.1";
			}
		}
		else
		{
			switch (version.dwMinorVersion)
			{
			case 0:
			case 1:
				systemInfo.vendorName = (0 == version.dwMinorVersion ? "Windows 2008" : "Windows 2008 R2");
				systemInfo.vendorVersion = "2008";
				break;
			case 2:
			case 3:
			default:
				systemInfo.vendorName = (2 == version.dwMinorVersion ? "Windows 2012" : "Windows 2012 R2");
				systemInfo.vendorVersion = "2012";
				break;
			}
		}
	}
	else if (version.dwMajorVersion == 10)
	{
		// Win10以后，版本号不再变化 ，大概是微软有意弱化Windows的版本，主推UWP(Universal Windows Platform)
		if (version.wProductType == VER_NT_WORKSTATION)
		{
			switch (version.dwMinorVersion)
			{
			default:
			case 0:
				systemInfo.vendorName = "Windows 10";
				systemInfo.vendorVersion = "10";
				break;
			}
		}
		else
		{
			switch (version.dwMinorVersion)
			{
			default:
			case 0:
				systemInfo.vendorName = "Windows Server 2016";
				systemInfo.vendorVersion = "2016";
				break;
			}
		}
	}
	else
	{
		systemInfo.vendorName = "Windows Unknown";
		systemInfo.vendorVersion = "unknown";
	}

	systemInfo.name = "Win32";
	systemInfo.vendor = "Microsoft";

#ifdef _M_X64
	systemInfo.arch = "x64";
#else
	systemInfo.arch = "x86";
#endif
	systemInfo.version = std::to_string(version.dwMajorVersion) + "." + std::to_string(version.dwMinorVersion);
	systemInfo.description = systemInfo.vendor + " " + systemInfo.vendorName;

	return SIC_EXECUTABLE_SUCCESS;
}

int Win32SystemInformationCollector::PdhDiskAddCounter(const std::string& dirName, std::string& errorMessage) {
	auto disk = std::make_shared<HDiskCounter>();

	struct tagEntry {
		const char* pattern;
		HCOUNTER* counter;
	}entries[] = {
		{"\\LogicalDisk({})\\Disk Read Bytes/sec", &disk->readBytes},
		{"\\LogicalDisk({})\\Disk Write Bytes/sec", &disk->writeBytes},
		{"\\LogicalDisk({})\\Disk Reads/sec", &disk->reads},
		{"\\LogicalDisk({})\\Disk Writes/sec", &disk->writes},

		{"\\LogicalDisk({})\\% Disk Read Time", &disk->readTime},
		{"\\LogicalDisk({})\\% Disk Write Time", &disk->writeTime},
		{"\\LogicalDisk({})\\% Disk Time", &disk->time},
		{"\\LogicalDisk({})\\% Idle Time", &disk->idleTime},
	};
	for (const auto& entry : entries) {
		std::string counterPath = fmt::format(entry.pattern, dirName);
		PDH_STATUS status = PdhAddCounter(SicPtr()->diskCounterQuery, counterPath.c_str(), NULL, entry.counter);
		if (ERROR_SUCCESS != status) {
			errorMessage = PdhErrorStr(status, fmt::format("PdhAddCounter({})", counterPath));
			return SIC_EXECUTABLE_FAILED;
		}
	}
	mDiskCounter[dirName] = disk;

	// 预加载一下，成功与否不重要
	PdhCollectQueryData(SicPtr()->diskCounterQuery);

	return SIC_EXECUTABLE_SUCCESS;
}

int Win32SystemInformationCollector::PdhDiskInit(std::string& errorMessage) {
	// OS Need: Windows Server 2003 
	PDH_STATUS status = PdhOpenQuery(NULL, 0, &SicPtr()->diskCounterQuery);

	if (ERROR_SUCCESS != status) {
		errorMessage += PdhErrorStr(status, "PdhOpenQuery");
		return SIC_EXECUTABLE_FAILED;
	}
	return SIC_EXECUTABLE_SUCCESS;
}

int Win32SystemInformationCollector::PdhDiskDestroy() {
	mDiskCounter.clear();
	//PdhCloseQuery(SicPtr()->diskCounterQuery);
	return SIC_EXECUTABLE_SUCCESS;
}

std::map<std::string, std::string> Win32SystemInformationCollector::queryDevSerialId(const std::string &) {
	std::vector<WmiPartitionInfo> partitions;
	SicWMI::GetPartitionList(partitions);

	std::map<std::string, std::string> ret;
	for (const auto& part : partitions) {
		ret[part.partition] = part.sn;
	}
	RETURN_RVALUE(ret);
}

std::shared_ptr<SystemInformationCollector> SystemInformationCollector::New() {
    auto collector = std::make_shared<Win32SystemInformationCollector>();
    return SIC_EXECUTABLE_SUCCESS == collector->SicInitialize()? collector: nullptr;
}
