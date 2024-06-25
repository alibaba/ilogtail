//
// Created by liuze.xlz on 2020/11/23.
//
#pragma comment(lib, "wbemuuid.lib") // wbemidl.h, IID_IWbemLocator

#include <unordered_map>
#include <set>

#include <windows.h>
#include <objbase.h>
#include <comdef.h>

#include <boost/json.hpp>
#include <fmt/format.h>

#include "common/SafeMap.h"
#include "common/Logger.h"
#include "common/TimeProfile.h"

#include "sic/wmi.h"
#include "sic/win32_sic_utils.h"

SicWMI::SicWMI()
{
    wbem = nullptr;
    result = S_OK;
    // Initializes the COM library for use by the calling thread, sets the thread's concurrency model, and creates a new apartment for the thread if one is required.
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);  // @depend(Windows 2000 Server)
}

SicWMI::~SicWMI()
{
    Close();
    CoUninitialize();
}

std::unordered_map< HRESULT, int> hResult2Err = {
    {S_OK, ERROR_SUCCESS},
    {WBEM_E_NOT_FOUND, ERROR_NOT_FOUND},
    {WBEM_E_ACCESS_DENIED, ERROR_ACCESS_DENIED},
    {WBEM_E_NOT_SUPPORTED, -1},
    //{WBEM_E_INVALID_PARAMETER, ERROR_BAD_ARGUMENTS},
};
int SicWMI::GetLastError()
{
    auto it = hResult2Err.find(result);
    return it != hResult2Err.end() ? it->second : ERROR_INVALID_FUNCTION;
}

bool SicWMI::Open()
{
    if (wbem)
    {
        result = S_OK;
        return true;
    }

    TimeProfile tp;
	result = CoInitializeSecurity(nullptr,                        //Security Descriptor
		-1,                          //COM authentication
		nullptr,                        //Authentication services
		nullptr,                        //Reserved
		RPC_C_AUTHN_LEVEL_DEFAULT,   //Default authentication
		RPC_C_IMP_LEVEL_IMPERSONATE, //Default Impersonation
		nullptr,                        //Authentication info
		EOAC_NONE,                   //Additional capabilities
		nullptr);                       //Reserved
    LogDebug("CoInitializeSecurity() => {}({}), cost: {}", result, (SUCCEEDED(result) ? "SUCCEEDED" : "FAIL"), tp.cost());

	IWbemLocator* locator = nullptr;
    if (SUCCEEDED(result)) {
        result = CoCreateInstance(CLSID_WbemLocator,
            nullptr, /* IUnknown */
            CLSCTX_INPROC_SERVER,
            IID_IWbemLocator,
            (LPVOID*)&locator);
        LogDebug("CoCreateInstance() => {}({}), cost: {}", result, (SUCCEEDED(result) ? "SUCCEEDED" : "FAIL"), tp.cost());
    }

    if (SUCCEEDED(result))
	{
		std::shared_ptr<IWbemLocator> _guard{ locator, [](IWbemLocator* loc) {loc->Release(); } };
		const LPCWSTR computerName = L".";
		// wsprintfW: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-wsprintfw
		wchar_t path[MAX_PATH];
		wsprintfW(path, LR"(\\%s\ROOT\CIMV2)", computerName);

        LPCWSTR user = nullptr;
        LPCWSTR pass = nullptr;
        result = locator->ConnectServer(bstr_t(path), // Object path of SicWMI namespace
			nullptr, //User name. NULL = current user
			nullptr, //User password. NULL = current
			nullptr,         //Locale. NULL indicates current
			0,            //Security flags
			nullptr,         //Authority (e.g. Kerberos)
			nullptr,         //Context object
			&wbem);       //pointer to IWbemServices proxy
        LogDebug("ConnectServer() => {}({}), cost: {}", result, (SUCCEEDED(result) ? "SUCCEEDED" : "FAIL"), tp.cost());

        //if (SUCCEEDED(result)) {
        //    result = CoSetProxyBlanket(
        //        wbem,                        // Indicates the proxy to set
        //        RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
        //        RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
        //        NULL,                        // Server principal name 
        //        RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
        //        RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
        //        NULL,                        // client identity
        //        EOAC_NONE                    // proxy capabilities 
        //    );

        //    //if (FAILED(result))
        //    //{
        //    //    pSvc->Release();
        //    //    pLoc->Release();
        //    //    CoUninitialize();
        //    //    return 1;               // Program has failed.
        //    //}
        //}
    }

    return SUCCEEDED(result);
}

void SicWMI::Close()
{
    if (wbem)
    {
        TimeProfile tp;
        wbem->Release();
        wbem = NULL;
        result = S_OK;

        LogDebug("Release(), cost: {}", tp.cost());
    }
}

BSTR SicWMI::GetProcQuery(DWORD pid)
{
    wchar_t query[56];
    wsprintfW(query, L"Win32_Process.Handle=%d", pid);
	return bstr_t{ query };
}

// Wmi查询，请参考：https://github.com/MicrosoftDocs/win32/blob/docs/desktop-src/WmiSdk/wmi-tasks--processes.md
// https://github.com/MicrosoftDocs/win32/blob/docs/desktop-src/WmiSdk/example--getting-wmi-data-from-the-local-computer.md
bool SicWMI::ProcessStringPropertyGet(DWORD pid, WmiProcessInfo &procInfo)
{
    TimeProfile tp;

	IWbemClassObject *obj = nullptr;
    result = wbem->GetObject(GetProcQuery(pid), 0, nullptr, &obj, nullptr);

	// 这个很耗时，差不多需要5+秒。
	LogDebug("wbem->GetObject() => {}({}), cost: {}", result, (SUCCEEDED(result) ? "SUCCEEDED" : "FAIL"), tp.cost());

	procInfo.pid = pid;
    if (SUCCEEDED(result))
    {
        defer(obj->Release());
		struct {
			WCHAR* name; //  names[] = { L"ExecutablePath",  L"CommandLine" };
            int& errorCode;
			std::function<void(BSTR)> set;
		} items[] = {
            {L"ExecutablePath", procInfo.exeErrorCode, [&](BSTR bstrVal) { procInfo.exe = WcharToChar(bstrVal, CP_ACP); }},
            {L"CommandLine",    procInfo.argsErrorCode, [&](BSTR bstrVal) { procInfo.args = bstr_t{bstrVal}; } },
		};

        for (size_t i = 0; i < sizeof(items)/sizeof(items[0]); i++) {
            VARIANT var;
            VariantInit(&var);
            result = obj->Get(items[i].name, 0, &var, nullptr, nullptr); // @depend(Windows Server 2008)
            defer(VariantClear(&var));

            items[i].errorCode = 0;
            if (SUCCEEDED(result))
            {
                if (var.vt == VT_NULL)
                {
                    result = WBEM_E_INVALID_PARAMETER;
                }
                else
                {
                    items[i].set(var.bstrVal);
                    //lstrcpynW(value, var.bstrVal, len);
                }
            }
            if (!SUCCEEDED(result)) {
                items[i].errorCode = GetLastError();
            }
            LogDebug("obj->Get({}) => {}({}), item.errorCode: {}, cost: {}", WcharToChar(items[i].name, CP_ACP), 
                result, (SUCCEEDED(result) ? "SUCCEEDED" : "FAIL"), items[i].errorCode, tp.cost());
        }
    }

    return SUCCEEDED(result);
}

//  Wmi查询，请参考：https://github.com/MicrosoftDocs/win32/blob/docs/desktop-src/WmiSdk/wmi-tasks--processes.md
bool SicWMI::EnumProcesses(std::vector<WmiProcessInfo >&processes)
{
    TimeProfile tp;
    IEnumWbemClassObject * pEnumerator = nullptr;
    result = wbem->ExecQuery(bstr_t("WQL"), 
        bstr_t("SELECT * FROM Win32_Process"), 
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &pEnumerator);

	// 这个很耗时，差不多需要5+秒。
    LogDebug("wbem->ExecQuery(Win32_Process) => {}({}), cost: {}", result, (SUCCEEDED(result) ? "SUCCEEDED" : "FAIL"), tp.cost());
    if (SUCCEEDED(result))
    {
        defer(pEnumerator->Release());
        
		struct {
			WCHAR* name;
			std::function<int& (WmiProcessInfo&)> errorCode;
			std::function<void(WmiProcessInfo&, const VARIANT&)> proc;
		} items[] = {
			{L"ProcessId",
			    nullptr,
			    [](WmiProcessInfo& procInfo, const VARIANT& var) { procInfo.pid = var.intVal; }},
			{L"ExecutablePath",
				[](WmiProcessInfo& procInfo) -> int& {return procInfo.exeErrorCode; },
				[](WmiProcessInfo& procInfo, const VARIANT& var) { procInfo.exe = WcharToChar(var.bstrVal, CP_ACP); }},
			{L"CommandLine",
			    [](WmiProcessInfo& procInfo) -> int& {return procInfo.argsErrorCode; },
			    [](WmiProcessInfo& procInfo, const VARIANT& var) { procInfo.args = bstr_t{var.bstrVal}; }},
		};

        while (pEnumerator) {
            IWbemClassObject* pclsObj = nullptr;
            ULONG uReturn = 0;
            result = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
            if (0 == uReturn)
            {
                break;
            }
            defer(pclsObj->Release());

            WmiProcessInfo procInfo;
            for (size_t i = 0; i < sizeof(items) / sizeof(items[0]); i++) {
                VARIANT var;
                VariantInit(&var);
                result = pclsObj->Get(items[i].name, 0, &var, nullptr, nullptr); // @depend(Windows Server 2008)
                defer(VariantClear(&var));

                if (SUCCEEDED(result))
                {
                    if (var.vt == VT_NULL)
                    {
                        result = WBEM_E_INVALID_PARAMETER;
                    }
                    else
                    {
                        items[i].proc(procInfo, var);
                        //lstrcpynW(value, var.bstrVal, len);
                    }
                }
				if (items[i].errorCode) {
					items[i].errorCode(procInfo) = SUCCEEDED(result) ? 0 : GetLastError();
				}
            }

            processes.push_back(procInfo);
        }
    }

    return SUCCEEDED(result);
}

//bool SicWMI::ProcessExecutablePathGet(DWORD pid, WCHAR *value)
//{
//    return ProcessStringPropertyGet(pid, L"ExecutablePath", value, MAX_PATH);
//}
//
//bool SicWMI::ProcessCommandLineGet(DWORD pid, WCHAR *value)
//{
//    return ProcessStringPropertyGet(pid, L"CommandLine", value, static_cast<DWORD>(WinConst::SIC_CMDLINE_MAX));
//}

//#define USE_PER_PROCESS

#ifdef USE_PER_PROCESS
static SafeMap<DWORD, WmiProcessInfo> cachedWmiProcess;

WmiProcessInfo GetProcessInfoFromWmi(DWORD pid) {
    WmiProcessInfo procInfo;

    SicWMI wmi;
    if (wmi.Open()) {
        wmi.ProcessStringPropertyGet(pid, procInfo);
    }
    return procInfo;
}

int SicWMI::GetProcessExecutablePath(DWORD pid, std::string &exe) {
    WmiProcessInfo procInfo = cachedWmiProcess.SetIfAbsent(pid, [&]() {return GetProcessInfoFromWmi(pid); });
    exe = procInfo.exe;
    return procInfo.exeErrorCode;
}
int SicWMI::GetProcessArgs(DWORD pid, bstr_t &args) {
    WmiProcessInfo procInfo = cachedWmiProcess.SetIfAbsent(pid, [&]() {return GetProcessInfoFromWmi(pid); });
    args = procInfo.args;
    return procInfo.argsErrorCode;
}

void SicWMI::UpdateProcesses(const std::vector<DWORD>& pids) {
    std::set<DWORD> pidSet{ pids.begin(), pids.end() };

	ThreadSafe(cachedWmiProcess, {
		auto it = cachedWmiProcess.begin();
	    while (it != cachedWmiProcess.end()) {
		    if (pidSet.find(it->first) == pidSet.end()) {
			    it = cachedWmiProcess.erase(it);
		    }
		    else {
			    ++it;
		    }
	    }
    });
}
#else
#include "common/SafeShared.h"
static SafeSharedMap<DWORD, WmiProcessInfo> cachedWmiProcess;

WmiProcessInfo GetProcessInfoFromWmi(DWORD pid) {
    WmiProcessInfo proc;
    Sync(cachedWmiProcess) {
        auto it = cachedWmiProcess->find(pid);
        bool found = it != cachedWmiProcess->end();
        if (!found) {
            std::vector<WmiProcessInfo> procInfos;

            SicWMI wmi;
            if (wmi.Open()) {
                wmi.EnumProcesses(procInfos);
            }

            if (!procInfos.empty()) {
                auto mapProc = std::make_shared<std::map<DWORD, WmiProcessInfo>>();
                for (auto& proc : procInfos) {
                    mapProc->emplace(proc.pid, proc);
                }
                cachedWmiProcess = mapProc;

                it = cachedWmiProcess->find(pid);
                found = it != cachedWmiProcess->end();
            }
        }
        if (found) {
            proc = it->second;
        }
    }}}
    return proc;
}

int SicWMI::GetProcessExecutablePath(DWORD pid, std::string& exe) {
    WmiProcessInfo procInfo = GetProcessInfoFromWmi(pid);
    exe = procInfo.exe;
    return procInfo.exeErrorCode;
}
int SicWMI::GetProcessArgs(DWORD pid, bstr_t& args) {
    WmiProcessInfo procInfo = GetProcessInfoFromWmi(pid);
    args = procInfo.args;
    return procInfo.argsErrorCode;
}

void SicWMI::UpdateProcesses(const std::vector<DWORD>& pids) {
    std::set<DWORD> pidSet{ pids.begin(), pids.end() };

    Sync(cachedWmiProcess) {
        auto end = cachedWmiProcess->end();
        for (auto it = cachedWmiProcess->begin(); it != end; ) {
            if (pidSet.find(it->first) == pidSet.end()) {
                it = cachedWmiProcess->erase(it);
            }
            else {
                ++it;
            }
        }
    }}}
}
#endif

//
// DeviceID: \\.\PHYSICALDRIVE0
// Index: 0
// SerialNumber: bp1462m6o0nclbrkjsdb
struct tagDiskDrive{
    int index;
    std::string deviceId;
    std::string serialNumber;

    bool empty() const {
        return deviceId.empty() || serialNumber.empty();
    }
};

// Get - WmiObject - Class Win32_DiskDrive | fl *
static bool EnumDiskDrive(IWbemServices* wbem, std::map<int, tagDiskDrive> &data)
{
    TimeProfile tp;
    IEnumWbemClassObject* pEnumerator = nullptr;
    HRESULT result = wbem->ExecQuery(bstr_t("WQL"),
        bstr_t("SELECT * FROM Win32_DiskDrive"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &pEnumerator);
    // 这个很耗时，差不多需要500+ ms。
    Log((SUCCEEDED(result)?LEVEL_DEBUG:LEVEL_WARN), "wbem->ExecQuery(Win32_DiskDrive) => {}({}), cost: {}",
		result, (SUCCEEDED(result) ? "SUCCEEDED" : "FAIL"), tp.cost());

	if (SUCCEEDED(result))
    {
        defer(pEnumerator->Release());

		struct {
			WCHAR* name;
			std::function<void(tagDiskDrive&, const VARIANT&)> proc;
		} items[] = {
            {L"Index", // Example: 0
                [](tagDiskDrive& disk, const VARIANT& var) {disk.index = var.intVal; }},  // VT_I4
            {L"DeviceId", // Example: \\.\PHYSICALDRIVE0
				[](tagDiskDrive& disk, const VARIANT& var) {disk.deviceId = WcharToChar(var.bstrVal, CP_ACP); }},
			{L"SerialNumber",  // Example:  bp1462m6o0nclbrkjsdb
				[](tagDiskDrive& disk, const VARIANT& var) {disk.serialNumber = WcharToChar(var.bstrVal, CP_ACP); }},
		};

        while (pEnumerator && SUCCEEDED(result)) {
            IWbemClassObject* pclsObj = nullptr;
            ULONG uReturn = 0;
            result = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
            if (0 == uReturn)
            {
                break;
            }
            defer(pclsObj->Release());

            tagDiskDrive disk;
            for (size_t i = 0; i < sizeof(items) / sizeof(items[0]) && SUCCEEDED(result); i++) {
                VARIANT var;
                VariantInit(&var);
                result = pclsObj->Get(items[i].name, 0, &var, nullptr, nullptr); // @depend(Windows Server 2008)
                defer(VariantClear(&var));

                if (SUCCEEDED(result))
                {
                    if (var.vt == VT_NULL)
                    {
                        result = WBEM_E_INVALID_PARAMETER;
                    }
                    else
                    {
                        items[i].proc(disk, var);
                    }
                }
                //if (items[i].errorCode) {
                //    items[i].errorCode(procInfo) = SUCCEEDED(result) ? 0 : GetLastError();
                //}
            }
            if (SUCCEEDED(result) && !disk.empty()) {
                data[disk.index] = disk;
            }
        }
    }

    return SUCCEEDED(result);
}

struct tagLogicalDiskToPartition {
    int diskId = -1;
    std::string partition;

    bool empty() const {
        return diskId < 0 || partition.empty();
    }
};

static bool EnumLogicalDiskToPartition(IWbemServices* wbem, std::map<std::string, tagLogicalDiskToPartition>& data)
{
    TimeProfile tp;
    IEnumWbemClassObject* pEnumerator = nullptr;
    HRESULT result = wbem->ExecQuery(bstr_t("WQL"),
        bstr_t("SELECT * FROM Win32_LogicalDiskToPartition"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &pEnumerator);
    // 这个很耗时，差不多需要500+ ms。
    LogDebug("wbem->ExecQuery(Win32_LogicalDiskToPartition) => {}({}), cost: {}",
             result, (SUCCEEDED(result) ? "SUCCEEDED" : "FAIL"), tp.cost());

	if (SUCCEEDED(result))
    {
        defer(pEnumerator->Release());

        struct {
            WCHAR* name;
            std::function<void(tagLogicalDiskToPartition&, const VARIANT&)> proc;
        } items[] = {
            {L"Antecedent", [](tagLogicalDiskToPartition& disk, const VARIANT& var) {
                std::string s = WcharToChar(var.bstrVal, CP_ACP);
                // example: \\e011164204198-e\root\cimv2:Win32_DiskPartition.DeviceID="Disk #0, Partition #0"
                const std::string key = "DeviceID=\"Disk #";
                size_t pos = s.find(key);
                if (pos != std::string::npos) {
                    disk.diskId = convert<int>(s.substr(pos + key.size()));
                }
            }},
            {L"Dependent", [](tagLogicalDiskToPartition& disk, const VARIANT& var) {
                std::string s = WcharToChar(var.bstrVal, CP_ACP);
                // example: \\e011164204198-e\root\cimv2:Win32_LogicalDisk.DeviceID="C:"
                const std::string key = "Win32_LogicalDisk.DeviceID=\"";
                size_t pos = s.find(key);
                if (pos != std::string::npos) {
                    pos += key.size();
                    disk.partition = s.substr(pos, s.size() - pos - 1); // -1 : skip 尾部双引号
                }
            }},
        };

        while (pEnumerator && SUCCEEDED(result)) {
            IWbemClassObject* pclsObj = nullptr;
            ULONG uReturn = 0;
            result = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
            if (0 == uReturn)
            {
                break;
            }
            defer(pclsObj->Release());

            tagLogicalDiskToPartition disk;
            for (size_t i = 0; i < sizeof(items) / sizeof(items[0]) && SUCCEEDED(result); i++) {
                VARIANT var;
                VariantInit(&var);
                result = pclsObj->Get(items[i].name, 0, &var, nullptr, nullptr); // @depend(Windows Server 2008)
                defer(VariantClear(&var));

                if (SUCCEEDED(result))
                {
                    if (var.vt == VT_NULL)
                    {
                        result = WBEM_E_INVALID_PARAMETER;
                    }
                    else
                    {
                        items[i].proc(disk, var);
                    }
                }
                //if (items[i].errorCode) {
                //    items[i].errorCode(procInfo) = SUCCEEDED(result) ? 0 : GetLastError();
                //}
            }
            if (SUCCEEDED(result) && !disk.empty()) {
                data[disk.partition] = disk;
            }
        }
    }

    return SUCCEEDED(result);
}

#if 0
// https://learn.microsoft.com/en-us/windows/win32/cimwin32prov/win32-logicaldisktopartition
// get Drives, logical Drives and Driveletters
BOOL wmi_getDriveLetters(IWbemServices* pSvc)
{
    using namespace std;
    // Use the IWbemServices pointer to make requests of WMI. 
    // Make requests here:
    HRESULT hres;
    IEnumWbemClassObject* pEnumerator = NULL;

    // get localdrives
    hres = pSvc->ExecQuery(
        bstr_t("WQL"),
        bstr_t("SELECT * FROM Win32_DiskDrive"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnumerator);

    if (FAILED(hres)) {
        cout << "Query for processes failed. "
            << "Error code = 0x"
            << hex << hres << endl;
        //pSvc->Release();
        //pLoc->Release();
        //CoUninitialize();
        return FALSE;               // Program has failed.
    }
    else {

        IWbemClassObject* pclsObj;
        ULONG uReturn = 0;
        while (pEnumerator) {
            hres = pEnumerator->Next(WBEM_INFINITE, 1,
                &pclsObj, &uReturn);
            if (0 == uReturn) break;

            VARIANT vtProp;
            hres = pclsObj->Get(_bstr_t(L"DeviceID"), 0, &vtProp, 0, 0);

            // adjust string
            wstring tmp = vtProp.bstrVal;
            tmp = tmp.substr(4);

            wstring wstrQuery = LR"(Associators of {Win32_DiskDrive.DeviceID='\\.\)";
            wstrQuery += tmp;
            wstrQuery += L"'} where AssocClass=Win32_DiskDriveToDiskPartition";

            // reference drive to partition
            IEnumWbemClassObject* pEnumerator1 = NULL;
            hres = pSvc->ExecQuery(
                bstr_t("WQL"),
                bstr_t(wstrQuery.c_str()),
                WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                NULL,
                &pEnumerator1);

            if (FAILED(hres)) {
                cout << "Query for processes failed. "
                    << "Error code = 0x"
                    << hex << hres << endl;
                //pSvc->Release();
                //pLoc->Release();
                //CoUninitialize();
                return FALSE;               // Program has failed.
            }
            else {

                IWbemClassObject* pclsObj1;
                ULONG uReturn1 = 0;
                while (pEnumerator1) {
                    hres = pEnumerator1->Next(WBEM_INFINITE, 1,
                        &pclsObj1, &uReturn1);
                    if (0 == uReturn1) break;

                    // reference logical drive to partition
                    VARIANT vtProp1;
                    hres = pclsObj1->Get(_bstr_t(L"DeviceID"), 0, &vtProp1, 0, 0);
                    wstring wstrQuery = L"Associators of {Win32_DiskPartition.DeviceID='";
                    wstrQuery += vtProp1.bstrVal;
                    wstrQuery += L"'} where AssocClass=Win32_LogicalDiskToPartition";



                    IEnumWbemClassObject* pEnumerator2 = NULL;
                    hres = pSvc->ExecQuery(
                        bstr_t("WQL"),
                        bstr_t(wstrQuery.c_str()),
                        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                        NULL,
                        &pEnumerator2);





                    if (FAILED(hres)) {
                        cout << "Query for processes failed. "
                            << "Error code = 0x"
                            << hex << hres << endl;
                        //pSvc->Release();
                        //pLoc->Release();
                        //CoUninitialize();
                        return FALSE;               // Program has failed.
                    }
                    else {

                        // get driveletter
                        IWbemClassObject* pclsObj2;
                        ULONG uReturn2 = 0;
                        while (pEnumerator2) {
                            hres = pEnumerator2->Next(WBEM_INFINITE, 1,
                                &pclsObj2, &uReturn2);
                            if (0 == uReturn2) break;

                            VARIANT vtProp2;
                            hres = pclsObj2->Get(_bstr_t(L"DeviceID"), 0, &vtProp2, 0, 0);



                            // print result
                            printf("*** %ls : %ls\n", vtProp.bstrVal, vtProp2.bstrVal);

                            VariantClear(&vtProp2);
                        }
                        pclsObj1->Release();
                    }
                    VariantClear(&vtProp1);
                    pEnumerator2->Release();
                }
                pclsObj->Release();
            }
            VariantClear(&vtProp);
            pEnumerator1->Release();
        }
    }
    pEnumerator->Release();
    return TRUE;
}
#endif

// See: https://help.aliyun.com/zh/ecs/user-guide/query-the-serial-number-of-a-disk # 查看磁盘序列号
bool SicWMI::GetDiskSerialId(std::vector<WmiPartitionInfo> &disk) {
    std::map<int, tagDiskDrive> physicalDisk;
    bool ok = EnumDiskDrive(wbem, physicalDisk);
    if (!ok) {
        return false;
    }
    // key: Disk Index, value: Disk SerialNumber
    std::map<int, std::string> diskSn;
    for (const auto& it : physicalDisk) {
        diskSn[it.first] = it.second.serialNumber;
    }
    // EnumDiskDrive => 1, {0:"bp1462m6o0nclbrkjsdb"}
    LogInfo("EnumDiskDrive => {}, {}", diskSn.size(), boost::json::serialize(boost::json::value_from(diskSn)));

    /// //////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///
    std::map<std::string, tagLogicalDiskToPartition> partitions;
    ok = EnumLogicalDiskToPartition(wbem, partitions);
    if (!ok) {
        return false;
    }
    // key: Partition, such as “C:”, value： disk index
    std::map<std::string, int> partitionToDisk;
    for (const auto& it : partitions) {
        partitionToDisk[it.second.partition] = it.second.diskId;
    }

    // EnumLogicalDiskToPartition => 1, {"C:":0}
    LogInfo("EnumLogicalDiskToPartition => {}, {}", partitionToDisk.size(), boost::json::serialize(boost::json::value_from(partitionToDisk)));

    disk.reserve(partitionToDisk.size());
    for (const auto &partition : partitionToDisk) {
        auto it = diskSn.find(partition.second);
        if (it != diskSn.end()) {
            std::string partitionName = ToUpper(partition.first);
            disk.emplace_back(partitionName, it->second);
            if (!HasSuffix(partitionName, "\\")) {
                disk.emplace_back(partitionName + "\\", it->second);
            }
        }
    }

    return true;
}

bool SicWMI::GetPartitionList(std::vector<WmiPartitionInfo>& disk) {
    SicWMI wmi;
    if (wmi.Open()) {
        wmi.GetDiskSerialId(disk);
    }
    return !disk.empty();
}