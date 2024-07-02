//
// Created by liuze.xlz on 2020/11/23.
//

#ifndef ARGUS_SIC_SYSTEM_INFORMATION_COLLECTOR_H
#define ARGUS_SIC_SYSTEM_INFORMATION_COLLECTOR_H

#include <cstdint>
#include <string>
#include <vector>
#include <chrono>
#include <unordered_map>
#include <map>
#include <set>
#include <sstream>
#include <functional>
#include <memory> // shared_ptr
#include "system_information_collector_util.h"
#include "common/Arithmetic.h"
#include "common/Common.h" // pid_t

class sout;

struct SicThreadCpu {
    std::chrono::microseconds user{0};
    std::chrono::microseconds sys{0};
    std::chrono::microseconds total{0};
};

template<typename T>
struct CpuBase {
    typedef T type;

#define FOR_EACH(Expr) \
    Expr(user); \
    Expr(sys); \
    Expr(nice); \
    Expr(idle); \
    Expr(wait); \
    Expr(irq); \
    Expr(softIrq); \
    Expr(stolen)

    // declare fields
#define DECLARE_FIELDS(F) T F{0}
    FOR_EACH(DECLARE_FIELDS);
#undef DECLARE_FIELDS

    // union for each
#define ZIP_CALLBACK(F) fnCallback(#F, this->F, b.F)

    template<typename T2>
    void zip(const T2 &b, const std::function<void(const char *name, T &a, const typename T2::type &b)> &fnCallback) {
        FOR_EACH(ZIP_CALLBACK);
    }

    template<typename T2>
    void zip(const T2 &b,
             const std::function<void(const char *name, const T &a, const typename T2::type &b)> &fnCallback) const {
        FOR_EACH(ZIP_CALLBACK);
    }

#undef ZIP_CALLBACK

#define FOREACH_CALLBACK(F) fnCallback(#F, this->F)

    void foreach(const std::function<void(const char *name, T &a)> &fnCallback) {
        FOR_EACH(FOREACH_CALLBACK);
    }

    void foreach(const std::function<void(const char *name, const T &a)> &fnCallback) const {
        FOR_EACH(FOREACH_CALLBACK);
    }

#undef FOREACH_CALLBACK

#undef FOR_EACH
};

struct SicCpuPercent : CpuBase<double> {
    double combined = 0.0;

    std::string string(const char *lf = "\n", const char *tab = "    ") const;
};

struct SicCpuInformation : CpuBase<std::chrono::microseconds> {
    SicCpuInformation &operator+=(const SicCpuInformation &r);
    void reset();
    std::chrono::microseconds total() const;
    std::string str(bool includeClassName = false, const char *lf = "\n") const;
};

SicCpuInformation operator-(const SicCpuInformation &, const SicCpuInformation &);
SicCpuPercent operator/(const SicCpuInformation &, const SicCpuInformation &);

// memory information in byte
struct SicMemoryInformation {
    uint64_t ram = 0;
    uint64_t total = 0;
    uint64_t used = 0;
    uint64_t free = 0;
    uint64_t actualUsed = 0;
    uint64_t actualFree = 0;
    double usedPercent = 0.0;
    double freePercent = 0.0;
};

struct SicLoadInformation {
    double load1 = 0.0;
    double load5 = 0.0;
    double load15 = 0.0;
};

struct SicSwapInformation {
    uint64_t total = 0;
    uint64_t used = 0;
    uint64_t free = 0;
    uint64_t pageIn = 0;
    uint64_t pageOut = 0;
};

struct SicNetAddress {
    enum {
        SIC_AF_UNSPEC,
        SIC_AF_INET,
        SIC_AF_INET6,
        SIC_AF_LINK
    } family;
    union {
        uint32_t in;
        uint32_t in6[4];
        unsigned char mac[8];
    } addr;

    SicNetAddress();
    std::string str() const;
};

struct SicInterfaceConfig {
    std::string name;
    std::string type;
    std::string description;
    SicNetAddress hardWareAddr;

    SicNetAddress address;
    SicNetAddress destination;
    SicNetAddress broadcast;
    SicNetAddress netmask;

    SicNetAddress address6;
    int prefix6Length = 0;
    int scope6 = 0;

    uint64_t mtu = 0;
    uint64_t metric = 0;
    int txQueueLen = 0;
};

struct SicInterfaceConfigList {
    std::set<std::string> names;
};

template<typename T>
struct NetInterfaceMetric {
    // received
    T rxPackets = 0;
    T rxBytes = 0;
    T rxErrors = 0;
    T rxDropped = 0;
    T rxOverruns = 0;
    T rxFrame = 0;
    // transmitted
    T txPackets = 0;
    T txBytes = 0;
    T txErrors = 0;
    T txDropped = 0;
    T txOverruns = 0;
    T txCollisions = 0;
    T txCarrier = 0;
};

struct SicNetInterfaceInformation : NetInterfaceMetric<uint64_t> {
    uint64_t speed = 0;
};

enum EnumTcpState : int8_t {
    SIC_TCP_ESTABLISHED = 1,
    SIC_TCP_SYN_SENT,
    SIC_TCP_SYN_RECV,
    SIC_TCP_FIN_WAIT1,
    SIC_TCP_FIN_WAIT2,
    SIC_TCP_TIME_WAIT,
    SIC_TCP_CLOSE,
    SIC_TCP_CLOSE_WAIT,
    SIC_TCP_LAST_ACK,
    SIC_TCP_LISTEN,
    SIC_TCP_CLOSING,
    SIC_TCP_IDLE,
    SIC_TCP_BOUND,
    SIC_TCP_UNKNOWN,
    SIC_TCP_TOTAL,
    SIC_TCP_NON_ESTABLISHED,

    SIC_TCP_STATE_END, // 仅用于状态计数
};
std::string GetTcpStateName(EnumTcpState n);

struct SicNetConnectionInformation {
    unsigned long localPort = 0;
    SicNetAddress localAddress;
    unsigned long remotePort = 0;
    SicNetAddress remoteAddress;
    unsigned long uid = 0;
    unsigned long inode = 0;
    int type = 0;
    int state = 0;
    unsigned long sendQueue = 0;
    unsigned long receiveQueue = 0;
};

enum {
    SIC_NET_CONN_TCP = 0x1000,
    SIC_NET_CONN_UDP
};

struct SicNetConnectionInformationList {
    unsigned long number;
    unsigned long size;
    std::vector<SicNetConnectionInformation> data;
};

struct SicNetState {
    int tcpStates[SIC_TCP_STATE_END] = {0};
    unsigned int tcpInboundTotal = 0;
    unsigned int tcpOutboundTotal = 0;
    unsigned int allInboundTotal = 0;
    unsigned int allOutboundTotal = 0;
    unsigned int udpSession = 0;

    void calcTcpTotalAndNonEstablished();
    std::string toString(const char *lf = "\n", const char *tab = "    ") const;
    bool operator==(const SicNetState &) const;

    inline bool operator!=(const SicNetState &r) const {
        return !(*this == r);
    }
};

enum SicFileSystemType {
    SIC_FILE_SYSTEM_TYPE_UNKNOWN = 0,
    SIC_FILE_SYSTEM_TYPE_NONE,
    SIC_FILE_SYSTEM_TYPE_LOCAL_DISK,
    SIC_FILE_SYSTEM_TYPE_NETWORK,
    SIC_FILE_SYSTEM_TYPE_RAM_DISK,
    SIC_FILE_SYSTEM_TYPE_CDROM,
    SIC_FILE_SYSTEM_TYPE_SWAP,
    SIC_FILE_SYSTEM_TYPE_MAX
};
std::string GetName(SicFileSystemType fs);

struct SicFileSystem {
    std::string dirName;
    std::string devName;
    std::string typeName;
    std::string sysTypeName;
    std::string options;
    SicFileSystemType type = SIC_FILE_SYSTEM_TYPE_UNKNOWN;
    unsigned long flags = 0;
};

struct SicDiskUsage {
    std::string dirName;
    std::string devName;
#ifdef WIN32
    // windows 中磁盘读写占用时间的 time 受到 raid 和非 raid 影响（占用率会超过100%），
    // 所以对于 windows 使用 1 - idle 使用率的方式来获取磁盘 util
    double time = 0;
    double rTime = 0;
    double wTime = 0;
    double qTime = 0;

    double idle = 0;
    uint64_t tick = 0;

    double reads = 0;
    double writes = 0;

    double writeBytes = 0;
    double readBytes = 0;
    int64_t frequency = 0;
#else
    uint64_t time = 0;
    uint64_t rTime = 0;
    uint64_t wTime = 0;
    uint64_t qTime = 0;

    uint64_t reads = 0;
    uint64_t writes = 0;

    uint64_t writeBytes = 0;
    uint64_t readBytes = 0;
    double snapTime = 0;
#endif
    double serviceTime = 0.0;
    double queue = 0.0;

    std::string string() const;
};

struct SicFileSystemUsage {
    SicDiskUsage disk;
    double use_percent = 0;
    // usage in KB
    uint64_t total = 0;
    uint64_t free = 0;
    uint64_t used = 0;
    uint64_t avail = 0;
    uint64_t files = 0;
    uint64_t freeFiles = 0;
};

struct SicProcessList {
    std::vector<pid_t> pids = {};
};

struct SicWin32ProcessInfo {
    uint64_t pid = 0;
    int parentPid = 0;
    int priority = 0;
    std::chrono::steady_clock::time_point expire;
    uint64_t size = 0;          // Virtual Bytes
    uint64_t resident = 0;      // Memory Size Bytes
    uint64_t share = 0;
    std::string name;
    char state = '\0';
    uint64_t handles = 0;
    uint64_t threads = 0;
    uint64_t pageFaults = 0;
    uint64_t bytesRead = 0;
    uint64_t bytesWritten = 0;

    uint64_t tick = 0;
    int64_t frequency = 0;
};

struct SicLinuxProcessInfo {
    pid_t pid = 0;
    // time_t mTime = 0;
    uint64_t vSize = 0;
    uint64_t rss = 0;
    uint64_t minorFaults = 0;
    uint64_t majorFaults = 0;
    pid_t parentPid = 0;
    int tty = 0;
    int priority = 0;
    int nice = 0;
    int numThreads = 0;
    std::chrono::system_clock::time_point startTime;
    std::chrono::milliseconds utime{0};
    std::chrono::milliseconds stime{0};
    std::chrono::milliseconds cutime{0};
    std::chrono::milliseconds cstime{0};
    std::string name;
    char state = '\0';
    int processor = 0;

    int64_t startMillis() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(startTime.time_since_epoch()).count();
    }
};

const char SIC_PROC_STATE_SLEEP = 'S';
const char SIC_PROC_STATE_RUN = 'R';
const char SIC_PROC_STATE_STOP = 'T';
const char SIC_PROC_STATE_ZOMBIE = 'Z';
const char SIC_PROC_STATE_IDLE = 'D';

struct SicProcessState {
    std::string name;
    char state = '\0'; // See: SIC_PROC_STATE_XXX
    int tty = 0;
    int priority = 0;
    int nice = 0;
    int processor = 0;
    pid_t parentPid = 0;
    uint64_t threads = 0;
};

struct SicProcessTime {
    std::chrono::system_clock::time_point startTime;
    std::chrono::milliseconds cutime{0};
    std::chrono::milliseconds cstime{0};

    std::chrono::milliseconds user{0}; // utime + cutime
    std::chrono::milliseconds sys{0}; // stime + cstime

    std::chrono::milliseconds total{0}; // user + sys

    std::chrono::milliseconds utime() const {
        return user - cutime;
    }

    std::chrono::milliseconds stime() const {
        return sys - cstime;
    }

    int64_t startMillis() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(startTime.time_since_epoch()).count();
    }
};

struct SicProcessCpuInformation {
    int64_t startTime = 0;
    std::chrono::steady_clock::time_point lastTime;
    uint64_t user = 0;
    uint64_t sys = 0;
    uint64_t total = 0;
    double percent = 0.0;
};

struct SicProcessMemoryInformation {
    uint64_t size = 0;
    uint64_t resident = 0;
    uint64_t share = 0;
    uint64_t minorFaults = 0;
    uint64_t majorFaults = 0;
    uint64_t pageFaults = 0;

#ifdef WIN32
    uint64_t tick = 0;
    int64_t frequency = 0;
#endif
};

struct SicProcessArgs {
    std::vector<std::string> args;
};

struct SicProcessExe {
    std::string name;
    std::string cwd;
    std::string root;
};

struct SicProcessCredName {
    std::string user;
    std::string group;
};

struct SicProcessFd {
    uint64_t total = 0;
    bool exact = true;  // total是否是一个精确值，在Linux下进程打开文件数超10,000时，将不再继续统计，以防出现性能问题
};

struct SicProcessCpuInformationCache {
    SicProcessCpuInformation processCpu;
    std::chrono::steady_clock::time_point expireTime;
};

struct SicSystemInfo {
    std::string name;
    std::string version;
    std::string arch;
    std::string machine;
    std::string description;
    std::string vendor;
    std::string vendorVersion;
    std::string vendorName;
};

struct SicLinuxProcessInfoCache : SicLinuxProcessInfo {
    int result = SIC_EXECUTABLE_FAILED;
};

struct SicBase {
    std::string errorMessage;

    struct Cache {
        std::chrono::microseconds entryExpirePeriod{0};
        std::chrono::microseconds cleanPeriod{0};
        std::chrono::steady_clock::time_point nextCleanTime;

        struct key {
            pid_t pid = 0;
            bool includeCTime = false;

            key() = default;

            key(pid_t n, bool b) : pid(n), includeCTime(b) {}

            bool operator==(const key &r) const {
                return pid == r.pid && includeCTime == r.includeCTime;
            }

            struct key_hash {
                size_t operator()(const key &r) const {
                    return std::hash<pid_t>{}(r.pid) ^ std::hash<bool>{}(r.includeCTime);
                }
            };
        };

        std::unordered_map<key, SicProcessCpuInformationCache, typename key::key_hash> entries;
    } processCpuCache;
    // net localPort to LocalAddress
    std::unordered_map<unsigned long, SicNetAddress> netListenCache;

    SicBase();
    virtual ~SicBase() = 0;

    void SetErrorMessage(std::string const &message) {
        errorMessage = message;
    }
};

void ExitArgus(int status);

#include "common/test_support"

class SystemInformationCollector {
public:
    explicit SystemInformationCollector(std::shared_ptr<SicBase> mSic) : mSicPtr(std::move(mSic)) {}

    virtual ~SystemInformationCollector() = default;

    virtual int SicGetThreadCpuInformation(pid_t pid, int tid, SicThreadCpu &) = 0;
    virtual int SicGetCpuInformation(SicCpuInformation &information) = 0;
    virtual int SicGetCpuListInformation(std::vector<SicCpuInformation> &) = 0;
    virtual int SicGetMemoryInformation(SicMemoryInformation &information) = 0;
    virtual int SicGetUpTime(double &upTime) = 0;
    virtual int SicGetLoadInformation(SicLoadInformation &information) = 0;
    virtual int SicGetSwapInformation(SicSwapInformation &information) = 0;

    virtual int SicGetInterfaceConfigList(SicInterfaceConfigList &interfaceConfigList) = 0;
    virtual int SicGetInterfaceConfigList(std::vector<SicInterfaceConfig> &interfaceConfig);
    virtual int SicGetInterfaceConfig(SicInterfaceConfig &interfaceConfig, const std::string &name) = 0;
    virtual int SicGetInterfaceInformation(const std::string &name, SicNetInterfaceInformation &information) = 0;
    virtual int SicGetNetState(SicNetState &netState, bool useClient, bool useServer,
                               bool useUdp, bool useTcp, bool useUnix, int option = -1) = 0;
    virtual int SicGetFileSystemListInformation(std::vector<SicFileSystem> &) = 0;
    virtual int SicGetDiskUsage(SicDiskUsage &diskUsage, std::string dirName) = 0;
    virtual int SicGetFileSystemUsage(SicFileSystemUsage &fileSystemUsage, std::string dirName) = 0;
    int SicGetDiskSerialId(const std::string &devName, std::string &serialId);

    virtual int SicGetProcessList(SicProcessList &processList, size_t limit, bool &overflow) = 0;
    virtual int SicGetProcessState(pid_t pid, SicProcessState &processState) = 0;
    virtual int SicGetProcessTime(pid_t pid, SicProcessTime &processTime, bool includeCTime = false) = 0;
    int SicGetProcessCpuInformation(pid_t pid, SicProcessCpuInformation &information, bool includeCTime = false);
    virtual int SicGetProcessMemoryInformation(pid_t pid, SicProcessMemoryInformation &information) = 0;
    virtual int SicGetProcessArgs(pid_t pid, SicProcessArgs &processArgs) = 0;
    virtual int SicGetProcessExe(pid_t pid, SicProcessExe &processExe) = 0;
    virtual int SicGetProcessCredName(pid_t pid, SicProcessCredName &processCredName) = 0;
    virtual int SicGetProcessFd(pid_t pid, SicProcessFd &processFd) = 0;

    virtual int SicGetSystemInfo(SicSystemInfo &systemInfo) = 0;
    static std::shared_ptr<SystemInformationCollector> New();
    static bool SicGetFileSystemType(const std::string &fsTypeName, SicFileSystemType &fsType,
                                     std::string &fsTypeDisplayName);

    int ErrNo(const char *op = nullptr);

    int ErrNo(const std::string &s) {
        return ErrNo(s.c_str());
    }


    int ErrNo(int errNo, const char *op = nullptr);

    int ErrNo(int errNo, const std::string &s) {
        return ErrNo(errNo, s.c_str());
    }

    int ErrNo(const sout &s);
    int ErrNo(int errNo, const sout &s);

    std::string errorMessage() const {
        return mSicPtr->errorMessage;
    }

    std::shared_ptr<SicBase> SicPtr() const {
        return mSicPtr;
    }

protected:
    virtual int SicInitialize() = 0;
    virtual std::map<std::string, std::string> queryDevSerialId(const std::string &devName) = 0;
    SicProcessCpuInformation &SicGetProcessCpuInCache(pid_t pid, bool includeCTime);
    void SicCleanProcessCpuCacheIfNecessary() const;

    std::shared_ptr<SicBase> mSicPtr;
};

#include "common/test_support"

#endif // !ARGUS_SIC_SYSTEM_INFORMATION_COLLECTOR_H
