//
// Created by 韩呈杰 on 2023/10/5.
// For macOS(Darwin) / FreeBSD
// darwin/macosx is based on freebsd (or something like that)
//
// #define WITH_SIGAR
#include "../system_information_collector_darwin.h"
#include <iostream>
#include <cstdlib> // exit, getloadavg

extern "C" {
    #include <sigar.h>
    #include <sigar_log.h>
}
#ifdef disk_reads
#   undef disk_reads // 跟macOS中的定义有冲突
#   undef disk_writes
#   undef disk_write_bytes
#   undef disk_read_bytes
#   undef disk_queue
#   undef disk_service_time
#endif // disk_reads

#include <fmt/format.h>

#include "common/Defer.h"

static_assert(SIC_SUCCESS == SIGAR_OK, "SIC_SUCCESS must be equal to SIGAR_OK");

#define COUNT(mib) (sizeof(mib)/sizeof(mib[0]))
#define STATIC_ASSERT_ENUM(l, r) static_assert((int)(l) == (int)(r), "unexpect "#l)

void ExitArgus(int status) {
    exit(status);
}


/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Sic
struct Sic : SicBase {
    sigar_t *sigar = nullptr;

    Sic();
    int initialize();
    ~Sic() override;
};

Sic::Sic() {
    initialize();
}

Sic::~Sic() {
    if (sigar != nullptr) {
        sigar_close(sigar);
        sigar = nullptr;
    }
}

#if defined(ENABLE_COVERAGE)
extern "C" void sigarLogImpl(sigar_t *, void *, int /*level*/, char *msg) {
    std::cout << "[sigar]: " << msg << std::endl;
}
#endif

int Sic::initialize() {
    int ret = SIGAR_OK;
    if (sigar == nullptr) {
        ret = sigar_open(&sigar);
#if defined(ENABLE_COVERAGE)
        if (ret == SIGAR_OK) {
            sigar_log_impl_set(sigar, nullptr, &sigarLogImpl);
            sigar_log_level_set(sigar, SIGAR_LOG_TRACE);
        }
#endif // ENABLE_COVERAGE
    }
    return ret;
}

#define RETURN(status, prompt) if((status) != SIGAR_OK) { \
    SicPtr()->errorMessage = fmt::format("{} error: {}, {}", (prompt), (status), sigar_strerror(SicPtr()->sigar, (status))); \
    }                                             \
    return (status)

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DarwinSystemInformationCollector
DarwinSystemInformationCollector::DarwinSystemInformationCollector()
        : SystemInformationCollector(std::make_shared<Sic>()) {

}

std::shared_ptr<Sic> DarwinSystemInformationCollector::SicPtr() {
    return std::dynamic_pointer_cast<Sic>(mSicPtr);
}

std::shared_ptr<const Sic> DarwinSystemInformationCollector::SicPtr() const {
    return std::const_pointer_cast<const Sic>(std::dynamic_pointer_cast<Sic>(mSicPtr));
}

void convert(const sigar_thread_cpu_t &src, SicThreadCpu &info) {
    // macOS 线程cpu是纳秒级，进程却是毫和秒级
    using namespace std::chrono;
    info.user = duration_cast<microseconds>(nanoseconds(src.user));
    info.sys = duration_cast<microseconds>(nanoseconds(src.sys));
    info.total = duration_cast<microseconds>(nanoseconds(src.total));
}

int DarwinSystemInformationCollector::SicGetThreadCpuInformation(pid_t pid, int tid, SicThreadCpu &info) {
    sigar_thread_cpu_t sigarThreadCpu;
    int status = sigar_thread_cpu_get(SicPtr()->sigar, tid, &sigarThreadCpu);
    if (status == SIGAR_OK) {
        convert(sigarThreadCpu, info);
    }
    RETURN(status, __FUNCTION__);
}

static void convert(const sigar_cpu_t &r, SicCpuInformation &info) {
    // sigar cpu的单位是毫秒
    using namespace std::chrono;
    info.user = milliseconds(r.user);
    info.sys = milliseconds(r.sys);
    info.nice = milliseconds(r.nice);
    info.idle = milliseconds(r.idle);
    info.wait = milliseconds(r.wait);
    info.irq = milliseconds(r.irq);
    info.softIrq = milliseconds(r.soft_irq);
    info.stolen = milliseconds(r.stolen);
}

int DarwinSystemInformationCollector::SicGetCpuInformation(SicCpuInformation &info) {
    sigar_cpu_t cpu;
    int status = sigar_cpu_get(SicPtr()->sigar, &cpu);
    if (status == SIGAR_OK) {
        convert(cpu, info);
    }
    RETURN(status, __FUNCTION__);
}

int DarwinSystemInformationCollector::SicGetCpuListInformation(std::vector<SicCpuInformation> &cpuCores) {
    sigar_cpu_list_t cpuList;
    memset(&cpuList, 0, sizeof(cpuList));
    defer(sigar_cpu_list_destroy(SicPtr()->sigar, &cpuList));

    int status = sigar_cpu_list_get(SicPtr()->sigar, &cpuList);
    if (status == SIGAR_OK && cpuList.number > 0) {
        cpuCores.resize(cpuList.number);
        for (decltype(cpuList.number) i = 0; i < cpuList.number; i++) {
            convert(cpuList.data[i], cpuCores[i]);
        }
    }
    RETURN(status, __FUNCTION__);
}

void convert(const sigar_mem_t &r, SicMemoryInformation &dst) {
    dst.ram = r.ram;
    dst.total = r.total;
    dst.used = r.used;
    dst.free = r.free;
    dst.actualUsed = r.actual_used;
    dst.usedPercent = r.used_percent;
    dst.actualFree = r.actual_free;
    dst.freePercent = r.free_percent;
}

int DarwinSystemInformationCollector::SicGetMemoryInformation(SicMemoryInformation &mem) {
    sigar_mem_t sigarMem{};
    int status = sigar_mem_get(SicPtr()->sigar, &sigarMem);
    if (status == SIGAR_OK) {
        convert(sigarMem, mem);
    }
    RETURN(status, __FUNCTION__);
}

int DarwinSystemInformationCollector::SicGetUpTime(double &upTime) {
    sigar_uptime_t sigarUptime{};
    int status = sigar_uptime_get(SicPtr()->sigar, &sigarUptime);
    if (status == SIGAR_OK) {
        upTime = sigarUptime.uptime;
    }
    RETURN(status, __FUNCTION__);
}

int DarwinSystemInformationCollector::SicGetLoadInformation(SicLoadInformation &info) {
    // #if define(__APPLE__) || defined(__FreeBSD__)
    // https://developer.apple.com/library/archive/documentation/System/Conceptual/ManPages_iPhoneOS/man3/getloadavg.3.html
    // double loadavg[3] = {0};
    // int status = getloadavg(loadavg, COUNT(loadavg));
    // if (status > 0) {
    //     info.load1 = loadavg[0];
    //     info.load5 = loadavg[1];
    //     info.load15 = loadavg[2];
    //     status = SIGAR_OK;
    // }
    // #endif
    sigar_loadavg_t load{0};
    int status = sigar_loadavg_get(SicPtr()->sigar, &load);
    if (status == SIGAR_OK) {
        info.load1 =  load.loadavg[0];
        info.load5 =  load.loadavg[1];
        info.load15 = load.loadavg[2];
    }
    RETURN(status, __FUNCTION__);
}

void convert(const sigar_swap_t &swap, SicSwapInformation &info) {
    info.total = swap.total;
    info.used = swap.used;
    info.free = swap.free;
    info.pageIn = swap.page_in;
    info.pageOut = swap.page_out;
}

int DarwinSystemInformationCollector::SicGetSwapInformation(SicSwapInformation &info) {
    sigar_swap_t swap;
    int status = sigar_swap_get(SicPtr()->sigar, &swap);
    if (status == SIGAR_OK) {
        convert(swap, info);
    }
    RETURN(status, __FUNCTION__);
}

int DarwinSystemInformationCollector::SicGetInterfaceConfigList(SicInterfaceConfigList &ifNames) {
    sigar_t *sigar = SicPtr()->sigar;

    sigar_net_interface_list_t sigarList;
    memset(&sigarList, 0, sizeof(sigarList));
    defer(sigar_net_interface_list_destroy(sigar, &sigarList));

    int ret = sigar_net_interface_list_get(sigar, &sigarList);
    if (ret == SIGAR_OK) {
        ifNames.names.clear();
        for (unsigned long i = 0; i < sigarList.number; i++) {
            ifNames.names.insert(sigarList.data[i]);
        }
    }

    RETURN(ret, __FUNCTION__);
}

STATIC_ASSERT_ENUM(SicNetAddress::SIC_AF_UNSPEC, sigar_net_address_t::SIGAR_AF_UNSPEC);
STATIC_ASSERT_ENUM(SicNetAddress::SIC_AF_INET, sigar_net_address_t::SIGAR_AF_INET);
STATIC_ASSERT_ENUM(SicNetAddress::SIC_AF_INET6, sigar_net_address_t::SIGAR_AF_INET6);
STATIC_ASSERT_ENUM(SicNetAddress::SIC_AF_LINK, sigar_net_address_t::SIGAR_AF_LINK);

void convert(const sigar_net_address_t &from, SicNetAddress &to) {
    to.family = (decltype(to.family))(from.family);
    static_assert(sizeof(to.addr) == sizeof(from.addr), "unexpected sizeof(SicNetAddress.addr)");
    memcpy(&to.addr, &from.addr, sizeof(from.addr));
}

void convert(const sigar_net_interface_config_t &from, SicInterfaceConfig &to) {
    to.name = from.name;
    to.type = from.type;
    to.description = from.description;
    convert(from.hwaddr, to.hardWareAddr);

    convert(from.address, to.address);
    convert(from.destination, to.destination);
    convert(from.broadcast, to.broadcast);
    convert(from.netmask, to.netmask);

    convert(from.address6, to.address6);
    to.prefix6Length = from.prefix6_length;
    to.scope6 = from.scope6;

    // to.flags = from.flags;
    to.mtu = from.mtu;
    to.metric = from.metric;
    to.txQueueLen = from.tx_queue_len;
}

int DarwinSystemInformationCollector::SicGetInterfaceConfig(SicInterfaceConfig &ifConfig, const std::string &name) {
    sigar_net_interface_config_t sigarNetInterfaceConfig;
    int status = sigar_net_interface_config_get(SicPtr()->sigar, name.c_str(), &sigarNetInterfaceConfig);
    if (status == SIGAR_OK) {
        convert(sigarNetInterfaceConfig, ifConfig);
    }
    RETURN(status, fmt::format("{}({})", __FUNCTION__, name));
}

void convert(const sigar_net_interface_stat_t &netIf, SicNetInterfaceInformation &info) {
    info.rxPackets = netIf.rx_packets;
    info.rxBytes = netIf.rx_bytes;
    info.rxErrors = netIf.rx_errors;
    info.rxDropped = netIf.rx_dropped;
    info.rxOverruns = netIf.rx_overruns;
    info.rxFrame = netIf.rx_frame;

    info.txPackets = netIf.tx_packets;
    info.txBytes = netIf.tx_bytes;
    info.txErrors = netIf.tx_errors;
    info.txDropped = netIf.tx_dropped;
    info.txOverruns = netIf.tx_overruns;
    info.txCollisions = netIf.tx_collisions;
    info.txCarrier = netIf.tx_carrier;

    info.speed = netIf.speed;
}

int DarwinSystemInformationCollector::SicGetInterfaceInformation(const std::string &name,
                                                                 SicNetInterfaceInformation &info) {
    sigar_net_interface_stat_t netIf;
    int status = sigar_net_interface_stat_get(SicPtr()->sigar, name.c_str(), &netIf);
    if (status == SIGAR_OK) {
        convert(netIf, info);
    }
    RETURN(status, fmt::format("{}({})", __FUNCTION__, name));
}


STATIC_ASSERT_ENUM(EnumTcpState::SIC_TCP_ESTABLISHED, SIGAR_TCP_ESTABLISHED);
STATIC_ASSERT_ENUM(EnumTcpState::SIC_TCP_SYN_SENT, SIGAR_TCP_SYN_SENT);
STATIC_ASSERT_ENUM(EnumTcpState::SIC_TCP_SYN_RECV, SIGAR_TCP_SYN_RECV);
STATIC_ASSERT_ENUM(EnumTcpState::SIC_TCP_FIN_WAIT1, SIGAR_TCP_FIN_WAIT1);
STATIC_ASSERT_ENUM(EnumTcpState::SIC_TCP_FIN_WAIT2, SIGAR_TCP_FIN_WAIT2);
STATIC_ASSERT_ENUM(EnumTcpState::SIC_TCP_TIME_WAIT, SIGAR_TCP_TIME_WAIT);
STATIC_ASSERT_ENUM(EnumTcpState::SIC_TCP_CLOSE, SIGAR_TCP_CLOSE);
STATIC_ASSERT_ENUM(EnumTcpState::SIC_TCP_CLOSE_WAIT, SIGAR_TCP_CLOSE_WAIT);
STATIC_ASSERT_ENUM(EnumTcpState::SIC_TCP_LAST_ACK, SIGAR_TCP_LAST_ACK);
STATIC_ASSERT_ENUM(EnumTcpState::SIC_TCP_LISTEN, SIGAR_TCP_LISTEN);
STATIC_ASSERT_ENUM(EnumTcpState::SIC_TCP_CLOSING, SIGAR_TCP_CLOSING);
STATIC_ASSERT_ENUM(EnumTcpState::SIC_TCP_IDLE, SIGAR_TCP_IDLE);
STATIC_ASSERT_ENUM(EnumTcpState::SIC_TCP_BOUND, SIGAR_TCP_BOUND);
STATIC_ASSERT_ENUM(EnumTcpState::SIC_TCP_UNKNOWN, SIGAR_TCP_UNKNOWN);

void convert(const sigar_net_stat_t &src, SicNetState &dst) {
    memcpy(dst.tcpStates, src.tcp_states, std::min(sizeof(src.tcp_states), sizeof(dst.tcpStates)));
    dst.tcpInboundTotal = src.tcp_inbound_total;
    dst.tcpOutboundTotal = src.tcp_outbound_total;
    dst.allInboundTotal = src.all_inbound_total;
    dst.allOutboundTotal = src.all_outbound_total;
    dst.udpSession = src.udp_session;

    dst.calcTcpTotalAndNonEstablished();
}

int DarwinSystemInformationCollector::SicGetNetState(SicNetState &netState,
                                                     bool useClient,
                                                     bool useServer,
                                                     bool useUdp,
                                                     bool useTcp,
                                                     bool useUnix,
                                                     int) {
    int flags = (useClient ? SIGAR_NETCONN_CLIENT : 0) | (useServer ? SIGAR_NETCONN_SERVER : 0)
                | (useTcp ? SIGAR_NETCONN_TCP : 0) | (useUdp ? SIGAR_NETCONN_UDP : 0)
                | (useUnix ? SIGAR_NETCONN_UNIX : 0);

    sigar_net_stat_t sigarNetStat;
    int status = sigar_net_stat_get(SicPtr()->sigar, &sigarNetStat, flags);
    if (status == SIGAR_OK) {
        convert(sigarNetStat, netState);
    }
    RETURN(status, fmt::format("{}(client:{}, server:{}, udp: {}, tcp: {}, unix: {})",
                               __FUNCTION__, useClient, useServer, useUdp, useTcp, useUnix));
}

STATIC_ASSERT_ENUM(SicFileSystemType::SIC_FILE_SYSTEM_TYPE_UNKNOWN, SIGAR_FSTYPE_UNKNOWN);
STATIC_ASSERT_ENUM(SicFileSystemType::SIC_FILE_SYSTEM_TYPE_NONE, SIGAR_FSTYPE_NONE);
STATIC_ASSERT_ENUM(SicFileSystemType::SIC_FILE_SYSTEM_TYPE_LOCAL_DISK, SIGAR_FSTYPE_LOCAL_DISK);
STATIC_ASSERT_ENUM(SicFileSystemType::SIC_FILE_SYSTEM_TYPE_NETWORK, SIGAR_FSTYPE_NETWORK);
STATIC_ASSERT_ENUM(SicFileSystemType::SIC_FILE_SYSTEM_TYPE_RAM_DISK, SIGAR_FSTYPE_RAM_DISK);
STATIC_ASSERT_ENUM(SicFileSystemType::SIC_FILE_SYSTEM_TYPE_CDROM, SIGAR_FSTYPE_CDROM);
STATIC_ASSERT_ENUM(SicFileSystemType::SIC_FILE_SYSTEM_TYPE_SWAP, SIGAR_FSTYPE_SWAP);
STATIC_ASSERT_ENUM(SicFileSystemType::SIC_FILE_SYSTEM_TYPE_MAX, SIGAR_FSTYPE_MAX);

void convert(const sigar_file_system_t &fs, SicFileSystem &dst) {
    dst.dirName = fs.dir_name;
    dst.devName = fs.dev_name;
    dst.typeName = fs.type_name;
    dst.sysTypeName = fs.sys_type_name;
    dst.options = fs.options;
    dst.type = (SicFileSystemType) fs.type;
    dst.flags = fs.flags;
}

int DarwinSystemInformationCollector::SicGetFileSystemListInformation(std::vector<SicFileSystem> &infos) {
    sigar_file_system_list_t fsList;
    memset(&fsList, 0, sizeof(fsList));
    defer(sigar_file_system_list_destroy(SicPtr()->sigar, &fsList));

    int status = sigar_file_system_list_get(SicPtr()->sigar, &fsList);
    if (status == SIGAR_OK && fsList.number > 0) {
        infos.resize(fsList.number);
        for (decltype(fsList.number) i = 0; i < fsList.number; ++i) {
            convert(fsList.data[i], infos[i]);
        }
    }
    RETURN(status, __FUNCTION__);
}

void convert(const sigar_disk_usage_t &usage, SicDiskUsage &sic) {
    sic.reads = usage.reads;
    sic.writes = usage.writes;
    sic.writeBytes = usage.write_bytes;
    sic.readBytes = usage.read_bytes;
    sic.rTime = usage.rtime;
    sic.wTime = usage.wtime;
    sic.qTime = usage.qtime;
    sic.time = usage.time;
    sic.snapTime = static_cast<double>(usage.snaptime);
    sic.serviceTime = usage.service_time;
    sic.queue = usage.queue;
}

int DarwinSystemInformationCollector::SicGetDiskUsage(SicDiskUsage &diskUsage, std::string dirName) {
    sigar_disk_usage_t usage;
    int status = sigar_disk_usage_get(SicPtr()->sigar, dirName.c_str(), &usage);
    if (status == SIGAR_OK) {
        convert(usage, diskUsage);
        diskUsage.dirName = dirName;
    }
    RETURN(status, fmt::format("({})", __FUNCTION__, dirName));
}

void convert(const sigar_file_system_usage_t &src, SicFileSystemUsage &fsUsage) {
    convert(src.disk, fsUsage.disk);
    fsUsage.use_percent = src.use_percent;
    fsUsage.total = src.total;
    fsUsage.free = src.free;
    fsUsage.used = src.used;
    fsUsage.avail = src.avail;
    fsUsage.files = src.files;
    fsUsage.freeFiles = src.free_files;
}

int DarwinSystemInformationCollector::SicGetFileSystemUsage(SicFileSystemUsage &fsUsage, std::string dirName) {
    sigar_file_system_usage_t sigarUsage;
    int status = sigar_file_system_usage_get(SicPtr()->sigar, dirName.c_str(), &sigarUsage);
    if (status == SIGAR_OK) {
        convert(sigarUsage, fsUsage);
    }
    RETURN(status, fmt::format("{}({})", __FUNCTION__, dirName));
}

int DarwinSystemInformationCollector::SicGetProcessList(SicProcessList &processList, size_t limit, bool &overflow) {
    overflow = false;
    sigar_proc_list_t sigarProcList;
    memset(&sigarProcList, 0, sizeof(sigarProcList));
    defer(sigar_proc_list_destroy(SicPtr()->sigar, &sigarProcList));

    int status = sigar_proc_list_get(SicPtr()->sigar, &sigarProcList);
    if (status == SIGAR_OK && sigarProcList.number > 0) {
        processList.pids.resize(sigarProcList.number);
        for (decltype(sigarProcList.number) i = 0; i < sigarProcList.number; ++i) {
            processList.pids[i] = sigarProcList.data[i];
        }
        overflow = limit != 0 && processList.pids.size() > limit;
    }
    RETURN(status, __FUNCTION__);
}

void convert(const sigar_proc_state_t &src, SicProcessState &dst) {
    dst.name = src.name;
    dst.state = src.state;
    dst.priority = src.priority;
    dst.nice = src.nice;
    dst.parentPid = src.ppid;
    if (src.tty != SIGAR_FIELD_NOTIMPL) {
        dst.tty = src.tty;
    }
    if (src.processor != SIGAR_FIELD_NOTIMPL) {
        dst.processor = src.processor;
    }
    if (src.threads != SIGAR_FIELD_NOTIMPL) {
        dst.threads = src.threads;
    }
}

int DarwinSystemInformationCollector::SicGetProcessState(pid_t pid, SicProcessState &processState) {
    sigar_proc_state_t sigarProcState;
    int status = sigar_proc_state_get(SicPtr()->sigar, pid, &sigarProcState);
    if (status == SIGAR_OK) {
        convert(sigarProcState, processState);
    }
    RETURN(status, fmt::format("{}({})", __FUNCTION__, pid));
}

void convert(const sigar_proc_time_t &from, SicProcessTime &to) {
    using namespace std::chrono;
    to.startTime = system_clock::time_point{milliseconds{from.start_time}};
    to.user = milliseconds(from.user);
    to.sys = milliseconds(from.sys);
    to.total = milliseconds(from.total);
}

int DarwinSystemInformationCollector::SicGetProcessTime(pid_t pid, SicProcessTime &processTime, bool) {
    sigar_proc_time_t sigarProcTime;
    int status = sigar_proc_time_get(SicPtr()->sigar, pid, &sigarProcTime);
    if (status == SIGAR_OK) {
        convert(sigarProcTime, processTime);
    }
    RETURN(status, fmt::format("{}({})", __FUNCTION__, pid));
}

static_assert(sizeof(sigar_proc_mem_t) == sizeof(SicProcessMemoryInformation),
              "sizeof(SicProcessMemoryInformation) must equal to sigar_proc_mem_t");

void convert(const sigar_proc_mem_t &from, SicProcessMemoryInformation &to) {
    to.size = from.size;
    to.resident = from.resident;
    to.share = from.share;
    to.minorFaults = from.minor_faults;
    to.majorFaults = from.major_faults;
    to.pageFaults = from.page_faults;
}

int DarwinSystemInformationCollector::SicGetProcessMemoryInformation(pid_t pid, SicProcessMemoryInformation &info) {
    sigar_proc_mem_t sigarProcMem;
    int status = sigar_proc_mem_get(SicPtr()->sigar, pid, &sigarProcMem);
    if (status == SIGAR_OK) {
        convert(sigarProcMem, info);
    }
    RETURN(status, fmt::format("{}({})", __FUNCTION__, pid));
}

int DarwinSystemInformationCollector::SicGetProcessArgs(pid_t pid, SicProcessArgs &processArgs) {
    sigar_proc_args_t sigarProcArgs;
    memset(&sigarProcArgs, 0, sizeof(sigarProcArgs));
    defer(sigar_proc_args_destroy(SicPtr()->sigar, &sigarProcArgs));

    int status = sigar_proc_args_get(SicPtr()->sigar, pid, &sigarProcArgs);
    if (status == SIGAR_OK && sigarProcArgs.number > 0) {
        processArgs.args.resize(sigarProcArgs.number);
        for (decltype(sigarProcArgs.number) i = 0; i < sigarProcArgs.number; ++i) {
            processArgs.args[i] = sigarProcArgs.data[i];
        }
    }
    RETURN(status, fmt::format("{}({})", __FUNCTION__, pid));
}

void convert(const sigar_proc_exe_t &from, SicProcessExe &to) {
    to.name = from.name;
    to.cwd = from.cwd;
    to.root = from.root;
}

int DarwinSystemInformationCollector::SicGetProcessExe(pid_t pid, SicProcessExe &processExe) {
    sigar_proc_exe_t sigarProcExe;
    int status = sigar_proc_exe_get(SicPtr()->sigar, pid, &sigarProcExe);
    if (status == SIGAR_OK) {
        convert(sigarProcExe, processExe);
    }
    RETURN(status, fmt::format("{}({})", __FUNCTION__, pid));
}

void convert(const sigar_proc_cred_name_t &from, SicProcessCredName &to) {
    to.user = from.user;
    to.group = from.group;
}

int DarwinSystemInformationCollector::SicGetProcessCredName(pid_t pid, SicProcessCredName &processCredName) {
    sigar_proc_cred_name_t sigarProcCredName;
    int status = sigar_proc_cred_name_get(SicPtr()->sigar, pid, &sigarProcCredName);
    if (status == SIGAR_OK) {
        convert(sigarProcCredName, processCredName);
    }
    RETURN(status, fmt::format("{}({})", __FUNCTION__, pid));
}

void convert(const sigar_proc_fd_t &from, SicProcessFd &to) {
    to.total = from.total;
    to.exact = true;
}

int DarwinSystemInformationCollector::SicGetProcessFd(pid_t pid, SicProcessFd &processFd) {
    // unix like系统下只有FreeBSD实现了该接口
    sigar_proc_fd_t sigarProcFd;
    int status = sigar_proc_fd_get(SicPtr()->sigar, pid, &sigarProcFd);
    if (status == SIGAR_OK) {
        convert(sigarProcFd, processFd);
    } else if (status == SIGAR_ENOTIMPL) {
        status = SIGAR_OK;
        processFd.total = 0;
        processFd.exact = true;
    }
    RETURN(status, fmt::format("{}({})", __FUNCTION__, pid));
}

void convert(const sigar_sys_info_t &src, SicSystemInfo &dst) {
    dst.name = src.name;
    dst.version = src.version;
    dst.arch = src.arch;
    dst.machine = src.machine;
    dst.vendor = src.vendor;
    dst.vendorName = src.vendor_name;
    dst.vendorVersion = src.vendor_version;
}

int DarwinSystemInformationCollector::SicGetSystemInfo(SicSystemInfo &systemInfo) {
    sigar_sys_info_t sigarSysInfo;
    int status = sigar_sys_info_get(SicPtr()->sigar, &sigarSysInfo);
    if (status == SIGAR_OK) {
        convert(sigarSysInfo, systemInfo);
    }
    RETURN(status, __FUNCTION__);
}

int DarwinSystemInformationCollector::SicInitialize() {
    return SicPtr()->initialize();
}

std::map<std::string, std::string> DarwinSystemInformationCollector::queryDevSerialId(const std::string &) {
    return std::map<std::string, std::string>{};
}

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
std::shared_ptr<SystemInformationCollector> SystemInformationCollector::New() {
    auto ptr = std::make_shared<DarwinSystemInformationCollector>();
    return ptr->SicInitialize() == SIGAR_OK ? ptr : nullptr;
}
