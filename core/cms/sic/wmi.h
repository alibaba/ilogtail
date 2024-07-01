//
// Created by liuze.xlz on 2020/11/23.
//

#ifndef SIC_INCLUDE_WMI_H_
#define SIC_INCLUDE_WMI_H_
#include <wbemidl.h>
#include <comutil.h> // bstr_t
#include <string>
#include <vector>

struct WmiProcessInfo {
    DWORD pid = 0;
    int exeErrorCode = ERROR_INVALID_FUNCTION;
    std::string exe;
    int argsErrorCode = ERROR_INVALID_FUNCTION;
    bstr_t args;
};

struct WmiPartitionInfo {
    std::string partition;  // 分区，如C:
    std::string sn;         // 磁盘序列号

    WmiPartitionInfo() = default;
    WmiPartitionInfo(const std::string& p, const std::string& s) :partition(p), sn(s) {
    }
};
#include "common/test_support"
class SicWMI {
public:
    static int GetProcessExecutablePath(DWORD pid, std::string& exe);
    static int GetProcessArgs(DWORD pid, bstr_t &);
    static void UpdateProcesses(const std::vector<DWORD>&);
    static bool GetPartitionList(std::vector<WmiPartitionInfo>& disk);

public:
    SicWMI();
    ~SicWMI();
    bool Open();
    void Close();

    bool ProcessStringPropertyGet(DWORD pid, WmiProcessInfo &);
    int GetLastError();
    bool EnumProcesses(std::vector<WmiProcessInfo>&);

    bool GetDiskSerialId(std::vector<WmiPartitionInfo> &disk);

private:
    IWbemServices *wbem;
    HRESULT result;
    BSTR GetProcQuery(DWORD pid);
};
#include "common/test_support"

#endif //SIC_INCLUDE_WMI_H_
