//
// Created by 韩呈杰 on 2023/2/4.
//
// strerrorlen_s要求该宏定义，参见：https://en.cppreference.com/w/c/string/byte/strerror
#define __STDC_WANT_LIB_EXT1__  1

#include <cstring> // strerror、strerror_s
#include <unordered_map>
#include <boost/core/demangle.hpp>

#include "sic/system_information_collector.h"
#include "common/FieldEntry.h"
#include "common/Arithmetic.h"
#include "common/TimeFormat.h"
#include "common/ExpiredMap.h"
#include "common/Chrono.h"
#include "common/StringUtils.h"
#include "common/Common.h"

static_assert(sizeof(SicProcessMemoryInformation::resident) == 8, "unsigned long long not an uint64_t");

template<typename T>
T Max(const std::initializer_list<T> &l) {
    T target = *l.begin();
    for (const T &v: l) {
        if (target < v) {
            target = v;
        }
    }

    return target;
}

const struct {
    SicFileSystemType fs;
    const char *name;
} fsTypeNames[] = {
        {SIC_FILE_SYSTEM_TYPE_UNKNOWN,    "unknown"},
        {SIC_FILE_SYSTEM_TYPE_NONE,       "none"},
        {SIC_FILE_SYSTEM_TYPE_LOCAL_DISK, "local"},
        {SIC_FILE_SYSTEM_TYPE_NETWORK,    "remote"},
        {SIC_FILE_SYSTEM_TYPE_RAM_DISK,   "ram"},
        {SIC_FILE_SYSTEM_TYPE_CDROM,      "cdrom"},
        {SIC_FILE_SYSTEM_TYPE_SWAP,       "swap"},
};
constexpr size_t fsTypeNamesCount = sizeof(fsTypeNames) / sizeof(fsTypeNames[0]);
static_assert(SIC_FILE_SYSTEM_TYPE_MAX == fsTypeNamesCount, "fsTypeNames size not matched");

std::string GetName(SicFileSystemType fs) {
    int idx = static_cast<int>(fs);
    if (0 <= idx && (size_t)idx < fsTypeNamesCount && fsTypeNames[idx].fs == fs) {
        return fsTypeNames[idx].name;
    }
    return "";
}

std::string SicCpuPercent::string(const char *lf, const char *tab) const {
    std::ostringstream ss;
    ss << "SicCpuPercent{" << lf;
    this->foreach([&](const char *name, const double &v) {
        ss << tab << name << ": " << v << lf;
    });
    ss << "}" << lf;
    return ss.str();
}

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EnumTcpState
constexpr const char *str_offset(const char *s, int offset) {
    return s + offset;
}

#define VN(V, OFFSET) {V, str_offset(#V, OFFSET)}
const struct {
    const EnumTcpState state;
    const char *name;
} tcpStateNames[] = {
        {EnumTcpState(0), "NO_USED"},
        VN(SIC_TCP_ESTABLISHED, 8),
        VN(SIC_TCP_SYN_SENT, 8),
        VN(SIC_TCP_SYN_RECV, 8),
        VN(SIC_TCP_FIN_WAIT1, 8),
        VN(SIC_TCP_FIN_WAIT2, 8),
        VN(SIC_TCP_TIME_WAIT, 8),
        VN(SIC_TCP_CLOSE, 8),
        VN(SIC_TCP_CLOSE_WAIT, 8),
        VN(SIC_TCP_LAST_ACK, 8),
        VN(SIC_TCP_LISTEN, 8),
        VN(SIC_TCP_CLOSING, 8),
        VN(SIC_TCP_IDLE, 8),
        VN(SIC_TCP_BOUND, 8),
        VN(SIC_TCP_UNKNOWN, 8),
        VN(SIC_TCP_TOTAL, 4),
        VN(SIC_TCP_NON_ESTABLISHED, 8),
        VN(SIC_TCP_STATE_END, 4),
};
#undef VN
const int tcpStateNameSize = sizeof(tcpStateNames) / sizeof(tcpStateNames[0]);
static_assert(tcpStateNameSize == SIC_TCP_STATE_END + 1, "tcpStateNames unexpected");

std::string GetTcpStateName(EnumTcpState n) {
    std::string ret;
    if (0 <= n && n < tcpStateNameSize) {
        auto target = &tcpStateNames[n];
        if (target->state == n) {
            ret = target->name;
        }
    }
    return ret;
}


/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SicNetState
void SicNetState::calcTcpTotalAndNonEstablished() {
    tcpStates[SIC_TCP_TOTAL] = 0;
    for (int i = SIC_TCP_ESTABLISHED; i <= SIC_TCP_UNKNOWN; ++i) {
        tcpStates[SIC_TCP_TOTAL] += tcpStates[i];
    }
    tcpStates[SIC_TCP_NON_ESTABLISHED] = tcpStates[SIC_TCP_TOTAL] - tcpStates[SIC_TCP_ESTABLISHED];
}

bool SicNetState::operator==(const SicNetState &r) const {
    return 0 == memcmp(this, &r, sizeof(*this));
}
std::string SicNetState::toString(const char *lf, const char *tab) const {
    std::ostringstream oss;
    oss << "SicNetState{" << lf
        << tab << "tcpStates {" << lf;
    for (int i = 1; i < SIC_TCP_STATE_END; i++) {
        oss << tab << tab << GetTcpStateName((EnumTcpState) i) << ": " << tcpStates[i] << lf;
    }
    oss << tab << "}" << lf
        << tab << "udpSession: " << udpSession << lf
        << "}";
    return oss.str();
}

std::string SicDiskUsage::string() const {
    std::stringstream os;
    os << "SicDiskUsage(dir=\"" << dirName << "\", dev=\"" << devName << "\") {" << std::endl
       << "         time: " << time << "," << std::endl
       << "        rTime: " << rTime << " ms," << std::endl
       << "        wTime: " << wTime << " ms," << std::endl
       << "        qTime: " << qTime << "," << std::endl
       << "        reads: " << reads << "," << std::endl
       << "       writes: " << writes << "," << std::endl
       << "    readBytes: " << readBytes << "," << std::endl
       << "   writeBytes: " << writeBytes << "," << std::endl
       #ifdef WIN32
       << "         tick: " << tick << "," << std::endl
       << "         idle: " << idle << "," << std::endl
       << "    frequency: " << frequency << "," << std::endl
       #else
       << "     snapTime: " << snapTime << ", " << std::endl
       #endif
       << "  serviceTime: " << serviceTime << "," << std::endl
       << "        queue: " << queue << "," << std::endl
       << "}";
    return os.str();
}

// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SicNetAddress
SicNetAddress::SicNetAddress() {
    memset(this, 0, sizeof(*this));
}

static const auto &mapNetAddressStringer = *new std::map<int, std::function<std::string(const SicNetAddress *)>>{
        {SicNetAddress::SIC_AF_UNSPEC, [](const SicNetAddress *me) { return std::string{}; }},
        {SicNetAddress::SIC_AF_INET,   [](const SicNetAddress *me) { return IPv4String(me->addr.in); }},
        {SicNetAddress::SIC_AF_INET6,  [](const SicNetAddress *me) { return IPv6String(me->addr.in6); }},
        {SicNetAddress::SIC_AF_LINK,   [](const SicNetAddress *me) { return MacString(me->addr.mac); }},
};

std::string SicNetAddress::str() const {
    std::string name;
    auto it = mapNetAddressStringer.find(this->family);
    if (it != mapNetAddressStringer.end()) {
        name = it->second(this);
    }
    return name;
}

// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SicCpuInformation

SicCpuInformation &SicCpuInformation::operator+=(const SicCpuInformation &r) {
    this->zip(r, [](const char *, type &a, const type &b) { a += b; });
    return *this;
}

SicCpuInformation operator-(const SicCpuInformation &a, const SicCpuInformation &b) {
    SicCpuInformation ret{a};
    ret.zip(b, [](const char *name, std::chrono::microseconds &v1, const std::chrono::microseconds &v2) {
        v1 = Diff(v1, v2);
    });

    return ret;
}

SicCpuPercent operator/(const SicCpuInformation &a, const SicCpuInformation &b) {
    SicCpuPercent cpuPercent;

    SicCpuInformation diff = b - a;
    auto const total = static_cast<double>(diff.total().count());
    if (total > 0) {
        cpuPercent.zip(diff, [&](const char *, double &a, const std::chrono::microseconds &b) {
            a = static_cast<double>(b.count()) / total;
        });

        cpuPercent.combined = cpuPercent.user + cpuPercent.sys + cpuPercent.nice + cpuPercent.wait;
    }
    return cpuPercent;
}

void SicCpuInformation::reset() {
    this->foreach([](const char *, std::chrono::microseconds &a) {
        a = std::chrono::microseconds{0};
    });
}

std::chrono::microseconds SicCpuInformation::total() const {
    std::chrono::microseconds sum{0};
    this->foreach([&](const char *, const type &a) {
        sum += a;
    });
    return sum;
}

std::string SicCpuInformation::str(bool includeClassName, const char *lf) const {
    // 字段名称的最大长度
    const std::string keyTotal = "total";
    size_t maxNameLen = 0;
    if (strcmp(lf, "\n") == 0 || strcmp(lf, "\r\n") == 0) {
        maxNameLen = keyTotal.size();
        foreach([&](const char *name, const std::chrono::microseconds &) {
            maxNameLen = Max({maxNameLen, strlen(name)});
        });
    }

    auto printLine = [&](std::ostream &os, const std::string &name, const std::chrono::microseconds &v) {
        if (maxNameLen > 0) {
            os << std::setw((int) maxNameLen) << std::setfill(' ');
        }
        os << name << ": " << v << "," << lf;
    };

    std::stringstream ss;
    if (includeClassName) {
        ss << boost::core::demangle(typeid(*this).name());
    }
    ss << "{" << lf;
    this->foreach([&](const char *name, const type &v) {
        printLine(ss, name, v);
    });
    printLine(ss, keyTotal, total());
    ss << "}";

    return ss.str();
}

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SicBase
SicBase::SicBase() {
    processCpuCache.cleanPeriod = 10_min; // std::chrono::minutes{10};
    processCpuCache.entryExpirePeriod = 20_min; // std::chrono::minutes{20};
    processCpuCache.nextCleanTime = std::chrono::steady_clock::now() + processCpuCache.cleanPeriod;
}

// GCC下纯虚析构也是需要函数体的
SicBase::~SicBase() = default;

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SystemInformationCollector
/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int SystemInformationCollector::SicGetInterfaceConfigList(std::vector<SicInterfaceConfig> &interfaceConfigs) {
    SicInterfaceConfigList ifs;
    int ret = this->SicGetInterfaceConfigList(ifs);
    if (ret == SIC_EXECUTABLE_SUCCESS && !ifs.names.empty()) {
        interfaceConfigs.reserve(ifs.names.size());
        for (const auto &ifName: ifs.names) {
            SicInterfaceConfig ifConfig;
            ret = SicGetInterfaceConfig(ifConfig, ifName);
            if (ret != SIC_EXECUTABLE_SUCCESS) {
                break;
            }
            interfaceConfigs.push_back(ifConfig);
        }
    }

    return ret;
}

void SystemInformationCollector::SicCleanProcessCpuCacheIfNecessary() const {
    auto &cache = SicPtr()->processCpuCache;
    if (cache.cleanPeriod.count() > 0) {
        const auto now = std::chrono::steady_clock::now();
        if (now >= cache.nextCleanTime) {
            cache.nextCleanTime = now + cache.cleanPeriod;
            for (auto entry = cache.entries.begin(); entry != cache.entries.end();) {
                if (entry->second.expireTime < now) {
                    //  no one access this entry for too long - need clean
                    cache.entries.erase(entry++);
                } else {
                    ++entry;
                }
            }
        }
    }
}

SicProcessCpuInformation &SystemInformationCollector::SicGetProcessCpuInCache(pid_t pid, bool includeCTime) {
    SicCleanProcessCpuCacheIfNecessary();

    SicBase::Cache::key key{pid, includeCTime};
    auto &cache = SicPtr()->processCpuCache;
    auto &entry = cache.entries[key]; // 没有则创建
    entry.expireTime = std::chrono::steady_clock::now() + cache.entryExpirePeriod;
    return entry.processCpu;
}

int SystemInformationCollector::SicGetProcessCpuInformation(pid_t pid, SicProcessCpuInformation &information,
                                                            bool includeCTime) {
    // int64_t nowMillis = NowMillis();
    const auto now = std::chrono::steady_clock::now();

    auto &prev = SicGetProcessCpuInCache(pid, includeCTime);
    // int64_t timeDiff = nowMillis - prev.lastTime;
    //proc/[pid]/stat的统计粒度通常为10ms，timeDiff应足够大以使数据平滑。
    if (now < prev.lastTime + 1_s) {
        information = prev;
        return SIC_EXECUTABLE_SUCCESS;
    }
    int64_t timeDiff = ToMillis(now - prev.lastTime);
    // prev.lastTime = nowMillis;
    information.lastTime = now;
    // unsigned long long oTime = prev.total;

    SicProcessTime processTime{};
    int res = SicGetProcessTime(pid, processTime, includeCTime);
    if (res != SIC_EXECUTABLE_SUCCESS) {
        return res;
    }

    using namespace std::chrono;
    information.startTime = ToMillis(processTime.startTime);
    information.user = processTime.user.count();
    information.sys = processTime.sys.count();
    information.total = processTime.total.count();

    // unsigned long long oTime = (information.total < prev.total ? 0 : prev.total);
    // if (information.total < otime) {
    //     // this should not happen
    //     otime = 0;
    // }

    if (information.total < prev.total || IsZero(prev.lastTime)) {
        // first time called
        information.percent = 0.0;
    } else {
        auto totalDiff = static_cast<double>(information.total - prev.total);
        information.percent = totalDiff / static_cast<double>(timeDiff);
    }
    prev = information;

    return SIC_EXECUTABLE_SUCCESS;
}

// 已知文件系统
const auto &knownFileSystem = *new std::unordered_map<std::string, SicFileSystemType>{
        {"adfs",          SIC_FILE_SYSTEM_TYPE_LOCAL_DISK},
        {"affs",          SIC_FILE_SYSTEM_TYPE_LOCAL_DISK},
        {"anon-inode FS", SIC_FILE_SYSTEM_TYPE_LOCAL_DISK},
        {"befs",          SIC_FILE_SYSTEM_TYPE_LOCAL_DISK},
        {"bfs",           SIC_FILE_SYSTEM_TYPE_LOCAL_DISK},
        {"btrfs",         SIC_FILE_SYSTEM_TYPE_LOCAL_DISK},
        {"ecryptfs",      SIC_FILE_SYSTEM_TYPE_LOCAL_DISK},
        {"efs",           SIC_FILE_SYSTEM_TYPE_LOCAL_DISK},
        {"futexfs",       SIC_FILE_SYSTEM_TYPE_LOCAL_DISK},
        {"gpfs",          SIC_FILE_SYSTEM_TYPE_NETWORK},
        {"hpfs",          SIC_FILE_SYSTEM_TYPE_LOCAL_DISK},
        {"hfs",           SIC_FILE_SYSTEM_TYPE_LOCAL_DISK},
        {"isofs",         SIC_FILE_SYSTEM_TYPE_LOCAL_DISK},
        {"k-afs",         SIC_FILE_SYSTEM_TYPE_NETWORK},
        {"lustre",        SIC_FILE_SYSTEM_TYPE_NETWORK},
        {"nilfs",         SIC_FILE_SYSTEM_TYPE_LOCAL_DISK},
        {"openprom",      SIC_FILE_SYSTEM_TYPE_LOCAL_DISK},
        {"reiserfs",      SIC_FILE_SYSTEM_TYPE_LOCAL_DISK},
        {"vzfs",          SIC_FILE_SYSTEM_TYPE_LOCAL_DISK},
        {"xfs",           SIC_FILE_SYSTEM_TYPE_LOCAL_DISK},
        {"xiafs",         SIC_FILE_SYSTEM_TYPE_LOCAL_DISK},

        // CommonFileSystem
        {"ntfs",          SIC_FILE_SYSTEM_TYPE_LOCAL_DISK},
        {"smbfs",         SIC_FILE_SYSTEM_TYPE_NETWORK},
        {"smb",           SIC_FILE_SYSTEM_TYPE_NETWORK},
        {"swap",          SIC_FILE_SYSTEM_TYPE_SWAP},
        {"afs",           SIC_FILE_SYSTEM_TYPE_NETWORK},
        {"iso9660",       SIC_FILE_SYSTEM_TYPE_CDROM},
        {"cvfs",          SIC_FILE_SYSTEM_TYPE_LOCAL_DISK},
        {"cifs",          SIC_FILE_SYSTEM_TYPE_NETWORK},
        {"msdos",         SIC_FILE_SYSTEM_TYPE_LOCAL_DISK},
        {"minix",         SIC_FILE_SYSTEM_TYPE_LOCAL_DISK},
        {"vxfs",          SIC_FILE_SYSTEM_TYPE_LOCAL_DISK},
        {"vfat",          SIC_FILE_SYSTEM_TYPE_LOCAL_DISK},
        {"zfs",           SIC_FILE_SYSTEM_TYPE_LOCAL_DISK},
};
const struct {
    const char *prefix;
    const SicFileSystemType fsType;
} knownFileSystemPrefix[] = {
        {"ext",   SIC_FILE_SYSTEM_TYPE_LOCAL_DISK},
        {"gfs",   SIC_FILE_SYSTEM_TYPE_NETWORK},
        {"jffs",  SIC_FILE_SYSTEM_TYPE_LOCAL_DISK},
        {"jfs",   SIC_FILE_SYSTEM_TYPE_LOCAL_DISK},
        {"minix", SIC_FILE_SYSTEM_TYPE_LOCAL_DISK},
        {"ocfs",  SIC_FILE_SYSTEM_TYPE_NETWORK},
        {"psfs",  SIC_FILE_SYSTEM_TYPE_LOCAL_DISK},

        {"nfs",   SIC_FILE_SYSTEM_TYPE_NETWORK},
        {"fat",   SIC_FILE_SYSTEM_TYPE_LOCAL_DISK}
};

bool SystemInformationCollector::SicGetFileSystemType(const std::string &fsTypeName,
                                                      SicFileSystemType &fsType,
                                                      std::string &fsTypeDisplayName) {
    bool found = fsType != SIC_FILE_SYSTEM_TYPE_UNKNOWN;
    if (!found) {
        auto it = knownFileSystem.find(fsTypeName);
        found = it != knownFileSystem.end();
        if (found) {
            fsType = it->second;
        } else {
            for (auto &entry: knownFileSystemPrefix) {
                found = HasPrefix(fsTypeName, entry.prefix);
                if (found) {
                    fsType = entry.fsType;
                    break;
                }
            }
        }
    }

    if (!found || fsType >= SIC_FILE_SYSTEM_TYPE_MAX) {
        fsType = SIC_FILE_SYSTEM_TYPE_NONE;
    }
    fsTypeDisplayName = GetName(fsType);

    return found;
}

static auto *diskSerialId = new ExpiredMap<std::string, std::string>(std::chrono::hours{24});

int SystemInformationCollector::SicGetDiskSerialId(const std::string &devName, std::string &serialId) {
    auto query = [this](const std::string &devName, std::map<std::string, std::string> &v) {
        v = this->queryDevSerialId(devName);
        return !v.empty();
    };

#ifdef WIN32
#   define ToUnify(s) ToUpper(s)
#else
#   define ToUnify(s) (s)
#endif
    auto r = diskSerialId->Compute(ToUnify(devName), query);
#undef ToUnify

    if (r.ok) {
        serialId = r.v;
    }
    return r.ok? SIC_EXECUTABLE_SUCCESS: SIC_EXECUTABLE_FAILED;
}

int SystemInformationCollector::ErrNo(const char *op) {
    return ErrNo(errno, op);
}

int SystemInformationCollector::ErrNo(int errNo, const char *op) {
    mSicPtr->errorMessage = StrError(errNo, op);
    return errNo;
}

int SystemInformationCollector::ErrNo(const sout &s) {
    return ErrNo(s.str());
}

int SystemInformationCollector::ErrNo(int errNo, const sout &s) {
    return ErrNo(errNo, s.str());
}
