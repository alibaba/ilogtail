#ifndef CMS_CPU_COLLECT_H_
#define CMS_CPU_COLLECT_H_
#include <string>
#include "base_collect.h"
#include "metric_calculate_2.h"

namespace common {
    struct MetricData;
}

namespace cloudMonitor {
    struct CpuMetric {
        double sys = 0;
        double user = 0;
        double wait = 0;
        double idle = 0;
        double other = 0;
        double total = 0;

        explicit CpuMetric(const SicCpuPercent &cpuPercent);
        CpuMetric() = default;
    };
    // For MetricCalculate2<CpuMetric, double>
    void enumerate(const std::function<void(const FieldName<CpuMetric, double> &)>&);

#include "common/test_support"
    class CpuCollect : public BaseCollect {
    public:
        CpuCollect();

        int Init(int totalCount);

        ~CpuCollect();

        int Collect(std::string &collectData);

    private:
        void GetCollectData(std::string &collectStr);

        void GetMetricData(const CpuMetric &maxCpuMetric,
                           const CpuMetric &minCpuMetric,
                           const CpuMetric &avgCpuMetric,
                           const std::string &metricName,
                           int metricValue,
                           size_t index,
                           common::MetricData &metricData) const;

    private:
        int mCount = 0;
        int mTotalCount = 0;
        size_t mCoreNumber = 0; // 含cpu指标，因此会比实际核数多1
        const std::string mModuleName = "cpu";

        std::vector<SicCpuInformation> mCurrentCpuStats;
        std::vector<SicCpuInformation> mLastCpuStats;

        std::vector<MetricCalculate2<CpuMetric, double>> mMetricCalculates;
    };
#include "common/test_support"
}
#endif
