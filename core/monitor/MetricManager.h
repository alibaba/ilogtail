/*
 * Copyright 2023 iLogtail Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "MetricRecord.h"
#include "common/Lock.h"
#include "protobuf/sls/sls_logs.pb.h"

namespace logtail {

class MetricManager {
private:
    MetricManager() = default;
    // write list
    std::mutex mWriteListMutex;
    MetricsRecord* mWriteListHead = nullptr;
    void ClearWriteList();
    MetricsRecord* GetWriteListHead();
    MetricsRecord* DoSnapshot();
    // read list
    mutable ReadWriteLock mReadListLock;
    MetricsRecord* mReadListHead = nullptr;
    void ClearReadList();
    MetricsRecord* GetReadListHead();

public:
    ~MetricManager();
    static MetricManager* GetInstance() {
        static MetricManager* ptr = new MetricManager();
        return ptr;
    }

    // write
    void PrepareMetricsRecordRef(MetricsRecordRef& ref,
                                 const std::string& category,
                                 MetricLabels&& labels,
                                 DynamicMetricLabels&& dynamicLabels = {});
    void CreateMetricsRecordRef(MetricsRecordRef& ref,
                                const std::string& category,
                                MetricLabels&& labels,
                                DynamicMetricLabels&& dynamicLabels = {});
    void CommitMetricsRecordRef(MetricsRecordRef& ref);

    // read
    void UpdateMetrics();
    void ReadAsLogGroup(const std::string& regionFieldName,
                        const std::string& defaultRegion,
                        std::map<std::string, sls_logs::LogGroup*>& logGroupMap) const;
    void ReadAsFileBuffer(std::string& metricsContent) const;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class MetricManagerUnittest;
#endif
};

} // namespace logtail
