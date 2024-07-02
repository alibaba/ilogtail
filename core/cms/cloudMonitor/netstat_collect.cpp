#include "netstat_collect.h"

#include <set>

#include "common/Logger.h"
#include "common/TimeProfile.h"
#include "common/ModuleData.h"
#include "common/Config.h"

using namespace std;
using namespace common;
using namespace cloudMonitor;

NetStatCollect::NetStatCollect() : mModuleName{"netstat"} {
    LogInfo("load netstat");
}

NetStatCollect::~NetStatCollect() {
    LogInfo("unload netstat");
}

int NetStatCollect::Init() {
    mUseTcpAllState = SingletonConfig::Instance()->GetValue("cms.tcp.full", false);
    LogInfo("UseTcpAllState : {}", mUseTcpAllState);
    return 0;
}

static constexpr const bool simpleTcpState[] = {
        false,
        true, // SIC_TCP_ESTABLISHED
        false, false, false, false, false, false, false, false,
        true, // SIC_TCP_LISTEN
        false, false, false, false,
        true, // SIC_TCP_TOTAL
        true, // SIC_TCP_NON_ESTABLISHED
};
static constexpr const size_t simpleTcpStateCount = sizeof(simpleTcpState) / sizeof(simpleTcpState[0]);
static_assert(simpleTcpStateCount == sizeof(NetStat::tcpStates) / sizeof(NetStat::tcpStates[0]), "unmatched size");
static_assert(!simpleTcpState[0], "unexpected simpleTcpState[0]");
static_assert(simpleTcpState[SIC_TCP_ESTABLISHED], "unexpected simpleTcpState[SIC_TCP_ESTABLISHED]");
static_assert(simpleTcpState[SIC_TCP_LISTEN], "unexpected simpleTcpState[SIC_TCP_LISTEN]");
static_assert(simpleTcpState[SIC_TCP_TOTAL], "unexpected simpleTcpState[SIC_TCP_TOTAL]");
static_assert(simpleTcpState[SIC_TCP_NON_ESTABLISHED], "unexpected simpleTcpState[SIC_TCP_NON_ESTABLISHED]");

#ifdef ENABLE_COVERAGE

// 确保整体初始化正确: false和true的设置都是符合预期的
size_t countSimpleTcpState() {
    size_t count = 0;
    for (bool b: simpleTcpState) {
        count += (b ? 1 : 0);
    }
    return count;
}

#endif

int NetStatCollect::Collect(string &data) {
    TimeProfile timeProfile;
    NetStat netStat;
    if (GetNetStat(netStat) != 0) {
        return -1;
    }
    CollectData collectData;
    collectData.moduleName = mModuleName;

    for (int i = 1; i < SIC_TCP_STATE_END; i++) {
        if (mUseTcpAllState || simpleTcpState[i]) {
            {
                MetricData metricDataSystem;
                GetNetStatMetricData(netStat, "system.tcp", "acs/host", i, metricDataSystem);
                collectData.dataVector.push_back(metricDataSystem);
            }
            {
                MetricData metricDataVm;
                GetNetStatMetricData(netStat, "vm.TcpCount", "acs/ecs", i, metricDataVm);
                collectData.dataVector.push_back(metricDataVm);
            }
        }
    }
    // metrichub不再支持system.udp
#ifdef ENABLE_UDP_SESSION
    MetricData metricDataUdp;
    GetNetStatMetricData("system.udp", netStat.updSession, metricDataUdp);
    collectData.dataVector.push_back(metricDataUdp);
#endif
    ModuleData::convertCollectDataToString(collectData, data);
    LogDebug("collect netstat spend {} ms", timeProfile.millis());
    return static_cast<int>(data.size());
}

void NetStatCollect::GetNetStatMetricData(const NetStat &netStat,
                                          const string &metricName,
                                          const string &ns,
                                          int index,
                                          MetricData &metricData) {
    metricData.tagMap["metricName"] = metricName;
    metricData.tagMap["state"] = GetTcpStateName(static_cast<EnumTcpState>(index));
    metricData.tagMap["ns"] = ns;
    metricData.valueMap["metricValue"] = netStat.tcpStates[index];
}

void NetStatCollect::GetNetStatMetricData(const string &metricName, int udpSession, MetricData &metricData) {
    metricData.tagMap["metricName"] = metricName;
    metricData.valueMap["session"] = udpSession;
    metricData.valueMap["metricValue"] = 0;
    metricData.tagMap["ns"] = "acs/host";
}

// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DLL
#include "cloud_module_macros.h"

IMPLEMENT_CMS_MODULE(netstat) {
    return module::NewHandler<NetStatCollect>();
}
