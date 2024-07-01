#include "rdma_collect.h"

#include "core/TaskManager.h"
#include "common/Logger.h"
#include "common/FileUtils.h"
#include "common/StringUtils.h"
#include "common/Common.h"
#include "common/TimeProfile.h"
#include "common/Chrono.h"
#include "common/Config.h"

#ifdef max
#   undef max
#endif

using namespace std;
using namespace argus;
using namespace common;

namespace cloudMonitor {
    const string RdmaCollect::RDMA_DEVICES[RDMA_DEVICE_COUNT] = {"bond0", "slave0", "slave1"};

    void DeviceRdmaData::UpdateValue(int64_t n, int64_t nowMillis) {
        //干掉非法数据
        if (n >= lastValue) {
            using namespace std::chrono;

            if (lastValue != -1) {
                double interval = static_cast<double>(nowMillis - lastMillis) / 1000.0;
                if (interval > 0) {
                    auto delta = static_cast<double>(n - lastValue);
                    value.Update(static_cast<decltype(value)::Type>(delta / interval));
                }
            }
            lastValue = n;
            lastMillis = nowMillis;
        }
    }

    RdmaCollect::RdmaCollect() : mModuleName{"rdma"} {
        LogInfo("load {}", mModuleName);
    }

    int RdmaCollect::Init(int rdmaCount,
                          int qosCount,
                          const string &rdmaFile,
                          const string &rPingFile,
                          const string &qosCheckFile) {
        mRdmaCount = rdmaCount;
        mQosCount = qosCount;
        mRdmaFile = rdmaFile;
        mRPingFile = rPingFile;
        mQosCheckFile = qosCheckFile;
        mCount = 0;
        return 0;
    }

    RdmaCollect::~RdmaCollect() {
        LogInfo("unload {}", mModuleName);
    }

    int RdmaCollect::Collect(string &data) {
        HpcClusterItem hpcClusterItem;
        SingletonTaskManager::Instance()->GetHpcClusterItem(hpcClusterItem);
        if (!hpcClusterItem.isValid) {
            LogDebug("hpcClusterItem.isValid = false, skip rdma");
            return 0;
        }

        TimeProfile timeProfile;
        mCount++;
        UpdateRdmaData();

        CollectData collectData;
        collectData.moduleName = mModuleName;
        if (mCount % mRdmaCount == 0) {
            //打包rdma数据
            for (size_t i = 0; i < RDMA_DEVICE_COUNT; i++) {
                MetricData metricData;
                if (GetRdmaMetricData(RDMA_DEVICES[i], mRdmaDataMap[i], metricData)) {
                    collectData.dataVector.push_back(metricData);
                }
            }
            //采集Rping数据
            vector<RPingData> pPingDatas;
            CollectRPingData(pPingDatas);
            //打包Rping数据
            for (const auto &pPingData: pPingDatas) {
                MetricData rPingMetricData;
                GetRPingMetricData(pPingData, rPingMetricData);
                collectData.dataVector.push_back(rPingMetricData);
            }

        }
        if (mCount % mQosCount == 0) {
            MetricData metricData;
            if (GetQosMetricData(metricData)) {
                collectData.dataVector.push_back(metricData);
            }
        }

        int collectedCount = 0;
        if (!collectData.dataVector.empty()) {
            ModuleData::convertCollectDataToString(collectData, data);
            collectedCount = static_cast<int>(data.size());
        }
        LogDebug("collect rdma spend {} ms", timeProfile.millis());

        return collectedCount;
    }

    int64_t RdmaCollect::UpdateRdmaData() {
        using namespace std::chrono;
        int64_t millis = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();

        vector<string> lines;
        FileUtils::ReadFileLines(mRdmaFile, lines);
        for (const auto &line: lines) {
            std::vector<std::string> columns = ParseLines(line);
            if (!columns.empty()) {
                std::string metricName = columns[0];
                for (size_t i = 1; i < columns.size() && i < (1 + RDMA_DEVICE_COUNT); i++) {
                    auto &device = mRdmaDataMap[i - 1];
                    if (metricName == "interface") {
                        device.interfaceName = TrimSpace(columns[i]);
                    } else {
                        device.metric[metricName].UpdateValue(convert<int64_t>(columns[i]), millis);
                    }
                }
            }
        }

        return millis;
    }

    // rdma.log支持两种形式的数据:
    // 1. rx_buf_discard: 18432 (9216/9216)
    // 2. num_of_qp: 2
    std::vector<std::string> RdmaCollect::ParseLines(const string &line) {
        vector<string> fields = StringUtils::split(line, ":", true);
        if (fields.size() != 2 || fields[0].empty()) {
            return {};
        }

        std::vector<std::string> columns;
        columns.reserve(4);

        size_t index1 = fields[1].find('(');
        if (index1 == string::npos) {
            columns.push_back(fields[0]); // metricName
            columns.push_back(fields[1]);
        } else {
            size_t index2 = fields[1].find('/', index1 + 1);
            size_t index3 = (index2 == std::string::npos ? index2 : fields[1].find(')', index2 + 1));
            if (index2 != string::npos && index3 != string::npos) {
                columns.push_back(fields[0]); // metricName
                columns.push_back(fields[1].substr(0, index1));
                columns.push_back(fields[1].substr(index1 + 1, index2 - (index1 + 1)));
                columns.push_back(fields[1].substr(index2 + 1, index3 - (index2 + 1)));
            }
        }

        RETURN_RVALUE(columns);
    }

    bool RdmaCollect::GetRdmaMetricData(const string &device,
                                        const RdmaInterfaceMetric &rdmaInterface,
                                        MetricData &metricData) {
        if (rdmaInterface.metric.empty()) {
            return false;
        }
        HpcClusterItem hpcClusterItem;
        SingletonTaskManager::Instance()->GetHpcClusterItem(hpcClusterItem);
        if (!hpcClusterItem.isValid) {
            return false;
        }
        metricData.tagMap["hpcClusterId"] = hpcClusterItem.clusterId;
        metricData.tagMap["device"] = device;
        metricData.tagMap["interface"] = (rdmaInterface.interfaceName.empty() ? device : rdmaInterface.interfaceName);
        metricData.tagMap["ns"] = "acs/rdma";
        metricData.tagMap["metricName"] = "net";
        metricData.valueMap["metricValue"] = 0;
        for (const auto &it: rdmaInterface.metric) {
            const SlidingWindow<int64_t> &value = it.second.value;
            metricData.valueMap[it.first + "_min"] = static_cast<double>(value.Min());
            metricData.valueMap[it.first + "_max"] = static_cast<double>(value.Max());
            metricData.valueMap[it.first + "_avg"] = value.Avg();
            metricData.valueMap[it.first + "_last"] = static_cast<double>(it.second.lastValue);
            metricData.valueMap[it.first + "_count"] = static_cast<double>(value.AccumulateCount());
        }
        return true;
    }

    void RdmaCollect::CollectRPingData(vector <RPingData> &rPingData) const {
        vector<string> lines;
        FileUtils::ReadFileLines(mRPingFile, lines);

        int64_t now = NowMillis();
        for (const auto &line: lines) {
            vector<string> fields = StringUtils::split(line, " ", true);
            if (fields.size() != 3) {
                continue;
            }
            RPingData data;
            data.ip = fields[0];
            data.rt = convert<int64_t>(fields[1]);
            data.ts = convert<int64_t>(fields[2]);
            int64_t delta = data.ts > now ? data.ts - now : now - data.ts;
            if (delta > 60 * 1000) {
                LogInfo("log time({:L}) - system time({:L}) > 1 min, discard: {}",
                        std::chrono::FromMillis(data.ts), std::chrono::FromMicros(now), line);
                continue;
            }
            rPingData.push_back(data);
        }
    }

    void RdmaCollect::GetRPingMetricData(const RPingData &rPingdata, MetricData &metricData) {
        HpcClusterItem hpcClusterItem;
        SingletonTaskManager::Instance()->GetHpcClusterItem(hpcClusterItem);
        metricData.tagMap["hpcClusterId"] = hpcClusterItem.clusterId;
        metricData.tagMap["ns"] = "acs/rdma";
        metricData.tagMap["metricName"] = "rping";
        metricData.valueMap["metricValue"] = 0;
        metricData.tagMap["ip"] = rPingdata.ip;
        metricData.valueMap["rt"] = static_cast<double>(rPingdata.rt);
        metricData.valueMap["rping_ts"] = static_cast<double>(rPingdata.ts);
    }

    bool RdmaCollect::GetQosMetricData(MetricData &metricData) const {
        HpcClusterItem hpcClusterItem;
        SingletonTaskManager::Instance()->GetHpcClusterItem(hpcClusterItem);
        if (!hpcClusterItem.isValid) {
            return false;
        }
        //check mQosCheckFile--/usr/local/bin/rdma_qos_check exist;
        // char buf[2048];
        std::string buf;
        int exitCode = getCmdOutput(mQosCheckFile, buf);
        //127为二进制不存在
        if (exitCode == 127) {
            return false;
        }
        metricData.tagMap["hpcClusterId"] = hpcClusterItem.clusterId;
        metricData.tagMap["ns"] = "acs/rdma";
        metricData.tagMap["metricName"] = "qos_check";
        metricData.valueMap["metricValue"] = 0;
        metricData.valueMap["exitValue"] = exitCode;

        return true;
    }
}

// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// for dll
#include "cloud_module_macros.h"

IMPLEMENT_CMS_MODULE(rdma) {
    const auto *config = SingletonConfig::Instance();

    int rdmaInterval = config->GetValue("cms.rdma.interval", 15);
    rdmaInterval = rdmaInterval <= 0 ? 15 : rdmaInterval;
    int qosInterval = std::max(60, config->GetValue("cms.qos.interval", 60)); // 最小60s
    std::string rdmaFile = config->GetValue("cms.rdma.path", "/dev/shm/rdma.log");
    std::string rpingFile = config->GetValue("cms.rping.path", "/dev/shm/rping.log");
    std::string qpsCmd = config->GetValue("cms.qos.cmd", "/usr/local/bin/rdma_qos_check");
    return module::NewHandler<cloudMonitor::RdmaCollect>(rdmaInterval, qosInterval, rdmaFile, rpingFile, qpsCmd);
}
