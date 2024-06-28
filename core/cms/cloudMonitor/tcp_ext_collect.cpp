//
// Created by 许刘泽 on 2021/2/20.
//

#include "tcp_ext_collect.h"

#include <unordered_map>

#include "common/Logger.h"
#include "common/ModuleData.h"
#include "common/FieldEntry.h"
#include "common/Chrono.h"
#include "common/FileUtils.h"
#include "common/Config.h"

#include "cloud_monitor_const.h"

using namespace std;
using namespace common;
using namespace std::chrono;
using namespace cloudMonitor;

template<typename T>
static T delta(const T v1, const T v2) {
    return v1 > v2 ? v1 - v2 : 0;
}

static double safeDivide(const double v1, const double v2) {
    return v2 != 0 && v1 > 0 ? (1.0 * v1) / v2 : 0;
}

namespace cloudMonitor {
#define TCP_EXT_METRIC_META_ENTRY(Name, Value) FIELD_NAME_ENTRY(Name, TcpExtMetric, Value)
    // extern - 允许本文件外使用该常量(迫使编译器分配空间)
    extern const FieldName<TcpExtMetric, double> tcpExtMetricMeta[] = {
            TCP_EXT_METRIC_META_ENTRY("DelayedACKs", delayedACKs),
            TCP_EXT_METRIC_META_ENTRY("ListenOverflows", listenOverflows),
            TCP_EXT_METRIC_META_ENTRY("ListenDrops", listenDrops),
            TCP_EXT_METRIC_META_ENTRY("TCPPrequeued", tcpPrequeued),
            TCP_EXT_METRIC_META_ENTRY("TCPPrequeueDropped", tcpPrequeueDropped),
            TCP_EXT_METRIC_META_ENTRY("TCPPureAcks", tcpPureAcks),
            TCP_EXT_METRIC_META_ENTRY("TCPFACKReorder", tcpFACKReorder),
            TCP_EXT_METRIC_META_ENTRY("TCPSACKReorder", tcpSACKReorder),
            TCP_EXT_METRIC_META_ENTRY("TCPLossProbes", tcpLossProbes),
            TCP_EXT_METRIC_META_ENTRY("TCPLossProbeRecovery", tcpLossProbeRecovery),
            TCP_EXT_METRIC_META_ENTRY("TCPLostRetransmit", tcpLostRetransmit),
            TCP_EXT_METRIC_META_ENTRY("TCPFastRetrans", tcpFastRetrans),
            TCP_EXT_METRIC_META_ENTRY("TCPSlowStartRetrans", tcpSlowStartRetrans),
            TCP_EXT_METRIC_META_ENTRY("TCPTimeouts", tcpTimeouts),
    };
    const size_t tcpExtMetricMetaSize = sizeof(tcpExtMetricMeta) / sizeof(tcpExtMetricMeta[0]);
    const FieldName<TcpExtMetric, double> *tcpExtMetricMetaEnd = tcpExtMetricMeta + tcpExtMetricMetaSize;
#undef TCP_EXT_METRIC_META_ENTRY

    // 保证tcpExtMetricMeta与TcpExtMetric定义的成员一致
    static_assert(tcpExtMetricMetaSize == sizeof(TcpExtMetric) / sizeof(double), "tcpExtMetricMeta not expected");

    void enumerate(const std::function<void(const MetricCalculate2<TcpExtMetric>::FieldMeta &)> &callback) {
        for (auto it = tcpExtMetricMeta; it < tcpExtMetricMetaEnd; ++it) {
            callback(*it);
        }
    }
}

TcpExtCollect::TcpExtCollect()
        : mModuleName{"tcpExt"} {
    LogInfo("load tcp ext");
}

TcpExtCollect::~TcpExtCollect() {
    LogInfo("unload tcp ext");
}

int TcpExtCollect::Init(int totalCount) {
    mActivate = SingletonConfig::Instance()->GetValue("cms.module.tcpExt", "false") == "true";
    mTotalCount = totalCount;
    mCount = 0;
    // mLastTime = 0;
    mLastTime = steady_clock::time_point{};
    collectCount = 0;
    LogInfo("module tcpExt activate: {}", mActivate);
    return 0;
}

void TcpExtCollect::Calculate() {
    // auto *res = (double *) (&mTcpExtMetric);
    // auto *last = (double *) (&mLastStat);
    // auto *cur = (double *) (&mCurrentStat);
    // double timestamp = (delta(mCurrentTime, mLastTime)) / (1000.0 * 1000);
    // for (int i = 0; i < tcpExtMetricMetaSize; i++) {
    //     double value = delta(cur[i], last[i]);
    //     res[i] = safeDivide(value, timestamp);
    // }

    double timestamp = duration_cast<fraction_seconds>(mCurrentTime - mLastTime).count();
    for (auto const &field: tcpExtMetricMeta) {
        double value = delta(field.value(mCurrentStat), field.value(mLastStat));
        field.value(mTcpExtMetric) = safeDivide(value, timestamp);
    }
}

int TcpExtCollect::ReadTcpExtFile(TcpExtMetric &tcpExtMetric) {
    std::string errorMessage;
    std::vector<std::string> tcpExtLines = {};
    int ret = GetFileLines(TCP_EXT_FILE, tcpExtLines, true, errorMessage);
    if (ret != 0 || tcpExtLines.empty()) {
        LogError("{}", errorMessage);
        return ret;
    }

    if (tcpExtLines.size() >= 2 && tcpExtLines[1].size() >= 6) {
        std::vector<std::string> tcpExtMetricName = StringUtils::split(tcpExtLines[0], ' ', false);
        std::vector<std::string> tcpExtMetricValue = StringUtils::split(tcpExtLines[1], ' ', false);

        if (tcpExtMetricName.size() != tcpExtMetricValue.size() || tcpExtMetricName.size() <= 1) {
            LogError("invalid netstat file {}, {}", tcpExtLines[0], tcpExtLines[1]);
            return -1;
        }
        std::unordered_map<std::string, double> metricMap{tcpExtMetricName.size()};
        // skip "tcpExt:"
        for (size_t i = 1; i < tcpExtMetricName.size(); ++i) {
            metricMap[tcpExtMetricName[i]] = convert<double>(tcpExtMetricValue[i]);
        }

        for (auto const &entry: tcpExtMetricMeta) {
            entry.value(tcpExtMetric) = metricMap[entry.name];
        }
    }

    return 0;
}

int TcpExtCollect::Collect(string &data) {
    data = "";
    if (!mActivate) {
        return 0;
    }
    system_clock::time_point start = system_clock::now();

    mLastStat = mCurrentStat;

    int ret = -1;
    if (ReadTcpExtFile(mCurrentStat) >= 0) {
        mLastTime = mCurrentTime;
        mCurrentTime = steady_clock::now();

        collectCount++;
        if (1 == collectCount) { // mFirstCollect) {
            LogInfo("TcpExtCollect first time");
            // mMetricCalculate = new MetricCalculate<double>(tcpExtMetricMetaSize);
            // mFirstCollect = false;
            return 0;
        }
        Calculate();

        mMetricCalculate.AddValue(mTcpExtMetric);

        mCount++;
        auto cost = duration_cast<fraction_millis>(system_clock::now() - start);
        LogDebug("collect tcpExt spend {:.3f} ms", cost.count());
        if (mCount < mTotalCount) {
            return 0;
        }

        mMetricCalculate.GetMaxValue(mMaxMetrics);
        mMetricCalculate.GetMinValue(mMinMetrics);
        mMetricCalculate.GetAvgValue(mAvgMetrics);

        GetCollectData(data);
        mCount = 0;
        mMetricCalculate.Reset();
        ret = static_cast<int>(data.size());
    }
    return ret;
}

void TcpExtCollect::GetCollectData(string &collectStr) {
    CollectData collectData;
    collectData.moduleName = mModuleName;
    string metricName = "system.tcpExt";
    MetricData metricData;
    metricData.tagMap["metricName"] = metricName;
    metricData.tagMap["ns"] = "acs/host";
    metricData.valueMap["metricValue"] = 0;

    for (const auto &entry: tcpExtMetricMeta) {
        metricData.valueMap[std::string("max_") + entry.name] = entry.value(mMaxMetrics);
        metricData.valueMap[std::string("min_") + entry.name] = entry.value(mMinMetrics);
        metricData.valueMap[std::string("avg_") + entry.name] = entry.value(mAvgMetrics);
    }
    collectData.dataVector.push_back(metricData);
    ModuleData::convertCollectDataToString(collectData, collectStr);
}

// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DLL
#include "cloud_module_macros.h"

IMPLEMENT_CMS_MODULE(tcpExt) {
    int totalCount = static_cast<int>(ToSeconds(cloudMonitor::kDefaultInterval));
    if (args != nullptr) {
        int count = convert<int>(args);
        if (count > 0) {
            totalCount = count;
        }
    }
    return module::NewHandler<cloudMonitor::TcpExtCollect>(totalCount);
}
