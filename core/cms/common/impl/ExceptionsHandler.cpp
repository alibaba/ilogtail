/*
相关文档：
https://github.com/chronoxor/CppCommon/blob/master/source/errors/exceptions_handler.cpp # CppComon的exceptions_handler源码
https://blog.csdn.net/u011305137/article/details/38727873 # 《异常处理与MiniDump详解(4) MiniDump》
https://www.cnblogs.com/lehoho/p/9364974.html  # 《异常处理与MiniDump详解(4) MiniDump》
https://chronoxor.github.io/CppCommon/exceptions__handler_8cpp_source.html
https://github.com/hanbo79/CrashProcMinidumpDll/tree/master/CrashProcCtrlStaticDll
https://www.cnblogs.com/FCoding/archive/2012/07/04/2576890.html  # 《Runtime不产生core dump文件的解决办法》
https://www.cnblogs.com/daxingxing/p/3986262.html # 《_set_invalid_parameter_handler异常处理函数》
*/
#include "common/ExceptionsHandler.h"
#include "common/Config.h"

#ifdef WIN32
#include <iostream>
#include "common/Base64.h"
#endif

#if defined(_WINDOWS)
#   include <io.h>                // _get_osfhandle, _fileno
#   define fileno _fileno

#   include <tchar.h>             // _T
#   include <processthreadsapi.h> // TerminateProcess
#   include <new.h>               // _set_new_handler这个是C++的特性

#   ifdef _MSC_VER
#       pragma warning(push)
#       pragma warning(disable : 4091) // C4091: 'keyword' : ignored on left of 'type' when no variable is declared
#   endif
#   include <Dbghelp.h>
#   ifdef _MSC_VER
#       pragma warning(pop)
#       pragma comment(lib, "DbgHelp")
#   endif // _MSC_VER
#endif // _WINDOWS

#include <sstream>
#include <mutex>
#include <csignal>
#include <string>
#include <unordered_map>

#include <boost/stacktrace.hpp>

#include <typeinfo>
#include <boost/core/demangle.hpp>

#include "common/TimeFormat.h"
#include "common/ThrowWithTrace.h"
#include "common/BoostStacktraceMt.h"
#include "common/FilePathUtils.h"
#include "common/ChronoLiteral.h"
#include "common/Defer.h"
#include "common/Logger.h"
#include "common/Common.h"        // typeName
#ifdef WIN32
#include "common/FileUtils.h"    // ReadFileBinary
#endif
#include "common/ThreadUtils.h" // SetThreadName

using namespace std;
using namespace common;

// #pragma auto_inline(off) // 避免内联，以便于观察调用栈
struct tagSignalInfo {
    const char *name;
    const char *prompt;
};
#define SIGNAL_ENTRY(SIGNO, PROMPT) {SIGNO, tagSignalInfo{#SIGNO, PROMPT}}
static const auto &mapSignalPrompt = *new std::unordered_map<int, tagSignalInfo>{
        SIGNAL_ENTRY(SIGABRT, "Caught abnormal program termination (SIGABRT) signal"),
        SIGNAL_ENTRY(SIGINT, "Caught terminal interrupt (SIGINT) signal"),
        SIGNAL_ENTRY(SIGTERM, "Caught termination request (SIGTERM) signal"),
        SIGNAL_ENTRY(SIGFPE, "Caught floating point exception (SIGFPE) signal"),
        SIGNAL_ENTRY(SIGILL, "Caught illegal instruction (SIGILL) signal"),
        SIGNAL_ENTRY(SIGSEGV, "Caught illegal storage access error (SIGSEGV) signal"),
#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
        SIGNAL_ENTRY(SIGALRM, "Caught alarm clock (SIGALRM) signal"),
        SIGNAL_ENTRY(SIGBUS, "Caught memory access error (SIGBUS) signal"),
        SIGNAL_ENTRY(SIGHUP, "Caught hangup instruction (SIGHUP) signal"),
        SIGNAL_ENTRY(SIGPIPE, "Caught pipe write error (SIGPIPE) signal"),
        SIGNAL_ENTRY(SIGPROF, "Caught profiling timer expired error (SIGPROF) signal"),
        SIGNAL_ENTRY(SIGQUIT, "Caught terminal quit (SIGQUIT) signal"),
        SIGNAL_ENTRY(SIGSYS, "Caught bad system call (SIGSYS) signal"),
        SIGNAL_ENTRY(SIGXCPU, "Caught CPU time limit exceeded (SIGXCPU) signal"),
        SIGNAL_ENTRY(SIGXFSZ, "Caught file size limit exceeded (SIGXFSZ) signal"),
#endif
};

const tagSignalInfo *getSignalInfo(int signo) {
    const tagSignalInfo *signalInfo = nullptr;
    if (signo) {
        auto it = mapSignalPrompt.find(signo);
        if (it != mapSignalPrompt.end()) {
            signalInfo = &it->second;
        }
    }
    return signalInfo;
}

const char *signalName(int signo, const char *def) {
    const char *name = def;
    const tagSignalInfo *signalInfo = getSignalInfo(signo);
    if (signalInfo != nullptr) {
        name = signalInfo->name;
    }
    return name;
}

static const char *getSignalPrompt(int signo, const char *def = nullptr) {
    const char *prompt = def;
    const tagSignalInfo *signalInfo = getSignalInfo(signo);
    if (signalInfo != nullptr) {
        prompt = signalInfo->prompt;
    }
    return prompt;
}

// Output error
std::string signalPrompt(int signo) {
    std::string prompt = getSignalPrompt(signo, "");
    if (prompt.empty()) {
        prompt = fmt::format("Caught unknown signal - {}", signo);
    }
    return prompt;
}

static std::once_flag dump_once_flag;
static fs::path dump_directory;
static std::string dump_file_name_base;
static const char dump_file_ext[] = ".dmp"; // dump文件的扩展名

std::string new_dump_file_path(int signo) {
    std::call_once(dump_once_flag, [&]() {
        dump_directory = SingletonConfig::Instance()->getLogDir();
        if (dump_directory.empty()) {
            // 跟EXE在同一文件夹下，方便打包
            fs::path execPath = GetExecPath();
            dump_directory = execPath.parent_path();
        }
        LogInfo("dump directory: {}", dump_directory.string());
        fs::create_directories(dump_directory);

        std::string name = GetExecPath().stem().string();
        dump_file_name_base = name + ".";
    });

    std::stringstream ss;
    ss << dump_file_name_base;

    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::string dt = date::format(now, "L%Y%m%d-%H%M%S");
    ss << dt;

    const char *name = signalName(signo);
    if (name != nullptr) {
        ss << "." << name;
    }

    ss << dump_file_ext;

    return (dump_directory / ss.str()).string();
}

struct tagDumpFileParser {
    std::string stacktrace;
    bool succeed = false;
    std::string dumpTime;
    fs::path dumpFile;
    uint64_t dumpFileSize;

    tagDumpFileParser(const fs::path &file, std::chrono::seconds timeout) {
        using namespace std::chrono;
        succeed = false;

        boost::system::error_code ec;
        std::time_t t = boost::filesystem::last_write_time(file, ec);
        if (!ec.failed()) {
            system_clock::time_point coreDownTime = system_clock::from_time_t(t);
            if (coreDownTime + timeout >= system_clock::now()) {
                dumpTime = date::format<0>(coreDownTime);
                succeed = true;
                dumpFile = file;
                dumpFileSize = fs::file_size(file);
#ifndef WIN32
                std::ifstream ifs(file.string());
                defer(ifs.close());
                using namespace boost::stacktrace;
                stacktrace = mt::to_string(stacktrace::from_dump(ifs));
#endif
            }
        }
#ifdef ENABLE_COVERAGE
        else {
            LogError("{}: {}", file.string(), ec.message());
        }
#endif
    }
};

static std::string FormatDumpInfo(const fs::path &file) {
    std::stringstream ss;

    tagDumpFileParser parser(file, 1_h);
    if (parser.succeed) {
        ss << "CORE DUMP (" << file << ") at: " << parser.dumpTime;
#ifdef WIN32
        ss << ", PLEASE contact to cms team to solve it!!!";
#else
        ss << std::endl << parser.stacktrace;
#endif
    }

    return ss.str();
}

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DumpCache
DumpCache::DumpCache(size_t cap, bool readonly) : clean(!readonly), capacity(cap) {
    Clean();
}

bool existDumpFile(const std::vector<fs::path> &dumpFiles) {
    return !dumpFiles.empty() && (std::string::npos == dumpFiles[0].filename().string().find("SIGQUIT"));
}

bool DumpCache::PrintLastCoreDown() {
    bool dumped = false;
    // SIGQUIT是主动退出，就不打日志了
    if (existDumpFile(dumpFiles)) {
        std::string dumpStr = FormatDumpInfo(dumpFiles[0]);
        dumped = !dumpStr.empty();
        if (dumped) {
            LogError("{}", dumpStr);
        }
    }
    return dumped;
}

void DumpCache::Append(const fs::path &dumpFile) {
    auto it = dumpFiles.begin();
    for (; it != dumpFiles.end(); it++) {
        if (*it <= dumpFile) {
            break;
        }
    }
    dumpFiles.insert(it, dumpFile);
    if (dumpFiles.size() > capacity) {
        if (clean) {
            LogInfo("remove[{}] {}", (removeCount++), dumpFiles.rbegin()->string());
            boost::filesystem::remove(*dumpFiles.rbegin());
        }
        dumpFiles.pop_back();
    }
}

void DumpCache::Clean() {
    LogInfo("{}({}/**/*{})", __FUNCTION__, dump_directory.string(), dump_file_ext);
    if (fs::exists(dump_directory)) {
        const auto &skip_denied = fs::directory_options::skip_permission_denied;
        for (auto const &it: fs::directory_iterator{dump_directory, skip_denied}) {
            fs::path file = it.path();
            // LogDebug("file: {}, ext: {}", file.string(), file.extension().string());
            if (fs::is_regular_file(file) && file.extension().string() == dump_file_ext) {
                this->Append(file);
            }
        }
    }
}

boost::json::object tagCoreDumpFile::toJson() const {
    std::string signalName = filename;
    if (HasSuffix(signalName, dump_file_ext)) {
        signalName = signalName.substr(0, signalName.size() - (sizeof(dump_file_ext) - 1));
    }
    size_t pos = signalName.find_last_of('.');
    if (pos != std::string::npos) {
        signalName = signalName.substr(pos + 1);
    }
    signalName = HasPrefix(signalName, "SIG") ? signalName : std::string{};

    boost::json::object obj{
            {"dumpTime",      dumpTime},
            {"filename",      filename},
            {"fileSize",      filesize},
            {"callstackType", callstackType},
            {"callstack",     callstack},
    };
    if (!signalName.empty()) {
        obj["signal"] = signalName;
    }

    return obj;
}

tagCoreDumpFile DumpCache::GetLastCoreDown(std::chrono::seconds timeout) const {
    tagCoreDumpFile ret;
    if (existDumpFile(dumpFiles)) {
        tagDumpFileParser parser(dumpFiles[0], timeout);
        if (parser.succeed) {
            ret.filename = parser.dumpFile.string();
            ret.dumpTime = parser.dumpTime;
            ret.filesize = parser.dumpFileSize;
#ifdef WIN32
            ret.callstackType = "Base64";
            std::vector<char> bin = ReadFileBinary(parser.dumpFile);
            ret.callstack = common::Base64::encode(reinterpret_cast<unsigned char*>(&bin[0]),
                static_cast<unsigned int>(bin.size()));
#else
            ret.callstackType = "Text";
            ret.callstack = parser.stacktrace;
#endif
        }
    }
    return ret;
}

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
void printStacktrace(int signo, const std::string &caller, size_t skip) {
    using namespace boost::stacktrace;
    std::string strStackTrace = mt::to_string(stacktrace(skip, detail::max_frames_dump));
    const char *name = signalName(signo, "");
    LogError("{}({}): \n{}", caller, (name == nullptr ? "" : name), strStackTrace);
}

#if defined(_WINDOWS)

static void GetExceptionPointers(DWORD dwExceptionCode, EXCEPTION_POINTERS **ppExceptionPointers);
static void DeleteExceptionPointers(EXCEPTION_POINTERS *pExceptionPointers);

std::mutex UnhandledExceptionMutex;

LONG WINAPI MyUnhandledExceptionFilter(struct _EXCEPTION_POINTERS *ExceptionInfo, int signo, bool terminate = true)
{
    // Windows的SEH不适合boost的stacktrace
    printStacktrace(signo, "UnhandledExceptionFilter");

    std::lock_guard<std::mutex> _guard{UnhandledExceptionMutex};

    // HANDLE lhDumpFile = CreateFile(_T("DumpFile.dmp"), GENERIC_WRITE, 0, NULL,
    //     CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    std::string fileName = new_dump_file_path(signo);
    FILE *fp = fopen(fileName.c_str(), "w");
    if (fp != nullptr)
    {
        defer(fclose(fp));
        HANDLE lhDumpFile = (HANDLE)_get_osfhandle(fileno(fp));

        MINIDUMP_EXCEPTION_INFORMATION loExceptionInfo;
        loExceptionInfo.ExceptionPointers = ExceptionInfo;
        loExceptionInfo.ThreadId = GetCurrentThreadId();
        loExceptionInfo.ClientPointers = TRUE;

        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), lhDumpFile, MiniDumpNormal, &loExceptionInfo, NULL, NULL);

        // CloseHandle(lhDumpFile);
        // std::cout << "complete dump " << fileName << std::endl;
        LogError("dump to: {}", fileName);
    }

    // EXCEPTION_CONTINUE_EXECUTION(-1)   异常被忽略，控制流将在异常出现的点之后，继续恢复运行。
    // EXCEPTION_CONTINUE_SEARCH(0)       异常不被识别，也即当前的这个__except模块不是这个异常错误所对应的正确的异常处理模块。 系统将继续到上个__try - __except域中继续查找一个恰当的__except模块。
    // EXCEPTION_EXECUTE_HANDLER(1)       异常已经被识别，控制流将进入到__except模块中运行异常处理代码
    return terminate ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH; // 如果不是SEH，那么程序到这里就结束了
}

LONG WINAPI MyUnhandledExceptionFilter00(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
#ifndef NDEBUG
    std::cout << "MyUnhandledExceptionFilter00" << std::endl;
#endif
    return MyUnhandledExceptionFilter(ExceptionInfo, 0);
}

LONG WINAPI ArgusUnhandledExceptionFilter(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
#ifndef NDEBUG
    std::cout << "ArgusUnhandledExceptionFilter" << std::endl;
#endif
    return MyUnhandledExceptionFilter(ExceptionInfo, 0);
}

void ProcessUnhandledException(int signo, bool terminate = true)
{
    EXCEPTION_POINTERS *pExceptionPtrs = nullptr;
    GetExceptionPointers(0, &pExceptionPtrs);
    MyUnhandledExceptionFilter(pExceptionPtrs, signo, terminate);
    DeleteExceptionPointers(pExceptionPtrs);
    if (terminate)
    {
        TerminateProcess(GetCurrentProcess(), 1);
    }
}

#if defined(_MSC_VER)

void InvalidParameterHandler(const wchar_t *expression,
                             const wchar_t *function,
                             const wchar_t *file,
                             unsigned int line,
                             uintptr_t pReserved)
{
    // function、file、line在Release下无效
#ifndef NDEBUG
    wprintf(L"Invalid parameter detected in function %s. File: %s Line: %d\n", function, file, line);
    wprintf(L"Expression: %s\n", expression);
#endif
    ThrowWithTrace<std::invalid_argument>("InvalidParameterException");
}

class PureCallException : public std::runtime_error
{
public:
    PureCallException() : std::runtime_error("PureCallException")
    {
    }
};

void PureCallHandler(void)
{
    // std::cout << "Pure virtual function call" << std::endl
    //           << boost::stacktrace::stacktrace();
    // ProcessUnhandledException();
    // 必须抛出异常，否则无法定位错误位置
    ThrowWithTrace(PureCallException{});
}

class NewOperatorException : public std::out_of_range
{
public:
    NewOperatorException(size_t size) : std::out_of_range(prompt(size))
    {
    }

private:
    static std::string prompt(size_t size)
    {
        std::stringstream ss;
        ss << "new(" << size << ") operator memory allocation exception";
        return ss.str();
    }
};

static int __cdecl NewHandler(size_t size)
{
    LogError("'new({})' operator memory allocation exception:\n{}", size,
        boost::stacktrace::mt::to_string(boost::stacktrace::stacktrace()));
    // std::cout << "'new(" << size << ")' operator memory allocation exception" << std::endl
    //           << boost::stacktrace::stacktrace();
    // ProcessUnhandledException();
    // // unreachable code
    // throw 1;
    ThrowWithTrace(NewOperatorException{size});
    return 0; // unreachable code
}

// CRT terminate() call handler
static void __cdecl TerminateHandler()
{
    // Output error
    LogError("Abnormal program termination (terminate() function was called): \n{}",
              boost::stacktrace::mt::to_string(boost::stacktrace::stacktrace()));

    ProcessUnhandledException(0);
}

// CRT unexpected() call handler
static void __cdecl UnexpectedHandler()
{
    // Output error
   LogError("Unexpected error (unexpected() function was called): {}",
              boost::stacktrace::mt::to_string(boost::stacktrace::stacktrace()));

    ProcessUnhandledException(0);
}
#endif // _MSC_VER

// CRT SIGABRT signal handler
static void SignalHandlerWin32(int signum)
{
    // Output error
    ProcessUnhandledException(signum);
}

static void GetExceptionPointers(DWORD dwExceptionCode, EXCEPTION_POINTERS **ppExceptionPointers)
{
    CONTEXT ContextRecord;
    memset(&ContextRecord, 0, sizeof(CONTEXT));

    EXCEPTION_RECORD ExceptionRecord;
    memset(&ExceptionRecord, 0, sizeof(EXCEPTION_RECORD));

#if defined(_M_IX86)
    // On x86, we reserve some extra stack which won't be used.  That is to
    // preserve as much of the call frame as possible when the function with
    // the buffer overrun entered __security_check_cookie with a JMP instead
    // of a CALL, after the calling frame has been released in the epilogue
    // of that function.
    volatile ULONG dw[(sizeof(CONTEXT) + sizeof(EXCEPTION_RECORD)) / sizeof(ULONG)];

    // Save the state in the context record immediately.  Hopefully, since
    // opts are disabled, this will happen without modifying ECX, which has
    // the local cookie which failed the check.
    __asm
        {
             mov dword ptr [ContextRecord.Eax  ], eax
             mov dword ptr [ContextRecord.Ecx  ], ecx
             mov dword ptr [ContextRecord.Edx  ], edx
             mov dword ptr [ContextRecord.Ebx  ], ebx
             mov dword ptr [ContextRecord.Esi  ], esi
             mov dword ptr [ContextRecord.Edi  ], edi
             mov word ptr  [ContextRecord.SegSs], ss
             mov word ptr  [ContextRecord.SegCs], cs
             mov word ptr  [ContextRecord.SegDs], ds
             mov word ptr  [ContextRecord.SegEs], es
             mov word ptr  [ContextRecord.SegFs], fs
             mov word ptr  [ContextRecord.SegGs], gs
             pushfd
             pop [ContextRecord.EFlags]

            // Set the context EBP/EIP/ESP to the values which would be found
            // in the caller to __security_check_cookie.
             mov eax, [ebp]
             mov dword ptr [ContextRecord.Ebp], eax
             mov eax, [ebp + 4]
             mov dword ptr [ContextRecord.Eip], eax
             lea eax, [ebp + 8]
             mov dword ptr [ContextRecord.Esp], eax

                                                                                                                                                                             // Make sure the dummy stack space looks referenced.
             mov eax, dword ptr dw
        }

    ContextRecord.ContextFlags = CONTEXT_CONTROL;
    ExceptionRecord.ExceptionAddress = (PVOID)(ULONG_PTR)ContextRecord.Eip;
#elif defined(_M_X64)
    ULONG64 ControlPc;
    ULONG64 EstablisherFrame;
    ULONG64 ImageBase;
    PRUNTIME_FUNCTION FunctionEntry;
    PVOID HandlerData;

    RtlCaptureContext(&ContextRecord);

    ControlPc = ContextRecord.Rip;
    FunctionEntry = RtlLookupFunctionEntry(ControlPc, &ImageBase, nullptr);

    if (FunctionEntry != nullptr)
    {
        RtlVirtualUnwind(UNW_FLAG_NHANDLER, ImageBase, ControlPc, FunctionEntry, &ContextRecord, &HandlerData, &EstablisherFrame, nullptr);
    }

#if defined(__GNUC__)
    void *return_address = __builtin_return_address(0);
    ContextRecord.Rip = (ULONGLONG)return_address;
    ContextRecord.Rsp = (ULONGLONG)__builtin_extract_return_addr(return_address) + 8;
#elif defined(_MSC_VER)
    ContextRecord.Rip = (ULONGLONG)_ReturnAddress();
    ContextRecord.Rsp = (ULONGLONG)_AddressOfReturnAddress() + 8;
#endif
    ExceptionRecord.ExceptionAddress = (PVOID)ContextRecord.Rip;
#else
#error Unsupported architecture
#endif
    ExceptionRecord.ExceptionCode = dwExceptionCode;
#if defined(__GNUC__)
    ExceptionRecord.ExceptionAddress = __builtin_return_address(0);
#elif defined(_MSC_VER)
    ExceptionRecord.ExceptionAddress = _ReturnAddress();
#endif

    CONTEXT *pContextRecord = new CONTEXT;
    memcpy(pContextRecord, &ContextRecord, sizeof(CONTEXT));

    EXCEPTION_RECORD *pExceptionRecord = new EXCEPTION_RECORD;
    memcpy(pExceptionRecord, &ExceptionRecord, sizeof(EXCEPTION_RECORD));

    *ppExceptionPointers = new EXCEPTION_POINTERS;
    (*ppExceptionPointers)->ContextRecord = pContextRecord;
    (*ppExceptionPointers)->ExceptionRecord = pExceptionRecord;
}

static void DeleteExceptionPointers(EXCEPTION_POINTERS *pExceptionPointers)
{
    delete pExceptionPointers->ContextRecord;
    delete pExceptionPointers->ExceptionRecord;
    delete pExceptionPointers;
}

#elif defined(__unix__) || defined(__linux__) || defined(__APPLE__)

pid_t mainPid = 0;
// Signal handler
static void SignalHandler(int signo, siginfo_t *, void *) {
    if (mainPid == GetPid()) {
        // 当使用类似popen之类的系统调用时，代码会运行在子进程中，此时如果触发了中断，不需要打印日志。(打印时有死锁风险)
        std::string targetFile = new_dump_file_path(signo);
        boost::stacktrace::safe_dump_to(targetFile.c_str());

        // Output error
        LogError("argusagent caught signal {}({}). Exiting...", signalName(signo, ""), signo);
        LogError("{}\n{}", signalPrompt(signo), boost::stacktrace::mt::to_string(boost::stacktrace::stacktrace()));
    }
    // Prepare signal action structure
    struct sigaction sa{};
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_DFL;

    // Set up the default signal handler and rise it!
    int result = sigaction(signo, &sa, nullptr);
    if (result == 0) {
        raise(signo);
    } else {
        kill(getpid(), SIGKILL);
    }
}

#endif

static std::once_flag setupProcessOnce;

void SetupProcess() {
    std::call_once(setupProcessOnce, [&]() {
        new_dump_file_path(0); // 初始化dump相关变量
#ifdef NDEBUG
#   define MAX_DUMP 10
#else
#   define MAX_DUMP 2
#endif
        DumpCache dumpCache(MAX_DUMP);
        dumpCache.PrintLastCoreDown();

#if defined(_WINDOWS)
        // Install top-level SEH handler
        SetUnhandledExceptionFilter(MyUnhandledExceptionFilter00);

#if defined(_MSC_VER)
        // Catch pure virtual function calls
        // Because there is one _purecall_handler for the whole process,
        // calling this function immediately impacts all threads. The last
        // caller on any thread sets the handler.
        // http://msdn.microsoft.com/en-us/library/t296ys27.aspx
        _set_purecall_handler(PureCallHandler);

        // Catch new operator memory allocation exceptions
        _set_new_handler(NewHandler);

        // Catch invalid parameter exceptions
        _set_invalid_parameter_handler(InvalidParameterHandler);

        // Set up C++ signal handlers
        _set_abort_behavior(_CALL_REPORTFAULT, _CALL_REPORTFAULT);
#endif
        // Catch an abnormal program termination
        signal(SIGABRT, SignalHandlerWin32);

        // Catch an illegal instruction error
        signal(SIGINT, SignalHandlerWin32);

        // Catch a termination request
        signal(SIGTERM, SignalHandlerWin32);

        //throw std::logic_error("heheheh");
#elif defined(__unix__) || defined(__linux__) || defined(__APPLE__)
        mainPid = GetPid();

        // Prepare signal action structure
        struct sigaction sa{};
        memset(&sa, 0, sizeof(sa));
        sa.sa_sigaction = SignalHandler;
        sa.sa_flags = SA_SIGINFO;

        // Define signals to catch
        // Setup corresponding signals handlers
        for (const auto &entry: mapSignalPrompt) {
            if (entry.first != SIGQUIT) {
                // 脚本停止进程时使用的kill -3 (SIGQUIT)
                sigaction(entry.first, &sa, nullptr);
            }
        }
#else
#error unsupported OS
#endif
    });

    SetupThread();
}

void SetupThread(const std::string &threadName) {
    if (!threadName.empty()) {
        SetThreadName(threadName);
    }
#if defined(_WINDOWS)
#if defined(_MSC_VER)
    // Catch terminate() calls
    // In a multithreaded environment, terminate functions are maintained
    // separately for each thread. Each new thread needs to install its own
    // terminate function. Thus, each thread is in charge of its own termination handling.
    // http://msdn.microsoft.com/en-us/library/t6fk7h29.aspx
    set_terminate(TerminateHandler);

    // Catch unexpected() calls
    // In a multithreaded environment, unexpected functions are maintained
    // separately for each thread. Each new thread needs to install its own
    // unexpected function. Thus, each thread is in charge of its own unexpected handling.
    // http://msdn.microsoft.com/en-us/library/h46t5b69.aspx
    set_unexpected(UnexpectedHandler);
#endif

    // Catch a floating point exception
    signal(SIGFPE, SignalHandlerWin32);

    // Catch an illegal instruction
    signal(SIGILL, SignalHandlerWin32);

    // Catch an illegal storage access error
    signal(SIGSEGV, SignalHandlerWin32);
#endif
}

// void printException(const char *file, size_t line, const char *func, const std::exception &ex) {
//     // 定位错误位置，输出log
//     std::stringstream ss;
//     ss << ":" << func << "(...), catch(" << boost::core::demangle(typeid(ex).name()) << "): " << ex.what();
//     const boost::stacktrace::stacktrace *st = boost::get_error_info<traced>(ex);
//     if (st) {
//         ss << std::endl << boost::stacktrace::mt::to_string(*st);
//     }
//     LogError("{}", ss.str().c_str());
// }

void safeRun(const std::function<void()> &fn) {
#ifdef _WINDOWS
#ifdef ENABLE_COVERAGE
    __try {
#endif
        fn();
#ifdef ENABLE_COVERAGE
    }
        __except (ArgusUnhandledExceptionFilter(GetExceptionInformation())){
    }
#endif
#else
    try {
        fn();
    } catch (const std::exception &ex) {
        LogError("caught {}: {}", typeName(&ex), ex.what());
    } catch (...) {
        LogError("caught unexpected exception");
    }
#endif
}

#ifdef __GNUC__
#define ARGUS_NOINLINE  __attribute__((noinline))
#else
#define ARGUS_NOINLINE
#endif

void ARGUS_NOINLINE safeRunEnd() {
}