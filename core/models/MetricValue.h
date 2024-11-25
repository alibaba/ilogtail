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

#pragma once

#include <map>
#include <variant>

#ifdef APSARA_UNIT_TEST_MAIN
#include <json/json.h>

#include <string>
#endif

#include "common/memory/SourceBuffer.h"
#include "models/PipelineEvent.h"
#include "models/StringView.h"

namespace logtail {

struct UntypedSingleValue {
    double mValue;

    constexpr size_t DataSize() const { return sizeof(UntypedSingleValue); }

#ifdef APSARA_UNIT_TEST_MAIN
    Json::Value ToJson() const;
    void FromJson(const Json::Value& value);
#endif
};

struct UntypedMultiValues {
    std::map<StringView, double> mValues;
    PipelineEvent* mMetricEventPtr;

    UntypedMultiValues(PipelineEvent* ptr) : mMetricEventPtr(ptr) {}
    UntypedMultiValues(std::map<StringView, double> values, PipelineEvent* ptr)
        : mValues(values), mMetricEventPtr(ptr) {}

    bool Get(StringView key, double& val) const;
    bool Has(StringView key) const;
    void Set(const std::string& key, double val);
    void Set(StringView key, double val);
    void SetNoCopy(const StringBuffer& key, double val);
    void SetNoCopy(StringView key, double val);
    void Del(StringView key);

    std::map<StringView, double>::const_iterator MultiKeyValusBegin() const;
    std::map<StringView, double>::const_iterator MultiKeyValusEnd() const;
    size_t MultiKeyValusSize() const;

    size_t DataSize() const;
    void ResetPipelineEvent(PipelineEvent* ptr) { mMetricEventPtr = ptr; }

#ifdef APSARA_UNIT_TEST_MAIN
    Json::Value ToJson() const;
    void FromJson(const Json::Value& value);
#endif
};

using MetricValue = std::variant<std::monostate, UntypedSingleValue, UntypedMultiValues>;

size_t DataSize(const MetricValue& value);

} // namespace logtail
