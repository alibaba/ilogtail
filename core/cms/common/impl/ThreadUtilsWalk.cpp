//
// Created by 韩呈杰 on 2024/5/11.
//
#if !defined(WITHOUT_MINI_DUMP)
#include "common/ThreadUtils.h"

#include "common/BoostStacktraceMt.h"
#include "common/Common.h"  // GetThisThreadId
#include "common/SyncQueue.h"
#include "common/Defer.h"

#include <mutex>
#include <thread>
#include <list>
#include <atomic>

#include <boost/stacktrace.hpp>

using namespace std;

/// https://github.com/albertz/openlierox/blob/0.59/src/common/Debug_GetCallstack.cpp

// When implementing iterating over threads on Mac, this might be useful:
// http://llvm.org/viewvc/llvm-project/lldb/trunk/tools/darwin-threads/examine-threads.c?view=markup

// For getting the callback, maybe libunwind can be useful: http://www.nongnu.org/libunwind/

#ifndef HAVE_EXECINFO
#	if defined(__linux__)
#		define HAVE_EXECINFO 1
#	elif defined(__DARWIN_VERS_1050)
#		define HAVE_EXECINFO 1
#	else
#		define HAVE_EXECINFO 0
#	endif
#endif

#if HAVE_EXECINFO
#include <chrono>
#include <execinfo.h>
#include <csignal>
#include <cstdio>
// See: https://stackoverflow.com/questions/27789033/pthread-kill-gives-segmentation-fault-when-called-from-second-thread
#include <pthread.h>

typedef pthread_t ThreadId;
#else
#include <Windows.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <dbghelp.h> // StackWalk
#pragma comment(lib, "dbghelp.lib")

#include <boost/stacktrace/frame.hpp>

typedef HANDLE ThreadId; // Win32
#endif

tagThreadStack::tagThreadStack():
        tagThreadStack("") {
}

tagThreadStack::tagThreadStack(const std::string &trace) :
        tagThreadStack(GetThisThreadId(), trace) {
}

tagThreadStack::tagThreadStack(int threadId, const std::string &trace) {
    this->threadId = threadId;
    this->threadName = GetThreadName(threadId);
    this->stackTrace = trace;
}

std::string tagThreadStack::str(int index) const {
    std::stringstream ss;
    if (index >= 0) {
        ss << "[" << index << "] ";
    }
    ss << "ThreadId: " << threadId << ", ThreadName: " << threadName << ", stack: " << std::endl << stackTrace;
    return ss.str();
}

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if HAVE_EXECINFO
static std::mutex callstackMutex;
static std::atomic<SyncQueue<tagThreadStack> *> threadCallStacks(nullptr);

#define CALLSTACK_SIG SIGUSR2

__attribute__((noinline))
static void callstack_signal_handler(int signr, siginfo_t *info, void *secret) {
    using namespace boost::stacktrace;
    // skip this function
    // skip caller, usually in pthread
    *threadCallStacks << tagThreadStack(mt::to_string(stacktrace(2, std::numeric_limits<size_t>::max())));
}

static std::once_flag once;

static void setup_callstack_signal_handler() {
    std::call_once(once, []() {
        struct sigaction sa;
        sigfillset(&sa.sa_mask);
        sa.sa_flags = SA_SIGINFO;
        sa.sa_sigaction = callstack_signal_handler;
        sigaction(CALLSTACK_SIG, &sa, NULL);
    });
}

// ThreadId ThreadSelf() {
//     return pthread_self();
// }

// bool is_pthread_alive(pthread_t tid) {
//     if (tid) {
//         int ret = pthread_tryjoin_np(tid, NULL);
//         // see: https://blog.csdn.net/jctian000/article/details/80362656
//         return ret == EBUSY || ret == 22;
//     }
//     return false;
// }

__attribute__((noinline))
void WalkThreadStack(std::list<tagThreadStack> &lstTrace) {
    std::lock_guard<std::mutex> lock(callstackMutex);

    std::map<int, pthread_t> threads;
    EnumThreads(threads);

    SyncQueue<tagThreadStack> queue(lstTrace);
    {
        threadCallStacks = &queue;
        defer(threadCallStacks = nullptr);

        setup_callstack_signal_handler();

        size_t expectCount = 0;
        for (const auto &thread: threads) {
            // call _callstack_signal_handler in target thread
            if (pthread_kill(thread.second, CALLSTACK_SIG) == 0) {
                expectCount++;
            } // else fail, ignore
        }
        if (expectCount > 0) {
            // 最多等待5秒
            using namespace std::chrono;
            steady_clock::time_point expire = steady_clock::now() + seconds{1};
            while (steady_clock::now() < expire && queue.Count() < expectCount) {
                std::this_thread::sleep_for(milliseconds{100});
            }
        }
    }
}

#else // !HAVE_EXECINFO

// 该函数不能对当前线程使用
static std::string StackTrack(HANDLE hProcess, DWORD threadId)
{
    HANDLE hThread = OpenThread(THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, threadId);
    defer(CloseHandle(hThread));

    // SuspendThread(hThread);
    // defer(ResumeThread(hThread));

    CONTEXT context = { 0 };
    context.ContextFlags = CONTEXT_ALL;
    GetThreadContext(hThread, &context);
    //__asm {call $ + 5}
    //__asm {pop eax}
    //__asm {mov context.Eip, eax}
    //__asm {mov context.Ebp, ebp}
    //__asm {mov context.Esp, esp}

    auto offset = [](ADDRESS& adr) -> decltype(adr.Offset) & {
        adr.Mode = AddrModeFlat;
        adr.Offset = 0;
        adr.Segment = 0;
        return adr.Offset;
    };
    STACKFRAME stackFrame = { 0 };
#if defined(_M_IX86)
#   define OS_DWORD DWORD
    const DWORD dwMachineType = IMAGE_FILE_MACHINE_I386;
    offset(stackFrame.AddrPC) = context.Eip;
    offset(stackFrame.AddrFrame) = context.Ebp;
    offset(stackFrame.AddrStack) = context.Esp;
#elif defined(_M_IA64)
#   define OS_DWORD DWORD64
    const DWORD dwMachineType = IMAGE_FILE_MACHINE_IA64;
    offset(stackFrame.AddrPC) = context.StIIP;
    offset(stackFrame.AddrFrame) = context.IntSp;
    offset(stackFrame.AddrStack) = context.IntSp;
    offset(stackFrame.AddrBStore) = context.RsBSP;
#elif defined(_M_X64)
#   define OS_DWORD DWORD64
    const DWORD dwMachineType = IMAGE_FILE_MACHINE_AMD64;
    offset(stackFrame.AddrPC) = context.Rip;
    offset(stackFrame.AddrFrame) = context.Rsp;
    offset(stackFrame.AddrStack) = context.Rsp;
#else
#error unknown cpu architecture
#endif

    typedef struct tag_SYMBOL_INFO
    {
        IMAGEHLP_SYMBOL symInfo;
        TCHAR szBuffer[MAX_PATH];
    } SYMBOL_INFO, * LPSYMBOL_INFO;

    OS_DWORD dwDisplament = 0;
    SYMBOL_INFO stack_info = { 0 };
    PIMAGEHLP_SYMBOL pSym = (PIMAGEHLP_SYMBOL)&stack_info;
    pSym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
    pSym->MaxNameLength = sizeof(SYMBOL_INFO) - offsetof(SYMBOL_INFO, symInfo.Name);
    //IMAGEHLP_LINE ImageLine = { 0 };
    //ImageLine.SizeOfStruct = sizeof(IMAGEHLP_LINE);

    std::vector<boost::stacktrace::frame> frames;
    frames.reserve(128);
    while (StackWalk(dwMachineType, hProcess, hThread, &stackFrame, &context, NULL, SymFunctionTableAccess, SymGetModuleBase, NULL))
    {
        SymGetSymFromAddr(hProcess, stackFrame.AddrPC.Offset, &dwDisplament, pSym);
        //SymGetLineFromAddr(hProcess, stackFrame.AddrPC.Offset, &dwDisplament, &ImageLine);
        //printf("%2d#: %08x+%s(FILE[%s]LINE[%d])\n", index++, pSym->Address, pSym->Name, ImageLine.FileName, ImageLine.LineNumber);
        frames.push_back(boost::stacktrace::frame((void*)pSym->Address));
    }
    return boost::stacktrace::mt::to_string(frames);
}

BOOL ListProcessThreads(std::list<tagThreadStack>& lstTrace)
{
    const pid_t dwOwnerPID = GetPid();

    HANDLE hProcess = GetCurrentProcess();
    defer(CloseHandle(hProcess));

    SymInitialize(hProcess, NULL, TRUE);
    defer(SymCleanup(hProcess));

    // Take a snapshot of all running threads
    // zero of the seconds parameter means the current process
    HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hThreadSnap == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    defer(CloseHandle(hThreadSnap));

    THREADENTRY32 te32;
    // Fill in the size of the structure before using it.
    te32.dwSize = sizeof(THREADENTRY32);

    // Retrieve information about the first thread,
    // and exit if unsuccessful
    if (!Thread32First(hThreadSnap, &te32))
    {
        //printError(TEXT("Thread32First"));  // Show cause of failure
        //CloseHandle(hThreadSnap);     // Must clean up the snapshot object!
        return FALSE;
    }

    // Now walk the thread list of the system,
    // and display information about each thread
    // associated with the specified process
    do
    {
        if (te32.th32OwnerProcessID == dwOwnerPID)
        {
            //_tprintf(TEXT("     THREAD ID      = %d%s\n"), te32.th32ThreadID, (te32.th32ThreadID == GetCurrentThreadId()? " <--": ""));
            //_tprintf(TEXT("     base priority  = %d\n"), te32.tpBasePri);
            //_tprintf(TEXT("     delta priority = %d\n"), te32.tpDeltaPri);
            std::string strTrace;
            if (te32.th32ThreadID == GetCurrentThreadId()) {
                strTrace = boost::stacktrace::mt::to_string(boost::stacktrace::stacktrace());
            } else {
                strTrace = StackTrack(hProcess, te32.th32ThreadID);
            }
            lstTrace.emplace_back(te32.th32ThreadID, strTrace);
        }
    } while (Thread32Next(hThreadSnap, &te32));

    //_tprintf(TEXT("\n"));

    //  Don't forget to clean up the snapshot object.
    //CloseHandle(hThreadSnap);
    return(TRUE);
}

void WalkThreadStack(std::list<tagThreadStack>& lstTrace) {
    ListProcessThreads(lstTrace);
}

#endif

void WalkThreadStack(const std::function<void(int, const tagThreadStack &)> &fnPrint) {
    std::list<tagThreadStack> lstTrace;
    WalkThreadStack(lstTrace);

    int index = 0;
    for (const auto &stack: lstTrace) {
        fnPrint(++index, stack);
    }
}
#endif // !WITHOUT_MINI_DUMP