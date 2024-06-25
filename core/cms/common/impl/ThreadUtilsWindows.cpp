//
// Created by 韩呈杰 on 2024/5/11.
//
#ifdef WIN32
#include <windows.h>
// #else
//
// #ifndef _GNU_SOURCE
// // see https://man7.org/linux/man-pages/man3/pthread_setname_np.3.html
// #   define _GNU_SOURCE
// #endif
//
// #endif
//
//
//
// #ifdef WIN32
#include "common/ThreadUtils.h"
#include <locale>
#include <codecvt>
#include <string>

#include "common/ScopeGuard.h"
#include "common/StringUtils.h"
#include "common/Common.h" // GetThisThreadId

// The GetThreadDescription API is available since Windows 10, version 1607.
// The reason why this API is bound in this way rather than just using the
// Windows SDK, is that this API isn't yet available in the SDK that Chrome
// builds with.
// Binding SetThreadDescription API in Chrome can only be done by
// GetProcAddress, rather than the import library.
typedef HRESULT (WINAPI *FnGetThreadDescription)(HANDLE hThread, PWSTR* threadDescription);
typedef HRESULT (WINAPI *FnSetThreadDescription)(HANDLE hThread, PCWSTR lpThreadDescription);

static HMODULE hKernelModule = ::GetModuleHandle("kernel32.dll");

template<typename F>
F GetKernel32ProcAddressImpl(const char* procName) {
	return hKernelModule ? reinterpret_cast<F>(::GetProcAddress(hKernelModule, procName)) : nullptr;
}
#define GetKernel32ProcAddress(F) GetKernel32ProcAddressImpl<Fn ## F>(#F)

bool IsSupportThreadName() {
    return nullptr != GetKernel32ProcAddress(SetThreadDescription);
}

void SetThreadName(const std::string &name) {
	auto SetThreadDescription = GetKernel32ProcAddress(SetThreadDescription);
    if (SetThreadDescription) {
        std::wstring wString = to_wstring(name);
        if (wString.size() > MAX_THREAD_NAME_LEN) {
            wString = wString.substr(0, MAX_THREAD_NAME_LEN);
        }
        SetThreadDescription(GetCurrentThread(), wString.data());
    }
//     // See https://learn.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2015/debugger/how-to-set-a-thread-name-in-native-code?view=vs-2015&redirectedfrom=MSDN
//     THREADNAME_INFO info;
//     info.dwType = 0x1000;
//     info.szName = threadName;
//     info.dwThreadID = -1;
//     info.dwFlags = 0;
//
// #pragma warning(push)
// #pragma warning(disable: 6320 6322)
//     __try{
//         RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
//     }
//     __except (EXCEPTION_EXECUTE_HANDLER){
//     }
// #pragma warning(pop)
}

std::string GetThreadName(TID tid) {
    std::string name;

    auto GetThreadDescription = GetKernel32ProcAddress(GetThreadDescription);
    if (GetThreadDescription) {
        bool isThis = (tid <= 0 || tid == GetThisThreadId());
        HANDLE hThread = (isThis? GetCurrentThread(): OpenThread(THREAD_QUERY_INFORMATION, FALSE, tid));
        if (hThread != nullptr) {
            defer(if (isThis) CloseHandle(hThread));

            PWSTR data = nullptr;
            HRESULT hr = GetThreadDescription(hThread, &data);
            if (SUCCEEDED(hr)) {
                name = from_wstring(data);
                LocalFree(data);
            }
        }
    }

    return name;
}

#endif
