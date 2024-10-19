/*
 * Copyright 2023 iLogtail Authors
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
#pragma once

#include "models/LogEvent.h"
#include "plugin/interface/Processor.h"

namespace logtail {

class SpanEvent {
public:
    bool HasTag(const StringView& key) const {
        // 模拟检查标签是否存在
        return true;
    }

    StringView GetTag(const StringView& key) const {
        // 模拟获取标签值
        return StringView("value");
    }

    void SetTag(const std::string& key, const std::string& value) {
        // 模拟设置标签
    }

    template <typename T>
    bool Is() const {
        return false;
    }

    template <typename T>
    T Cast() const {
        return T();
    }
};    

class LabelingK8sMetadata {
public:
    void AddLabelToLogGroup(PipelineEventGroup& logGroup);
    bool ProcessEvent(PipelineEventPtr& e,  std::vector<std::string>& container_vec,  std::vector<std::string>& remote_ip_vec);
    bool AddLabelToMetric(MetricEvent& e, std::vector<std::string>& container_vec, std::vector<std::string>& remote_ip_vec);
    bool AddLabelToSpan(SpanEvent& e, std::vector<std::string>& container_vec, std::vector<std::string>& remote_ip_vec);
protected:
    bool IsSupportedEvent(const PipelineEventPtr& e) const;
};

} // namespace logtail
