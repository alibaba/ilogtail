// Copyright 2024 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "common/compression/Compressor.h"

#include <chrono>

using namespace std;

namespace logtail {

void Compressor::SetMetricRecordRef(MetricLabels&& labels, DynamicMetricLabels&& dynamicLabels) {
    WriteMetrics::GetInstance()->PrepareMetricsRecordRef(
        mMetricsRecordRef, std::move(labels), std::move(dynamicLabels));
    mInItemsCnt = mMetricsRecordRef.CreateCounter("in_items_cnt");
    mInItemSizeBytes = mMetricsRecordRef.CreateCounter("in_item_size_bytes");
    mOutItemsCnt = mMetricsRecordRef.CreateCounter("out_items_cnt");
    mOutItemSizeBytes = mMetricsRecordRef.CreateCounter("out_item_size_bytes");
    mDiscardedItemsCnt = mMetricsRecordRef.CreateCounter("discarded_items_cnt");
    mDiscardedItemSizeBytes = mMetricsRecordRef.CreateCounter("discarded_item_size_bytes");
    mTotalDelayMs = mMetricsRecordRef.CreateCounter("total_delay_ms");
}

bool Compressor::DoCompress(const string& input, string& output, string& errorMsg) {
    if (mMetricsRecordRef != nullptr) {
        mInItemsCnt->Add(1);
        mInItemSizeBytes->Add(input.size());
    }

    auto before = chrono::system_clock::now();
    auto res = Compress(input, output, errorMsg);

    if (mMetricsRecordRef != nullptr) {
        mTotalDelayMs->Add(chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - before).count());
        if (res) {
            mOutItemsCnt->Add(1);
            mOutItemSizeBytes->Add(output.size());
        } else {
            mDiscardedItemsCnt->Add(1);
            mDiscardedItemSizeBytes->Add(input.size());
        }
    }
    return res;
}

} // namespace logtail
