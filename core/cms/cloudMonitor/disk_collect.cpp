#include "disk_collect.h"

#include <algorithm>

#ifdef WIN32
#include <boost/algorithm/string/replace.hpp>  // replace_all_copy
#endif

#include "common/Arithmetic.h"
#include "common/JsonValueEx.h"
#include "common/Logger.h"
#include "common/StringUtils.h"
#include "common/TimeProfile.h"
#include "common/Chrono.h"
#include "common/Defer.h"
#include "common/Config.h"

#include "cloud_monitor_const.h"
#include "cloud_monitor_common.h"
#include "common/FieldEntry.h"

using namespace std;
using namespace common;
using namespace std::chrono;

#ifdef max
#   undef max
#   undef min
#endif

namespace cloudMonitor {
#define DISK_METRIC_META_ENTRY(Name, Value) FIELD_NAME_ENTRY(Name, DiskMetric, Value)
    // extern - 允许本文件外使用该常量(迫使编译器分配空间)
    extern const FieldName<DiskMetric, double> diskMetricMeta[] = {
            DISK_METRIC_META_ENTRY("reads", reads),
            DISK_METRIC_META_ENTRY("writes", writes),
            DISK_METRIC_META_ENTRY("write_bytes", writeBytes),
            DISK_METRIC_META_ENTRY("read_bytes", readBytes),
#ifndef WIN32
            DISK_METRIC_META_ENTRY("avgqu-sz", avgqu_sz),
            DISK_METRIC_META_ENTRY("svctm", svctm),
            DISK_METRIC_META_ENTRY("await", await),
            DISK_METRIC_META_ENTRY("r_await", r_await),
            DISK_METRIC_META_ENTRY("w_await", w_await),
            DISK_METRIC_META_ENTRY("avgrq-sz", avgrq_sz),
            DISK_METRIC_META_ENTRY("util", util),
#endif
    };
    const size_t diskMetricMetaSize = sizeof(diskMetricMeta) / sizeof(diskMetricMeta[0]);
    const FieldName<DiskMetric, double> *diskMetricMetaEnd = diskMetricMeta + diskMetricMetaSize;
#undef DISK_METRIC_META_ENTRY
    static_assert(diskMetricMetaSize == sizeof(DiskMetric) / sizeof(double), "diskMetricMeta size unexpected");

    void enumerate(const std::function<void(const FieldName<DiskMetric, double> &)> &callback) {
        for (auto it = diskMetricMeta; it < diskMetricMetaEnd; ++it) {
            callback(*it);
        }
    }

    DiskCollect::DiskCollect() : mModuleName("disk") {
        // https://aliyuque.antfin.com/cloudmonitor/dataservice/tyxm84ynck9xuld4#AlOJQ
        size_t dirNameLimit = SingletonConfig::Instance()->GetValue("agent.resource.dirName.limit", size_t(2048));
        maxDirSize = std::max(size_t(1024), dirNameLimit); // 配置项不得低于1024， 否则按1024处理
        LogInfo("load disk");
    }

    DiskCollect::~DiskCollect() {
        LogInfo("unload disk");
    }

    string DiskCollect::FormatDir(const string &dir) {
        string newDir = dir;
#ifdef WIN32
        const char sep = '\\';
        newDir = boost::replace_all_copy(dir, "/", "\\");
#else
        const char sep = '/';
#endif
        if (!newDir.empty() && *newDir.rbegin() != sep) {
            newDir += sep;
        }
        return newDir;
    }

    // 支持两种格式:
    // 1) 纯字符串, 相当于[{"dir":"/var/lib","includeSubDir":true}]， 例如mountpoint.exclude=/var/lib
    // 2) jsonArray, 格式: [{"dir":"xxx","includeSubDir":true}]
    int DiskCollect::GetExcludeMountPathMap(const string &excludeMountStr, map<string, bool> &excludeMountPathMap) {
        const char *keyDir = "dir";
        const char *keyIncludeSubDir = "includeSubDir";
        if (excludeMountStr.empty()) {
            return 0;
        }
        if (!StringUtils::StartWith(excludeMountStr, "[")) {
            excludeMountPathMap[FormatDir(excludeMountStr)] = true;
        } else {
            // 格式: [{"dir":"xxx","includeSubDir":true}]
            // parse json
            std::string error;
            json::Array jValue = json::parseArray(excludeMountStr, error);
            if (jValue.isNull()) {
                LogWarn("not valid json: {}, json: {}", error, excludeMountStr);
                return 0;
            }
            for (size_t i = 0; i < jValue.size(); i++) {
                json::Object item = jValue[i].asObject();

                string dir = item.getString(keyDir);
                if (dir.empty()) {
                    LogWarn("skip one item with key ({}) is not string", keyDir);
                    continue;
                }

                excludeMountPathMap[FormatDir(dir)] = item.getBool(keyIncludeSubDir);
            }

        }
        return static_cast<int>(excludeMountPathMap.size());
    }

    int DiskCollect::PrintExcludeMountPathMap(const map<string, bool> &excludeMountPathMap) {
        int index = 0;
        for (const auto &it: excludeMountPathMap) {
            LogInfo("excludeDir[{}]: dir={}, includeSubDir={}", (index++), it.first, it.second);
        }
        return 0;
    }

    bool DiskCollect::IsCollectMountPath(const map<string, bool> &excludePathMap, const string &mountPath) {
        string formatMountPath = FormatDir(mountPath);
        for (auto const &it: excludePathMap) {
            if (StringUtils::StartWith(formatMountPath, it.first)) {
                bool excludeSubDir = it.second;
                if (excludeSubDir || it.first == formatMountPath) {
                    return false;
                }
            }
        }
        return true;
    }

    int DiskCollect::Init(int totalCount, const string &excludeMountStr) {
        mTotalCount = totalCount;
        mCount = 0;
        mLastTime = std::chrono::steady_clock::time_point{};
        mDeviceMountMapExpireTime = std::chrono::steady_clock::time_point{};
        GetExcludeMountPathMap(excludeMountStr, mExcludeMountPathMap);
        PrintExcludeMountPathMap(mExcludeMountPathMap);
        return 0;
    }

    int DiskCollect::Collect(string &data) {
        using namespace std::chrono;

        TimeProfile tp;
        std::chrono::steady_clock::time_point currentTime = tp.lastTime();
        map<string, DiskCollectStat> diskCollectStatMap;
        if (GetDiskCollectStatMap(diskCollectStatMap) <= 0) {
            LogWarn("collect disk error");
            return -1;
        }
        mCurrentDiskCollectStatMap = diskCollectStatMap;
        if (IsZero(mLastTime)) {
            LogInfo("collect disk first time");
            mLastDiskCollectStatMap = mCurrentDiskCollectStatMap;
            mLastTime = currentTime;
            return 0;
        }
        if (mLastTime + 1_ms >= currentTime) {
            //调度间隔不能低于1ms
            LogInfo("collect disk too frequency");
            return -1;
        }

        auto interval = duration_cast<fraction_seconds>(currentTime - mLastTime);
        mLastTime = currentTime;
        mCount++;

        for (auto &it: mCurrentDiskCollectStatMap) {
            const string &devName = it.first;
            if (mLastDiskCollectStatMap.find(devName) != mLastDiskCollectStatMap.end()) {
                const DiskCollectStat &currentStat = mCurrentDiskCollectStatMap[devName];
                const DiskCollectStat &lastStat = mLastDiskCollectStatMap[devName];
                DiskMetric diskMetric;
                CalcDiskMetric(currentStat.diskStat, lastStat.diskStat, interval.count(), diskMetric);
                mMetricCalculateMap[devName].AddValue(diskMetric);
            }
        }
        mLastDiskCollectStatMap = mCurrentDiskCollectStatMap;
        LogDebug("collect disk spend {:.3f}ms", tp.cost<fraction_millis>().count());
        if (mCount < mTotalCount) {
            return 0;
        }
        mCount = 0;

        //格式化数据
        CollectData collectData;
        collectData.moduleName = mModuleName;

        //专有云需要按照设置维度的使用率
        struct PartitionStatEx : PartitionStat {
            std::string serialId; // 磁盘序列号
        };
        map<string, PartitionStatEx> totalDiskMap;
        for (auto itMc = mMetricCalculateMap.begin(); itMc != mMetricCalculateMap.end();) {
            string devName = itMc->first;
            if (mCurrentDiskCollectStatMap.find(devName) == mCurrentDiskCollectStatMap.end()) {
                itMc = mMetricCalculateMap.erase(itMc);
            } else {
                std::string diskSerialId;
                this->collector()->SicGetDiskSerialId(devName, diskSerialId);
                DiskCollectStat diskCollectStat = mCurrentDiskCollectStatMap[devName];
                string diskName = GetDiskName(devName);
                auto it = totalDiskMap.find(diskName);
                if (it == totalDiskMap.end()) {
                    dynamic_cast<PartitionStat &>(totalDiskMap[diskName]) = diskCollectStat.space;
                    totalDiskMap[diskName].serialId = diskSerialId;
                } else {
                    it->second.serialId = diskSerialId;
                    it->second.free += diskCollectStat.space.free;
                    it->second.total += diskCollectStat.space.total;
                    it->second.used += diskCollectStat.space.used;
                }
                MetricData spaceMetricData;
                GetPartitionMetricData("system.disk",
                                       diskCollectStat.deviceMountInfo,
                                       diskSerialId,
                                       diskCollectStat.space,
                                       diskCollectStat.spaceAvail,
                                       spaceMetricData);
                collectData.dataVector.push_back(spaceMetricData);

                MetricData oldSpaceMetricData;
                GetPartitionMetricData("vm.DiskUtilization",
                                       diskCollectStat.deviceMountInfo,
                                       diskSerialId,
                                       diskCollectStat.space.usePercent,
                                       oldSpaceMetricData);
                collectData.dataVector.push_back(oldSpaceMetricData);
#ifndef WIN32
                MetricData inodeMetricData, oldInodeMetricData;
                GetPartitionMetricData("system.inode",
                                       diskCollectStat.deviceMountInfo,
                                       diskSerialId,
                                       diskCollectStat.inode,
                                       inodeMetricData);
                GetPartitionMetricData("vm.DiskIusedUtilization",
                                       diskCollectStat.deviceMountInfo,
                                       diskSerialId,
                                       diskCollectStat.inode.usePercent,
                                       oldInodeMetricData);
                collectData.dataVector.push_back(inodeMetricData);
                collectData.dataVector.push_back(oldInodeMetricData);
#endif

                MetricCalculate2<DiskMetric> &pMetricCalculate = itMc->second;
                itMc++;
                defer(pMetricCalculate.Reset());

                DiskMetric avgDiskMetric, maxDiskMetric, minDiskMetric, lastDiskMetric;
                pMetricCalculate.Stat(maxDiskMetric, minDiskMetric, avgDiskMetric, &lastDiskMetric);
                MetricData diskMetricData;
                GetDiskMetricData("system.io",
                                  diskCollectStat.deviceMountInfo,
                                  diskSerialId,
                                  pMetricCalculate.Count(),
                                  avgDiskMetric,
                                  minDiskMetric,
                                  maxDiskMetric,
                                  diskMetricData);
                collectData.dataVector.push_back(diskMetricData);

                MetricData oldReadDiskMetricData;
                GetDiskMetricData("vm.DiskIORead", devName, diskSerialId, lastDiskMetric.readBytes / 1024,
                                  oldReadDiskMetricData);
                collectData.dataVector.push_back(oldReadDiskMetricData);

                MetricData oldWriteDiskMetricData;
                GetDiskMetricData("vm.DiskIOWrite", devName, diskSerialId, lastDiskMetric.writeBytes / 1024,
                                  oldWriteDiskMetricData);
                collectData.dataVector.push_back(oldWriteDiskMetricData);
            }
        }

        for (auto &it: totalDiskMap) {
            double util = it.second.total != 0 ? it.second.used / it.second.total * 100.0 : 0.0;
            MetricData totalDiskData;
            GetDiskMetricData("system.disk.total", it.first, it.second.serialId, util, "acs/host", totalDiskData);
            collectData.dataVector.push_back(totalDiskData);
        }
        ModuleData::convertCollectDataToString(collectData, data);
        return static_cast<int>(data.size());
    }

    void DiskCollect::CalcDiskMetric(const DiskStat &current, const DiskStat &last, double interval,
                                     DiskMetric &diskMetric) {
        // GetRatios((uint64_t *) &current, (uint64_t *) &last, interval, 4, (double *) &diskMetric);
        diskMetric.reads = GetRatio(current.reads, last.reads, interval);
        diskMetric.writes = GetRatio(current.writes, last.writes, interval);
        diskMetric.writeBytes = GetRatio(current.writeBytes, last.writeBytes, interval);
        diskMetric.readBytes = GetRatio(current.readBytes, last.readBytes, interval);
#ifndef WIN32
        diskMetric.avgqu_sz = current.queue;
        diskMetric.svctm = current.service_time;
        uint64_t rd_t = Delta(current.rtime, last.rtime);
        uint64_t wr_t = Delta(current.wtime, last.wtime);
        uint64_t rd_ios = Delta(current.reads, last.reads);
        uint64_t wr_ios = Delta(current.writes, last.writes);
        uint64_t rd_sec = Delta(current.readBytes, last.readBytes) / 512;
        uint64_t wr_sec = Delta(current.writeBytes, last.writeBytes) / 512;
        uint64_t tick = Delta(current.time, last.time);
        diskMetric.w_await = wr_ios > 0 ? wr_t / wr_ios : 0.0;
        diskMetric.r_await = rd_ios > 0 ? rd_t / rd_ios : 0.0;
        diskMetric.await = (rd_ios + wr_ios) > 0 ? (wr_t + rd_t) / (rd_ios + wr_ios) : 0.0;
        diskMetric.avgrq_sz = (rd_ios + wr_ios) > 0 ? (rd_sec + wr_sec) / (rd_ios + wr_ios) : 0.0;
        diskMetric.util = tick / (10.0 * interval);
#endif
    }

    int DiskCollect::GetDeviceMountMap(std::map<std::string, DeviceMountInfo> &deviceMountMap) {
        auto now = std::chrono::steady_clock::now(); // NowSeconds();
        if (now < mDeviceMountMapExpireTime) {
            deviceMountMap = mDeviceMountMap;
            return static_cast<int>(deviceMountMap.size());
        }
        mDeviceMountMapExpireTime = now + 60_s; // CloudMonitorConst::kDefaultMountInfoInterval;
        deviceMountMap.clear();

        vector<FileSystemInfo> fileSystemInfos;
        if (GetFileSystemInfos(fileSystemInfos) != 0) {
            // 走到这里时，就意味着mDeviceMountMapExpire又续了一条命
            return -1;
        }

        map<string, FileSystemInfo> mountMap;
        for (auto &fileSystemInfo: fileSystemInfos) {
            if (IsCollectMountPath(mExcludeMountPathMap, fileSystemInfo.dirName)) {
                mountMap[fileSystemInfo.dirName] = fileSystemInfo;
            }
        }

        for (auto &it: mountMap) {
            string devName = it.second.devName;
            if (deviceMountMap.find(devName) == deviceMountMap.end()) {
                DeviceMountInfo deviceMountInfo;
                deviceMountInfo.devName = devName;
                deviceMountInfo.type = it.second.type;
                deviceMountMap[devName] = deviceMountInfo;
            }
            deviceMountMap[devName].mountPaths.push_back(it.second.dirName);
        }
        //sort the dirName;

        for (auto &itD: deviceMountMap) {
            sort(itD.second.mountPaths.begin(), itD.second.mountPaths.end());
        }
        mDeviceMountMap = deviceMountMap;
        return static_cast<int>(deviceMountMap.size());
    }

    int DiskCollect::GetDiskCollectStatMap(std::map<std::string, DiskCollectStat> &diskCollectStatMap) {
        std::map<string, DeviceMountInfo> deviceMountMap;
        int num = GetDeviceMountMap(deviceMountMap);
        if (num <= 0) {
            return num;
        }

        for (auto &it: deviceMountMap) {
            string dirName = it.second.mountPaths[0];
            // string devName = it.second.devName;
            SicFileSystemUsage fileSystemStat;
            // 只有在获取文件系统信息成功之后才进行磁盘信息的获取
            if (GetFileSystemStat(dirName, fileSystemStat) != 0) {// || GetDiskStat(devName, fileSystemStat) != 0) {
                continue;
            }
            if (std::isinf(fileSystemStat.disk.queue) && !this->SicPtr()->errorMessage.empty()) {
                LogInfo("{}", SicPtr()->errorMessage);
            }
            DiskCollectStat diskCollectStat;
            diskCollectStat.deviceMountInfo = it.second;
#define CastUint64(Expr) static_cast<uint64_t>(Expr)
#define CastDouble(Expr) static_cast<double>(Expr)
            diskCollectStat.space.total = CastDouble(fileSystemStat.total);
            diskCollectStat.space.free = CastDouble(fileSystemStat.free);
            diskCollectStat.space.used = CastDouble(fileSystemStat.used);
            diskCollectStat.space.usePercent = fileSystemStat.use_percent * 100.0;
            diskCollectStat.spaceAvail = CastDouble(fileSystemStat.avail);

            diskCollectStat.inode.total = CastDouble(fileSystemStat.files);
            diskCollectStat.inode.free = CastDouble(fileSystemStat.freeFiles);
            diskCollectStat.inode.used = fileSystemStat.files > fileSystemStat.freeFiles
                                         ? CastDouble(fileSystemStat.files - fileSystemStat.freeFiles) : 0.0;
            if (fileSystemStat.files != 0) {
                diskCollectStat.inode.usePercent =
                        (diskCollectStat.inode.used * 100.0) / (diskCollectStat.inode.total * 1.0);
            }

            diskCollectStat.diskStat.reads = CastUint64(fileSystemStat.disk.reads);
            diskCollectStat.diskStat.writes = CastUint64(fileSystemStat.disk.writes);
            diskCollectStat.diskStat.writeBytes = CastUint64(fileSystemStat.disk.writeBytes);
            diskCollectStat.diskStat.readBytes = CastUint64(fileSystemStat.disk.readBytes);
            diskCollectStat.diskStat.rtime = CastUint64(fileSystemStat.disk.rTime);
            diskCollectStat.diskStat.wtime = CastUint64(fileSystemStat.disk.wTime);
            diskCollectStat.diskStat.qtime = CastUint64(fileSystemStat.disk.qTime);
            diskCollectStat.diskStat.time = CastUint64(fileSystemStat.disk.time);
            diskCollectStat.diskStat.service_time =
                    CastDouble(fileSystemStat.disk.serviceTime >= 0 ? fileSystemStat.disk.serviceTime : 0.0);
            diskCollectStat.diskStat.queue = fileSystemStat.disk.queue >= 0 ? fileSystemStat.disk.queue : 0.0;
            diskCollectStatMap[it.first] = diskCollectStat;
#undef CastDouble
#undef CastUint64
        }
        return static_cast<int>(diskCollectStatMap.size());
    }

    void PartitionStat::setValueMap(std::map<std::string, double> &valueMap) const {
        valueMap["total"] = this->total;
        valueMap["free"] = this->free;
        valueMap["used"] = this->used;
        valueMap["use_percent"] = this->usePercent;
    }

    void DiskCollect::GetPartitionMetricData(const string &metricName,
                                             const DeviceMountInfo &deviceMountInfo,
                                             const std::string &diskSerialId,
                                             const PartitionStat &partitionStat,
                                             double avail,
                                             MetricData &metricData) const {
        metricData.tagMap["metricName"] = metricName;
        metricData.tagMap["dir_name"] = join_n(deviceMountInfo.mountPaths, ",", maxDirSize);
        metricData.tagMap["ns"] = "acs/host";
        metricData.tagMap["device"] = deviceMountInfo.devName;
        metricData.tagMap["dev_type"] = deviceMountInfo.type;
        if (!diskSerialId.empty()) {
            // https://aone.alibaba-inc.com/v2/project/844208/req/51228413
            metricData.tagMap["id_serial"] = diskSerialId;
        }

        metricData.valueMap["metricValue"] = 0;
        partitionStat.setValueMap(metricData.valueMap);
        metricData.valueMap["avail"] = avail;
    }

    void DiskCollect::GetPartitionMetricData(const string &metricName,
                                             const DeviceMountInfo &deviceMountInfo,
                                             const std::string &diskSerialId,
                                             const PartitionStat &partitionStat,
                                             MetricData &metricData) const {
        metricData.tagMap["metricName"] = metricName;
        metricData.tagMap["dir_name"] = join_n(deviceMountInfo.mountPaths, ",", maxDirSize);
        metricData.tagMap["ns"] = "acs/host";
        metricData.tagMap["device"] = deviceMountInfo.devName;
        metricData.tagMap["dev_type"] = deviceMountInfo.type;
        if (!diskSerialId.empty()) {
            // https://aone.alibaba-inc.com/v2/project/844208/req/51228413
            metricData.tagMap["id_serial"] = diskSerialId;
        }

        metricData.valueMap["metricValue"] = 0;
        partitionStat.setValueMap(metricData.valueMap);
    }

    void DiskCollect::GetPartitionMetricData(const string &metricName,
                                             const DeviceMountInfo &deviceMountInfo,
                                             const std::string &diskSerialId,
                                             double value,
                                             MetricData &metricData) const {
        metricData.tagMap["metricName"] = metricName;
        metricData.tagMap["mountpoint"] = join_n(deviceMountInfo.mountPaths, ",", maxDirSize);
        metricData.tagMap["ns"] = "acs/ecs";
        if (!diskSerialId.empty()) {
            // https://aone.alibaba-inc.com/v2/project/844208/req/51228413
            metricData.tagMap["id_serial"] = diskSerialId;
        }
        metricData.valueMap["metricValue"] = value;
    }

    void DiskCollect::GetDiskMetricData(const string &metricName,
                                        const DeviceMountInfo &deviceMountInfo,
                                        const std::string &diskSerialId,
                                        unsigned int count,
                                        const DiskMetric &avgDiskMetric,
                                        const DiskMetric &minDiskMetric,
                                        const DiskMetric &maxDiskMetric,
                                        MetricData &metricData) const {
        metricData.tagMap["metricName"] = metricName;
        metricData.tagMap["dir_name"] = join_n(deviceMountInfo.mountPaths, ",", maxDirSize);
        metricData.tagMap["device"] = deviceMountInfo.devName;
        metricData.tagMap["dev_type"] = deviceMountInfo.type;
        metricData.tagMap["ns"] = "acs/host";
        metricData.valueMap["metricValue"] = 0;
        if (!diskSerialId.empty()) {
            // https://aone.alibaba-inc.com/v2/project/844208/req/51228413
            metricData.tagMap["id_serial"] = diskSerialId;
        }
        metricData.valueMap["count"] = count;  // 参与聚合的数据点数
        for (auto it = diskMetricMeta; it < diskMetricMetaEnd; ++it) {
            metricData.valueMap[string("avg_") + it->name] = it->value(avgDiskMetric);
            metricData.valueMap[string("min_") + it->name] = it->value(minDiskMetric);
            metricData.valueMap[string("max_") + it->name] = it->value(maxDiskMetric);
        }
    }

    void DiskCollect::GetDiskMetricData(const std::string &metricName,
                                        const std::string &devName,
                                        const std::string &diskSerialId,
                                        double value,
                                        MetricData &metricData) {
        metricData.tagMap["metricName"] = metricName;
        metricData.tagMap["diskname"] = devName;
        metricData.valueMap["metricValue"] = value;
        metricData.tagMap["ns"] = "acs/ecs";
        if (!diskSerialId.empty()) {
            // https://aone.alibaba-inc.com/v2/project/844208/req/51228413
            metricData.tagMap["id_serial"] = diskSerialId;
        }
    }

    void DiskCollect::GetDiskMetricData(const std::string &metricName,
                                        const std::string &devName,
                                        const std::string &diskSerialId,
                                        double value,
                                        const string &ns,
                                        MetricData &metricData) {
        metricData.tagMap["metricName"] = metricName;
        metricData.tagMap["diskname"] = devName;
        metricData.valueMap["metricValue"] = value;
        metricData.tagMap["ns"] = ns;
        if (!diskSerialId.empty()) {
            // https://aone.alibaba-inc.com/v2/project/844208/req/51228413
            metricData.tagMap["id_serial"] = diskSerialId;
        }
    }

    // 获取设备的名称
    // input:/dev/sda1, output:sda
    // input:/dev/sda10,output:sda
    string DiskCollect::GetDiskName(const string &dev) {
        string device = dev;
        size_t index = device.find("/dev/");
        if (index != string::npos) {
            device = device.substr(5);
        }
        for (int i = static_cast<int>(device.size()) - 1; i >= 0; i--) {
            if (device[i] < '0' || device[i] > '9') {
                return device.substr(0, i + 1);
            }
        }
        return device;
    }
} // namespace cloudMonitor

// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DLL
#include "cloud_module_macros.h"

using cloudMonitor::DiskCollect;

IMPLEMENT_CMS_MODULE(disk) {
    int totalCount = static_cast<int>(ToSeconds(cloudMonitor::kDefaultInterval));
    string excludeMountStr;
    if (args != nullptr) {
        int count = convert<int>(args);
        if (count > 0) {
            totalCount = count;
        }
    }
    excludeMountStr = SingletonConfig::Instance()->GetValue("mountpoint.exclude", "");

    return module::NewHandler<DiskCollect>(totalCount, excludeMountStr);
}
