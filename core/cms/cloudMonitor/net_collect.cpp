#include "net_collect.h"

#include "common/Logger.h"
#include "common/ModuleData.h"
#include "common/TimeProfile.h"
#include "common/Defer.h"
#include "common/Chrono.h"
#include "common/StringUtils.h"

#include "cloud_monitor_const.h"
#include "cloud_monitor_common.h"

using namespace std;
using namespace common;
using namespace std::chrono;
using namespace cloudMonitor;

namespace cloudMonitor {
/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NetMetric
#define NET_METRIC_META_ENTRY(Name, Value, BOOL) METRIC_FIELD_NAME_ENTRY(Name, NetMetric, Value, BOOL)
    // extern - 允许本文件外使用该常量(迫使编译器分配空间)
    extern const MetricFieldName<NetMetric, double> netMetricMeta[] = {
            NET_METRIC_META_ENTRY("rx_packets", rxPackets, true),
            NET_METRIC_META_ENTRY("rx_bytes", rxBytes, true),
            NET_METRIC_META_ENTRY("rx_errors", rxErrors, true),
            NET_METRIC_META_ENTRY("rx_dropped", rxDropped, true),
            NET_METRIC_META_ENTRY("rx_overruns", rxOverruns, true),
            NET_METRIC_META_ENTRY("rx_frame", rxFrame, true),
            NET_METRIC_META_ENTRY("tx_packets", txPackets, true),
            NET_METRIC_META_ENTRY("tx_bytes", txBytes, true),
            NET_METRIC_META_ENTRY("tx_errors", txErrors, true),
            NET_METRIC_META_ENTRY("tx_dropped", txDropped, true),
            NET_METRIC_META_ENTRY("tx_overruns", txOverruns, true),
            NET_METRIC_META_ENTRY("tx_collisions", txCollisions, false),
            NET_METRIC_META_ENTRY("tx_carrier", txCarrier, true),
            NET_METRIC_META_ENTRY("rx_error_ratio", rxErrorRatio, false),
            NET_METRIC_META_ENTRY("rx_drop_ratio", rxDropRatio, false),
            NET_METRIC_META_ENTRY("tx_error_ratio", txErrorRatio, false),
            NET_METRIC_META_ENTRY("tx_drop_ratio", txDropRatio, false),
    };
    const size_t netMetricMetaSize = sizeof(netMetricMeta) / sizeof(netMetricMeta[0]);
    const MetricFieldName<NetMetric, double> *netMetricMetaEnd = netMetricMeta + netMetricMetaSize;
#undef NET_METRIC_META_ENTRY
    static_assert(netMetricMetaSize == sizeof(NetMetric) / sizeof(double), "netMetricMeta size unexpected");

    void enumerate(const std::function<void(const FieldName<NetMetric, double> &)> &callback) {
        for (auto it = netMetricMeta; it < netMetricMetaEnd; ++it) {
            callback(*it);
        }
    }
}

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NetCollect
NetCollect::NetCollect() : mModuleName("net") {
    LogInfo("load {}", mModuleName);
}

NetCollect::~NetCollect() {
    mMetricCalculateMap.clear();
    LogInfo("unload {}", mModuleName);
}

int NetCollect::Init(int totalCount) {
    mTotalCount = totalCount;
    mCount = 0;
    mInterfaceConfigExpireTime = steady_clock::time_point{};
    mLastTime = steady_clock::time_point{};
    return 0;
}
int NetCollect::Collect(string &data)
{
    TimeProfile tp;
    std::chrono::steady_clock::time_point start = tp.lastTime();
    if (start < mLastTime + 1_ms) {
        //不应该发生,调度间隔不应该小于1ms
        return -1;
    }

    defer(mLastTime = start);
    map<string, InterfaceConfig> interfaceConfigMap;
    ReadInterfaceConfigs(interfaceConfigMap);

    map<string, SicNetInterfaceInformation> currentInterfaceStatMap;
    for (auto &entry: interfaceConfigMap) {
        SicNetInterfaceInformation interfaceStat;
        if (GetInterfaceStat(entry.first, interfaceStat) == 0) {
            currentInterfaceStatMap[entry.first] = interfaceStat;
        }
    }
    if (IsZero(mLastTime)) {
        LogInfo("collect net with the first time");
        mLastInterfaceStatMap = currentInterfaceStatMap;
        return 0;
    }
    mCount++;
    double interval = std::chrono::duration_cast<std::chrono::fraction_seconds>(start - mLastTime).count();
    // mLastTime = start;
    //计算每个采集点
    LogDebug("calculate net interface metrics, size={}", currentInterfaceStatMap.size());
    for (auto &it: currentInterfaceStatMap) {
        string name = it.first;
        //skip empty interface
        if (it.second.rxPackets == 0) {
            LogDebug("skip empty interface: {}", name);
            continue;
        }
        if (mLastInterfaceStatMap.find(name) != mLastInterfaceStatMap.end()) {
            SicNetInterfaceInformation lastStat = mLastInterfaceStatMap[name];
            SicNetInterfaceInformation currentStat = it.second;
            NetMetric netMetric;
            GetNetMetric(currentStat, lastStat, netMetric, interval);
            mMetricCalculateMap[name].AddValue(netMetric);
        }
    }
    mLastInterfaceStatMap = currentInterfaceStatMap;

    LogDebug("collect net spend {:.3}", tp.cost<fraction_millis>().count());
    if (mCount < mTotalCount) {
        return 0;
    }

    mCount = 0;
    //产生数据
    CollectData collectData;
    collectData.moduleName = mModuleName;

    for (auto itMc = mMetricCalculateMap.begin(); itMc != mMetricCalculateMap.end();) {
        string deviceName, deviceIp;
        deviceName = itMc->first;
        if (currentInterfaceStatMap.find(deviceName) == currentInterfaceStatMap.end()
            || !GetDeviceIp(interfaceConfigMap, deviceName, deviceIp)) {
            LogDebug("skip not device ip interface: {}", deviceName.c_str());
            itMc = mMetricCalculateMap.erase(itMc);
        } else {
            MetricCalculate2<NetMetric, double> &pMetricCalculate{itMc->second};
            itMc++;
            NetMetric avgNetMetric, maxNetMetric, minNetMetric;
            pMetricCalculate.GetMaxValue(maxNetMetric);
            pMetricCalculate.GetMinValue(minNetMetric);
            pMetricCalculate.GetAvgValue(avgNetMetric);
            MetricData metricData, metricDataRx, metricDataTx;
            GetMetricData(maxNetMetric, minNetMetric, avgNetMetric, "system.net", deviceIp, deviceName, metricData);
            GetMetricData("vm.InternetNetworkRX", deviceName, (avgNetMetric.rxBytes * 8) / 1024, metricDataRx);
            GetMetricData("vm.InternetNetworkTX", deviceName, (avgNetMetric.txBytes * 8) / 1024, metricDataTx);
            collectData.dataVector.push_back(metricData);
            collectData.dataVector.push_back(metricDataRx);
            collectData.dataVector.push_back(metricDataTx);
            pMetricCalculate.Reset();
        }
    }
    ModuleData::convertCollectDataToString(collectData, data);
    return static_cast<int>(data.size());

}

void NetCollect::ReadInterfaceConfigs(map<string, InterfaceConfig> &interfaceConfigMap) {
    auto now = steady_clock::now();
    if (mInterfaceConfigExpireTime < now) {
        mInterfaceConfigExpireTime = now + 300_s;
        vector<InterfaceConfig> interfaceConfigs;
        GetInterfaceConfigs(interfaceConfigs);
        mInterfaceConfigMap.clear();
        for (auto &interfaceConfig: interfaceConfigs) {
            mInterfaceConfigMap[interfaceConfig.name] = interfaceConfig;
        }
        interfaceConfigMap = mInterfaceConfigMap;
    } else {
        interfaceConfigMap = mInterfaceConfigMap;
    }

}

void NetCollect::GetNetMetric(const SicNetInterfaceInformation &currentStat,
                              const SicNetInterfaceInformation &lastStat,
                              NetMetric &netMetric,
                              double interval) {
    GetRatios(currentStat, lastStat, interval, netMetric);
    netMetric.rxDropRatio = netMetric.rxPackets == 0 ? 0.0 : netMetric.rxDropped / netMetric.rxPackets * 100.0;
    netMetric.rxErrorRatio = netMetric.rxPackets == 0 ? 0.0 : netMetric.rxErrors / netMetric.rxPackets * 100.0;
    netMetric.txDropRatio = netMetric.txPackets == 0 ? 0.0 : netMetric.txDropped / netMetric.txPackets * 100.0;
    netMetric.txErrorRatio = netMetric.txPackets == 0 ? 0.0 : netMetric.txErrors / netMetric.txPackets * 100.0;
}

bool NetCollect::GetDeviceIp(const map<string, InterfaceConfig> &interfaceConfigMap,
                             const string &deviceName,
                             string &deviceIp) {
    for (const auto &entry: interfaceConfigMap) {
        if (entry.second.name == deviceName) {
            deviceIp = entry.second.ipv4;
            return true;
        }
    }
    return false;
}

void NetCollect::GetMetricData(const NetMetric &maxNetMetric,
                               const NetMetric &minNetMetric,
                               const NetMetric &avgNetMetric,
                               const string &metricName,
                               const string &deviceIp,
                               const string &deviceName,
                               MetricData &metricData) {
    metricData.tagMap["metricName"] = metricName;
    metricData.tagMap["ip"] = deviceIp;
    metricData.tagMap["device"] = deviceName;
    metricData.tagMap["ns"] = "acs/host";
    metricData.valueMap["metricValue"] = 0;

    for (auto it = netMetricMeta; it < netMetricMetaEnd; ++it) {
        if (it->isMetric) {
            metricData.valueMap[string("max_") + it->name] = it->value(maxNetMetric);
            metricData.valueMap[string("min_") + it->name] = it->value(minNetMetric);
            metricData.valueMap[string("avg_") + it->name] = it->value(avgNetMetric);
        }
    }
}

void NetCollect::GetMetricData(const string &metricName,
                               const string &deviceName,
                               double metricValue,
                               MetricData &metricData) {
    metricData.tagMap["metricName"] = metricName;
    metricData.tagMap["netname"] = deviceName;
    metricData.valueMap["metricValue"] = metricValue;
    metricData.tagMap["ns"] = "acs/ecs";
}

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DLL
#include "cloud_module_macros.h"
#include "common/ModuleTool.hpp"

IMPLEMENT_CMS_MODULE(net) {
    int totalCount = static_cast<int>(ToSeconds(cloudMonitor::kDefaultInterval));
    if (args != nullptr) {
        int count = convert<int>(args);
        if (count > 0) {
            totalCount = count;
        }
    }
    return module::NewHandler<NetCollect>(totalCount);
}
