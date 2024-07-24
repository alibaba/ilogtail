/*
 * Copyright 2024 iLogtail Authors
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
#include "ProcessorRelabelMetricNative.h"

#include <json/json.h>

#include <cstddef>

#include "PipelineEventGroup.h"
#include "PipelineEventPtr.h"
#include "models/MetricEvent.h"

using namespace std;
namespace logtail {

const string ProcessorRelabelMetricNative::sName = "processor_relabel_metric_native";

// only for inner processor
bool ProcessorRelabelMetricNative::Init(const Json::Value& config) {
    LOG_INFO(sLogger, ("processor init", config.toStyledString()));
    std::string errorMsg;
    if (config.isMember("metric_relabel_configs") && config["metric_relabel_configs"].isArray()
        && config["metric_relabel_configs"].size() > 0) {
        for (const auto& item : config["metric_relabel_configs"]) {
            mRelabelConfigs.emplace_back(item);
            if (!mRelabelConfigs.back().Validate()) {
                errorMsg = "metric_relabel_configs is invalid";
                LOG_ERROR(sLogger, ("init prometheus processor failed", errorMsg));
                return false;
            }
        }
        return true;
    } else {
        return false;
    }
}

void ProcessorRelabelMetricNative::Process(PipelineEventGroup& metricGroup) {
    if (metricGroup.GetEvents().empty()) {
        return;
    }

    EventsContainer& events = metricGroup.MutableEvents();

    size_t wIdx = 0;
    for (size_t rIdx = 0; rIdx < events.size(); ++rIdx) {
        if (ProcessEvent(events[rIdx])) {
            if (wIdx != rIdx) {
                events[wIdx] = std::move(events[rIdx]);
            }
            ++wIdx;
        }
    }
    events.resize(wIdx);
    return;
}

bool ProcessorRelabelMetricNative::IsSupportedEvent(const PipelineEventPtr& e) const {
    return e.Is<MetricEvent>();
}

bool ProcessorRelabelMetricNative::ProcessEvent(PipelineEventPtr& e) {
    if (!IsSupportedEvent(e)) {
        return false;
    }
    MetricEvent& sourceEvent = e.Cast<MetricEvent>();

    Labels labels;

    // TODO: 使用Labels类作为对MetricEvent操作的适配器
    labels.Reset(&sourceEvent);
    Labels result;

    // if keep this sourceEvent
    if (relabel::Process(labels, mRelabelConfigs, result)) {
        // if k/v in labels by not result, then delete it
        labels.Range([&result, &sourceEvent](const Label& label) {
            if (result.Get(label.name).empty()) {
                sourceEvent.DelTag(StringView(label.name));
            }
        });

        // for each k/v in result, set it to sourceEvent
        result.Range([&sourceEvent](const Label& label) { sourceEvent.SetTag(label.name, label.value); });

        // set metricEvent name
        if (!result.Get("__name__").empty()) {
            sourceEvent.SetName(result.Get("__name__"));
        }
        return true;
    }
    return false;
}

} // namespace logtail
