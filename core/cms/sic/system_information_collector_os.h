//
// Created by liuze.xlz on 2020/11/23.
//

#ifndef SIC_INCLUDE_SYSTEM_INFORMATION_COLLECTOR_OS_H_
#define SIC_INCLUDE_SYSTEM_INFORMATION_COLLECTOR_OS_H_
#ifdef WIN32
#include <winreg.h>
#include <winperf.h>

#include <ipmib.h>
#include <tcpmib.h>
#include <udpmib.h>

// ------ vvvvvv ------ winternl.h -------------------------------------------------------------------------------------
// 参见: https://www.geoffchappell.com/studies/windows/km/ntoskrnl/api/ex/sysinfo/processor_performance.htm
typedef struct {
    LARGE_INTEGER IdleTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER DpcTime;
    LARGE_INTEGER InterruptTime;
    ULONG InterruptCount;
} SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION;

// 参见: https://www.geoffchappell.com/studies/windows/km/ntoskrnl/inc/api/ntexapi/system_information_class.htm?spm=ata.21735953.0.0.2b49752471XzVg
#define SystemProcessorPerformanceInformation 8
// See https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/7440cfac-6052-4925-84e4-c32e417de300
//#include <ntstatus.h>
#define STATUS_SUCCESS                   0x00000000L
#define STATUS_INFO_LENGTH_MISMATCH      0xC0000004L
#define STATUS_BUFFER_TOO_SMALL          0xC0000023L
// ------ ^^^^^^ ------ winternl.h -------------------------------------------------------------------------------------

/* PEB decls from msdn docs w/ slight mods */
#define ProcessBasicInformation 0

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _PEB_LDR_DATA {
    BYTE Reserved1[8];
    PVOID Reserved2[3];
    LIST_ENTRY InMemoryOrderModuleList;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef struct RTL_DRIVE_LETTER_CURDIR {
    USHORT              Flags;
    USHORT              Length;
    ULONG               TimeStamp;
    UNICODE_STRING      DosPath;
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;

/* from: http://source.winehq.org/source/include/winternl.h */
typedef struct _RTL_USER_PROCESS_PARAMETERS {
    ULONG               AllocationSize;
    ULONG               Size;
    ULONG               Flags;
    ULONG               DebugFlags;
    HANDLE              hConsole;
    ULONG               ProcessGroup;
    HANDLE              hStdInput;
    HANDLE              hStdOutput;
    HANDLE              hStdError;
    UNICODE_STRING      CurrentDirectoryName;
    HANDLE              CurrentDirectoryHandle;
    UNICODE_STRING      DllPath;
    UNICODE_STRING      ImagePathName;
    UNICODE_STRING      CommandLine;
    PWSTR               Environment;
    ULONG               dwX;
    ULONG               dwY;
    ULONG               dwXSize;
    ULONG               dwYSize;
    ULONG               dwXCountChars;
    ULONG               dwYCountChars;
    ULONG               dwFillAttribute;
    ULONG               dwFlags;
    ULONG               wShowWindow;
    UNICODE_STRING      WindowTitle;
    UNICODE_STRING      Desktop;
    UNICODE_STRING      ShellInfo;
    UNICODE_STRING      RuntimeInfo;
    RTL_DRIVE_LETTER_CURDIR DLCurrentDirectory[0x20];
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

typedef struct _PEB {
    BYTE Reserved1[2];
    BYTE BeingDebugged;
    BYTE Reserved2[1];
    PVOID Reserved3[2];
    PPEB_LDR_DATA Ldr;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    BYTE Reserved4[104];
    PVOID Reserved5[52];
    /*PPS_POST_PROCESS_INIT_ROUTINE*/ PVOID PostProcessInitRoutine;
    BYTE Reserved6[128];
    PVOID Reserved7[1];
    ULONG SessionId;
} PEB, *PPEB;

typedef struct _PROCESS_BASIC_INFORMATION {
    PVOID Reserved1;
    PPEB PebBaseAddress;
    PVOID Reserved2[2];
    /*ULONG_PTR*/ UINT_PTR UniqueProcessId;
    PVOID Reserved3;
} PROCESS_BASIC_INFORMATION;

// ntdll.dll
typedef DWORD (NTAPI *SicNtQuerySystemInformation)(DWORD, PVOID, ULONG, PULONG);

typedef DWORD (NTAPI *SicNtQueryProcessInformation)(HANDLE, DWORD, PVOID, ULONG, PULONG);

// kernel32.dll
typedef BOOL (WINAPI *SicKernelMemoryStatus)(MEMORYSTATUSEX *);

/* iphlpapi.dll */
typedef DWORD (WINAPI *SicIphlpapiGetIpForwardTable)(PMIB_IPFORWARDTABLE, PULONG, BOOL);

typedef DWORD (WINAPI *SicIphlpapiGetIpAddrTable)(PMIB_IPADDRTABLE, PULONG, BOOL);

typedef DWORD (WINAPI *SicIphlpapiGetIfTable)(PMIB_IFTABLE, PULONG, BOOL);

typedef DWORD (WINAPI *SicIphlpapiGetIfEntry)(PMIB_IFROW);

typedef DWORD (WINAPI *SicIphlpapiGetNumIf)(PDWORD);

typedef DWORD (WINAPI *SicIphlpapiGetTcpTable)(PMIB_TCPTABLE, PDWORD, BOOL);

typedef DWORD (WINAPI *SicIphlpapiGetUdpTable)(PMIB_UDPTABLE, PDWORD, BOOL);

typedef DWORD (WINAPI *SicIphlpapiGetTcpStats)(PMIB_TCPSTATS);

typedef DWORD (WINAPI *SicIphlpapiGetNetParams)(PFIXED_INFO, PULONG);

typedef DWORD (WINAPI *SicIphlpapiGetAdaptersInfo)(PIP_ADAPTER_INFO, PULONG);

typedef BOOL (WINAPI *SicMprGetNetConnection)(LPCTSTR, LPTSTR, LPDWORD);

// psapi
typedef BOOL (WINAPI *SicPsapiEnumModules)(HANDLE, HMODULE *, DWORD, LPDWORD);

typedef DWORD (WINAPI *SicPsapiGetModuleName)(HANDLE, HMODULE, LPTSTR, DWORD);

typedef BOOL (WINAPI *SICPsapiEnumProcesses)(DWORD *, DWORD, DWORD *);

#endif //WIN32
#endif //SIC_INCLUDE_SYSTEM_INFORMATION_COLLECTOR_OS_H_
