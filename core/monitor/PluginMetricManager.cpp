/*
 * Copyright 2024 iLogtail Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "PluginMetricManager.h"

namespace logtail {

ReusableMetricsRecordRef PluginMetricManager::GetOrCreateReusableMetricsRecordRef(MetricLabels labels) {
    std::string key = GenerateKey(labels);
    // try get
    {
        std::shared_lock lock(mutex);
        auto it = mReusableMetricsRecordRefsMap.find(key);
        if (it != mReusableMetricsRecordRefsMap.end()) {
            return it->second;
        }
    }
    // create
    {
        std::unique_lock lock(mutex);
        MetricLabels newLabels = mDefaultLabels;
        newLabels.insert(newLabels.end(), labels.begin(), labels.end());

        ReusableMetricsRecordRef ptr = std::make_shared<ReusableMetricsRecord>();
        ptr->Init(newLabels, mCounterKeys, mGaugeKeys);

        mReusableMetricsRecordRefsMap.emplace(key, ptr);
        if (mSizeGauge != nullptr) {
            mSizeGauge->Set(mReusableMetricsRecordRefsMap.size());
        }
        return ptr;
    }
}

void PluginMetricManager::ReleaseReusableMetricsRecordRef(MetricLabels labels) {
    std::string key = GenerateKey(labels);
    // try get
    {
        std::shared_lock lock(mutex);
        auto it = mReusableMetricsRecordRefsMap.find(key);
        if (it == mReusableMetricsRecordRefsMap.end()) {
            return;
        } else if (it->second.use_count() > 2) {
            // mMetricsRecordRefMaps中一次引用，待删除的实例中有一次引用
            // 如果引用数大于二，说明还有其他地方在使用这个MetricsRecordRef，mMetricsRecordRefMaps中就不将它删除
            return;
        }
    }
    // delete
    {
        std::unique_lock lock(mutex);
        mReusableMetricsRecordRefsMap.erase(key);
        if (mSizeGauge != nullptr) {
            mSizeGauge->Set(mReusableMetricsRecordRefsMap.size());
        }
    }
}

std::string PluginMetricManager::GenerateKey(MetricLabels& labels) {
    std::sort(labels.begin(), labels.end());
    std::ostringstream oss;
    for (const auto& pair : labels) {
        oss << pair.first << "=" << pair.second << ";";
    }
    return oss.str();
}

} // namespace logtail