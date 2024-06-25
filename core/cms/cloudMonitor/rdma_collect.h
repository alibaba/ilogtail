#ifndef CMS_RDMA_COLLECT_H
#define CMS_RDMA_COLLECT_H
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include "common/ModuleData.h"
#include "sliding_window.h"

namespace cloudMonitor {
    struct DeviceRdmaData {
        SlidingWindow<int64_t> value{15};
        int64_t lastMillis = 0; // 毫秒级时间戳
        int64_t lastValue = -1;

        void UpdateValue(int64_t n, int64_t millis);

    };

    struct RPingData {
        std::string ip;
        int64_t rt;
        int64_t ts;
    };

    struct RdmaInterfaceMetric {
        std::string interfaceName;
        std::map<std::string, DeviceRdmaData> metric;
    };

#include "common/test_support"
class RdmaCollect {
public:

public:
    RdmaCollect();

    int Init(int rdmaCount, int qosCount, const std::string &rdmaFile, const std::string &rPingFile, const std::string &qosCheckFile);

    ~RdmaCollect();

    int Collect(std::string &data);

private:

    int64_t UpdateRdmaData();
    static std::vector<std::string> ParseLines(const std::string &line);

    static bool GetRdmaMetricData(const std::string &device, const RdmaInterfaceMetric &, common::MetricData &metricData);

    void CollectRPingData(std::vector <RPingData> &rPingData) const;

    static void GetRPingMetricData(const RPingData &rPingdata, common::MetricData &metricData);

    bool GetQosMetricData(common::MetricData &metricData) const;

private:
    const std::string mModuleName;
    int mRdmaCount = 0;
    int mQosCount = 0;
    uint64_t mCount = 0;
    std::string mRdmaFile;
    std::string mRPingFile;
    std::string mQosCheckFile;
    static const size_t RDMA_DEVICE_COUNT = 3;
    RdmaInterfaceMetric mRdmaDataMap[RDMA_DEVICE_COUNT];
    static const std::string RDMA_DEVICES[RDMA_DEVICE_COUNT];
};
#include "common/test_support"

}
#endif