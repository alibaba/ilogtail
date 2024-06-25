#ifndef CMS_DISK_COLLECT_H
#define CMS_DISK_COLLECT_H

#include <chrono>
#include <list>
#include <type_traits>

#include "base_collect.h"
#include "metric_calculate_2.h"
#include "common/ModuleData.h"

namespace cloudMonitor {
    struct DiskMetric {
        double reads = 0;
        double writes = 0;
        double writeBytes = 0;
        double readBytes = 0;
#ifndef WIN32
        double avgqu_sz = 0;
        double svctm = 0;
        double await = 0;
        double r_await = 0;
        double w_await = 0;
        double avgrq_sz = 0;
        double util = 0;
#endif
    };
    void enumerate(const std::function<void(const FieldName<DiskMetric, double> &)> &);

    struct DeviceMountInfo {
        std::string devName;
        std::vector<std::string> mountPaths;
        std::string type;
    };
    struct DiskStat {
        uint64_t reads = 0;
        uint64_t writes = 0;
        uint64_t writeBytes = 0;
        uint64_t readBytes = 0;
        uint64_t rtime = 0;
        uint64_t wtime = 0;
        uint64_t qtime = 0;
        uint64_t time = 0;
        double service_time = 0;
        double queue = 0;
    };

    struct PartitionStat {
        double total = 0;
        double free = 0;
        double used = 0;
        double usePercent = 0;

        void setValueMap(std::map<std::string, double> &valueMap) const;
    };

    struct DiskCollectStat {
        PartitionStat space;
        PartitionStat inode;
        double spaceAvail = 0;
        DiskStat diskStat;
        DeviceMountInfo deviceMountInfo;
    };

    // 最少一条
    template<typename T>
    std::string join_n(const T &v, const std::string &splitter, size_t n) {
        static_assert(std::is_base_of<std::vector<std::string>, T>::value
                      || std::is_base_of<std::set<std::string>, T>::value
                      || std::is_base_of<std::list<std::string>, T>::value,
                      "type must be std::vector<std::string> or std::list<std::string> or std::set<std::string>");
        std::string result;
        auto begin = v.begin();
        auto end = v.end();
        if (begin != end) {
            result = *begin++;
            n = (n == 0 ? std::numeric_limits<size_t>::max() : n);
            for (auto it = begin; it != end && result.size() + splitter.size() + it->size() <= n; ++it) {
                result.append(splitter);
                result.append(*it);
            }
        }
        //去掉最后一个分隔符
        return result;
    }

#include "common/test_support"
class DiskCollect : public BaseCollect {
public:
    DiskCollect();

    int Init(int totalCount, const std::string &excludeMountPointPath);

    ~DiskCollect();

    int Collect(std::string &data);

private:
    static int
    GetExcludeMountPathMap(const std::string &excludeMountStr, std::map<std::string, bool> &excludeMountPathMap);
    static int PrintExcludeMountPathMap(const std::map<std::string, bool> &excludeMountPathMap);
    static bool IsCollectMountPath(const std::map<std::string, bool> &excludePathMap, const std::string &mountPath);

    static std::string FormatDir(const std::string &dir);

    static void CalcDiskMetric(const DiskStat &current, const DiskStat &last, double interval, DiskMetric &diskMetric);

    int GetDeviceMountMap(std::map<std::string, DeviceMountInfo> &mountMap);

    int GetDiskCollectStatMap(std::map<std::string, DiskCollectStat> &diskCollectStatMap);

    void GetPartitionMetricData(const std::string &metricName,
                                const DeviceMountInfo &deviceMountInfo,
                                const std::string &diskSerialId,
                                const PartitionStat &partitionStat,
                                common::MetricData &metricData) const;

    void GetPartitionMetricData(const std::string &metricName,
                                const DeviceMountInfo &deviceMountInfo,
                                const std::string &diskSerialId,
                                double value,
                                common::MetricData &metricData) const;

    void GetPartitionMetricData(const std::string &metricName,
                                const DeviceMountInfo &deviceMountInfo,
                                const std::string &diskSerialId,
                                const PartitionStat &partitionStat,
                                double avail,
                                common::MetricData &metricData) const;

    void GetDiskMetricData(const std::string &metricName,
                           const DeviceMountInfo &deviceMountInfo,
                           const std::string &diskSerialId,
                           unsigned int count,
                           const DiskMetric &avgDiskMetric,
                           const DiskMetric &minDiskMetric,
                           const DiskMetric &maxDiskMetric,
                           common::MetricData &metricData) const;

    static void GetDiskMetricData(const std::string &metricName,
                                  const std::string &devName,
                                  const std::string &diskSerialId,
                                  double value,
                                  const std::string &ns,
                                  common::MetricData &metricData);

    static void GetDiskMetricData(const std::string &metricName,
                                  const std::string &devName,
                                  const std::string &diskSerialId,
                                  double value,
                                  common::MetricData &metricData);

    static std::string GetDiskName(const std::string &device);

private:
    std::map<std::string, DeviceMountInfo> mDeviceMountMap;
    std::chrono::steady_clock::time_point  mDeviceMountMapExpireTime;
    std::map<std::string, DiskCollectStat> mCurrentDiskCollectStatMap;
    std::map<std::string, DiskCollectStat> mLastDiskCollectStatMap;
    std::map<std::string, bool> mExcludeMountPathMap;
    std::map<std::string, MetricCalculate2<DiskMetric>> mMetricCalculateMap;
    const std::string mModuleName;
    int mTotalCount = 0;
    int mCount = 0;
    size_t maxDirSize = 0;
    std::chrono::steady_clock::time_point mLastTime; // 上次获取磁盘信息的时间
};
#include "common/test_support"

}
#endif