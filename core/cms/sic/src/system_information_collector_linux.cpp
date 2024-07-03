//
// Created by 许刘泽 on 2020/12/2.
//
#include <functional> // std::bind
#include <cerrno>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <regex>
#include <utility>  // std::forward
#include <map>
#include <thread>
#include <cctype> // isdigit
#include <chrono>
#include <cstdlib> // exit
#include <cmath>   // std::isinf
#include <boost/json.hpp>

#include <unistd.h> // sysconf, pid_t

#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <sys/statvfs.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <sys/param.h>
#include <netinet/in.h>

// #include <linux/netlink.h>不能放在 #include <sys/socket.h> 前面
#if defined(__linux__) || defined(__unix__)
#include <mntent.h>
#include <linux/netlink.h>
#include <linux/inet_diag.h>
#include <sys/sysmacros.h> // In the GNU C Library, "minor" is defined by <sys/sysmacros.h>
#endif

// #include <sys/types.h>  // gnu_dev_major、gnu_dev_minor
#include <net/if.h>
#include <sys/ioctl.h>
#include <cstring>
#include <sys/stat.h>
#include <arpa/inet.h>
#include "sic/linux_system_information_collector.h"
#include "sic/system_information_collector_util.h"
#include "common/ScopeGuard.h"
#include "common/Arithmetic.h"
#include "common/TimeFormat.h"
#include "common/Logger.h"
#include "common/TimeProfile.h"
#include "common/Exceptions.h"
#include "common/StringUtils.h"
#include "common/ExecCmd.h"
#include "common/FileUtils.h"
#include "common/ChronoLiteral.h"
#include "common/ArgusMacros.h"

using namespace std::chrono;
using namespace common::StringUtils;

const char *const PROCESS_STAT = "stat";
const char *const PROCESS_MEMINFO = "meminfo";
const char *const PROCESS_MTRR = "mtrr";
const char *const PROCESS_VMSTAT = "vmstat";
const char *const PROCESS_STATM = "statm";
const char *const PROCESS_LOADAVG = "loadavg";
const char *const PROCESS_CMDLINE = "cmdline";
const char *const PROCESS_EXE = "exe";
const char *const PROCESS_CWD = "cwd";
const char *const PROCESS_ROOT = "root";
const char *const PROCESS_STATUS = "status";
const char *const PROCESS_FD = "fd";
const char *const PROCESS_UPTIME = "uptime";
const char *const PROCESS_NET_DEV = "net/dev";
const char *const PROCESS_NET_TCP = "net/tcp";
const char *const PROCESS_NET_TCP6 = "net/tcp6";
const char *const PROCESS_NET_UDP = "net/udp";
const char *const PROCESS_NET_UDP6 = "net/udp6";
const char *const PROCESS_NET_SOCKSTAT = "net/sockstat";
const char *const PROCESS_NET_SOCKSTAT6 = "net/sockstat6";
const char *const PROCESS_DISKSTATS = "diskstats";
const char *const PROCESS_PARTITIONS = "partitions";
const char *const PROCESS_NET_IF_INET6 = "net/if_inet6";
const char *const SYS_BLOCK = "/sys/block";

const static int SIC_NET_INTERFACE_LIST_MAX = 20;

extern const int SRC_ROOT_OFFSET;

static const auto &linuxVendorVersion = *new std::unordered_map<std::string, std::string>{
        {"/etc/fedora-release",      "Fedora"},
        {"/etc/SuSE-release",        "SuSE"},
        {"/etc/gentoo-release",      "Gentoo"},
        {"/etc/slackware-version",   "Slackware"},
        {"/etc/mandrake-release",    "Mandrake"},
        {"/proc/vmware/version",     "VMware"},
        {"/etc/xensource-inventory", "XenSource"},
        {"/etc/redhat-release",      "Red Hat"},
        {"/etc/lsb-release",         "lsb"},
        {"/etc/debian_version",      "Debian"}
};

struct SicProcessCred {
    uid_t uid;   //real user ID
    gid_t gid;   //real group ID
    uid_t euid;  //effective user ID
    gid_t egid;  //effective group ID
};

struct SicNetLinkRequest {
    struct nlmsghdr nlh;
    struct inet_diag_req r;
};

unsigned int Hex2Int(const std::string &s) {
    return convertHex<unsigned int>(s);
}

inline in_addr_t ifreq_s_addr(ifreq &ifr) {
    return ((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr.s_addr;
}

#ifdef ENABLE_COVERAGE
std::string __printDirStat(const char *file, int line, const std::string &dirName, struct stat &ioStat) {
    std::stringstream ss;
    ss << (file + SRC_ROOT_OFFSET) << ":" << line << ", dirName: " << dirName
       << ", isBlock: " << std::boolalpha << S_ISBLK(ioStat.st_mode) << std::noboolalpha
       << ", str_rdev: " << std::hex << (int) ioStat.st_rdev << std::dec
       << ", st_ino: " << ioStat.st_ino << ", st_dev: " << ioStat.st_dev;
    return ss.str();
}
#endif

// #ifdef ENABLE_COVERAGE
// #   define print(dirName, ioStat) std::cout << __printDirStat(__FILE__, __LINE__, dirName, ioStat) << std::endl
// #else
#   define print(dirName, ioStat) do{}while(false)
// #endif

void ExitArgus(int status) {
    exit(status);
}

static std::string getOrDefault(const std::string &v, const std::string &def) {
    return v.empty() ? def : v;
}

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
namespace sic {
    namespace detail {
        template<typename T>
        constexpr bool ValidMillis() {
            return std::is_same<T, uint64_t>::value
                   || std::is_same<T, std::chrono::milliseconds>::value;
        }
    }
}

extern const int ClkTck = sysconf(_SC_CLK_TCK); // 一般为100

static uint64_t Tick2Millisecond(uint64_t tick) {
    constexpr const uint64_t MILLISECOND = 1000;
    return tick * MILLISECOND / ClkTck;
}

static uint64_t Tick2Millisecond(const std::string &tick) {
    return Tick2Millisecond(convert<uint64_t>(tick));
}

// T: uint64_t、std::chrono::milliseconds
template<typename T, typename std::enable_if<sic::detail::ValidMillis<T>(), int>::type = 0>
T Tick2(const std::string &tick) {
    return T{Tick2Millisecond(tick)};
}

// TIndex为索引的Vector
template<typename TIndex>
class TVector {
    const std::vector<std::string> &data;
    const TIndex offset;
public:
    const std::string empty{};

    TVector(const std::vector<std::string> &d, TIndex of = (TIndex) 0)
            : data(d), offset(of) {
    }

    const std::string &operator[](TIndex key) const {
        int index = (int) key - (int) offset;
        return 0 <= index && index < static_cast<int>(data.size()) ? data[index] : empty;
    }
};

template<typename T, typename TIndex>
class MillisVector {
    const TVector<TIndex> &v;
public:
    static constexpr const T zero{0};

    explicit MillisVector(const TVector<TIndex> &r) : v(r) {
    }

    T operator[](TIndex key) const {
        return Tick2<T>(v[key]);
    }
};

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
Sic::~Sic() {
    // if (interfaceBuffer != nullptr) {
    //     free(interfaceBuffer);
    //     interfaceBuffer = nullptr;
    // }
}

LinuxSystemInformationCollector::LinuxSystemInformationCollector(fs::path processDir)
        : SystemInformationCollector(std::make_shared<Sic>()), PROCESS_DIR(std::move(processDir)) {
}

LinuxSystemInformationCollector::LinuxSystemInformationCollector()
        : SystemInformationCollector(std::make_shared<Sic>()) {
}

int64_t LinuxSystemInformationCollector::SicGetBootSeconds() {
    std::vector<std::string> cpuLines = {};
    int ret = GetFileLines(PROCESS_DIR / PROCESS_STAT, cpuLines, true, SicPtr()->errorMessage);
    if (ret != SIC_EXECUTABLE_SUCCESS || cpuLines.empty()) {
        return duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
    }

    for (auto const &cpuLine: cpuLines) {
        auto cpuMetric = common::StringUtils::split(cpuLine, ' ', true);
        if (cpuMetric.size() >= 2 && cpuMetric[0] == "btime") {
            constexpr size_t bootTimeIndex = 1;
            return bootTimeIndex < cpuMetric.size() ? convert<int64_t>(cpuMetric[bootTimeIndex]) : 0;
        }
    }

    return duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
}

static SicLinuxIOType getIOType(const fs::path &procDir) {
    struct {
        fs::path filename;
        SicLinuxIOType type;
    } ioTypes[] = {
            {procDir / PROCESS_DISKSTATS,  IO_STATE_DISKSTATS},
            {SYS_BLOCK,                    IO_STAT_SYS},
            {procDir / PROCESS_PARTITIONS, IO_STATE_PARTITIONS},
    };

    SicLinuxIOType ioType = IO_STATE_NONE;
    for (size_t i = 0; i < sizeof(ioTypes) / sizeof(ioTypes[0]) && ioType == IO_STATE_NONE; i++) {
        auto &entry = ioTypes[i];
        struct stat ioStat{};
        if (stat(entry.filename.c_str(), &ioStat) == 0) {
            ioType = entry.type;
        }
    }

    return ioType;
}

int LinuxSystemInformationCollector::SicInitialize() {
    // SicPtr()->ticks = sysconf(_SC_CLK_TCK); // 一般为100
    auto pSic = SicPtr();
    pSic->pagesize = 0;
    int pagesize = getpagesize();
    while ((pagesize >>= 1) > 0) {
        pSic->pagesize++;
    }

    // PR(sysconf(_SC_CLK_TCK));
    // PR(CLOCKS_PER_SEC);
    // PR(std::thread::hardware_concurrency());
    // PR(get_nprocs());
    // 获取可用核心而不是配置核心
    // SicPtr()->cpu_list_cores = get_nprocs();
    pSic->cpu_list_cores = std::thread::hardware_concurrency();
    pSic->bootSeconds = SicGetBootSeconds();
    pSic->ioType = getIOType(PROCESS_DIR);

    return SIC_EXECUTABLE_SUCCESS;
}

// man proc: https://man7.org/linux/man-pages/man5/proc.5.html
// search key: /proc/stat
enum class EnumCpuKey : int {
    user = 1,
    nice,
    system,
    idle,
    iowait,      // since Linux 2.5.41
    irq,         // since Linux 2.6.0
    softirq,     // since Linux 2.6.0
    steal,       // since Linux 2.6.11
    guest,       // since Linux 2.6.24
    guest_nice,  // since Linux 2.6.33
};
static_assert(2 == (int) EnumCpuKey::nice, "unexpected EnumCpuKey::nice value");

// static
int LinuxSystemInformationCollector::GetCpuMetric(std::string const &cpuLine, SicCpuInformation &cpu) {
    auto cpuMetricPtr = common::StringUtils::split(cpuLine, ' ', true);
    int ret = (cpuMetricPtr[0].substr(0, 3) == "cpu" ? SIC_EXECUTABLE_SUCCESS : SIC_EXECUTABLE_FAILED);

    using namespace std::chrono;
    // See: http://man7.org/linux/man-pages/man5/proc.5.html
    if (SIC_EXECUTABLE_SUCCESS == ret) {
        TVector<EnumCpuKey> v{cpuMetricPtr};
        MillisVector<milliseconds, EnumCpuKey> cpuMetric{v};
        cpu.user = cpuMetric[EnumCpuKey::user];
        cpu.nice = cpuMetric[EnumCpuKey::nice];
        cpu.sys = cpuMetric[EnumCpuKey::system];
        cpu.idle = cpuMetric[EnumCpuKey::idle];
        // iowait: since kernel 2.5.14
        cpu.wait = cpuMetric[EnumCpuKey::iowait];
        // kernels 2.6+
        cpu.irq = cpuMetric[EnumCpuKey::irq];
        cpu.softIrq = cpuMetric[EnumCpuKey::softirq];
        // kernels 2.6.11+
        cpu.stolen = cpuMetric[EnumCpuKey::steal];
        // guest
        // guest_nice

        // cpu.total = cpu.user + cpu.nice + cpu.sys + cpu.idle + cpu.wait + cpu.irq + cpu.softIrq + cpu.stolen;
    }

    return ret;
}

int LinuxSystemInformationCollector::SicGetThreadCpuInformation(pid_t pid, int tid, SicThreadCpu &cpu) {
    fs::path file = PROCESS_DIR / pid / "task" / tid / "stat";

    SicLinuxProcessInfo cpuInfo;
    int result = ParseProcessStat(file, pid, cpuInfo);
    if (result == SIC_EXECUTABLE_SUCCESS) {
        cpu.sys = milliseconds{cpuInfo.stime};
        cpu.user = milliseconds{cpuInfo.utime};
        cpu.total = cpu.sys + cpu.user;
    }
    return result;
}

int LinuxSystemInformationCollector::SicGetCpuInformation(SicCpuInformation &information) {
    std::vector<std::string> cpuLines = {};
    int ret = GetFileLines(PROCESS_DIR / PROCESS_STAT, cpuLines, true, SicPtr()->errorMessage);
    if (ret == SIC_EXECUTABLE_SUCCESS && !cpuLines.empty()) {
        ret = GetCpuMetric(cpuLines.front(), information);
    }
    return ret;
}

int LinuxSystemInformationCollector::SicGetCpuListInformation(std::vector<SicCpuInformation> &informations) {
    std::vector<std::string> cpuLines = {};
    int ret = GetFileLines(PROCESS_DIR / PROCESS_STAT, cpuLines, true, SicPtr()->errorMessage);
    if (ret == SIC_EXECUTABLE_SUCCESS && !cpuLines.empty()) {
        if (cpuLines.size() + 1 <= SicPtr()->cpu_list_cores) {
            // likely only has total cpu
            SicCpuInformation information;
            if (SIC_EXECUTABLE_SUCCESS == GetCpuMetric(cpuLines.front(), information)) {
                informations.push_back(information);
                return SIC_EXECUTABLE_SUCCESS;
            }
        }

        for (size_t i = 1; i <= SicPtr()->cpu_list_cores && i < cpuLines.size(); ++i) {
            SicCpuInformation information;
            if (SIC_EXECUTABLE_SUCCESS == GetCpuMetric(cpuLines[i], information)) {
                informations.push_back(information);
            }
        }
    }

    return ret;
}

uint64_t LinuxSystemInformationCollector::GetMemoryValue(char unit, uint64_t value) {
    if (unit == 'k' || unit == 'K') {
        value *= 1024;
    } else if (unit == 'm' || unit == 'M') {
        value *= 1024 * 1024;
    }
    return value;
}

// /proc/mtrr
// reg00: base=0x00000000 (   0MB), size= 256MB: write-back, count=1
// reg01: base=0xe8000000 (3712MB), size=  32MB: write-combining, count=1
// /proc/mtrr格式2：
// [root@7227ded95607 ilogtail]# cat /proc/mtrr 
// reg00: base=0x000000000 (    0MB), size=262144MB, count=1: write-back
// reg01: base=0x4000000000 (262144MB), size=131072MB, count=1: write-back
// reg02: base=0x6000000000 (393216MB), size= 2048MB, count=1: write-back
// reg03: base=0x6070000000 (395008MB), size=  256MB, count=1: uncachable
// reg04: base=0x080000000 ( 2048MB), size= 2048MB, count=1: uncachable
// reg05: base=0x070000000 ( 1792MB), size=   64MB, count=1: uncachable
uint64_t parseProcMtrr(const std::vector<std::string> &lines) {
    uint64_t ram = 0;
    for (auto const &line: lines) {
        if (line.find("write-back") == std::string::npos) {
            continue;
        }
        size_t start = line.find("size=");
        if (start != std::string::npos) {
            start += 5; // 5 -> strlen("size=")
            size_t end = line.find("MB", start);
            if (end != std::string::npos) {
                std::string str = TrimSpace(line.substr(start, end));
                // std::cout << line << std::endl;
                ram += (convert<decltype(ram)>(str) * 1024 * 1024);
            }
        }
    }

    return ram;
}

int LinuxSystemInformationCollector::GetMemoryRam(uint64_t &ram) {
    auto mtrrLines = GetFileLines(PROCESS_DIR / PROCESS_MTRR, true, nullptr);

    ram = parseProcMtrr(mtrrLines);

    return SIC_EXECUTABLE_SUCCESS;
}

void completeSicMemoryInformation(SicMemoryInformation &memInfo,
                                  uint64_t buffers,
                                  uint64_t cached,
                                  uint64_t available,
                                  const std::function<void(uint64_t &)> &fnGetMemoryRam) {
    const uint64_t mb = 1024 * 1024;
    if (available != std::numeric_limits<uint64_t>::max()) {
        // 新内核，存在 MemAvailable
        memInfo.actualUsed = Diff(memInfo.total, available);
        memInfo.actualFree = available;
        memInfo.usedPercent =
                memInfo.total > 0 ? static_cast<double>(memInfo.actualUsed) * 100 / memInfo.total : 0.0;
        memInfo.freePercent =
                memInfo.total > 0 ? static_cast<double>(memInfo.actualFree) * 100 / memInfo.total : 0.0;
        memInfo.ram = memInfo.total / mb;
    } else {
        // 不存在 MemAvailable
        available = buffers + cached;
        memInfo.actualUsed = Diff(memInfo.used, available);
        memInfo.actualFree = memInfo.free + available;
        uint64_t ram;
        fnGetMemoryRam(ram);
        if (ram > 0 && ram <= memInfo.total + 256 * mb) {
            memInfo.ram = ram / mb;
        } else {
            uint64_t sram = memInfo.total / mb;
            uint64_t remainder = sram % 8;
            if (remainder > 0) {
                sram += 8 - remainder;
            }
            memInfo.ram = sram;
        }

        uint64_t diff = Diff(memInfo.total, memInfo.actualFree);
        memInfo.usedPercent = memInfo.total > 0 ? static_cast<double>(diff) * 100 / memInfo.total : 0.0;
        diff = Diff(memInfo.total, memInfo.actualUsed);
        memInfo.freePercent = memInfo.total > 0 ? static_cast<double>(diff) * 100 / memInfo.total : 0.0;
    }
}

/*
样例: /proc/meminfo:
MemTotal:        4026104 kB
MemFree:         2246280 kB
MemAvailable:    3081592 kB // 低版本Linux内核上可能没有该行
Buffers:          124380 kB
Cached:          1216756 kB
SwapCached:            0 kB
Active:           417452 kB
Inactive:        1131312 kB
 */
int LinuxSystemInformationCollector::SicGetMemoryInformation(SicMemoryInformation &information) {
    int ret = SIC_EXECUTABLE_FAILED;
    std::vector<std::string> memoryLines = {};
    ret = GetFileLines(PROCESS_DIR / PROCESS_MEMINFO, memoryLines, true, SicPtr()->errorMessage);
    if (ret != SIC_EXECUTABLE_SUCCESS || memoryLines.empty()) {
        return ret;
    }

    uint64_t buffers = -1, cached = -1, available = -1;
    std::unordered_map<std::string, uint64_t &> memoryProc{
            {"MemTotal:",     information.total},
            {"MemFree:",      information.free},
            {"MemAvailable:", available},
            {"Buffers:",      buffers},
            {"Cached:",       cached},
    };
    for (size_t i = 0; i < memoryLines.size() && !memoryProc.empty(); i++) {
        std::vector<std::string> words = common::StringUtils::split(memoryLines[i], ' ', true);

        auto entry = memoryProc.find(words[0]);
        if (entry != memoryProc.end()) {
            entry->second = GetMemoryValue(words.back()[0], convert<uint64_t>(words[1]));
            memoryProc.erase(entry);
        }
    }
    information.used = Diff(information.total, information.free);
#ifdef ENABLE_COVERAGE
    std::cout << (PROCESS_DIR / PROCESS_MEMINFO).string() << ": total:" << information.total << ", free:"
              << information.free << ",available:" << available << std::endl;
#endif

    using namespace std::placeholders;
    completeSicMemoryInformation(information, buffers, cached, available,
                                 std::bind(&LinuxSystemInformationCollector::GetMemoryRam, this, _1));

    return SIC_EXECUTABLE_SUCCESS;
}

/*
>>> cat /proc/loadavg
0.40 0.26 0.10 2/616 6
*/
int LinuxSystemInformationCollector::SicGetLoadInformation(SicLoadInformation &information) {
    std::vector<std::string> lines;
    const fs::path procFile = PROCESS_DIR / PROCESS_LOADAVG;
    int ret = GetFileLines(procFile, lines, true, SicPtr()->errorMessage);
    if (ret == SIC_EXECUTABLE_SUCCESS) {
        std::vector<std::string> loadavgMetric = split((lines.empty() ? "" : lines.front()), ' ', true);
        if (loadavgMetric.size() < 3) {
            SicPtr()->SetErrorMessage((sout{} << "invalid loadavg file: " << procFile.string()).str());
            ret = SIC_EXECUTABLE_FAILED;
        } else {
            information.load1 = convert<double>(loadavgMetric[0]);
            information.load5 = convert<double>(loadavgMetric[1]);
            information.load15 = convert<double>(loadavgMetric[2]);
        }
    }
    return ret;
}

int LinuxSystemInformationCollector::GetSwapPageInfo(SicSwapInformation &information) {
    auto procVmStat = [&](const std::string &vmLine) {
        // kernel 2.6
        std::vector<std::string> vmMetric = split(vmLine, ' ', true);

        bool stop = false;
        if (vmMetric.front() == "pswpin") {
            information.pageIn = convert<uint64_t>(vmMetric[1]);
        } else if (vmMetric.front() == "pswpout") {
            stop = true;
            information.pageOut = convert<uint64_t>(vmMetric[1]);
        }
        return stop;
    };
    auto procStat = [&](const std::string &statLine) {
        // kernel 2.2、2.4
        bool found = HasPrefix(statLine, "swap ");
        if (found) {
            std::vector<std::string> statMetric = split(statLine, ' ', true);

            information.pageIn = convert<uint64_t>(statMetric[1]);
            information.pageOut = convert<uint64_t>(statMetric[2]);
        }
        return found;
    };

    struct {
        fs::path file;
        std::function<bool(const std::string &)> fnProc;
    } candidates[] = {
            // kernel 2.6
            {PROCESS_DIR / PROCESS_VMSTAT, procVmStat},
            // kernel 2.2、2.4
            {PROCESS_DIR / PROCESS_STAT,   procStat},
    };
    int ret = SIC_EXECUTABLE_FAILED;
    for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]) && ret != SIC_EXECUTABLE_SUCCESS; i++) {
        std::vector<std::string> lines;
        ret = GetFileLines(candidates[i].file, lines, true, SicPtr()->errorMessage);
        if (ret == SIC_EXECUTABLE_SUCCESS) {
            for (auto const &line: lines) {
                if (candidates[i].fnProc(line)) {
                    break;
                }
            }
        }
    }
    return ret;
}

int LinuxSystemInformationCollector::SicGetSwapInformation(SicSwapInformation &swap) {
    std::vector<std::string> memoryLines{};
    int ret = GetFileLines(PROCESS_DIR / PROCESS_MEMINFO, memoryLines, true, SicPtr()->errorMessage);
    if (ret != SIC_EXECUTABLE_SUCCESS || memoryLines.empty()) {
        return ret;
    }

    // SwapTotal:             0 kB
    // SwapFree:              0 kB
    std::unordered_map<std::string, uint64_t &> keyMap{
            {"SwapTotal:", swap.total},
            {"SwapFree:",  swap.free},
    };
    for (size_t i = 0; i < memoryLines.size() && !keyMap.empty(); i++) {
        std::vector<std::string> words = split(memoryLines[i], ' ', true);

        auto it = keyMap.find(words.front());
        if (it != keyMap.end()) {
            it->second = GetMemoryValue(words.back()[0], convert<uint64_t>(words[1]));
            keyMap.erase(it);
        }
    }

    swap.used = Diff(swap.total, swap.free);
    swap.pageIn = swap.pageOut = -1;

    return GetSwapPageInfo(swap);
}

int LinuxSystemInformationCollector::SicGetInterfaceConfigList(SicInterfaceConfigList &interfaceConfigList) {
    std::vector<char> ifaceBuf;
    if (SicPtr()->interfaceLength > 0) {
        ifaceBuf.resize(SicPtr()->interfaceLength);
    }

    ifconf ifc{};
    {
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) {
            return ErrNo("socket(AF_INET, SOCK_DGRAM, 0)");
        }
        defer(close(sock));

        for (int lastLen = 0;;) {
            if (ifaceBuf.empty() || lastLen != 0) {
                SicPtr()->interfaceLength += sizeof(ifreq) * SIC_NET_INTERFACE_LIST_MAX;
                ifaceBuf.resize(SicPtr()->interfaceLength);
            }

            ifc.ifc_len = SicPtr()->interfaceLength;
            ifc.ifc_buf = &ifaceBuf[0];

            if (ioctl(sock, SIOCGIFCONF, &ifc) < 0) {
                if ((errno != EINVAL) || (lastLen == ifc.ifc_len)) {
                    // EINVAL : num_interfaces > ifc.ifc_len
                    return ErrNo("SicGetInterfaceConfigList");
                }
            }

            if (ifc.ifc_len < SicPtr()->interfaceLength) {
                // got all ifconfig
                break;
            }

            if (ifc.ifc_len != lastLen) {
                // if_buffer too small
                lastLen = ifc.ifc_len;
                continue;
            }

            break;
        }
    }

    interfaceConfigList.names.clear();
    ifreq *irq = ifc.ifc_req;
    for (int i = 0; i < ifc.ifc_len; ++irq, i += sizeof(ifreq)) {
        interfaceConfigList.names.insert(irq->ifr_name);
    }

    // gather extra interfaces such as VMware
    // are not retruned by ioctl(SIOCGIFCONF)
    // check in /proc/net/dev
    std::vector<std::string> netDevLines = {};
    int ret = GetFileLines(PROCESS_DIR / PROCESS_NET_DEV, netDevLines, true, SicPtr()->errorMessage);
    if (ret != SIC_EXECUTABLE_SUCCESS || netDevLines.empty()) {
        return ret;
    }

    for (size_t i = 2; i < netDevLines.size(); ++i) {
        std::string netDev = netDevLines[i];
        netDev = netDev.substr(0, netDev.find_first_of(':'));
        netDev = TrimSpace(netDev);
        interfaceConfigList.names.insert(netDev);
    }

    return SIC_EXECUTABLE_SUCCESS;
}

/*
cat /proc/net/dev
Inter-|   Receive                                                |  Transmit
 face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed
    lo:       0       0    0    0    0     0          0         0        0       0    0    0    0     0       0          0
 tunl0:       0       0    0    0    0     0          0         0        0       0    0    0    0     0       0          0
ip6tnl0:       0       0    0    0    0     0          0         0        0       0    0    0    0     0       0          0
  eth0:     200       2    0    0    0     0          0         0        0       0    0    0    0     0       0          0
*/
int LinuxSystemInformationCollector::SicGetInterfaceInformation(const std::string &name,
                                                                SicNetInterfaceInformation &information) {
    std::vector<std::string> netDevLines = {};
    int ret = GetFileLines(PROCESS_DIR / PROCESS_NET_DEV, netDevLines, true, SicPtr()->errorMessage);
    if (ret != SIC_EXECUTABLE_SUCCESS || netDevLines.empty()) {
        return ret;
    }

    for (size_t i = 2; i < netDevLines.size(); ++i) {
        auto pos = netDevLines[i].find_first_of(':');
        std::string devCounterStr = netDevLines[i].substr(pos + 1);
        std::string devName = netDevLines[i].substr(0, pos);
        std::vector<std::string> netDevMetric = split(devCounterStr, ' ', true);
        if (netDevMetric.size() >= 16) {
            int index = 0;
            devName = TrimSpace(devName);
            if (devName == name) {
                information.rxBytes = convert<uint64_t>(netDevMetric[index++]);
                information.rxPackets = convert<uint64_t>(netDevMetric[index++]);
                information.rxErrors = convert<uint64_t>(netDevMetric[index++]);
                information.rxDropped = convert<uint64_t>(netDevMetric[index++]);
                information.rxOverruns = convert<uint64_t>(netDevMetric[index++]);
                information.rxFrame = convert<uint64_t>(netDevMetric[index++]);
                // skip compressed multicast
                index += 2;
                information.txBytes = convert<uint64_t>(netDevMetric[index++]);
                information.txPackets = convert<uint64_t>(netDevMetric[index++]);
                information.txErrors = convert<uint64_t>(netDevMetric[index++]);
                information.txDropped = convert<uint64_t>(netDevMetric[index++]);
                information.txOverruns = convert<uint64_t>(netDevMetric[index++]);
                information.txCollisions = convert<uint64_t>(netDevMetric[index++]);
                information.txCarrier = convert<uint64_t>(netDevMetric[index++]);

                information.speed = -1;
                return SIC_EXECUTABLE_SUCCESS;
            }
        }
    }

    SicPtr()->SetErrorMessage("Dev " + name + " Not Found!\n");
    return SIC_EXECUTABLE_FAILED;
}

int LinuxSystemInformationCollector::SicGetInterfaceConfig(SicInterfaceConfig &interfaceConfig, const std::string &name) {
    int sock;
    ifreq ifr{};
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        return ErrNo("socket(AF_INET, SOCK_DGRAM, 0)");
    }
    {
        defer(close(sock));

        interfaceConfig.name = name;
        strncpy(ifr.ifr_name, name.c_str(), sizeof(ifr.ifr_name));
        ifr.ifr_name[sizeof(ifr.ifr_name) - 1] = '\0';

        if (!ioctl(sock, SIOCGIFADDR, &ifr)) {
            interfaceConfig.address.addr.in = ifreq_s_addr(ifr);
            interfaceConfig.address.family = SicNetAddress::SIC_AF_INET;
        }

        if (!ioctl(sock, SIOCGIFNETMASK, &ifr)) {
            interfaceConfig.netmask.addr.in = ifreq_s_addr(ifr);
            interfaceConfig.netmask.family = SicNetAddress::SIC_AF_INET;
        }

        if (!ioctl(sock, SIOCGIFMTU, &ifr)) {
            interfaceConfig.mtu = ifr.ifr_mtu;
        }

        if (!ioctl(sock, SIOCGIFMETRIC, &ifr)) {
            interfaceConfig.metric = ifr.ifr_metric ? ifr.ifr_metric : 1;
        }

        if (!ioctl(sock, SIOCGIFTXQLEN, &ifr)) {
            interfaceConfig.txQueueLen = ifr.ifr_qlen;
        } else {
            interfaceConfig.txQueueLen = -1; /* net-tools behaviour */
        }
    }

    interfaceConfig.description = name;
    interfaceConfig.address6.family = SicNetAddress::SIC_AF_INET6;
    interfaceConfig.prefix6Length = 0;
    interfaceConfig.scope6 = 0;

    // ipv6
    std::vector<std::string> netInet6Lines = {};
    int ret = GetFileLines(PROCESS_DIR / PROCESS_NET_IF_INET6, netInet6Lines, true, SicPtr()->errorMessage);
    if (ret != SIC_EXECUTABLE_SUCCESS || netInet6Lines.empty()) {
        // Failure should not be returned without "/proc/net/if_inet6"
        return SIC_EXECUTABLE_SUCCESS;
    }

    enum {
        Inet6Address,   // 长度为32的16进制IPv6地址
        Inet6DevNo,     // netlink设备号
        Inet6PrefixLen, // 16进制表示的 prefix length
        Inet6Scope,     // scope
    };
    for (auto &devLine: netInet6Lines) {
        std::vector<std::string> netInet6Metric = split(devLine, ' ', true);
        std::string inet6Name = netInet6Metric.back();
        inet6Name = TrimSpace(inet6Name);
        if (inet6Name == name) {
            // Doc: https://ata.atatech.org/articles/11020228072?spm=ata.25287382.0.0.1c647536bhA7NG#lyRD52DR
            if (Inet6Address < netInet6Metric.size()) {
                auto *addr6 = (unsigned char *) &(interfaceConfig.address6.addr.in6);

                std::string addr = netInet6Metric[Inet6Address];
                const char *ptr = const_cast<char *>(addr.c_str());
                const char *ptrEnd = ptr + addr.size();

                constexpr const int addrLen = 16;
                for (int i = 0; i < addrLen && ptr + 1 < ptrEnd; i++, ptr += 2) {
                    addr6[i] = (unsigned char) Hex2Int(std::string{ptr, ptr + 2});
                }
            }
            if (Inet6PrefixLen < netInet6Metric.size()) {
                interfaceConfig.prefix6Length = convertHex<int>(netInet6Metric[Inet6PrefixLen]);
            }
            if (Inet6Scope < netInet6Metric.size()) {
                interfaceConfig.scope6 = convertHex<int>(netInet6Metric[Inet6Scope]);
            }
        }
    }

    return SIC_EXECUTABLE_SUCCESS;
}

void LinuxSystemInformationCollector::SicGetNetAddress(const std::string &str, SicNetAddress &address) {
    constexpr const size_t hexIntLen = 8;
    if (str.size() > hexIntLen) {
        address.family = SicNetAddress::SIC_AF_INET6;
        for (size_t i = 0, pos = 0; i < 4 && str.size() >= pos + hexIntLen; ++i, pos += hexIntLen) {
            address.addr.in6[i] = Hex2Int(str.substr(pos, hexIntLen));
        }
    } else {
        address.family = SicNetAddress::SIC_AF_INET;
        address.addr.in = (str.size() == hexIntLen) ? Hex2Int(str) : 0;
    }
}

void LinuxSystemInformationCollector::NetListenAddressAdd(SicNetConnectionInformation &connectionInformation) {
    if (SicPtr()->netListenCache.find(connectionInformation.localPort) != SicPtr()->netListenCache.end()) {
        if (connectionInformation.localAddress.family == SicNetAddress::SIC_AF_INET6) {
            return;
        }
    }
    SicPtr()->netListenCache[connectionInformation.localPort] = connectionInformation.localAddress;
}

int LinuxSystemInformationCollector::SicUpdateNetState(SicNetConnectionInformation &netInfo,
                                                       SicNetState &netState) {
    const int state = netInfo.state;
    if (netInfo.type == SIC_NET_CONN_TCP) {
        ++netState.tcpStates[state];
        if (state == SIC_TCP_LISTEN) {
            NetListenAddressAdd(netInfo);
        } else {
            if (SicPtr()->netListenCache.find(netInfo.localPort) != SicPtr()->netListenCache.end()) {
                ++netState.tcpInboundTotal;
            } else {
                ++netState.tcpOutboundTotal;
            }
        }

    } else if (netInfo.type == SIC_NET_CONN_UDP) {
        ++netState.udpSession;
    }

    netState.allInboundTotal = netState.tcpInboundTotal;
    netState.allOutboundTotal = netState.tcpOutboundTotal;

    return SIC_EXECUTABLE_SUCCESS;
}

/* /proc/net/tcp: 各项参数说明 https://blog.csdn.net/justlinux2010/article/details/21028797
  sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode
   0: 020011AC:B168 24BE7DB9:0050 06 00000000:00000000 03:00000C51 00000000     0        0 0 3 0000000000000000
   1: 020011AC:EE2C 0444F26E:0050 06 00000000:00000000 03:000013DA 00000000     0        0 0 3 0000000000000000
   2: 020011AC:B16E 24BE7DB9:0050 06 00000000:00000000 03:00000C51 00000000     0        0 0 3 0000000000000000
   3: 020011AC:C268 74FEE182:0050 06 00000000:00000000 03:00000C4D 00000000     0        0 0 3 0000000000000000
*/
int LinuxSystemInformationCollector::SicReadNetFile(SicNetState &netState,
                                                    int type,
                                                    bool useClient,
                                                    bool useServer,
                                                    const fs::path &path) {
    if (path.empty()) {
        LogDebug("{}(type: 0x{:x}, useClient: {}, useServer: {}): path is empty",
                 __FUNCTION__, type, useClient, useServer);
        return SIC_EXECUTABLE_FAILED;
    }

    std::vector<std::string> netLines = {};
    int ret = GetFileLines(path, netLines, true, SicPtr()->errorMessage);
    if (ret != SIC_EXECUTABLE_SUCCESS || netLines.size() <= 1) {
        LogDebug("{}(type: 0x{:x}, useClient: {}, useServer: {}, path: {}) => {}, netLines.size: {}: {}",
                 __FUNCTION__, type, useClient, useServer, path.string(), ret, netLines.size(),
                 getOrDefault(SicPtr()->errorMessage, "Success"));
        return ret;
    }

    // [0] 为header
    for (size_t i = 1; i < netLines.size() && ret == SIC_EXECUTABLE_SUCCESS; ++i) {
        SicNetConnectionInformation netInfo;
        std::vector<std::string> netMetric = split(netLines[i], ' ', true);
        if (netMetric.size() < 10) {
            continue;
        }
        int index = 1;
        std::vector<std::string> localAddressMetric = split(netMetric[index++], ':', true);  // local_address
        netInfo.localPort = convertHex<unsigned long>(localAddressMetric.back()) & 0xffff;

        std::vector<std::string> remoteAddressMetric = split(netMetric[index++], ':', true); // rem_address
        netInfo.remotePort = convertHex<unsigned long>(remoteAddressMetric.back()) & 0xffff;

        if (!((netInfo.remotePort == 0 && useClient) || (netInfo.remotePort != 0 && useServer))) {
            continue;
        }

        netInfo.type = type;
        SicGetNetAddress(localAddressMetric.front(), netInfo.localAddress);
        SicGetNetAddress(remoteAddressMetric.front(), netInfo.remoteAddress);

        // st
        netInfo.state = Hex2Int(netMetric[index++]);

        std::vector<std::string> queueMetric = split(netMetric[index++], ':', {false, true}); // tx_queue : rx_queue
        netInfo.sendQueue = Hex2Int(queueMetric.front());
        netInfo.receiveQueue = Hex2Int(queueMetric.back());

        ++index; // skip tr tm->when
        ++index; // skip retrnsmt

        netInfo.uid = convert<unsigned long>(netMetric[index++]); // uid
        ++index; // skip timeout
        netInfo.inode = convert<unsigned long>(netMetric[index++]); // inode

        // add connections
        ret = SicUpdateNetState(netInfo, netState);
    }

    return ret;
}

#define OPTION_NETLINK 1
#define OPTION_SS 2
#define OPTION_FILE 4

int LinuxSystemInformationCollector::SicGetNetState(SicNetState &netState,
                                                    bool useClient,
                                                    bool useServer,
                                                    bool useUdp,
                                                    bool useTcp,
                                                    bool useUnix,
                                                    int option) {
    typedef decltype(&LinuxSystemInformationCollector::SicGetNetStateByNetLink) FnType;
    struct {
        int option;
        FnType fn;
    } funcs[] = {
            {OPTION_NETLINK, &LinuxSystemInformationCollector::SicGetNetStateByNetLink},
            {OPTION_SS,      &LinuxSystemInformationCollector::SicGetNetStateBySS},
            {OPTION_FILE,    &LinuxSystemInformationCollector::SicGetNetStateByReadFile},
    };
    const size_t funcSize = sizeof(funcs) / sizeof(funcs[0]);

    int ret = SIC_EXECUTABLE_FAILED;
    for (size_t i = 0; i < funcSize && ret != SIC_EXECUTABLE_SUCCESS; i++) {
        const auto &entry = funcs[i];
        if (option & entry.option) {
            ret = (this->*entry.fn)(netState, useClient, useServer, useUdp, useTcp, useUnix);
        }
    }
    return ret;
}

#ifndef MOUNTED
#   define MOUNTED "/etc/mtab"
#endif

int LinuxSystemInformationCollector::SicGetFileSystemListInformation(std::vector<SicFileSystem> &informations) {
    FILE *fp;

    // MOUNTED: /etc/mtab, defined in /usr/include/paths.h
    if (!(fp = setmntent(MOUNTED, "r"))) {
        return ErrNo((sout{} << "SicGetFileSystemListInformation, setmntent(" << MOUNTED << ", \"r\")").str());
    }
    defer(endmntent(fp));

    mntent ent{};
    std::vector<char> buffer((size_t)4096);
    while (getmntent_r(fp, &ent, &buffer[0], buffer.size())) {
        SicFileSystem fileSystem;
        fileSystem.type = SIC_FILE_SYSTEM_TYPE_UNKNOWN;
        fileSystem.dirName = ent.mnt_dir;
        fileSystem.devName = ent.mnt_fsname;
        fileSystem.sysTypeName = ent.mnt_type;
        fileSystem.options = ent.mnt_opts;

        SicGetFileSystemType(fileSystem.sysTypeName, fileSystem.type, fileSystem.typeName);
        informations.push_back(fileSystem);
    }
    // #ifdef ENABLE_COVERAGE
    //     std::stringstream ss;
    //     ss << (__FILE__ + SRC_ROOT_OFFSET) << ":" << __LINE__ << ": " << std::endl;
    //     int count = 0;
    //     for(auto &entry: informations) {
    //         ss << "[" << count++ << "] dirName: " << entry.dirName << ", dev: " << entry.devName
    //         << ", sysTypeName: " << entry.sysTypeName << ", sysType: " << entry.type
    //         << "(" << entry.typeName << ")" << std::endl;
    //     }
    //     std::cout << ss.str() << std::endl;
    // #endif

    return SIC_EXECUTABLE_SUCCESS;
}

/*
> cat /proc/uptime
183857.30 1969716.84
第一列: 系统启动到现在的时间（以秒为单位）；
第二列: 系统空闲的时间（以秒为单位）。
*/
int LinuxSystemInformationCollector::SicGetUpTime(double &uptime) {
    std::vector<std::string> uptimeLines;
    int ret = GetFileLines(PROCESS_DIR / PROCESS_UPTIME, uptimeLines, true, SicPtr()->errorMessage);
    if (ret == SIC_EXECUTABLE_SUCCESS) {
        std::vector<std::string> uptimeMetric = split((uptimeLines.empty() ? "" : uptimeLines.front()), ' ', true);
        uptime = convert<double>(uptimeMetric.front());
    }
    return ret;
}

bool IsDev(const std::string &dirName) {
    return HasPrefix(dirName, "/dev/");
}

static uint64_t cacheId(const struct stat &ioStat) {
    return S_ISBLK(ioStat.st_mode) ? ioStat.st_rdev : (ioStat.st_ino + ioStat.st_dev);
}

void LinuxSystemInformationCollector::RefreshLocalDisk() {
    auto &cache = SicPtr()->fileSystemCache;

    std::vector<SicFileSystem> fileSystemList;
    int ret = SicGetFileSystemListInformation(fileSystemList);
    if (ret == SIC_EXECUTABLE_SUCCESS) {
        for (auto const &fileSystem: fileSystemList) {
            if (fileSystem.type == SIC_FILE_SYSTEM_TYPE_LOCAL_DISK && IsDev(fileSystem.devName)) {
                struct stat ioStat{};
                if (stat(fileSystem.dirName.c_str(), &ioStat) < 0) {
                    continue;
                }
                uint64_t id = cacheId(ioStat);
                if (cache.find(id) == cache.end()) {
                    auto ioDev = std::make_shared<SicIODev>();
                    ioDev->isPartition = true;
                    ioDev->name = fileSystem.devName;
                    cache[id] = ioDev;
                }
            }
        }
    }
}

std::shared_ptr<SicIODev> LinuxSystemInformationCollector::SicGetIODev(std::string &dirName) {
    auto &cache = SicPtr()->fileSystemCache;

    if (!HasPrefix(dirName, "/")) {
        dirName = "/dev/" + dirName;
    }

    struct stat ioStat{};
    if (stat(dirName.c_str(), &ioStat) < 0) {
        SicPtr()->errorMessage = (sout{} << "stat(" << dirName << ") error: " << strerror(errno)).str();
        return std::shared_ptr<SicIODev>{};
    }
    print(dirName, ioStat);

    uint64_t targetId = cacheId(ioStat);
    if (cache.find(targetId) != cache.end()) {
        return cache[targetId];
    }

    if (IsDev(dirName)) {
        // 如果确定是设备文件，则直接缓存，无需再枚举设备列表
        auto ioDev = std::make_shared<SicIODev>();
        ioDev->name = dirName;
        cache[targetId] = ioDev;
        return ioDev;
    }

    RefreshLocalDisk();

    auto targetIt = cache.find(targetId);
    if (targetIt != cache.end() && !targetIt->second->name.empty()) {
        return targetIt->second;
    }
    SicPtr()->errorMessage = (sout{} << "<" << dirName << "> not a valid disk folder").str();
    return std::shared_ptr<SicIODev>{};
}

enum class EnumDiskStats {
    major,
    minor,
    devName,

    reads,
    readsMerged,
    readSectors,
    rMillis,

    writes,
    writesMerged,
    writeSectors,
    wMillis,

    ioCount,
    rwMillis, // 输入输出花费的毫秒数
    qMillis,  // 输入/输出操作花费的加权毫秒数

    count, // 这个用于收尾，不是实际的列号。
};
static_assert((int)EnumDiskStats::count == 14, "EnumDiskStats::count unexpected");

int LinuxSystemInformationCollector::GetDiskStat(dev_t rDev, const std::string &dirName,
                                                 SicDiskUsage &disk, SicDiskUsage &deviceUsage) {
    std::vector<std::string> diskLines = {};
    int ret = GetFileLines(PROCESS_DIR / PROCESS_DISKSTATS, diskLines, true, SicPtr()->errorMessage);
    if (ret == SIC_EXECUTABLE_SUCCESS) {
        for (auto const &diskLine: diskLines) {
            std::vector<std::string> diskMetric = split(diskLine, ' ', true);
            if (diskMetric.size() < (size_t)EnumDiskStats::count) {
                continue;
            }

            auto get_int = [&](EnumDiskStats key) {
                return convert<uint64_t>(diskMetric[(int)key]);
            };

            // int currentIndex = 0;
            // 1  major number
            auto devMajor = convert<decltype(major(rDev))>(diskMetric[(int)EnumDiskStats::major]);
            // 2  minor number
            auto devMinor = convert<decltype(minor(rDev))>(diskMetric[(int)EnumDiskStats::minor]);
            if (devMajor == major(rDev) && (0 == devMinor || devMinor == minor(rDev))) {
                // 3  device name
                // ++currentIndex;
                // 4  reads completed successfully
                disk.reads = get_int(EnumDiskStats::reads);
                // 5  reads merged
                // ++currentIndex;
                //	6  sectors read
                disk.readBytes = get_int(EnumDiskStats::readSectors) * 512;
                // 7  time spent reading (ms)
                disk.rTime = get_int(EnumDiskStats::rMillis);
                // 8  writes completed
                disk.writes = get_int(EnumDiskStats::writes);
                // 9  writes merged
                // ++currentIndex;
                // 10  sectors written
                disk.writeBytes = get_int(EnumDiskStats::writeSectors) * 512;
                // 11  time spent writing (ms)
                disk.wTime = get_int(EnumDiskStats::wMillis);
                // 12  I/Os currently in progress
                // ++currentIndex;
                // 13  time spent doing I/Os (ms)
                disk.time = get_int(EnumDiskStats::rwMillis);
                // 14  weighted time spent doing I/Os (ms)
                disk.qTime = get_int(EnumDiskStats::qMillis);
                if (devMinor == 0) {
                    deviceUsage = disk;
                }
                if (devMinor == minor(rDev)) {
                    return SIC_EXECUTABLE_SUCCESS;
                }
            }
        }
        SicPtr()->errorMessage = (sout{} << "<" << dirName << "> not exist or has been removed").str();
        ret = SIC_EXECUTABLE_FAILED;
    }

    return ret;
}

// dirName可以是devName，也可以是dirName
int LinuxSystemInformationCollector::SicGetIOstat(std::string &dirName,
                                                  SicDiskUsage &disk,
                                                  std::shared_ptr<SicIODev> &ioDev,
                                                  SicDiskUsage &deviceUsage) {
    // 本函数的思路dirName -> devName -> str_rdev(设备号)
    // 1. 通过dirName找到devName
    ioDev = SicGetIODev(dirName);
    if (!ioDev) {
        return SIC_EXECUTABLE_FAILED;
    }

    struct stat ioStat{};
    // 此处使用设备名，以获取 更多stat信息，如st_rdev(驱动号、设备号)
    // 其实主要目的就是为了获取st_rdev
    if (stat(ioDev->name.c_str(), &ioStat) < 0) {
        return ErrNo("stat(" + ioDev->name + ")");
    }
    print(ioDev->name, ioStat);

    // 2. 统计dev的磁盘使用情况
    return GetDiskStat(ioStat.st_rdev, dirName, disk, deviceUsage);
}

int LinuxSystemInformationCollector::SicGetDiskUsage(SicDiskUsage &diskUsage, std::string dirName) {
    if (SicPtr()->ioType != IO_STATE_DISKSTATS) {
        SicPtr()->SetErrorMessage("Linux kernel too old !\n");
        return ENOENT;
    }

    std::shared_ptr<SicIODev> ioDev;
    SicDiskUsage deviceUsage{};
    int status = SicGetIOstat(dirName, diskUsage, ioDev, deviceUsage);

    if (status == SIC_EXECUTABLE_SUCCESS && ioDev) {
        // if (ioDev->isPartition) {
        //     /* 2.6 kernels do not have per-partition times */
        //     diskUsage = deviceUsage;
        // }
        diskUsage.devName = ioDev->name;
        diskUsage.dirName = dirName;
        status = CalDiskUsage(*ioDev, (ioDev->isPartition ? deviceUsage : diskUsage));
        if (status == SIC_EXECUTABLE_SUCCESS && ioDev->isPartition) {
            diskUsage.serviceTime = deviceUsage.serviceTime;
            diskUsage.queue = deviceUsage.queue;
        }
    }

    return status;
}

// See sigar_disk_usage_get
int LinuxSystemInformationCollector::CalDiskUsage(SicIODev &ioDev, SicDiskUsage &diskUsage) {
    double uptime;
    int status = SicGetUpTime(uptime);
    if (status == SIC_EXECUTABLE_SUCCESS) {
        diskUsage.snapTime = uptime;

        double interval = diskUsage.snapTime - ioDev.diskUsage.snapTime;

        diskUsage.serviceTime = -1;
        if (diskUsage.time != std::numeric_limits<uint64_t>::max()) {
            uint64_t ios = Diff(diskUsage.reads, ioDev.diskUsage.reads)
                                     + Diff(diskUsage.writes, ioDev.diskUsage.writes);
            double tmp = ((double) ios) * HZ / interval;
            double util = ((double) (diskUsage.time - ioDev.diskUsage.time)) / interval * HZ;

            diskUsage.serviceTime = (tmp != 0 ? util / tmp : 0);
        }

        diskUsage.queue = -1;
        if (diskUsage.qTime != std::numeric_limits<uint64_t>::max()) {
            // 浮点运算：0.0/0.0 => nan, 1.0/0.0 => inf
            double util = ((double) (diskUsage.qTime - ioDev.diskUsage.qTime)) / interval;
            diskUsage.queue = util / 1000.0;
        }

        if (!std::isfinite(diskUsage.queue)) {
            std::stringstream ss;
            ss << "diskUsage.queue is not finite: " << diskUsage.queue << std::endl
               << "                       uptime: " << uptime << " s" << std::endl
               << "                     interval: " << interval << " s" << std::endl
               << "              diskUsage.qTime: " << diskUsage.qTime << std::endl
               << "        ioDev.diskUsage.qTime: " << ioDev.diskUsage.qTime << std::endl;
            SicPtr()->errorMessage = ss.str();
        }

        ioDev.diskUsage = diskUsage;
    }
    return status;
}

int LinuxSystemInformationCollector::SicGetFileSystemUsage(SicFileSystemUsage &fileSystemUsage, std::string dirName) {
    struct statvfs buffer{};
    int status = statvfs(dirName.c_str(), &buffer);
    if (status != 0) {
        return ErrNo("statvfs(" + dirName + ")");
    }

#define TEST_BIT(V, B) (V & B ? #B : "")
    LogDebug(R"(statvfs({}) {{
    f_bsize  : {}
    f_frsize : {}
    f_blocks : {}
    f_bfree  : {}
    f_bavail : {}
    f_files  : {}
    f_ffree  : {}
    f_favail : {}
    f_namemax: {}
    f_flag   : 0x{:x} {} {}
}})", dirName, buffer.f_bsize, buffer.f_frsize, buffer.f_blocks, buffer.f_bfree, buffer.f_bavail,
             buffer.f_files, buffer.f_ffree, buffer.f_favail, buffer.f_namemax, buffer.f_flag,
             TEST_BIT(buffer.f_flag, ST_RDONLY), TEST_BIT(buffer.f_flag, ST_NOSUID));
#undef TEST_BIT

    // chengjie.hcj 单位是: KB ?
    uint64_t bsize = buffer.f_frsize / 512;
    fileSystemUsage.total = ((buffer.f_blocks * bsize) >> 1);
    fileSystemUsage.free = ((buffer.f_bfree * bsize) >> 1);
    fileSystemUsage.avail = ((buffer.f_bavail * bsize) >> 1); // 非超级用户最大可使用的磁盘量
    fileSystemUsage.used = Diff(fileSystemUsage.total, fileSystemUsage.free);
    fileSystemUsage.files = buffer.f_files;
    fileSystemUsage.freeFiles = buffer.f_ffree;

    // 此处为用户可使用的磁盘量，可能会与fileSystemUsage.total有差异。也就是说:
    // 当total < fileSystemUsage.total时，表明即使磁盘仍有空间，用户也申请不到了
    // 毕竟OS维护磁盘，会占掉一部分，比如文件分配表，目录文件等。
    uint64_t total = fileSystemUsage.used + fileSystemUsage.avail;
    uint64_t used = fileSystemUsage.used;
    double percent = 0;
    if (total != 0) {
        // 磁盘占用率，使用的是用户最大可用磁盘总量来的，而非物理磁盘总量
        percent = (double) used / (double) total;
    }
    fileSystemUsage.use_percent = percent;

    SicGetDiskUsage(fileSystemUsage.disk, dirName);

    return SIC_EXECUTABLE_SUCCESS;
}

// udevadm info --query=property --name=/dev/vda | grep ID_SERIAL= | awk --field-separator='=' '{print $2}'
std::map<std::string, std::string> LinuxSystemInformationCollector::queryDevSerialId(const std::string &devName) {
    std::map<std::string, std::string> ret;
    fs::path path = Which("udevadm");
    if (!path.empty()) {
        std::string cmd = fmt::format("{} info --query=property --name={} 2>/dev/null", path.string(), devName);
        std::vector<std::string> lines;
        ExecCmd(cmd, lines, false);

        std::stringstream ss;
        for (const auto &line: lines) {
            ss << line << std::endl;
        }
        LogInfo("{}\n{}", cmd, TrimRightSpace(ss.str()));

        ret[devName] = {};
        const std::string prefix = "ID_SERIAL=";
        for (const auto &line: lines) {
            if (HasPrefix(line, prefix)) {
                ret[devName] = TrimSpace(line.substr(prefix.size()));
                break;
            }
        }
    }
    LogInfo("queryDevSerialId: [{}]{}", ret.size(), boost::json::serialize(boost::json::value_from(ret)));

	RETURN_RVALUE(ret);
}

// 遍历指定目录下的数字目录
int LinuxSystemInformationCollector::walkDigitDirs(const fs::path &root,
                                                   const std::function<void(const std::string &)> &callback) {
    if (!fs::exists(root) || !fs::is_directory(root)) {
        return ErrNo(ENOENT, root.string());
    }

    for (const auto &dirEntry: fs::directory_iterator{root, fs::directory_options::skip_permission_denied}) {
        std::string filename = dirEntry.path().filename().string();
        if (IsInt(filename)) {
            callback(filename);
        }
    }
    return SIC_EXECUTABLE_SUCCESS;
}

int LinuxSystemInformationCollector::SicGetProcessList(SicProcessList &processList, size_t limit, bool &overflow) {
    processList.pids.clear();
    processList.pids.reserve(2048);
    overflow = false;
    int ret = SIC_EXECUTABLE_SUCCESS;
    try {
        ret = walkDigitDirs(PROCESS_DIR, [&](const std::string &dirName) {
            auto pid = convert<pid_t>(dirName);
            if (pid != 0) {
                processList.pids.push_back(pid);
                if (limit != 0 && processList.pids.size() > limit) {
                    throw OverflowException();
                }
            }
        });
    } catch (const OverflowException &) {
        overflow = true;
    }
    return ret;
}

// See https://man7.org/linux/man-pages/man5/proc.5.html
enum class EnumProcessStat : int {
    pid,                   // 0
    comm,                  // 1
    state,                 // 2
    ppid,                  // 3
    pgrp,                  // 4
    session,               // 5
    tty_nr,                // 6
    tpgid,                 // 7
    flags,                 // 8
    minflt,                // 9
    cminflt,               // 10
    majflt,                // 11
    cmajflt,               // 12
    utime,                 // 13
    stime,                 // 14
    cutime,                // 15
    cstime,                // 16
    priority,              // 17
    nice,                  // 18
    num_threads,           // 19
    itrealvalue,           // 20
    starttime,             // 21
    vsize,                 // 22
    rss,                   // 23
    rsslim,                // 24
    startcode,             // 25
    endcode,               // 26
    startstack,            // 27
    kstkesp,               // 28
    kstkeip,               // 29
    signal,                // 30
    blocked,               // 31
    sigignore,             // 32
    sigcatch,              // 33
    wchan,                 // 34
    nswap,                 // 35
    cnswap,                // 36
    exit_signal,           // 37
    processor,             // 38 <--- 至少需要有该字段
    rt_priority,           // 39
    policy,                // 40
    delayacct_blkio_ticks, // 41
    guest_time,            // 42
    cguest_time,           // 43
    start_data,            // 44
    end_data,              // 45
    start_brk,             // 46
    arg_start,             // 47
    arg_end,               // 48
    env_start,             // 49
    env_end,               // 50
    exit_code,             // 51

    _count, // 只是用于计数，非实际字段
};
static_assert((int) EnumProcessStat::comm == 1, "EnumProcessStat invalid");
static_assert((int) EnumProcessStat::processor == 38, "EnumProcessStat invalid");

constexpr int operator-(EnumProcessStat a, EnumProcessStat b) {
    return (int) a - (int) b;
}

int LinuxSystemInformationCollector::ParseProcessStat(const fs::path &processStat, pid_t pid,
                                                      SicLinuxProcessInfo &processInfo) {
    processInfo.pid = pid;
    // processInfo.mTime = time(nullptr);

    std::string line = ReadFileContent(processStat, &SicPtr()->errorMessage);
    if (line.empty()) {
        return SIC_EXECUTABLE_FAILED;
    }

    auto invalidStatError = [&processStat](const char *detail) {
        return (sout{} << "<" << processStat.string() << "> not a valid process stat file: " << detail).str();
    };

    auto nameStartPos = line.find_first_of('(');
    auto nameEndPos = line.find_last_of(')');
    if (nameStartPos == std::string::npos || nameEndPos == std::string::npos) {
        SicPtr()->errorMessage = invalidStatError("can't find process name");
        return SIC_EXECUTABLE_FAILED;
    }
    nameStartPos++; // 跳过左括号
    processInfo.name = line.substr(nameStartPos, nameEndPos - nameStartPos);
    line = line.substr(nameEndPos + 2); // 跳过右括号及空格

    std::vector<std::string> words = split(line, ' ', false);

    constexpr const EnumProcessStat offset = EnumProcessStat::state;  // 跳过pid, comm
    constexpr const int minCount = EnumProcessStat::processor - offset + 1;  // 37
    if (words.size() < minCount) {
        SicPtr()->errorMessage = invalidStatError("unexpected item count");
        return SIC_EXECUTABLE_FAILED;
    }

    TVector<EnumProcessStat> v{words, offset};

    processInfo.state = v[EnumProcessStat::state].front();
    processInfo.parentPid = convert<pid_t>(v[EnumProcessStat::ppid]);
    processInfo.tty = convert<int>(v[EnumProcessStat::tty_nr]);
    processInfo.minorFaults = convert<uint64_t>(v[EnumProcessStat::minflt]);
    processInfo.majorFaults = convert<uint64_t>(v[EnumProcessStat::majflt]);

    MillisVector<milliseconds, EnumProcessStat> mv{v};
    processInfo.utime = mv[EnumProcessStat::utime];
    processInfo.stime = mv[EnumProcessStat::stime];
    processInfo.cutime = mv[EnumProcessStat::cutime];
    processInfo.cstime = mv[EnumProcessStat::cstime];

    processInfo.priority = convert<int>(v[EnumProcessStat::priority]);
    processInfo.nice = convert<int>(v[EnumProcessStat::nice]);
    processInfo.numThreads = convert<int>(v[EnumProcessStat::num_threads]);

    processInfo.startTime = std::chrono::system_clock::time_point{
            mv[EnumProcessStat::starttime] + milliseconds{SicPtr()->bootSeconds * 1000}};
    processInfo.vSize = convert<uint64_t>(v[EnumProcessStat::vsize]);
    processInfo.rss = convert<uint64_t>(v[EnumProcessStat::rss]) << (SicPtr()->pagesize);
    processInfo.processor = convert<int>(v[EnumProcessStat::processor]);

    return SIC_EXECUTABLE_SUCCESS;
}

// 数据样例: /proc/1/stat
// 1 (cat) R 0 1 1 34816 1 4194560 1110 0 0 0 1 1 0 0 20 0 1 0 18938584 4505600 171 18446744073709551615 4194304 4238788 140727020025920 0 0 0 0 0 0 0 0 0 17 3 0 0 0 0 0 6336016 6337300 21442560 140727020027760 140727020027777 140727020027777 140727020027887 0
int LinuxSystemInformationCollector::SicReadProcessStat(pid_t pid, SicLinuxProcessInfoCache &processInfo) {
    // auto &processInfo = SicPtr()->linuxProcessInfo;
    //
    // time_t nowTime = time(nullptr);
    //
    // // short live cache
    // if (processInfo.pid == pid && (nowTime - processInfo.mTime) < SicPtr()->lastProcExpire) {
    //     return processInfo.result;
    // }

    return ParseProcessStat(PROCESS_DIR / pid / PROCESS_STAT, pid, processInfo);
    // return processInfo.result;
}

int LinuxSystemInformationCollector::SicGetProcessState(pid_t pid, SicProcessState &processState) {
    SicLinuxProcessInfoCache processInfo;
    int status = SicReadProcessStat(pid, processInfo);
    if (status != SIC_EXECUTABLE_SUCCESS) {
        return status;
    }
    // SicLinuxProcessInfo &processInfo = SicPtr()->linuxProcessInfo;

    processState.name = processInfo.name;
    processState.state = processInfo.state;
    processState.tty = processInfo.tty;
    processState.parentPid = processInfo.parentPid;
    processState.priority = processInfo.priority;
    processState.nice = processInfo.nice;
    processState.processor = processInfo.processor;
    processState.threads = processInfo.numThreads;
    if (!processState.threads) {
        SicGetProcessThreads(pid, processState.threads);
    }
    return SIC_EXECUTABLE_SUCCESS;
}

int LinuxSystemInformationCollector::SicGetProcessTime(pid_t pid, SicProcessTime &output, bool includeCTime) {
    SicLinuxProcessInfoCache processInfo;
    int status = SicReadProcessStat(pid, processInfo);
    if (status != SIC_EXECUTABLE_SUCCESS) {
        return status;
    }
    // const SicLinuxProcessInfo &processInfo = SicPtr()->linuxProcessInfo;

    output.startTime = processInfo.startTime;

    output.cutime = (includeCTime ? processInfo.cutime : 0_ms);
    output.cstime = (includeCTime ? processInfo.cstime : 0_ms);

    output.user = processInfo.utime + output.cutime;
    output.sys = processInfo.stime + output.cstime;

    output.total = output.user + output.sys;
    // #ifdef ENABLE_COVERAGE
    //     std::cout << __FILE__ << ":" << __LINE__ << ":" << date::format<3>(boost::chrono::system_clock::now()) << std::endl
    //               << "startTime: " << output.startTime << std::endl
    //               << "cutime:    " << processInfo.cutime << std::endl
    //               << "cstime:    " << processInfo.cstime << std::endl
    //               << " utime:    " << processInfo.utime << std::endl
    //               << " stime:    " << processInfo.stime << std::endl
    //               << "  user:    " << output.user << std::endl
    //               << "   sys:    " << output.sys << std::endl
    //               << " total:    " << output.total << std::endl
    //               << std::endl;
    // #endif

    return SIC_EXECUTABLE_SUCCESS;
}

int LinuxSystemInformationCollector::SicGetProcessMemoryInformation(pid_t pid,
                                                                    SicProcessMemoryInformation &information) {
    SicLinuxProcessInfoCache processInfo;
    int status = SicReadProcessStat(pid, processInfo);
    if (status == SIC_EXECUTABLE_SUCCESS) {
        // const SicLinuxProcessInfo &processInfo = SicPtr()->linuxProcessInfo;
        information.minorFaults = processInfo.minorFaults;
        information.majorFaults = processInfo.majorFaults;
        information.pageFaults = information.minorFaults + information.majorFaults;

        std::vector<std::string> lines = {};
        const auto procStatm = PROCESS_DIR / pid / PROCESS_STATM;
        status = GetFileLines(procStatm, lines, true, &SicPtr()->errorMessage);
        if (status == SIC_EXECUTABLE_SUCCESS) {
            std::vector<std::string> processMemoryMetric = split((lines.empty() ? "" : lines.front()), ' ', false);
            if (processMemoryMetric.size() < 3) {
                SicPtr()->errorMessage = (sout{} << "<" << procStatm.string() << "> not a valid statm file").str();
                return SIC_EXECUTABLE_FAILED;
            }
            int index = 0;
            information.size = (convert<uint64_t>(processMemoryMetric[index++])) << (SicPtr()->pagesize);
            information.resident = (convert<uint64_t>(processMemoryMetric[index++])) << (SicPtr()->pagesize);
            information.share = (convert<uint64_t>(processMemoryMetric[index++])) << (SicPtr()->pagesize);
        }
    }
    return status;
}

int LinuxSystemInformationCollector::SicGetProcessArgs(pid_t pid, SicProcessArgs &processArgs) {
#if 0
    std::vector<std::string> cmdLines = {};
    int ret = GetFileLines(PROCESS_DIR / pid / PROCESS_CMDLINE, cmdLines, true, &SicPtr()->errorMessage);
    if (ret != SIC_EXECUTABLE_SUCCESS || cmdLines.empty()) {
        return ret;
    }

    for (auto &cmdLine: cmdLines) {
        auto cmdlineMetric = split(cmdLine, '\0', {false, true});
        for (auto const &metric: *cmdlineMetric) {
            processArgs.args.push_back(metric);
        }
    }
#else
    SicPtr()->errorMessage = "";
    std::string cmd = ReadFileContent(PROCESS_DIR / pid / PROCESS_CMDLINE, &SicPtr()->errorMessage);
    if (cmd.empty()) {
        return SicPtr()->errorMessage.empty() ? SIC_EXECUTABLE_SUCCESS : SIC_EXECUTABLE_FAILED;
    }
    auto cmdlineMetric = split(cmd, '\0', {false, true});
    for (auto const &metric: cmdlineMetric) {
        processArgs.args.push_back(metric);
    }
#endif
    return 0;
}

int LinuxSystemInformationCollector::SicGetProcessExe(pid_t pid, SicProcessExe &processExe) {
    struct {
        fs::path path;
        std::string &var;
    } files[] = {
            {PROCESS_DIR / pid / PROCESS_CWD,  processExe.cwd},
            {PROCESS_DIR / pid / PROCESS_EXE,  processExe.name},
            {PROCESS_DIR / pid / PROCESS_ROOT, processExe.root},
    };

    for (auto &entry: files) {
        std::string buffer = ReadLink(entry.path.string());
        if (buffer.empty()) {
            return ErrNo("SicGetProcessExe(" + entry.path.string() + ")");
        }
        entry.var = buffer;
    }

    return SIC_EXECUTABLE_SUCCESS;
}

int LinuxSystemInformationCollector::SicGetProcessCredName(pid_t pid,
                                                           SicProcessCredName &processCredName) {
    std::vector<std::string> processStatusLines = {};
    int ret = GetFileLines(PROCESS_DIR / pid / PROCESS_STATUS, processStatusLines, true, &SicPtr()->errorMessage);
    if (ret != SIC_EXECUTABLE_SUCCESS || processStatusLines.size() < 2) {
        return ret;
    }

    SicProcessCred cred{};
    for (size_t i = 2; i < processStatusLines.size(); ++i) {
        auto metric = split(processStatusLines[i], '\t', false);
        if (metric.size() >= 3 && metric.front() == "Uid:") {
            int index = 1;
            cred.uid = convert<uint64_t>(metric[index++]);
            cred.euid = convert<uint64_t>(metric[index]);
        } else if (metric.size() >= 3 && metric.front() == "Gid:") {
            int index = 1;
            cred.gid = convert<uint64_t>(metric[index++]);
            cred.egid = convert<uint64_t>(metric[index]);
        }
    }

    passwd *pw = nullptr;
    passwd pwbuffer;
    char buffer[2048];
    if (getpwuid_r(cred.uid, &pwbuffer, buffer, sizeof(buffer), &pw) != 0) {
        return ErrNo(sout{} << "SicGetProcessCredName(" << pid << "), getpwuid_r");
    }
    if (pw == nullptr) {
        return ErrNo(ENOENT, (sout{} << "SicGetProcessCredName(" << pid << "), passwd is null"));
    }
    processCredName.user = pw->pw_name;

    group *grp = nullptr;
    group grpbuffer{};
    char groupBuffer[2048];
    if (getgrgid_r(cred.gid, &grpbuffer, groupBuffer, sizeof(groupBuffer), &grp)) {
        return ErrNo(sout{} << "SicGetProcessCredName(" << pid << "), getgrgid_r");
    }

    if (grp != nullptr && grp->gr_name != nullptr) {
        processCredName.group = grp->gr_name;
    }

    return SIC_EXECUTABLE_SUCCESS;
}

int LinuxSystemInformationCollector::SicGetProcessFd(pid_t pid, SicProcessFd &processFd) {
    fs::path path = PROCESS_DIR / pid / PROCESS_FD;

    const int maxCount = 10000;
    int count = 0;
    int ret;
    try {
        ret = walkDigitDirs(path, [&count](const std::string &) {
            ++count;
            if (count >= maxCount) {
                throw OverflowException();
            }
        });
        if (ret == SIC_EXECUTABLE_SUCCESS) {
            processFd.total = count;
            processFd.exact = true;
        }
    } catch (const OverflowException &ex) {
        ret = SIC_EXECUTABLE_SUCCESS;
        processFd.total = count;
        processFd.exact = false;
    }
    return ret;
}

// /proc/1/status
// Name:	cat
// Umask:	0022
// State:	R (running)
// ...
// CoreDumping:	0
// THP_enabled:	1
// Threads:	1
// SigQ:	1/15252
// SigPnd:	0000000000000000
// ShdPnd:	0000000000000000
// ...
int LinuxSystemInformationCollector::SicGetProcessThreads(pid_t pid, uint64_t &threadCount) {
    threadCount = 0;

    std::vector<std::string> lines;
    auto procFile = PROCESS_DIR / pid / PROCESS_STATUS;
    int ret = GetFileLines(procFile, lines, true, &SicPtr()->errorMessage);
    if (ret == SIC_EXECUTABLE_SUCCESS) {
        bool found = false;
        const std::string prefix = "Threads:";
        for (size_t i = 0; i < lines.size() && !found; i++) {
            found = HasPrefix(lines[i], prefix);
            if (found) {
                std::string v = TrimSpace(lines[i].substr(prefix.size()));
                threadCount = convert<int>(v);
            }
        }
        if (!found) {
            SicPtr()->errorMessage = (sout{} << "<" << procFile.string() << "> error, not found process thread").str();
            ret = SIC_EXECUTABLE_FAILED;
        }
    }
    return ret;
}

int LinuxSystemInformationCollector::SicGetNetStateByReadFile(SicNetState &netState,
                                                              bool useClient,
                                                              bool useServer,
                                                              bool useUdp,
                                                              bool useTcp,
                                                              bool useUnix) {
    int ret = ((useTcp || useUdp) ? SIC_EXECUTABLE_SUCCESS: SIC_EXECUTABLE_FAILED);
    if (useTcp) {
        ret = SicReadNetFile(netState, SIC_NET_CONN_TCP, useClient, useServer, PROCESS_DIR / PROCESS_NET_TCP);

        auto netTcp6 = PROCESS_DIR / PROCESS_NET_TCP6;
        if (ret == SIC_EXECUTABLE_SUCCESS && fs::exists(netTcp6)) {
            ret = SicReadNetFile(netState, SIC_NET_CONN_TCP, useClient, useServer, netTcp6);
        }
    }

    if (ret == SIC_EXECUTABLE_SUCCESS && useUdp) {
        ret = SicReadNetFile(netState, SIC_NET_CONN_UDP, useClient, useServer, PROCESS_DIR / PROCESS_NET_UDP);

        auto netUdp6 = PROCESS_DIR / PROCESS_NET_UDP6;
        if (ret == SIC_EXECUTABLE_SUCCESS && fs::exists(netUdp6)) {
            ret = SicReadNetFile(netState, SIC_NET_CONN_UDP, useClient, useServer, netUdp6);
        }
    }

    if (ret == SIC_EXECUTABLE_SUCCESS) {
        netState.calcTcpTotalAndNonEstablished();
    }
    return ret;
}

int LinuxSystemInformationCollector::ReadNetLink(std::vector<uint64_t> &tcpStateCount) {
    static uint32_t sequence_number = 1;
    int fd;
    // struct inet_diag_msg *r;
    //使用netlink socket与内核通信
    fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_INET_DIAG);
    if (fd < 0) {
        SicPtr()->SetErrorMessage("ReadNetLink, socket(AF_NETLINK, SOCK_RAW, NETLINK_INET_DIAG) failed:"
                                  + std::string(strerror(errno)));
        return SIC_EXECUTABLE_FAILED;
    }
    defer(close(fd));

    //存在多个netlink socket时，必须单独bind,并通过nl_pid来区分
    struct sockaddr_nl nladdr_bind{};
    memset(&nladdr_bind, 0, sizeof(nladdr_bind));
    nladdr_bind.nl_family = AF_NETLINK;
    nladdr_bind.nl_pad = 0;
    nladdr_bind.nl_pid = getpid();
    nladdr_bind.nl_groups = 0;
    if (bind(fd, (struct sockaddr *) &nladdr_bind, sizeof(nladdr_bind))) {
        SicPtr()->SetErrorMessage("ReadNetLink, bind netlink socket failed:" + std::string(strerror(errno)));
        return SIC_EXECUTABLE_FAILED;
    }
    struct sockaddr_nl nladdr{};
    memset(&nladdr, 0, sizeof(nladdr));
    nladdr.nl_family = AF_NETLINK;
    struct SicNetLinkRequest req{};
    memset(&req, 0, sizeof(req));
    req.nlh.nlmsg_len = sizeof(req);
    req.nlh.nlmsg_type = TCPDIAG_GETSOCK;
    req.nlh.nlmsg_flags = NLM_F_ROOT | NLM_F_MATCH | NLM_F_REQUEST;
    //sendto kernel
    req.nlh.nlmsg_pid = getpid();
    req.nlh.nlmsg_seq = ++sequence_number;
    req.r.idiag_family = AF_INET;
    req.r.idiag_states = 0xfff;
    req.r.idiag_ext = 0;
    struct iovec iov{};
    memset(&iov, 0, sizeof(iov));
    iov.iov_base = &req;
    iov.iov_len = sizeof(req);
    struct msghdr msg{};
    memset(&msg, 0, sizeof(msg));
    msg.msg_name = (void *) &nladdr;
    msg.msg_namelen = sizeof(nladdr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    if (sendmsg(fd, &msg, 0) < 0) {
        SicPtr()->SetErrorMessage("ReadNetLink, sendmsg(2) failed:" + std::string(strerror(errno)));
        // close(fd);
        return SIC_EXECUTABLE_FAILED;
    }
    char buf[8192];
    iov.iov_base = buf;
    iov.iov_len = sizeof(buf);
    while (true) {
        // struct nlmsghdr *h;
        memset(&msg, 0, sizeof(msg));
        msg.msg_name = (void *) &nladdr;
        msg.msg_namelen = sizeof(nladdr);
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        ssize_t status = recvmsg(fd, (struct msghdr *) &msg, 0);
        if (status < 0) {
            if (errno == EINTR || errno == EAGAIN) {
                continue;
            }
            SicPtr()->SetErrorMessage("ReadNetLink, recvmsg(2) failed:" + std::string(strerror(errno)));
            // close(fd);
            return SIC_EXECUTABLE_FAILED;
        } else if (status == 0) {
            SicPtr()->SetErrorMessage(
                    "ReadNetLink, Unexpected zero-sized  reply from netlink socket." + std::string(strerror(errno)));
            // close(fd);
            return 0;
        }

        // h = (struct nlmsghdr *) buf;
        for (auto h = (struct nlmsghdr *) buf; NLMSG_OK(h, status); h = NLMSG_NEXT(h, status)) {
            if (h->nlmsg_seq != sequence_number) {
                // sequence_number is not equal
                // h = NLMSG_NEXT(h, status);
                continue;
            }

            if (h->nlmsg_type == NLMSG_DONE) {
                // close(fd);
                return 0;
            } else if (h->nlmsg_type == NLMSG_ERROR) {
                if (h->nlmsg_len < NLMSG_LENGTH(sizeof(struct nlmsgerr))) {
                    SicPtr()->SetErrorMessage("ReadNetLink, message truncated");
                } else {
                    auto msg_error = (struct nlmsgerr *) NLMSG_DATA(h);
                    SicPtr()->SetErrorMessage("ReadNetLink, Received error:" + std::to_string(msg_error->error));
                }
                // close(fd);
                return SIC_EXECUTABLE_FAILED;
            }
            auto r = (struct inet_diag_msg *) NLMSG_DATA(h);
            /*This code does not(need to) distinguish between IPv4 and IPv6.*/
            if (r->idiag_state > SIC_TCP_CLOSING || r->idiag_state < SIC_TCP_ESTABLISHED) {
                // Ignoring connection with unknown state
                continue;
            }
            tcpStateCount[r->idiag_state]++;
            // h = NLMSG_NEXT(h, status);
        }
    }
    return 0;
}

int LinuxSystemInformationCollector::SicGetNetStateByNetLink(SicNetState &netState,
                                                             bool useClient,
                                                             bool useServer,
                                                             bool useUdp,
                                                             bool useTcp,
                                                             bool useUnix) {
    std::vector<uint64_t> tcpStateCount(SIC_TCP_CLOSING + 1, 0);
    if (ReadNetLink(tcpStateCount) != 0) {
        return SIC_EXECUTABLE_FAILED;
    }
    int udpSession = 0, udp = 0, tcp = 0, tcpSocketStat = 0;

    if (ReadSocketStat(PROCESS_DIR / PROCESS_NET_SOCKSTAT, udp, tcp) == 0) {
        udpSession += udp;
        tcpSocketStat += tcp;
    }
    if (ReadSocketStat(PROCESS_DIR / PROCESS_NET_SOCKSTAT6, udp, tcp) == 0) {
        udpSession += udp;
        tcpSocketStat += tcp;
    }
    int total = 0;
    for (int i = SIC_TCP_ESTABLISHED; i <= SIC_TCP_CLOSING; i++) {
        if (i == SIC_TCP_SYN_SENT || i == SIC_TCP_SYN_RECV) {
            total += tcpStateCount[i];
        }
        netState.tcpStates[i] = tcpStateCount[i];
    }
    //设置为-1表示没有采集
    netState.tcpStates[SIC_TCP_TOTAL] = total + tcpSocketStat;
    netState.tcpStates[SIC_TCP_NON_ESTABLISHED] =
            netState.tcpStates[SIC_TCP_TOTAL] - netState.tcpStates[SIC_TCP_ESTABLISHED];
    netState.udpSession = udpSession;
    return 0;
}

int LinuxSystemInformationCollector::ReadSocketStat(const fs::path &path, int &udp, int &tcp) {
    tcp = 0;
    udp = 0;
    int ret = SIC_EXECUTABLE_FAILED;
    if (!path.empty()) {
        std::vector<std::string> sockstatLines = {};
        ret = GetFileLines(path, sockstatLines, true, SicPtr()->errorMessage);
        if (ret == SIC_EXECUTABLE_SUCCESS && !sockstatLines.empty()) {
            for (auto const &line: sockstatLines) {
                auto metrics = split(line, ' ', false);
                std::string key = metrics.front();
                key = TrimSpace(key);
                if (metrics.size() >= 3 && (key == "UDP:" || key == "UDP6:")) {
                    udp += convert<int>(metrics[2]);
                }
                if (metrics.size() >= 9 && (key == "TCP:" || key == "TCP6:")) {
                    tcp += convert<int>(metrics[6]); // tw
                    tcp += convert<int>(metrics[8]); // alloc
                }
            }
        }
    }
    return ret;
}

int LinuxSystemInformationCollector::SicGetNetStateBySS(SicNetState &netState,
                                                        bool useClient,
                                                        bool useServer,
                                                        bool useUdp,
                                                        bool useTcp,
                                                        bool useUnix) {
    fs::path ss = Which("ss");
    int ret = ss.empty() ? SIC_EXECUTABLE_FAILED : SIC_EXECUTABLE_SUCCESS;
    if (ret == SIC_EXECUTABLE_SUCCESS) {
        std::string cmd = ss.string() + " -s";
        std::string res;
        ret = ExecCmd(cmd, &res);
        if (ret == SIC_EXECUTABLE_SUCCESS) {
#ifdef ENABLE_COVERAGE
            std::cout << "ss -s:" << std::endl << res << std::endl;
#endif
            ret = ParseSSResult(res, netState);
        }
    }
    return ret;
}

int LinuxSystemInformationCollector::ParseSSResult(std::string const &result, SicNetState &netState) {
    int tcpTotal = -1;
    int tcpEstablished = -1;
    int udp = -1;
    auto ssLines = split(result, '\n', false);
    for (auto const &ssLine: ssLines) {
        if (ssLine.size() >= 4 && ssLine.substr(0, 4) == "TCP:") {
            std::string s1, s2;
            std::stringstream(ssLine) >> s1 >> tcpTotal >> s2 >> tcpEstablished;
        } else if (ssLine.size() >= 3 && ssLine.substr(0, 3) == "UDP") {
            std::string s1;
            std::stringstream(ssLine) >> s1 >> udp;
        }
    }
    if (tcpTotal != -1 && tcpEstablished != -1 && udp != -1) {
        netState.udpSession = udp;
        netState.tcpStates[SIC_TCP_ESTABLISHED] = tcpEstablished;
        netState.tcpStates[SIC_TCP_TOTAL] = tcpTotal;
        netState.tcpStates[SIC_TCP_NON_ESTABLISHED] = tcpTotal - tcpEstablished;
    }
    return SIC_EXECUTABLE_SUCCESS;
}

int LinuxSystemInformationCollector::SicGetSystemInfo(SicSystemInfo &systemInfo) {
    struct utsname name{};
    uname(&name);

    systemInfo.version = name.release;
    systemInfo.vendorName = name.sysname;
    systemInfo.name = name.sysname;
    systemInfo.machine = name.machine;
    systemInfo.arch = name.machine;

    std::string release_file, vendor;
    for (auto const &it: linuxVendorVersion) {
        struct stat fileStat{};
        if (stat(it.first.c_str(), &fileStat) >= 0) {
            release_file = it.first;
            vendor = it.second;
            break;
        }
    }

    std::vector<std::string> releaseLines = {};
    int ret = GetFileLines(release_file, releaseLines, true, &SicPtr()->errorMessage);
    if (ret != SIC_EXECUTABLE_SUCCESS || releaseLines.empty()) {
        return ret;
    }

    std::string release;
    for (auto const &line: releaseLines) {
        release += line;
    }

    systemInfo.vendor = vendor;
    SicGetVendorVersion(release, systemInfo.vendorVersion);
    systemInfo.description = systemInfo.vendor + " " + systemInfo.vendorVersion;

    return SIC_EXECUTABLE_SUCCESS;
}

int LinuxSystemInformationCollector::SicGetVendorVersion(const std::string &release, std::string &version) {
    std::string pattern = R"((\d|\.)+\+?)";
    std::regex vendorVersionPattern(pattern);
    std::smatch result;
    if (std::regex_search(release, result, vendorVersionPattern)) {
        version = result[0];
        return SIC_EXECUTABLE_SUCCESS;
    } else {
        SicPtr()->SetErrorMessage("invalid release file! ");
        return SIC_EXECUTABLE_FAILED;
    }
}

std::shared_ptr<SystemInformationCollector> SystemInformationCollector::New() {
    auto collector = std::make_shared<LinuxSystemInformationCollector>();
    return collector->SicInitialize() == SIC_EXECUTABLE_SUCCESS ? collector : nullptr;
}
